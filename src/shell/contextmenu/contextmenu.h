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

struct owner_draw_menu_info {
  HBITMAP bitmap;
  int width, height;
};

struct menu_item {
  enum class type {
    button,
    spacer,
  } type = type::button;

  std::optional<owner_draw_menu_info> owner_draw{};
  std::optional<std::string> name;
  std::optional<std::function<void()>> action;
  std::optional<std::function<void(std::shared_ptr<menu_widget>)>> submenu;
  std::optional<size_t> icon_bitmap;
  std::optional<std::string> icon_svg;
  std::optional<std::string> hotkey;
  bool icon_updated = false;
  bool disabled = false;

  // the two below are only for information; set them changes nothing
  std::optional<size_t> wID;
  std::optional<std::string> name_resid;
  std::optional<std::string> origin_name;
};
} // namespace mb_shell