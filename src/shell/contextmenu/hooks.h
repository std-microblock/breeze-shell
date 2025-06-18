#pragma once
#include <atomic>
#include <expected>

namespace mb_shell {
struct context_menu_hooks {
static std::atomic_bool has_active_menu;
static void install_common_hook();
static void
install_SHCreateDefaultContextMenu_hook();
}; // namespace context_menu_hooks
} // namespace mb_shell