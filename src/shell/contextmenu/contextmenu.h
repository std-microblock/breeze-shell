#pragma once

#include "breeze_ui/nanovg_wrapper.h"
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <Windows.h>

namespace mb_shell {
struct menu_item;
struct menu_widget;
struct menu_render;
struct menu {
    std::vector<menu_item> items;
    void *parent_window = nullptr;
    void *native_handle = nullptr;
    bool is_top_level = false;

    // Kept so a deferred submenu can be re-read later: shell popups such as
    // "Send To" keep inserting items into their HMENU after we have already
    // built the widget for it.
    std::function<void(int, WPARAM, LPARAM)> handle_menu_msg;
    LPARAM init_popup_lparam = 0xFFFFFFFF;

    // send_init_msg = false re-reads the current contents of hMenu without
    // sending WM_INITMENUPOPUP again, so the menu owner is not asked to
    // populate (and therefore duplicate) the menu a second time.
    static menu construct_with_hmenu(
        HMENU hMenu, HWND hWnd, bool is_top = true,
        std::function<void(int, WPARAM, LPARAM)> HandleMenuMsg = {},
        LPARAM init_popup_lparam = 0xFFFFFFFF, bool send_init_msg = true);
};

std::optional<int>
track_popup_menu(menu menu, int x, int y,
                 std::function<void(menu_render &)> on_before_show = {},
                 bool run_js = true);

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
