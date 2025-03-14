
#include "GLFW/glfw3.h"
#include "blook/blook.h"

#include "config.h"
#include "entry.h"
#include "error_handler.h"
#include "res_string_loader.h"
#include "script/binding_types.h"
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
  fix_win11_menu::install();

  static std::atomic_bool has_active_menu = false;
  std::thread([]() {
    script_context ctx;

    auto data_dir = config::data_directory();
    auto script_dir = data_dir / "scripts";

    if (!std::filesystem::exists(script_dir))
      std::filesystem::create_directories(script_dir);

    ctx.watch_folder(script_dir, [&]() { return !has_active_menu.load(); });
  }).detach();

  auto proc = blook::Process::self();
  auto win32u = proc->module("win32u.dll");
  auto user32 = proc->module("user32.dll");

  auto NtUserTrackPopupMenu = win32u.value()->exports("NtUserTrackPopupMenuEx");
  static auto NtUserTrackHook = NtUserTrackPopupMenu->inline_hook();

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

  NtUserTrackHook->install(+[](HMENU hMenu, int64_t uFlags, int64_t x,
                               int64_t y, HWND hWnd, int64_t lptpm) {
    if (GetPropW(hWnd, L"COwnerDrawPopupMenu_This") &&
        config::current->context_menu.ignore_owner_draw) {
      return NtUserTrackHook->call_trampoline<int32_t>(hMenu, uFlags, x, y,
                                                       hWnd, lptpm);
    }
    
    entry::main_window_loop_hook.install(hWnd);

    has_active_menu = true;
    menu menu = menu::construct_with_hmenu(hMenu, hWnd);
    auto menu_render = menu_render::create(x, y, menu);

    menu_render.rt->last_time = menu_render.rt->clock.now();
    menu_render.rt->start_loop();

    has_active_menu = false;

    if (menu_render.selected_menu && !(uFlags & TPM_NONOTIFY)) {
      PostMessageW(hWnd, WM_COMMAND, *menu_render.selected_menu, 0);
      PostMessageW(hWnd, WM_NULL, 0, 0);
    }

    return (int32_t)menu_render.selected_menu.value_or(0);
  });
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