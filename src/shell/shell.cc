
#include "shell.h"
#include "menu_widget.h"
#include "ui.h"
#include "utils.h"
#include <codecvt>
#include <consoleapi.h>
#include <debugapi.h>
#include <string>
#include <thread>
#include <type_traits>
#include <winuser.h>

namespace mb_shell {
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
menu menu::construct_with_hmenu(HMENU hMenu, HWND hWnd) {
  menu m;

  for (int i = 0; i < GetMenuItemCount(hMenu); i++) {
    menu_item item;
    wchar_t buffer[256];
    MENUITEMINFOW info = {sizeof(MENUITEMINFO)};
    info.fMask = MIIM_STRING | MIIM_SUBMENU | MIIM_ID | MIIM_FTYPE |
                 MIIM_STATE | MIIM_BITMAP;
    info.dwTypeData = buffer;
    info.cch = 256;
    GetMenuItemInfoW(hMenu, i, TRUE, &info);

    if (info.hSubMenu) {
      item.submenu = construct_with_hmenu(info.hSubMenu, hWnd);
    }

    if (info.fType & MFT_SEPARATOR || info.fType & MFT_MENUBARBREAK ||
        info.fType & MFT_MENUBREAK) {
      item.type = menu_item::type::spacer;
    } else {
      item.name = wstring_to_utf8(buffer);
    }

    if (info.fType & MFT_BITMAP) {
      item.icon = info.hbmpItem;
    }

    item.action = [=]() mutable {
      std::cout << "Clicked " << item.to_string() << std::endl;
      PostMessageW(hWnd, WM_COMMAND, MAKELONG(info.wID, 0),
                   reinterpret_cast<LPARAM>(hWnd));
    };

    m.items.push_back(item);
  }

  return m;
}
} // namespace mb_shell
