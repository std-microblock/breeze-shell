#include "config.h"
#include <cctype>
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
    auto font_dir = ui::windows_font_directory();

    auto to_lower = [](std::string s) {
        for (auto &c : s)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return s;
    };

    auto add_with_system_variants =
        [&](std::vector<ui::weighted_font_face> &faces,
            const std::filesystem::path &user_path, const char *expected_name,
            int collection_index = 0) {
            if (user_path.empty())
                return;
            faces.push_back(
                {.weight = 400,
                 .source = {.path = user_path,
                            .collection_index = collection_index}});

            if (to_lower(user_path.filename().string()) !=
                to_lower(expected_name))
                return;

            struct variant_entry {
                int weight;
                const char *file;
            };
            const variant_entry *variants = nullptr;
            int variant_count = 0;

            // clang-format off
            if (to_lower(expected_name) == "segoeui.ttf") {
                static constexpr variant_entry v[] = {
                    {200, "segoeuisl.ttf"}, {300, "segoeuil.ttf"},
                    {600, "seguisb.ttf"},   {700, "segoeuib.ttf"},
                };
                variants = v; variant_count = 4;
            } else if (to_lower(expected_name) == "msyh.ttc") {
                static constexpr variant_entry v[] = {
                    {300, "msyhl.ttc"}, {700, "msyhbd.ttc"},
                };
                variants = v; variant_count = 2;
            } else if (to_lower(expected_name) == "consola.ttf") {
                static constexpr variant_entry v[] = {
                    {700, "consolab.ttf"},
                };
                variants = v; variant_count = 1;
            }
            // clang-format on

            for (int i = 0; i < variant_count; ++i) {
                auto p = font_dir / variants[i].file;
                if (std::filesystem::exists(p))
                    faces.push_back(
                        {.weight = variants[i].weight,
                         .source = {.path = std::move(p),
                                    .collection_index = collection_index}});
            }
        };

    // 1. System Segoe UI – always available as ultimate fallback
    {
        std::vector<ui::weighted_font_face> faces;
        add_with_system_variants(faces, font_dir / "segoeui.ttf",
                                 "segoeui.ttf");
        ui::register_font_family(nvg, {.family_name = "segoeui",
                                       .faces = faces});
    }

    // 2. Fallback font (user-configured) – falls back to segoeui
    //    For msyh.ttc use collection_index=1 → "Microsoft YaHei UI"
    {
        int fb_idx = 0;
        if (to_lower(font_path_fallback.filename().string()) == "msyh.ttc")
            fb_idx = 1;

        std::vector<ui::weighted_font_face> faces;
        add_with_system_variants(faces, font_path_fallback, "msyh.ttc",
                                 fb_idx);
        ui::register_font_family(
            nvg, {.family_name = "fallback",
                  .faces = faces,
                  .fallback_families = {"segoeui"}});
    }

    // 3. Main font (user-configured) – falls back to segoeui + fallback
    {
        std::vector<ui::weighted_font_face> faces;
        add_with_system_variants(faces, font_path_main, "segoeui.ttf");
        ui::register_font_family(
            nvg, {.family_name = "main",
                  .faces = faces,
                  .fallback_families = {"segoeui", "fallback"}});
    }

    // 4. Monospace font (user-configured) – falls back to main + segoeui +
    // fallback
    {
        std::vector<ui::weighted_font_face> faces;
        add_with_system_variants(faces, font_path_monospace, "consola.ttf");
        ui::register_font_family(
            nvg, {.family_name = "monospace",
                  .faces = faces,
                  .fallback_families = {"main", "segoeui", "fallback"}});
    }
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
