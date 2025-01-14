#pragma once

#include "nanovg_wrapper.h"
#include <functional>
#include <optional>
#include <string>
#include <vector>

#define NOMINMAX
#include <Windows.h>

namespace mb_shell {
struct menu_item;
struct menu {
  std::vector<menu_item> items;
  std::string to_string();
  void *parent_window = nullptr;

  static menu construct_with_hmenu(HMENU hMenu, HWND hWnd);
};

struct menu_item {
  enum class type {
    button,
    toggle,
    spacer,
  } type = type::button;

  std::optional<std::string> name;
  std::optional<std::function<void()>> action;
  std::optional<std::function<menu()>> submenu;
  bool checked = false;
  std::optional<HBITMAP> icon_bitmap;

  std::string to_string();
};
} // namespace mb_shell