#pragma once

#include "nanovg_wrapper.h"
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#define NOMINMAX
#include <Windows.h>

namespace mb_shell {
struct menu_item;
struct menu_widget;
struct menu {
  std::vector<menu_item> items;
  void *parent_window = nullptr;
  bool is_top_level = false;

  static menu construct_with_hmenu(HMENU hMenu, HWND hWnd, bool is_top = true);
};

struct menu_item {
  enum class type {
    button,
    toggle,
    spacer,
  } type = type::button;

  std::optional<std::string> name;
  std::optional<std::function<void()>> action;
  std::optional<std::function<void(std::shared_ptr<menu_widget>)>> submenu;
  bool checked = false;
  std::optional<size_t> icon_bitmap;
  std::optional<std::string> icon_svg;
  bool icon_updated = false;
};
} // namespace mb_shell