#pragma once
#include "breeze-js/script.h"
#include <atomic>
#include <filesystem>
namespace mb_shell {
struct script_context : breeze::script_context {
    std::atomic<bool> is_js_ready{false};

    script_context();
    void watch_folder(
        const std::filesystem::path &path,
        std::function<bool()> on_reload = []() { return true; });
};
} // namespace mb_shell
