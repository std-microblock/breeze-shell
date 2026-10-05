#include "config.h"
#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <thread>

#include "diag.h"
#include "logger.h"
#include "rfl.hpp"
#include "rfl/DefaultIfMissing.hpp"
#include "rfl/json.hpp"

#include "breeze_ui/font.h"
#include "utils.h"
#include <shlobj.h>
#include "windows.h"
#include "wtr/watcher.hpp"

namespace rfl {
template <> struct Reflector<mb_shell::paint_color> {
    using ReflType = std::string;

    static mb_shell::paint_color to(const ReflType &v) noexcept {
        return mb_shell::paint_color::from_string(v);
    }

    static ReflType from(const mb_shell::paint_color &v) {
        return v.to_string();
    }
};
} // namespace rfl

namespace mb_shell {
namespace {
std::mutex &config_snapshot_mutex() {
    static auto *mutex = new std::mutex();
    return *mutex;
}

std::mutex &config_read_mutex() {
    static auto *mutex = new std::mutex();
    return *mutex;
}

std::vector<std::unique_ptr<config>> &config_snapshots() {
    // Config watcher callbacks may still be active during process teardown.
    // Keep every immutable snapshot alive for the process lifetime so a reader
    // can safely finish after a newer snapshot is published.
    static auto *snapshots = new std::vector<std::unique_ptr<config>>();
    return *snapshots;
}
} // namespace

config::snapshot_ptr config::current;
config::snapshot_ptr &
config::snapshot_ptr::operator=(std::unique_ptr<config> next) {
    auto *published = next.get();
    {
        std::lock_guard lock(config_snapshot_mutex());
        config_snapshots().push_back(std::move(next));
    }
    value.store(published, std::memory_order_release);
    return *this;
}

config::animated_float_conf config::_default_animation{
    .duration = 150,
    .easing = ui::easing_type::ease_in_out,
    .delay_scale = 1,
};

void config::write_config() {
    auto config_file = data_directory() / "config.json";
    std::ofstream ofs(config_file);
    if (!ofs) {
        spdlog::error("Failed to write config file.");
        return;
    }

    ofs << rfl::json::write(*config::current);
}
void config::read_config() {
    std::lock_guard read_lock(config_read_mutex());
    auto config_file = data_directory() / "config.json";
    std::vector<diag::problem> problems;

#ifdef __llvm__
    if (!std::filesystem::exists(config_file)) {
        std::ofstream ofs(config_file);
        if (!ofs) {
            spdlog::error("Failed to write config file.");
        }

        ofs << R"({
  "$schema": "https://raw.githubusercontent.com/std-microblock/breeze-shell/refs/heads/master/resources/schema.json"
})";
    }

    std::ifstream ifs(config_file);
    if (!ifs) {
        spdlog::warn(
            "Config file could not be opened. Using default config instead.");
        config::current = std::make_unique<config>();
        problems.push_back({.severity = "warning",
                            .title = "Config file could not be opened",
                            .detail = "Breeze is running with the default "
                                      "config until the file becomes readable.",
                            .source = config_file.string()});
    } else {
        std::string json_str;
        std::copy(std::istreambuf_iterator<char>(ifs),
                  std::istreambuf_iterator<char>(),
                  std::back_inserter(json_str));

        if (auto json = rfl::json::read<config, rfl::NoExtraFields,
                                        rfl::DefaultIfMissing>(json_str)) {
            // parse twice for default value
            _default_animation = json.value().default_animation;
            json = rfl::json::read<config, rfl::NoExtraFields,
                                   rfl::DefaultIfMissing>(json_str);
            config::current = std::make_unique<config>(json.value());
            spdlog::info("Config reloaded.");
        } else {
            spdlog::warn(
                "Failed to read config file: {}\nUsing default config instead.",
                json.error().what());
            config::current = std::make_unique<config>();
            problems.push_back({.severity = "error",
                                .title = "Config file is invalid",
                                .detail = json.error().what(),
                                .source = config_file.string()});
        }
    }
#else
#pragma message                                                                \
    "We don't support loading config file on MSVC because of a bug in MSVC."
    spdlog::info("We don't support loading config file when compiled with MSVC "
                 "because of a bug in MSVC.");
    config::current = std::make_unique<config>();
    problems.push_back({.severity = "warning",
                        .title = "Config loading is unavailable",
                        .detail = "This build was compiled with MSVC, which "
                                  "cannot load config.json.",
                        .source = config_file.string()});
