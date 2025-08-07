
#include "GLFW/glfw3.h"
#include "blook/blook.h"

#include "config.h"
#include "contextmenu/hooks.h"
#include "entry.h"
#include "error_handler.h"
#include "res_string_loader.h"
#include "script/binding_types.hpp"
#include "script/quickjspp.hpp"
#include "script/script.h"
#include "ui.h"
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

namespace mb_shell {
window_proc_hook entry::main_window_loop_hook{};
void main() {
  set_thread_locale_utf8();

  AllocConsole();
  freopen("CONOUT$", "w", stdout);
  freopen("CONOUT$", "w", stderr);
  freopen("CONIN$", "r", stdin);
  ShowWindow(GetConsoleWindow(), SW_HIDE);

  install_error_handlers();
  config::run_config_loader();

  std::thread([]() {
    script_context ctx;

    auto data_dir = config::data_directory();
    auto script_dir = data_dir / "scripts";

    if (!std::filesystem::exists(script_dir))
      std::filesystem::create_directories(script_dir);

    ctx.watch_folder(script_dir, [&]() {
      return !context_menu_hooks::has_active_menu.load();
    });
  }).detach();

  std::set_terminate([]() {
    auto eptr = std::current_exception();
    if (eptr) {
      try {
        std::rethrow_exception(eptr);
      } catch (const std::exception &e) {
        std::cerr << "Uncaught exception: " << e.what() << std::endl;
      } catch (...) {
        std::cerr << "Uncaught exception of unknown type" << std::endl;
      }

      ShowWindow(GetConsoleWindow(), SW_SHOW);
      std::getchar();
    }
    std::abort();
  });

  wchar_t executable_path[MAX_PATH];
  if (GetModuleFileNameW(NULL, executable_path, MAX_PATH) == 0) {
    MessageBoxW(NULL, L"Failed to get executable path", L"Error", MB_ICONERROR);
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

  if (filename == "explorer.exe") {
    init_render_global();
    res_string_loader::init();
    context_menu_hooks::install_common_hook();
    fix_win11_menu::install();
  }

  if (filename == "onecommander.exe") {
    init_render_global();
    context_menu_hooks::install_common_hook();
    context_menu_hooks::install_SHCreateDefaultContextMenu_hook();
    res_string_loader::init();
  }

  if (filename == "rundll32.exe") {
    SetProcessDPIAware();
    CoInitialize(nullptr);
    std::thread([]() {
      SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
      taskbar_render taskbar;
      auto monitor = MonitorFromPoint({0, 0}, MONITOR_DEFAULTTOPRIMARY);
      if (!monitor) {
        MessageBoxW(NULL, L"Failed to get primary monitor", L"Error",
                    MB_ICONERROR);
        return;
      }
      taskbar.monitor.cbSize = sizeof(MONITORINFO);
      if (GetMonitorInfoW(monitor, &taskbar.monitor) == 0) {
        MessageBoxW(
            NULL,
            (L"Failed to get monitor info: " + std::to_wstring(GetLastError()))
                .c_str(),
            L"Error", MB_ICONERROR);
        return;
      }
      taskbar.position = taskbar_render::menu_position::bottom;
      if (auto res = taskbar.init(); !res) {
        MessageBoxW(NULL, L"Failed to initialize taskbar", L"Error",
                    MB_ICONERROR);
        return;
      }

      taskbar.rt.start_loop();
    }).detach();
  }
}
} // namespace mb_shell

int APIENTRY DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID lpvReserved) {
  switch (fdwReason) {
  case DLL_PROCESS_ATTACH: {
    auto cmdline = std::string(GetCommandLineA());

    std::ranges::transform(cmdline, cmdline.begin(), tolower);

    mb_shell::main();
    break;
  }
  }
  return 1;
}

extern "C" __declspec(dllexport) void func() {
  while (true) {
    // This function is called by rundll32.exe, which is used to run the taskbar
    // in a separate thread.
    // We can use this to keep the taskbar running without blocking the main
    // thread.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}