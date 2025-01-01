#include "blook/blook.h"

#include <ranges>
#include <string>
#include <string_view>

#include <Windows.h>

BOOL WINAPI NtUserTrackPopupMenu(HMENU hMenu, uint32_t uFlags, int x, int y, HWND hWnd, LPTPMPARAMS lptpm, int tfunc);

namespace mb_shell {
void main() {
  MessageBoxA(NULL, "Hello, World!", "mb_shell", MB_OK);
  auto proc = blook::Process::self();
  auto win32u = proc->module("win32u.dll");
  // hook NtUserTrackPopupMenu
  auto NtUserTrackPopupMenu = win32u.value()->exports("NtUserTrackPopupMenu");
  auto NtUserTrackHook = NtUserTrackPopupMenu->inline_hook();

  NtUserTrackHook->install([=](HMENU hMenu, uint32_t uFlags, int x, int y, HWND hWnd, LPTPMPARAMS lptpm, int tfunc) {
    MessageBoxA(NULL, "NtUserTrackPopupMenu", "mb_shell", MB_OK);
    return NtUserTrackHook->call_trampoline<bool>(hMenu, uFlags, x, y, hWnd, lptpm, tfunc);
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