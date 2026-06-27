
#include "contextmenu.h"
#include "breeze_ui/ui.h"
#include "menu_widget.h"
#include "shell/utils.h"
#include <algorithm>
#include <codecvt>

#include "menu_render.h"

#include "shell/config.h"
#include "shell/res_string_loader.h"

#include "shell/logger.h"

#include "shell/entry.h"

#include <consoleapi.h>
#include <debugapi.h>
#include <future>
#include <iostream>
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <GLFW/glfw3.h>
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

    SendMessageW(hwnd, WM_MEASUREITEM, 0,
                 reinterpret_cast<LPARAM>(&measureItem));

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
    DeleteDC(memDC);
    ReleaseDC(hwnd, hdc);
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
            while (i < str.size() && str[i] != ')') {
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

static std::vector<int> pre_parse_hotkeys(const std::string &hotkey_str) {
    static const auto translate_map =
        std::unordered_map<std::string, int>{
            {"ctrl", GLFW_KEY_LEFT_CONTROL},
            {"shift", GLFW_KEY_LEFT_SHIFT},
            {"alt", GLFW_KEY_LEFT_ALT},
            {"win", GLFW_KEY_LEFT_SUPER},
            {"a", GLFW_KEY_A},
            {"b", GLFW_KEY_B},
            {"c", GLFW_KEY_C},
            {"d", GLFW_KEY_D},
            {"e", GLFW_KEY_E},
            {"f", GLFW_KEY_F},
            {"g", GLFW_KEY_G},
            {"h", GLFW_KEY_H},
            {"i", GLFW_KEY_I},
            {"j", GLFW_KEY_J},
            {"k", GLFW_KEY_K},
            {"l", GLFW_KEY_L},
            {"m", GLFW_KEY_M},
            {"n", GLFW_KEY_N},
            {"o", GLFW_KEY_O},
            {"p", GLFW_KEY_P},
            {"q", GLFW_KEY_Q},
            {"r", GLFW_KEY_R},
            {"s", GLFW_KEY_S},
            {"t", GLFW_KEY_T},
            {"u", GLFW_KEY_U},
            {"v", GLFW_KEY_V},
            {"w", GLFW_KEY_W},
            {"x", GLFW_KEY_X},
            {"y", GLFW_KEY_Y},
            {"z", GLFW_KEY_Z},
            {"0", GLFW_KEY_0},
            {"1", GLFW_KEY_1},
            {"2", GLFW_KEY_2},
            {"3", GLFW_KEY_3},
            {"4", GLFW_KEY_4},
            {"5", GLFW_KEY_5},
            {"6", GLFW_KEY_6},
            {"7", GLFW_KEY_7},
            {"8", GLFW_KEY_8},
            {"9", GLFW_KEY_9},
        };

    std::vector<int> result;
    size_t start = 0;
    while (start < hotkey_str.size()) {
        auto end = hotkey_str.find('+', start);
        if (end == std::string::npos)
            end = hotkey_str.size();
        auto key_str = hotkey_str.substr(start, end - start);
        key_str.erase(key_str.find_last_not_of(" \t\n\r") + 1);
        key_str.erase(0, key_str.find_first_not_of(" \t\n\r"));
        for (auto &c : key_str)
            c = std::tolower(static_cast<unsigned char>(c));
        if (auto it = translate_map.find(key_str); it != translate_map.end())
            result.push_back(it->second);
        else
            return {};
        start = end + 1;
    }
    return result;
}

menu menu::construct_with_hmenu(
    HMENU hMenu, HWND hWnd, bool is_top,
    std::function<void(int, WPARAM, LPARAM)> HandleMenuMsg,
    LPARAM init_popup_lparam) {
    menu m;

    if (!HandleMenuMsg)
        HandleMenuMsg = [=](int message, WPARAM wParam, LPARAM lParam) {
            SendMessageW(hWnd, message, wParam, lParam);
        };

    HandleMenuMsg(WM_INITMENUPOPUP, reinterpret_cast<WPARAM>(hMenu),
                  init_popup_lparam);
    for (int i = 0; i < GetMenuItemCount(hMenu); i++) {
        menu_item item;
        wchar_t buffer[256] = {};
        MENUITEMINFOW info = {sizeof(MENUITEMINFO)};
        info.fMask = MIIM_STRING | MIIM_SUBMENU | MIIM_ID | MIIM_FTYPE |
                     MIIM_STATE | MIIM_BITMAP | MIIM_CHECKMARKS | MIIM_DATA;
        info.dwTypeData = buffer;
        info.cch = 256;
        if (!GetMenuItemInfoW(hMenu, i, TRUE, &info)) {
            spdlog::warn( "Failed to get menu item info: %lu", GetLastError());
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
                item.icon_svg = fmt::format(
                    R"#(<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 16 16"><path opacity="0.7" fill="none" stroke="{}" stroke-width="2" d="M2 8l4 4 8-8"/></svg>)#",
                    c ? "white" : "black");
            }
        }

        bool item_handled = false;
        if (info.hSubMenu) {
            bool flatten = false;
            if (config::current->context_menu.flatten_open_with_submenu) {
                std::wstring lower(buffer);
                for (auto &c : lower)
                    c = towlower(c);
                if (lower.find(L"open with") != std::wstring::npos ||
                    lower.find(L"打开方式") != std::wstring::npos) {
                    flatten = true;
                }
            }

            if (flatten) {
                auto sub = menu::construct_with_hmenu(
                    info.hSubMenu, hWnd, false, HandleMenuMsg,
                    MAKELPARAM(static_cast<WORD>(i), 0));
                for (auto &sub_item : sub.items)
                    m.items.push_back(std::move(sub_item));
                menu_item sep;
                sep.type = menu_item::type::spacer;
                m.items.push_back(std::move(sep));
                item_handled = true;
            } else {
                auto main_thread_id = GetCurrentThreadId();
                int submenu_pos = i;
                item.submenu = [=](std::shared_ptr<menu_widget> mw) {
                    auto task = [&]() {
                        mw->init_from_data(menu::construct_with_hmenu(
                            info.hSubMenu, hWnd, false, HandleMenuMsg,
                            MAKELPARAM(static_cast<WORD>(submenu_pos), 0)));
                    };
                    if (main_thread_id == GetCurrentThreadId())
                        task();
                    else
                        entry::main_window_loop_hook.add_task(task).wait();
                };
            }
        } else {
            item.action = [wID = info.wID]() mutable {
                auto current_render = menu_render::current;
                if (!current_render || !*current_render) {
                    spdlog::warn("menu_render::current is null when action fires");
                    return;
                }
                (*current_render)->selected_menu = wID;
                (*current_render)->rt->hide_as_close();
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
                    auto hotkey_str =
                        std::ranges::views::join_with(hotkeys, " + ") |
                        std::ranges::to<std::string>();
                    item.hotkey = hotkey_str;
                    item.parsed_hotkeys = pre_parse_hotkeys(hotkey_str);
                }
            }

            auto id_stripped = res_string_loader::string_to_id(buffer);
            if (std::get_if<size_t>(&id_stripped)) {
                item.name_resid = res_string_loader::string_to_id_string(
                    strip_extra_infos(buffer));
            } else {
                item.name_resid =
                    res_string_loader::string_to_id_string(buffer);
            }
        }

        if (info.hbmpItem) {
            item.icon_bitmap = (size_t)info.hbmpItem;
        } else if (info.hbmpChecked || info.hbmpUnchecked) {
            if (info.fState & MFS_CHECKED)
                item.icon_bitmap = (size_t)info.hbmpChecked;
            else
                item.icon_bitmap = (size_t)info.hbmpUnchecked;
        } else if (info.dwItemData) {
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
                        : std::vector<int>{0x018, 0x220, 0x020, 0x000, 0x010, 0x030, 0x040};
                for (int offset : offsets) {
                    auto pHBitmap =
                        reinterpret_cast<HBITMAP *>(info.dwItemData + offset);
                    if (!is_memory_readable(pHBitmap)) {
                        continue;
                    }

                    result = *pHBitmap;
                    if (result && GetObjectType(result) == OBJ_BITMAP) {
                        BITMAP bitmap{};
                        if (GetObjectW(result, sizeof(BITMAP), &bitmap) ==
                            sizeof(BITMAP)) {
                            auto bmWidthBytes =
                                ((bitmap.bmWidth * bitmap.bmBitsPixel + 31) /
                                 32) *
                                4;
                            if (bmWidthBytes == bitmap.bmWidthBytes &&
                                (bitmap.bmBitsPixel % 8 == 0 &&
                                 bitmap.bmBitsPixel < 128) &&
                                4 <= bitmap.bmWidth && bitmap.bmWidth <= 64 &&
                                4 <= bitmap.bmHeight && bitmap.bmHeight <= 64 &&
                                bitmap.bmPlanes == 1 &&
                                bitmap.bmBitsPixel <= 32 &&
                                bitmap.bmBits != nullptr &&
                                bitmap.bmBits != (void *)-1) {
                                item.icon_bitmap = (size_t)result;
                                if (config::current->context_menu
                                        .search_large_dwItemData_range) {
                                    spdlog::info("Found icon at offset: {}", offset);
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

        if (!item_handled)
            m.items.push_back(item);
    }

    m.parent_window = hWnd;
    m.native_handle = hMenu;
    m.is_top_level = is_top;
    return m;
}
} // namespace mb_shell
