#pragma once

#include "animator.h"
#include <filesystem>
#include <memory>
namespace mb_shell {

struct config {
  struct context_menu {
    struct theme {
      bool use_dwm_if_available = true;
      float background_opacity = 1;
      bool acrylic = true;
      float radius = 6;
      float font_size = 14;
      float item_height = 25;
      float item_gap = 5;
      float item_radius = 100;
      float margin = 5;
    } theme;
  } context_menu;

  static std::unique_ptr<config> current;
  static void read_config();
  static void write_config();
  static void run_config_loader();

  static std::filesystem::path data_directory();
};
} // namespace mb_shell