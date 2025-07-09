#include "./hooks.h"
#include "../config.h"
#include "../entry.h"
#include "../script/quickjspp.hpp"
#include "blook/memo.h"
#include "contextmenu.h"
#include "menu_render.h"

#include "blook/blook.h"
#include <atlcomcli.h>
#include <atomic>
#include <shobjidl_core.h>
#include <thread>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#include "shlobj_core.h"

std::atomic_bool mb_shell::context_menu_hooks::has_active_menu = false;

void mb_shell::context_menu_hooks::install_common_hook() {
  auto proc = blook::Process::self();
  auto win32u = proc->module("win32u.dll");
  auto user32 = proc->module("user32.dll");

  auto NtUserTrackPopupMenu = win32u.value()->exports("NtUserTrackPopupMenuEx");
  static auto NtUserTrackHook = NtUserTrackPopupMenu->inline_hook();

  static auto renderer = task_queue{};

  NtUserTrackHook->install(+[](HMENU hMenu, int64_t uFlags, int64_t x,
                               int64_t y, HWND hWnd, int64_t lptpm) {
    if (GetPropW(hWnd, L"COwnerDrawPopupMenu_This") &&
        config::current->context_menu.ignore_owner_draw) {
      return NtUserTrackHook->call_trampoline<int32_t>(hMenu, uFlags, x, y,
                                                       hWnd, lptpm);
    }

    entry::main_window_loop_hook.install(hWnd);
    has_active_menu = true;

    perf_counter perf("TrackPopupMenuEx");
    menu menu = menu::construct_with_hmenu(hMenu, hWnd);
    perf.end("construct_with_hmenu");

    auto thread_id_orig = GetCurrentThreadId();
    auto selected_menu_future = renderer.add_task([&]() {
      auto menu_render = menu_render::create(x, y, menu);
      menu_render.rt->last_time = menu_render.rt->clock.now();
      perf.end("menu_render::create");

      static HWND window = nullptr;
      window = (HWND)menu_render.rt->hwnd();
      // set keyboard hook to handle keyboard input
      auto hook = SetWindowsHookExW(
          WH_KEYBOARD,
          [](int nCode, WPARAM wParam, LPARAM lParam) -> LRESULT {
            if (nCode == HC_ACTION) {
              PostMessageW(window, lParam == WM_KEYDOWN ? WM_KEYDOWN : WM_KEYUP,
                           wParam, lParam);
              return 1;
            }
            return CallNextHookEx(NULL, nCode, wParam, lParam);
          },
          NULL, thread_id_orig);
        
      SetWindowLongPtrW(window, GWL_EXSTYLE,
                        GetWindowLongPtrW(window, GWL_EXSTYLE) |
                            WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW);

      menu_render.rt->start_loop();
      UnhookWindowsHookEx(hook);

      return menu_render.selected_menu;
    });
    qjs::wait_with_msgloop([&]() { selected_menu_future.wait(); });

    auto selected_menu = selected_menu_future.get();

    has_active_menu = false;

    if (selected_menu && !(uFlags & TPM_NONOTIFY)) {
      PostMessageW(hWnd, WM_COMMAND, *selected_menu, 0);
      PostMessageW(hWnd, WM_NULL, 0, 0);
    }

    return (int32_t)selected_menu.value_or(0);
  });
}

// source:
// https://stackoverflow.com/questions/61613374/how-to-use-shcreatedefaultcontextmenu-to-display-the-shell-context-menu-for-mu
// Try to convert pICv1 into IContextMenu2 or IcontextMenu3
// In case of success, release pICv1.
HRESULT UpgradeContextMenu(
    LPCONTEXTMENU pICv1, // In: The context menu version 1 to be converted.
    void **ppCMout,      // Out: The new context menu (or old one in case
                         // the convertion could not be done)
    int *pcmType)        // Out: The version number.
{
  HRESULT hr;
  // Try to get version 3 first.
  hr = pICv1->QueryInterface(IID_IContextMenu3, ppCMout);
  if (NOERROR == hr) {
    *pcmType = 3;
  } else {
    hr = pICv1->QueryInterface(IID_IContextMenu2, ppCMout);
    if (NOERROR == hr)
      *pcmType = 2;
  }

  if (*ppCMout) {
    pICv1->Release(); // free version 1
  } else {            // only version 1 is supported
    *pcmType = 1;
    *ppCMout = pICv1;
    hr = NOERROR;
  }
  return hr;
}

