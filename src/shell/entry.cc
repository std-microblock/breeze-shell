
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

#define NOMINMAX
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

  res_string_loader::init();

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

  std::thread([]() {
    if (auto res = ui::render_target::init_global(); !res) {
      MessageBoxW(NULL, L"Failed to initialize global render target", L"Error",
                  MB_ICONERROR);
      return;
    }
  }).detach();

  wchar_t executable_path[MAX_PATH];
  if (GetModuleFileNameW(NULL, executable_path, MAX_PATH) == 0) {
    MessageBoxW(NULL, L"Failed to get executable path", L"Error", MB_ICONERROR);
    return;
  }

  std::filesystem::path exe_path(executable_path);

  fix_win11_menu::install();

  context_menu_hooks::install_common_hook();
  if (exe_path.filename() == "OneCommander.exe") {
    context_menu_hooks::install_SHCreateDefaultContextMenu_hook();
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