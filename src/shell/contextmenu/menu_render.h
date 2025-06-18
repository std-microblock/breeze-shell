#pragma once
#include "../utils.h"
#include "contextmenu.h"
#include "ui.h"
#include <memory>
#include <optional>

namespace mb_shell {
struct menu_render {
  std::shared_ptr<ui::render_target> rt;
  std::optional<int32_t> selected_menu;
  bool light_color = is_light_mode();
  static std::optional<menu_render *> current;

  menu_render() = delete;
  menu_render(std::shared_ptr<ui::render_target> rt,
              std::optional<int> selected_menu);

  ~menu_render();
  const menu_render &operator=(const menu_render &) = delete;
  menu_render(menu_render &&t);
  menu_render &operator=(menu_render &&t);
  static menu_render create(int x, int y, menu menu);
};
} // namespace mb_shell