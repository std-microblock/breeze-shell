#pragma once

#include "animator.h"
#include <filesystem>
#include <memory>
#include <vector>
namespace mb_shell {

struct config {
  static std::filesystem::path default_main_font();
  static std::filesystem::path default_fallback_font();

  struct animated_float_conf {
    float duration = _default_animation.duration;
    ui::easing_type easing = _default_animation.easing;
    float delay_scale = _default_animation.delay_scale;

    void apply_to(ui::sp_anim_float &anim, float delay = 0);
    void operator()(ui::sp_anim_float &anim, float delay = 0);
  } default_animation;
  static animated_float_conf _default_animation;
  struct context_menu {
    struct theme {
      bool use_dwm_if_available = true;
      float background_opacity = 1;
      bool acrylic = true;
      float radius = 6;
      float font_size = 14;
      float item_height = 23;
      float item_gap = 3;
      float item_radius = 5;
      float margin = 5;
      float padding = 6;
      float text_padding = 8;
      float icon_padding = 4;
      float right_icon_padding = 20;
      float multibutton_line_gap = -6;
      float scrollbar_width = 6;
      float scrollbar_radius = 3;

      std::string acrylic_color_light = "#fefefe00";
      std::string acrylic_color_dark = "#28282800";

      // unused, only for backward compatibility
      float acrylic_opacity = 0.1;

      struct animation {
        struct main {
          animated_float_conf y;
        } main;
        struct item {
          animated_float_conf opacity;
          animated_float_conf x, y;
          animated_float_conf width;
        } item;
        struct bg {
          animated_float_conf opacity;
          animated_float_conf x, y, w, h;
        } main_bg, submenu_bg;
      } animation;
    } theme;

    bool vsync = true;
    bool ignore_owner_draw = true;
    bool reverse_if_open_to_up = true;
    bool experimental_ownerdraw_support = false;

    // debug purpose only
    bool search_large_dwItemData_range = false;

    struct position {
      int padding_vertical = 20;
      int padding_horizontal = 0;
    } position;
  } context_menu;
  bool debug_console = false;
  // Restart to apply font/hook changes
  std::filesystem::path font_path_main = default_main_font();
  std::filesystem::path font_path_fallback = default_fallback_font();
  bool res_string_loader_use_hook = false;
  bool avoid_resize_ui = false;
  std::vector<std::string> plugin_load_order = {};
  
  std::string $schema;
  static std::unique_ptr<config> current;
  static void read_config();
  static void write_config();
  static void run_config_loader();
  static std::string dump_config();

  static std::filesystem::path data_directory();
};
} // namespace mb_shell