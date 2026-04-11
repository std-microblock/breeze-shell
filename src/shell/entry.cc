
#include "GLFW/glfw3.h"
#include "blook/blook.h"

#include "breeze_ui/ui.h"
#include "config.h"
#include "contextmenu/hooks.h"
#include "entry.h"
#include "error_handler.h"
#include "res_string_loader.h"
#include "script/binding_types.hpp"
#include "breeze-js/quickjspp.hpp"
#include "script/script.h"
#include "utils.h"

#include "./contextmenu/contextmenu.h"
#include "./contextmenu/menu_render.h"
#include "./contextmenu/menu_widget.h"

#include "./taskbar/taskbar.h"

#include "fix_win11_menu.h"

#include <atomic>
#include <chrono>
#include <codecvt>
#include <condition_variable>
#include <consoleapi3.h>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <functional>
#include <future>
#include <iostream>
#include <objbase.h>
#include <optional>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <consoleapi.h>
#include <debugapi.h>
#include <type_traits>
#include <winreg.h>
#include <winuser.h>

#include <Windows.h>

#include "logger.h"

#include "async_simple/coro/SyncAwait.h"

namespace mb_shell {
window_proc_hook entry::main_window_loop_hook{};

static bool console_allocated = false;

void init_console(bool show) {
    if (show && !console_allocated) {
        AllocConsole();
        console_allocated = true;
        add_console_sink();
        ShowWindow(GetConsoleWindow(), SW_SHOW);
    } else if (show && console_allocated) {
        ShowWindow(GetConsoleWindow(), SW_SHOW);
        SetForegroundWindow(GetConsoleWindow());
    } else if (!show && console_allocated) {
        remove_console_sink();
        FreeConsole();
        console_allocated = false;
    }
}

void main() {
    set_thread_locale_utf8();

    init_logger();
    // install_error_handlers();
    config::run_config_loader();

    if (config::current->debug_console) {
        init_console(true);
    }

    static script_context script_ctx;
    std::thread([]() {
        auto data_dir = config::data_directory();
        auto script_dir = data_dir / "scripts";

        if (!std::filesystem::exists(script_dir))
            std::filesystem::create_directories(script_dir);

        script_ctx.watch_folder(script_dir, [&]() {
            return !context_menu_hooks::block_js_reload.load();
        });
    }).detach();

    std::set_terminate([]() {
        auto eptr = std::current_exception();
        if (eptr) {
            init_console(true);
            try {
                std::rethrow_exception(eptr);
            } catch (const std::exception &e) {
                spdlog::critical("Uncaught exception: {}", e.what());
            } catch (...) {
                spdlog::critical("Uncaught exception of unknown type");
            }

            std::getchar();
        }
        std::abort();
    });

    wchar_t executable_path[MAX_PATH];
    if (GetModuleFileNameW(NULL, executable_path, MAX_PATH) == 0) {
        MessageBoxW(NULL, L"Failed to get executable path", L"Error",
                    MB_ICONERROR);
        return;
    }

    auto init_render_global = [&]() {
        std::thread([]() {
            if (auto res = ui::render_target::init_global(); !res) {
                MessageBoxW(NULL, L"Failed to initialize global render target",
                            L"Error", MB_ICONERROR);
                return;
            }
        }).detach();
    };

    std::filesystem::path exe_path(executable_path);
    auto filename =
        exe_path.filename().string() |
        std::views::transform([](char c) { return std::tolower(c); }) |
        std::ranges::to<std::string>();

    if (filename == "explorer.exe" || filename == "360filebrowser64.exe" || filename == "desktopmgr64.exe") {
        init_render_global();
        res_string_loader::init();
        context_menu_hooks::install_NtUserTrackPopupMenuEx_hook();
        context_menu_hooks::install_menu_mutation_hooks();
        fix_win11_menu::install();
    }

    if (filename == "onecommander.exe") {
        init_render_global();
        context_menu_hooks::install_menu_mutation_hooks();
        context_menu_hooks::install_SHCreateDefaultContextMenu_hook();
        res_string_loader::init();
    }

    if (filename == "rundll32.exe") {
        auto command_line = std::wstring(GetCommandLineW());

        if (command_line.contains(L"-taskbar")) {
            SetProcessDPIAware();
            CoInitialize(nullptr);
            std::thread([]() {
                try {
                    SetThreadDpiAwarenessContext(
                        DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
                    taskbar_render taskbar;
                    auto monitor =
                        MonitorFromPoint({0, 0}, MONITOR_DEFAULTTOPRIMARY);
                    if (!monitor) {
                        MessageBoxW(NULL, L"Failed to get primary monitor",
                                    L"Error", MB_ICONERROR);
                        return;
                    }
                    taskbar.monitor.cbSize = sizeof(MONITORINFO);
                    if (GetMonitorInfoW(monitor, &taskbar.monitor) == 0) {
                        MessageBoxW(NULL,
                                    (L"Failed to get monitor info: " +
                                     std::to_wstring(GetLastError()))
                                        .c_str(),
                                    L"Error", MB_ICONERROR);
                        return;
                    }
                    taskbar.position = taskbar_render::menu_position::bottom;
                    if (auto res = taskbar.init(); !res) {
                        MessageBoxW(NULL, L"Failed to initialize taskbar",
                                    L"Error", MB_ICONERROR);
                        return;
                    }

                    taskbar.rt.start_loop();
                }
                catch(const std::exception &e) {
                    spdlog::error("Error in taskbar thread: {}", e.what());
                }
            }).detach();
        }
    }

    if (filename == "asan_test.exe") {
        // ASAN environment
        init_render_global();
        init_console(true);
        std::thread([]() {
            script_ctx.is_js_ready.wait(false);
            spdlog::info("Is js ready: %d", script_ctx.is_js_ready.load());
            try {
                auto res = syncAwait(script_ctx.eval_string("setInterval(globalThis.showConfigPage, 1300);",
                                           "asan.js")->await());
                spdlog::info("ASAN eval result: {}", res.as<std::string>());
            } catch (const std::exception &e) {
                spdlog::error("Error in ASAN test thread: {}", e.what());
            }
        }).detach();
    }
}
} // namespace mb_shell

int APIENTRY DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH: {
        mb_shell::main();
        break;
    }
    }
    return 1;
}

extern "C" __declspec(dllexport) void func() {
    while (true) {
        // This function is called by rundll32.exe, which is used to run the
        // taskbar in a separate thread. We can use this to keep the taskbar
        // running without blocking the main thread.
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