#endif

    auto checked = std::make_unique<config>(*config::current);
    for (auto [name, font, fallback] :
         {std::tuple{"font_path_main", &checked->font_path_main,
                     default_main_font()},
          std::tuple{"font_path_fallback", &checked->font_path_fallback,
                     default_fallback_font()},
          std::tuple{"font_path_monospace", &checked->font_path_monospace,
                     default_mono_font()}}) {
        std::error_code ec;
        if (!std::filesystem::exists(*font, ec)) {
            spdlog::warn("Font not found: {} ({})", name, font->string());
            problems.push_back(
                {.severity = "warning",
                 .title = std::format("Font not found: {}", name),
                 .detail = std::format("{} does not exist, using {} instead",
                                       font->string(), fallback.string()),
                 .source = config_file.string()});
            *font = fallback;
        }
    }
    config::current = std::move(checked);

    diag::set_problems("config", std::move(problems));
}

std::filesystem::path config::data_directory() {
    static std::optional<std::filesystem::path> path;
    static std::mutex mtx;
    std::lock_guard lock(mtx);

    if (!path) {
        std::filesystem::path base;
        if (auto profile = env("USERPROFILE");
            profile && !profile->empty()) {
            base = std::filesystem::path(*profile);
        } else if (auto home = env("HOME"); home && !home->empty()) {
            base = std::filesystem::path(*home);
        } else {
            wchar_t folder[MAX_PATH] = {0};
            if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PROFILE, nullptr, 0,
                                           folder))) {
                base = std::filesystem::path(folder);
            } else {
                base = std::filesystem::current_path();
            }
        }

        path = base / ".breeze-shell";
    }

    if (!std::filesystem::exists(*path)) {
        std::filesystem::create_directories(*path);
    }

    return path.value();
}
void config::run_config_loader() {
    auto config_path = config::data_directory() / "config.json";
    spdlog::info("config file: {}", config_path.string());
    config::read_config();

    static auto watcher =
        wtr::watch(config::data_directory(), [](const wtr::event &e) {
            if (e.path_name.filename() != "config.json") {
                return;
            }
            try {
                config::read_config();
            } catch (const std::exception &ex) {
                spdlog::warn("Config reload failed: {}", ex.what());
            } catch (...) {
                spdlog::warn("Config reload failed: unknown error");
            }
        });
}
void config::animated_float_conf::apply_to(ui::sp_anim_float &anim,
                                           float delay) {
    anim->set_duration(duration);
    anim->set_easing(easing);
    anim->set_delay(delay * delay_scale);
}
void config::animated_float_conf::operator()(ui::sp_anim_float &anim,
                                             float delay) {
    apply_to(anim, delay);
}

std::filesystem::path config::default_main_font() {
    return std::filesystem::path(env("WINDIR").value()) / "Fonts" /
           "segoeui.ttf";
}
std::filesystem::path config::default_fallback_font() {
    return std::filesystem::path(env("WINDIR").value()) / "Fonts" / "msyh.ttc";
}
std::string config::dump_config() { return rfl::json::write(*config::current); }
std::filesystem::path config::default_mono_font() {
    return std::filesystem::path(env("WINDIR").value()) / "Fonts" /
           "consola.ttf";
}
void config::apply_fonts_to_nvg(NVGcontext *nvg) {
    if (!nvg) {
        spdlog::error("Cannot register fonts without a NanoVG context.");
        return;
    }

    // Copy paths from this immutable snapshot before entering NanoVG. This
    // also makes it explicit that no config reload can affect the call.
    const auto main_font = font_path_main;
    const auto fallback_font = font_path_fallback;
    const auto monospace_font = font_path_monospace;
    ui::register_default_windows_font_suite(
        nvg, {.main_regular = {.path = main_font},
              .fallback_regular = {.path = fallback_font},
              .monospace_regular = {.path = monospace_font}});
}
void config::animated_float_conf::apply_to(ui::animated_color &anim,
                                           float delay) {
    apply_to(anim.r, delay);
    apply_to(anim.g, delay);
    apply_to(anim.b, delay);
    apply_to(anim.a, delay);
}
std::string config::dump_default_config() {
    std::lock_guard read_lock(config_read_mutex());
    return rfl::json::write(config{});
}
} // namespace mb_shell
