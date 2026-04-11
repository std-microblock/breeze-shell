#include "config.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <thread>

#include "entry.h"
#include "logger.h"
#include "rfl.hpp"
#include "rfl/DefaultIfMissing.hpp"
#include "rfl/json.hpp"

#include "breeze_ui/font.h"
#include "utils.h"
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
std::unique_ptr<config> config::current;
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
    auto config_file = data_directory() / "config.json";

#ifdef __llvm__
    std::ifstream ifs(config_file);
    if (!std::filesystem::exists(config_file)) {
        auto config_file = data_directory() / "config.json";
        std::ofstream ofs(config_file);
        if (!ofs) {
            spdlog::error("Failed to write config file.");
        }

        ofs << R"({
  "$schema": "https://raw.githubusercontent.com/std-microblock/breeze-shell/refs/heads/master/resources/schema.json"
})";
    }
    if (!ifs) {
        spdlog::warn("Config file could not be opened. Using default config instead.");
        config::current = std::make_unique<config>();
        config::current->debug_console = true;
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
            spdlog::error("Failed to read config file: {}\nUsing default config instead.", json.error().what());
            config::current = std::make_unique<config>();
            config::current->debug_console = true;
        }
    }
#else
#pragma message                                                                \
    "We don't support loading config file on MSVC because of a bug in MSVC."
    spdlog::info("We don't support loading config file when compiled with MSVC "
           "because of a bug in MSVC.");
    config::current = std::make_unique<config>();
    config::current->debug_console = true;
#endif

    if (config::current->debug_console) {
        init_console(true);
    } else {
        init_console(false);
    }
}

std::filesystem::path config::data_directory() {
    static std::optional<std::filesystem::path> path;
    static std::mutex mtx;
    std::lock_guard lock(mtx);

    if (!path) {
        path =
            std::filesystem::path(env("USERPROFILE").value()) / ".breeze-shell";
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
            if (e.path_name.filename() == "config.json") {
                config::read_config();
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
    ui::register_default_windows_font_suite(
        nvg, {.main_regular = {.path = font_path_main},
              .fallback_regular = {.path = font_path_fallback},
              .monospace_regular = {.path = font_path_monospace}});
}
void config::animated_float_conf::apply_to(ui::animated_color &anim,
                                           float delay) {
    apply_to(anim.r, delay);
    apply_to(anim.g, delay);
    apply_to(anim.b, delay);
    apply_to(anim.a, delay);
}
std::string config::dump_default_config() { return rfl::json::write(config{}); }
} // namespace mb_shell
