#include "hook_registry.h"

namespace mb_shell {
static std::vector<std::function<void()>> g_uninstallers;

void hook_registry::register_uninstaller(std::function<void()> fn) {
    g_uninstallers.push_back(std::move(fn));
}

void hook_registry::uninstall_all() {
    // Uninstall in reverse order (LIFO) so dependent hooks are removed first.
    for (auto it = g_uninstallers.rbegin(); it != g_uninstallers.rend(); ++it) {
        (*it)();
    }
    g_uninstallers.clear();
}
} // namespace mb_shell
