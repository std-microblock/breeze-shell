#pragma once
#include "ui.h"
#include <optional>

namespace mb_shell {
struct menu_render {
  std::unique_ptr<ui::render_target> rt;
  std::optional<int> selected_menu;
  thread_local static std::optional<menu_render*> current;

  menu_render(std::unique_ptr<ui::render_target> rt,
              std::optional<int> selected_menu);

  ~menu_render();
};
} // namespace mb_shell