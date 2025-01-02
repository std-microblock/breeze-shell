
#include "blook/blook.h"

#include "shell.h"
#include "entry.h"
#include "menu_widget.h"
#include "ui.h"
#include "utils.h"

#include <codecvt>
#include <thread>
#include <string>
#include <string_view>
#include <vector>

#include <consoleapi.h>
#include <debugapi.h>
#include <type_traits>

#define NOMINMAX
#include <Windows.h>


namespace mb_shell {

void main() {
  auto proc = blook::Process::self();
  AllocConsole();
  SetConsoleCP(CP_UTF8);
  freopen("CONOUT$", "w", stdout);
  freopen("CONOUT$", "w", stderr);
  freopen("CONIN$", "r", stdin);

  auto win32u = proc->module("win32u.dll");
  // hook NtUserTrackPopupMenu
  auto NtUserTrackPopupMenu = win32u.value()->exports("NtUserTrackPopupMenuEx");
  auto NtUserTrackHook = NtUserTrackPopupMenu->inline_hook();

  NtUserTrackHook->install([=](HMENU hMenu, uint32_t uFlags, int x, int y,
                               HWND hWnd, LPTPMPARAMS lptpm, int tfunc) {
    constexpr int l_pad = 100, t_pad = 100, width = 1200, height = 1200;
    auto menu = menu::construct_with_hmenu(hMenu, hWnd);

    std::cout << menu.to_string() << std::endl;

    std::thread([=]() {
      if (auto res = ui::render_target::init_global(); !res) {
        MessageBoxW(NULL, L"Failed to initialize global render target",
                    L"Error", MB_ICONERROR);
        return;
      }

      ui::render_target rt;

      if (auto res = rt.init(); !res) {
        MessageBoxW(NULL, L"Failed to initialize render target", L"Error",
                    MB_ICONERROR);
      }

      rt.set_position(x - l_pad, y - t_pad + 5);
      rt.resize(width, height);
      rt.root->emplace_child<menu_widget>(menu, l_pad, t_pad);

      nvgCreateFont(rt.nvg, "Segui", "C:\\WINDOWS\\FONTS\\msyh.ttc");

      rt.start_loop();
    }).detach();

    return NtUserTrackHook->call_trampoline<bool>(hMenu, uFlags, x, y, hWnd,
                                                  lptpm, tfunc);
    return false;
  });
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