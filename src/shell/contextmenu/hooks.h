#pragma once
#include <atomic>
#include <expected>
#include <memory>

namespace mb_shell {
struct menu_widget;

struct context_menu_hooks {
    static std::atomic_int block_js_reload;
    static void install_NtUserTrackPopupMenuEx_hook();
    static void install_menu_mutation_hooks();
    static void install_SHCreateDefaultContextMenu_hook();
    static void install_GetUIObjectOf_hook();
    static void set_active_root_menu_widget(
        std::shared_ptr<menu_widget> menu);
    static std::shared_ptr<menu_widget> active_root_menu_widget();
    static void clear_active_root_menu_widget(const menu_widget *expected =
                                                  nullptr);
}; // namespace context_menu_hooks
} // namespace mb_shell
