#pragma once
#include "shell.h"
#include "ui.h"
#include <optional>

namespace mb_shell {
struct menu_render {
  std::unique_ptr<ui::render_target> rt;
  std::optional<int> selected_menu;
  enum class menu_style {
    fluentui,
    materialyou
  } style = menu_style::materialyou;
  thread_local static std::optional<menu_render *> current;

  menu_render() = delete;
  menu_render(std::unique_ptr<ui::render_target> rt,
              std::optional<int> selected_menu);

  ~menu_render();
  const menu_render &operator=(const menu_render &) = delete;
  menu_render(menu_render &&t);
  menu_render &operator=(menu_render &&t);
  static menu_render create(int x, int y, menu menu);
};
} // namespace mb_shell