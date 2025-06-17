
#include "contextmenu.h"
#include "../utils.h"
#include "menu_widget.h"
#include "ui.h"
#include <algorithm>
#include <codecvt>

#include "menu_render.h"

#include "../config.h"
#include "../res_string_loader.h"

#include "../logger.h"

#include "../entry.h"

#include <consoleapi.h>
#include <debugapi.h>
#include <future>
#include <iostream>
#include <print>
#include <ranges>
#include <string>
#include <thread>
#include <type_traits>
#include <variant>

namespace mb_shell {
owner_draw_menu_info getBitmapFromOwnerDraw(MENUITEMINFOW *menuItemInfo,
                                            HWND hwnd) {
  owner_draw_menu_info result = {{}, 0, 0};
  MEASUREITEMSTRUCT measureItem = {0};
  measureItem.CtlType = ODT_MENU;
  measureItem.CtlID = 0;
  measureItem.itemID = menuItemInfo->wID;
  measureItem.itemData = (ULONG_PTR)(menuItemInfo->dwItemData);

  SendMessageW(hwnd, WM_MEASUREITEM, 0, reinterpret_cast<LPARAM>(&measureItem));

  result.width = measureItem.itemWidth;
  result.height = measureItem.itemHeight;

  if (result.width == 0 || result.height == 0) {
    return result;
  }

  HDC hdc = GetDC(hwnd);
  if (!hdc)
    return result;

  HDC memDC = CreateCompatibleDC(hdc);
  if (!memDC) {
    ReleaseDC(hwnd, hdc);
    return result;
  }

  RECT rcItem = {0, 0, static_cast<LONG>(result.width),
                 static_cast<LONG>(result.height)};
  FillRect(memDC, &rcItem, (HBRUSH)GetStockObject(WHITE_BRUSH));

  DRAWITEMSTRUCT drawItem = {0};
  drawItem.CtlType = ODT_MENU;
  drawItem.CtlID = 0;
  drawItem.itemID = menuItemInfo->wID;
  drawItem.itemAction = ODA_DRAWENTIRE;
  drawItem.itemState = 0;
  drawItem.hwndItem = (HWND)menuItemInfo->hSubMenu;
  drawItem.hDC = memDC;
  drawItem.rcItem = rcItem;
  drawItem.itemData = (ULONG_PTR)(menuItemInfo->dwItemData);

  SendMessageW(hwnd, WM_DRAWITEM, 0,
               reinterpret_cast<LPARAM>(&drawItem)); // 发送绘制消息

  result.bitmap = CreateCompatibleBitmap(hdc, result.width, result.height);
  if (!result.bitmap) {
    DeleteDC(memDC);
    ReleaseDC(hwnd, hdc);
    return result;
  }
  return result;
}

std::wstring strip_extra_infos(std::wstring_view str) {
  // 1. Test&C -> Test
  // 2. Test -> Test
  // 3. Test\txxx -> Test
  // 4. remove all unicode control characters

  std::wstring result;

  for (int i = 0; i < str.size(); i++) {
    if (str[i] == '(' && i + 1 < str.size() && str[i + 1] == '&') {
      while (str[i] != ')' && i < str.size()) {
        i++;
      }

      continue;
    }

    if (str[i] == '&') {
      continue;
    }

    if (str[i] == '\t') {
      break;
    }
    result.push_back(str[i]);
  }

  return result;
}

std::vector<std::string> extract_hotkeys(const std::string &name) {
  std::vector<std::string> keys;

  size_t pos = 0;
  while ((pos = name.find('&', pos)) != std::string::npos) {
    if (pos + 1 < name.length()) {
      char key = name[pos + 1];
      if (key != '&') { // Skip escaped ampersands (&&)
        keys.push_back(std::string(1, key));
      }
      pos += 2;
    } else {
      pos++;
    }
  }

  return keys;
}

menu menu::construct_with_hmenu(HMENU hMenu, HWND hWnd, bool is_top) {
  menu m;

  SendMessageW(hWnd, WM_INITMENUPOPUP, reinterpret_cast<WPARAM>(hMenu),
               0xFFFFFFFF);
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

    if ((info.fType & MFT_OWNERDRAW) &&
        config::current->context_menu.experimental_ownerdraw_support) {
      auto od = getBitmapFromOwnerDraw(&info, hWnd);
      if (od.width && od.height) {
        item.owner_draw = od;
      }
    }

    if (info.fType & MFT_RADIOCHECK || info.fState & MFS_CHECKED) {
      auto c = is_light_mode() ? 0 : 1;
      if ((!item.icon_bitmap && !item.icon_svg)) {
        item.icon_svg = std::format(
            R"#(<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 16 16"><path opacity="0.7" fill="none" stroke="{}" stroke-width="2" d="M2 8l4 4 8-8"/></svg>)#",
            c ? "white" : "black");
      }
    }

