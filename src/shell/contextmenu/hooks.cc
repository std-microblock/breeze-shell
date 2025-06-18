#include "./hooks.h"
#include "../config.h"
#include "contextmenu.h"
#include "menu_render.h"

#include "blook/blook.h"
#include <atlcomcli.h>
#include <shobjidl_core.h>

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

  NtUserTrackHook->install(+[](HMENU hMenu, int64_t uFlags, int64_t x,
                               int64_t y, HWND hWnd, int64_t lptpm) {
    if (GetPropW(hWnd, L"COwnerDrawPopupMenu_This") &&
        config::current->context_menu.ignore_owner_draw) {
      return NtUserTrackHook->call_trampoline<int32_t>(hMenu, uFlags, x, y,
                                                       hWnd, lptpm);
    }

    has_active_menu = true;

    perf_counter perf("TrackPopupMenuEx");
    menu menu = menu::construct_with_hmenu(hMenu, hWnd);
    perf.end("construct_with_hmenu");
    auto menu_render = menu_render::create(x, y, menu);
    menu_render.rt->last_time = menu_render.rt->clock.now();
    perf.end("menu_render::create");
    menu_render.rt->start_loop();

    has_active_menu = false;

    if (menu_render.selected_menu && !(uFlags & TPM_NONOTIFY)) {
      PostMessageW(hWnd, WM_COMMAND, *menu_render.selected_menu, 0);
      PostMessageW(hWnd, WM_NULL, 0, 0);
    }

    return (int32_t)menu_render.selected_menu.value_or(0);
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
    IContextMenu *pdcm = NULL;
    SHCreateDefaultContextMenuHook->uninstall();
    auto res =
        SHCreateDefaultContextMenu(def, riid, reinterpret_cast<void **>(&pdcm));
    SHCreateDefaultContextMenuHook->install();

    if (SUCCEEDED(res) && pdcm) {
      IContextMenu2 *pcm2 = NULL;

      int pcmType = 0;
      UpgradeContextMenu(pdcm, reinterpret_cast<void **>(&pcm2), &pcmType);

      IContextMenu* pCM(pcm2);

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
    }

    return S_FALSE;
  });
}
