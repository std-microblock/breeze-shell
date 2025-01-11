
#include "shell.h"
#include "menu_widget.h"
#include "ui.h"
#include "utils.h"
#include <algorithm>
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
    str += " -> (" + submenu.value()().to_string() + ")";
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

std::wstring strip_extra_infos(std::wstring_view str) {
  // 1. Test&C -> Test
  // 2. Test -> Test
  // 3. Test\txxx -> Test
  // 4. remove all unicode control characters

  std::wstring result;
  static const std::vector<wchar_t> whitespace_chars{
      0x9,    0xA,    0xB,    0xC,    0xD,    0x20,   0x85,   0xA0,   0x1680,
      0x2000, 0x2001, 0x2002, 0x2003, 0x2004, 0x2005, 0x2006, 0x2007, 0x2008,
      0x2009, 0x200A, 0x200B, 0x2028, 0x2029, 0x3000, 0xFEFF};

  for (int i = 0; i < str.size(); i++) {
    if (std::ranges::find(whitespace_chars, str[i]) != whitespace_chars.end()) {
      continue;
    }
    if (str[i] == '&' ||
        (str[i] == '(' && i + 1 < str.size() && str[i + 1] == '&')) {
      while(str[i] != ')' && i < str.size()) {
        i++;
      }

      continue;
    }
    if (str[i] == '\t') {
      break;
    }
    result.push_back(str[i]);
  }

  return result;
}

menu menu::construct_with_hmenu(HMENU hMenu, HWND hWnd) {
  menu m;

  for (int i = 0; i < GetMenuItemCount(hMenu); i++) {
    menu_item item;
    wchar_t buffer[256];
    MENUITEMINFOW info = {sizeof(MENUITEMINFO)};
    info.fMask = MIIM_STRING | MIIM_SUBMENU | MIIM_ID | MIIM_FTYPE |
                 MIIM_STATE | MIIM_BITMAP | MIIM_CHECKMARKS | MIIM_DATA;
    info.dwTypeData = buffer;
    info.cch = 256;
    if (!GetMenuItemInfoW(hMenu, i, TRUE, &info)) {
      std::cout << "Failed to get menu item info: " << GetLastError()
                << std::endl;
      continue;
    }
    if (info.hSubMenu) {
      item.submenu = [=]() {
        return construct_with_hmenu(info.hSubMenu, hWnd);
      };
    }

    if (info.fType & MFT_SEPARATOR || info.fType & MFT_MENUBARBREAK ||
        info.fType & MFT_MENUBREAK) {
      item.type = menu_item::type::spacer;
    } else {
      item.name = wstring_to_utf8(strip_extra_infos(buffer));
    }

    if (info.fType & MFT_BITMAP) {
      item.icon_bitmap = info.hbmpItem;
    } else if (info.hbmpChecked || info.hbmpUnchecked) {
      if (info.fState & MFS_CHECKED)
        item.icon_bitmap = info.hbmpChecked;
      else
        item.icon_bitmap = info.hbmpUnchecked;
    }

    if (info.dwItemData) {
      HBITMAP result{};

      auto is_valid_ptr = [](void *ptr) {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(ptr, &mbi, sizeof(mbi)) == sizeof(mbi)) {
          if (mbi.State == MEM_COMMIT && mbi.Protect != PAGE_NOACCESS)
            return true;
        }
        return false;
      };
      if (!IS_INTRESOURCE(info.dwItemData)) {
        for (int offset : {0x018, 0x220, 0x020, 0x000}) {
          auto pHBitmap = reinterpret_cast<HBITMAP *>(info.dwItemData + offset);
          if (!is_valid_ptr(pHBitmap)) {
            continue;
          }

          result = *pHBitmap;
          if (result && ::GetObjectType(result) == OBJ_BITMAP) {
            BITMAP bitmap{};
            if (::GetObjectW(result, sizeof(BITMAP), &bitmap) ==
                sizeof(BITMAP)) {
              auto bmWidthBytes =
                  ((bitmap.bmWidth * bitmap.bmBitsPixel + 31) / 32) * 4;
              if (bmWidthBytes == bitmap.bmWidthBytes &&
                  (bitmap.bmBitsPixel == 32 || bitmap.bmBitsPixel == 24 ||
                   bitmap.bmBitsPixel == 16 || bitmap.bmBitsPixel == 8) &&
                  bitmap.bmWidth >= 4 && bitmap.bmWidth <= 64 &&
                  bitmap.bmHeight >= 4 && bitmap.bmHeight <= 64) {
                item.icon_bitmap = result;
                break;
              }
            }
          }
        }
      }
    }
    item.action = [=]() mutable {
      menu_render::current.value()->selected_menu = info.wID;
      menu_render::current.value()->rt->close();
      std::cout << "Clicked " << info.wID << std::endl;
    };

    m.items.push_back(item);
  }

  m.parent_window = hWnd;

  return m;
}
} // namespace mb_shell