    if (info.hSubMenu) {
      PostMessageW(hWnd, WM_INITMENUPOPUP,
                   reinterpret_cast<WPARAM>(info.hSubMenu), 0xFFFFFFFF);
      auto main_thread_id = GetCurrentThreadId();
      item.submenu = [=](std::shared_ptr<menu_widget> mw) {
        auto task = [&]() {
          mw->init_from_data(
              menu::construct_with_hmenu(info.hSubMenu, hWnd, false));
        };
        if (main_thread_id == GetCurrentThreadId())
          task();
        else
          entry::main_window_loop_hook.add_task(task).wait();
      };
    } else {
      item.action = [=]() mutable {
        menu_render::current.value()->selected_menu = info.wID;
        menu_render::current.value()->rt->hide_as_close();
      };

      item.wID = info.wID;
    }

    if (info.fType & MFT_SEPARATOR || info.fType & MFT_MENUBARBREAK ||
        info.fType & MFT_MENUBREAK) {
      item.type = menu_item::type::spacer;
    } else {
      item.name = wstring_to_utf8(strip_extra_infos(buffer));
      item.origin_name = wstring_to_utf8(buffer);
      if (config::current->context_menu.hotkeys) {
        auto hotkeys = extract_hotkeys(item.origin_name.value());
        if (!hotkeys.empty()) {
          item.hotkey = std::ranges::views::join_with(hotkeys, " + ") |
                        std::ranges::to<std::string>();
        }
      }

      auto id_stripped = res_string_loader::string_to_id(buffer);
      if (std::get_if<size_t>(&id_stripped)) {
        item.name_resid =
            res_string_loader::string_to_id_string(strip_extra_infos(buffer));
      } else {
        item.name_resid = res_string_loader::string_to_id_string(buffer);
      }
    }

    if (info.fType & MFT_BITMAP) {
      item.icon_bitmap = (size_t)info.hbmpItem;
    } else if (info.hbmpChecked || info.hbmpUnchecked) {
      if (info.fState & MFS_CHECKED)
        item.icon_bitmap = (size_t)info.hbmpChecked;
      else
        item.icon_bitmap = (size_t)info.hbmpUnchecked;
    }

    if (info.dwItemData) {
      HBITMAP result{};

      if (!IS_INTRESOURCE(info.dwItemData)) {
        auto offsets =
            config::current->context_menu.search_large_dwItemData_range
                ? ([]() -> std::vector<int> {
                    std::vector<int> offsets;
                    for (int i = 0; i <= 0xfff; i++) {
                      offsets.push_back(i);
                    }
                    return offsets;
                  }())
                : std::vector<int>{0x018, 0x220, 0x020, 0x000};
        for (int offset : offsets) {
          auto pHBitmap = reinterpret_cast<HBITMAP *>(info.dwItemData + offset);
          if (!is_memory_readable(pHBitmap)) {
            continue;
          }

          result = *pHBitmap;
          if (result && GetObjectType(result) == OBJ_BITMAP) {
            BITMAP bitmap{};
            if (GetObjectW(result, sizeof(BITMAP), &bitmap) == sizeof(BITMAP)) {
              auto bmWidthBytes =
                  ((bitmap.bmWidth * bitmap.bmBitsPixel + 31) / 32) * 4;
              if (bmWidthBytes == bitmap.bmWidthBytes &&
                  (bitmap.bmBitsPixel % 8 == 0 && bitmap.bmBitsPixel < 128) &&
                  4 <= bitmap.bmWidth && bitmap.bmWidth <= 64 &&
                  4 <= bitmap.bmHeight && bitmap.bmHeight <= 64 &&
                  bitmap.bmPlanes == 1 && bitmap.bmBitsPixel <= 32 &&
                  bitmap.bmBits != nullptr && bitmap.bmBits != (void *)-1) {
                item.icon_bitmap = (size_t)result;
                if (config::current->context_menu
                        .search_large_dwItemData_range) {
                  dbgout("Found icon at offset: {}", offset);
                }
                break;
              }
            }
          }
        }
      }
    }

    if (info.fState & MFS_DISABLED) {
      item.disabled = true;
    }

    m.items.push_back(item);
  }

  m.parent_window = hWnd;
  m.is_top_level = is_top;
  return m;
}
} // namespace mb_shell
