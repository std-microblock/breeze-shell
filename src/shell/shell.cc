
#include "shell.h"
#include "menu_widget.h"
#include "ui.h"
#include "utils.h"
#include <codecvt>

#include "entry.h"

#include <consoleapi.h>
#include <debugapi.h>
#include <iostream>
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
    if(!GetMenuItemInfoW(hMenu, i, TRUE, &info)) {
      std::cout << "Failed to get menu item info: " << GetLastError() << std::endl;
      continue;
    }
    if (info.hSubMenu) {
    //  item.submenu = construct_with_hmenu(info.hSubMenu, hWnd);
    }

    WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), buffer, 256, NULL, NULL);

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
      menu_render::current.value()->selected_menu = info.wID;
      menu_render::current.value()->rt->close();
    };

    m.items.push_back(item);
  }

  return m;
}
} // namespace mb_shell