void mb_shell::context_menu_hooks::install_SHCreateDefaultContextMenu_hook() {
  // For OneCommander

  auto proc = blook::Process::self();
  auto shell32 = proc->module("shell32.dll");
  auto SHELL32_SHCreateDefaultContextMenu =
      shell32.value()->exports("SHELL32_SHCreateDefaultContextMenu");

  static std::atomic_bool close_next_create_window_exw_window = false;

  auto user32 = proc->module("user32.dll");
  auto CreateWindowExWFunc = user32.value()->exports("CreateWindowExW");
  if (!CreateWindowExWFunc) {
    std::cerr << "Failed to find CreateWindowExW in user32.dll" << std::endl;
  }
  static auto CreateWindowExWHook = CreateWindowExWFunc->inline_hook();
  CreateWindowExWHook->install(
      +[](DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName,
          DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent,
          HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) -> HWND {
        std::wstring class_name = [&] {
          if (!lpClassName) {
            return std::wstring(L"");
          }
          if (blook::Pointer((void *)lpClassName).try_read<int>()) {
            return std::wstring(lpClassName);
          } else {
            // read as registered class
            wchar_t class_name_buffer[256];
            if (GetClassNameW((HWND)lpClassName, class_name_buffer, 256) > 0) {
              return std::wstring(class_name_buffer);
            } else {
              return std::wstring(L"");
            }
          }
        }();

        bool should_close =
            close_next_create_window_exw_window &&
            class_name.starts_with(L"HwndWrapper[OneCommander.exe");

        if (should_close) {
          dwStyle &= ~WS_VISIBLE;
        }

        auto res = CreateWindowExWHook->call_trampoline<HWND>(
            dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth,
            nHeight, hWndParent, hMenu, hInstance, lpParam);
        if (res && should_close) {
          close_next_create_window_exw_window = false;
          PostMessageW(res, WM_CLOSE, 0, 0);
          CloseWindow(res);
        }
        return res;
      });

  /**
   prototype: SHSTDAPI SHCreateDefaultContextMenu(
    [in]  const DEFCONTEXTMENU *pdcm,
          REFIID               riid,
    [out] void                 **ppv
  );

  https://learn.microsoft.com/zh-cn/windows/win32/api/shlobj_core/nf-shlobj_core-shcreatedefaultcontextmenu
   */
  static auto SHCreateDefaultContextMenuHook =
      SHELL32_SHCreateDefaultContextMenu->inline_hook();
  SHCreateDefaultContextMenuHook->install(+[](DEFCONTEXTMENU *def, REFIID riid,
                                              void **ppv) -> HRESULT {
    SHCreateDefaultContextMenuHook->uninstall();
    auto res = SHCreateDefaultContextMenu(def, riid, ppv);
    SHCreateDefaultContextMenuHook->install();

    IContextMenu *pdcm = (IContextMenu *)*ppv;
    if (SUCCEEDED(res) && pdcm) {
      IContextMenu2 *pcm2 = NULL;

      int pcmType = 0;
      UpgradeContextMenu(pdcm, reinterpret_cast<void **>(&pcm2), &pcmType);

      IContextMenu *pCM(pcm2);

      HMENU hmenu = CreatePopupMenu();
      pCM->QueryContextMenu(hmenu, 0, 1, 0x7FFF, CMF_EXPLORE | CMF_CANRENAME);

      POINT pt;
      GetCursorPos(&pt);

      int res = TrackPopupMenuEx(
          hmenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y,
          def->hwnd, NULL);

      if (res > 0) {
        CMINVOKECOMMANDINFOEX ici = {};
        ici.cbSize = sizeof(CMINVOKECOMMANDINFOEX);
        ici.hwnd = def->hwnd;
        ici.fMask = CMIC_MASK_UNICODE;
        ici.lpVerb = MAKEINTRESOURCEA(res - 1);
        ici.lpVerbW = MAKEINTRESOURCEW(res - 1);
        ici.nShow = SW_SHOWNORMAL;

        pCM->InvokeCommand((LPCMINVOKECOMMANDINFO)&ici);
      }

      close_next_create_window_exw_window = true;
    }

    return res;
  });
}
