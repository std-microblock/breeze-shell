#pragma once
#include <atomic>
#include <expected>

namespace mb_shell {
struct context_menu_hooks {
    static std::atomic_int block_js_reload;
    static void install_common_hook();
    static void install_SHCreateDefaultContextMenu_hook();
}; // namespace context_menu_hooks
} // namespace mb_shell