
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

void main() {
  AllocConsole();
  SetConsoleCP(CP_UTF8);
  freopen("CONOUT$", "w", stdout);
  freopen("CONOUT$", "w", stderr);
  freopen("CONIN$", "r", stdin);
  ShowWindow(GetConsoleWindow(), SW_HIDE);

  install_error_handlers();
  config::run_config_loader();

  res_string_loader::init();

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
  // hook NtUserTrackPopupMenu

  auto NtUserTrackPopupMenu = win32u.value()->exports("NtUserTrackPopupMenuEx");
  auto NtUserTrackHook = NtUserTrackPopupMenu->inline_hook();

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

  NtUserTrackHook->install([=](HMENU hMenu, UINT uFlags, int x, int y,
                               HWND hWnd, LPTPMPARAMS lptpm) {
    if (GetPropW(hWnd, L"COwnerDrawPopupMenu_This") &&
        config::current->context_menu.ignore_owner_draw) {
      return NtUserTrackHook->call_trampoline<size_t>(hMenu, uFlags, x, y, hWnd,
                                                      lptpm);
    }

    has_active_menu = true;
    menu menu = menu::construct_with_hmenu(hMenu, hWnd);
    auto menu_render = menu_render::create(x, y, menu);

    menu_render.rt->last_time = menu_render.rt->clock.now();
    menu_render.rt->start_loop();

    has_active_menu = false;

    if (menu_render.selected_menu) {
      PostMessageW(hWnd, WM_COMMAND, *menu_render.selected_menu, 0);
      PostMessageW(hWnd, WM_NULL, 0, 0);
    }

    return menu_render.selected_menu.value_or(0);
  });

  // reg.exe add
  // "HKCU\Software\Classes\CLSID\{86ca1aa0-34aa-4e8b-a509-50c905bae2a2}\InprocServer32"
  // /f /ve
  RegSetKeyValueW(HKEY_CURRENT_USER,
                  L"Software\\Classes\\CLSID\\{86ca1aa0-34aa-4e8b-a509-"
                  L"50c905bae2a2}\\InprocServer32",
                  nullptr, REG_SZ, L"", sizeof(L""));
}
} // namespace mb_shell

int APIENTRY DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID lpvReserved) {
  switch (fdwReason) {
  case DLL_PROCESS_ATTACH: {
    auto cmdline = std::string(GetCommandLineA());

    std::ranges::transform(cmdline, cmdline.begin(), tolower);

    if (cmdline.contains("explorer")) {
      mb_shell::main();
    }
    break;
  }
  }
  return 1;
}