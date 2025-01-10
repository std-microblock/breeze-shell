#pragma once

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
  std::optional<menu> submenu;

  std::optional<HBITMAP> icon;

  std::string to_string();
};
} // namespace mb_shell