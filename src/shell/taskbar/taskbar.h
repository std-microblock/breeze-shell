#pragma once
#include "ui.h"
#include <expected>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#define NOMINMAX
#include <Windows.h>
namespace mb_shell {
struct taskbar_render {
  MONITORINFO monitor;
  ui::render_target rt;
  enum class menu_position { top, bottom } position = menu_position::bottom;

  std::expected<void, std::string> init();
};
} // namespace mb_shell