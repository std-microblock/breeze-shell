
#include "shell.h"
#include "menu.h"
#include "ui.h"
#include <codecvt>
#include <debugapi.h>
#include <string>
#include <thread>
#include <type_traits>
#include <winuser.h>


std::string wstring_to_utf8(std::wstring const &str) {
  std::wstring_convert<
      std::conditional_t<sizeof(wchar_t) == 4, std::codecvt_utf8<wchar_t>,
                         std::codecvt_utf8_utf16<wchar_t>>>
      converter;
  return converter.to_bytes(str);
}

std::wstring utf8_to_wstring(std::string const &str) {
  std::wstring_convert<
      std::conditional_t<sizeof(wchar_t) == 4, std::codecvt_utf8<wchar_t>,
                         std::codecvt_utf8_utf16<wchar_t>>>
      converter;
  return converter.from_bytes(str);
}

namespace mb_shell {

std::string ws2s(const std::wstring &ws) { return wstring_to_utf8(ws); }

void main() {
  auto proc = blook::Process::self();
  // MessageBoxW(NULL, L"Injected", L"Info", MB_ICONINFORMATION);

  auto win32u = proc->module("win32u.dll");
  // hook NtUserTrackPopupMenu
  auto NtUserTrackPopupMenu = win32u.value()->exports("NtUserTrackPopupMenuEx");
  auto NtUserTrackHook = NtUserTrackPopupMenu->inline_hook();

  NtUserTrackHook->install([=](HMENU hMenu, uint32_t uFlags, int x, int y,
                               HWND hWnd, LPTPMPARAMS lptpm, int tfunc) {
    std::thread([=]() {
      if (auto res = ui::render_target::init_global(); !res) {
        MessageBoxW(NULL, L"Failed to initialize global render target",
                    L"Error", MB_ICONERROR);
        return;
      }
      auto menu = menu::construct_with_hmenu(hMenu);

      ui::render_target rt;

      if (auto res = rt.init(); !res) {
        MessageBoxW(NULL, L"Failed to initialize render target", L"Error",
                    MB_ICONERROR);
      }

      rt.root->emplace_child<menu_widget>(menu, 20, 20);

      nvgCreateFont(rt.nvg, "Segui", "C:\\WINDOWS\\FONTS\\msyh.ttc");

      rt.start_loop();
    }).detach();

    return NtUserTrackHook->call_trampoline<bool>(hMenu, uFlags, x, y, hWnd,
                                                  lptpm, tfunc);
  });
}
std::string menu_item::to_string() {
  if (type == type::spacer) {
    return "spacer";
  }

  std::string str = name.value_or("Unnamed");

  if (submenu) {
    str += " -> (" + submenu->to_string() + ")";
  }

  return str;
}
std::string menu::to_string() {
  std::string str;
  for (auto &item : items) {
    str += item.to_string();
  }
  return str;
}
menu menu::construct_with_hmenu(HMENU hMenu) {
  menu m;

  for (int i = 0; i < GetMenuItemCount(hMenu); i++) {
    menu_item item;
    wchar_t buffer[256];
    MENUITEMINFOW info = {sizeof(MENUITEMINFO)};
    info.fMask = MIIM_STRING | MIIM_SUBMENU | MIIM_ID | MIIM_FTYPE | MIIM_STATE | MIIM_BITMAP;
    info.dwTypeData = buffer;
    info.cch = 256;
    GetMenuItemInfoW(hMenu, i, TRUE, &info);

    if (info.hSubMenu) {
      item.submenu = construct_with_hmenu(info.hSubMenu);
    }

    if (info.fType & MFT_SEPARATOR || info.fType & MFT_MENUBARBREAK ||
        info.fType & MFT_MENUBREAK) {
      item.type = menu_item::type::spacer;
    } else {
      item.name = ws2s(buffer);
    }

    if (info.fType & MFT_BITMAP) {
      item.icon = info.hbmpItem;
    }

    m.items.push_back(item);
  }

  return m;
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