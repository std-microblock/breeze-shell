#pragma once
#include <functional>
#include <vector>

namespace mb_shell {
// Collects hook-uninstall callbacks so DLL_PROCESS_DETACH can restore every
// patched call-site before the DLL image is unmapped.
struct hook_registry {
    static void register_uninstaller(std::function<void()> fn);
    static void uninstall_all();
};
} // namespace mb_shell
