#include "Windows.h"
#include "shlobj_core.h"

#include "hooks.h"
#include "blook/memo.h"
#include "breeze-js/quickjspp.hpp"
#include "contextmenu.h"
#include "menu_render.h"
#include "menu_widget.h"
#include "nanovg.h"
#include "shell/config.h"
#include "shell/entry.h"
#include "shell/utils.h"

#include "blook/blook.h"
#include <atlcomcli.h>
#include <atomic>
#include <mutex>
#include <memory>
#include <shobjidl_core.h>
#include <spdlog/spdlog.h>
#include <string>
#include <thread>


std::atomic_int mb_shell::context_menu_hooks::block_js_reload = 0;

static auto renderer_thread = mb_shell::task_queue{};

namespace {
std::atomic<void *> current_live_menu_handle = nullptr;
std::mutex active_root_menu_mutex;
std::weak_ptr<mb_shell::menu_widget> active_root_menu;

void *current_live_menu() {
    return current_live_menu_handle.load(std::memory_order_relaxed);
}

std::wstring strip_menu_item_text(std::wstring_view str) {
    std::wstring result;
    result.reserve(str.size());

    for (size_t i = 0; i < str.size(); i++) {
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

        if (iswcntrl(str[i])) {
            continue;
        }

        result.push_back(str[i]);
    }

    return result;
}

struct native_menu_item_identity {
    std::optional<UINT> wid;
    std::optional<std::string> name;
    std::optional<std::string> origin_name;
};

std::optional<native_menu_item_identity>
query_native_menu_item_identity(HMENU hMenu, UINT item, BOOL fByPosition) {
    MENUITEMINFOW info{sizeof(MENUITEMINFOW)};
    info.fMask = MIIM_ID | MIIM_FTYPE;
    if (!GetMenuItemInfoW(hMenu, item, fByPosition, &info)) {
        return std::nullopt;
    }

    native_menu_item_identity result;

    if (!(info.fType & MFT_SEPARATOR) && info.wID != 0) {
        result.wid = info.wID;
    }

    auto flags = fByPosition ? MF_BYPOSITION : MF_BYCOMMAND;
    auto text_length = GetMenuStringW(hMenu, item, nullptr, 0, flags);
    if (text_length > 0) {
        std::wstring buffer(text_length + 1, L'\0');
        auto copied =
            GetMenuStringW(hMenu, item, buffer.data(),
                           static_cast<int>(buffer.size()), flags);
        buffer.resize(std::max(0, copied));
        if (!buffer.empty()) {
            result.origin_name = mb_shell::wstring_to_utf8(buffer);

            auto stripped = strip_menu_item_text(buffer);
            if (!stripped.empty()) {
                result.name = mb_shell::wstring_to_utf8(stripped);
            }
        }
    }

    return result;
}

std::vector<std::shared_ptr<mb_shell::menu_item_widget>>
collect_matching_menu_items(
    const std::shared_ptr<mb_shell::menu_widget> &menu,
    const std::function<bool(const mb_shell::menu_item_widget &)> &predicate) {
    std::vector<std::shared_ptr<mb_shell::menu_item_widget>> matches;
    if (!menu) {
        return matches;
    }

    for (auto &child : menu->children) {
        auto item_widget = child->downcast<mb_shell::menu_item_widget>();
        if (item_widget && predicate(*item_widget)) {
            matches.push_back(item_widget);
        }
    }

    return matches;
}

std::shared_ptr<mb_shell::menu_item_widget>
find_menu_item_widget_by_identity(
    const std::shared_ptr<mb_shell::menu_widget> &menu,
    const native_menu_item_identity &identity) {
    if (identity.wid) {
        auto wid_matches = collect_matching_menu_items(
            menu, [&](const mb_shell::menu_item_widget &item_widget) {
                return item_widget.item.wID &&
                       item_widget.item.wID.value() == identity.wid.value();
            });
        if (wid_matches.size() == 1) {
            return wid_matches.front();
        }
    }

    const auto matches_name = [&](const mb_shell::menu_item_widget &item_widget)
        -> bool {
        if (identity.origin_name && item_widget.item.origin_name &&
            item_widget.item.origin_name.value() == identity.origin_name.value()) {
            return true;
        }

        if (identity.name && item_widget.item.name &&
            item_widget.item.name.value() == identity.name.value()) {
            return true;
        }

        if (identity.name && item_widget.item.origin_name) {
            auto stripped =
                mb_shell::wstring_to_utf8(strip_menu_item_text(
                    mb_shell::utf8_to_wstring(
                        item_widget.item.origin_name.value())));
            return stripped == identity.name.value();
        }

        return false;
    };

    auto name_matches = collect_matching_menu_items(menu, matches_name);
    if (name_matches.size() == 1) {
        return name_matches.front();
    }

    return nullptr;
}

std::shared_ptr<mb_shell::menu_widget>
find_menu_widget_by_handle(const std::shared_ptr<mb_shell::menu_widget> &menu,
                           HMENU hMenu) {
    if (!menu) {
        return nullptr;
    }

    if (menu->menu_data.native_handle == hMenu) {
        return menu;
    }

    for (auto &child : menu->children) {
        auto normal = child->downcast<mb_shell::menu_item_normal_widget>();
        if (!normal || !normal->submenu_wid) {
            continue;
        }

        if (auto found =
                find_menu_widget_by_handle(normal->submenu_wid, hMenu)) {
            return found;
        }
    }

    for (auto &submenu : menu->rendering_submenus) {
        auto sub = submenu->downcast<mb_shell::menu_widget>();
        if (!sub) {
            continue;
        }

        if (auto found = find_menu_widget_by_handle(sub, hMenu)) {
            return found;
        }
    }

    return nullptr;
}

std::shared_ptr<mb_shell::menu_widget> current_root_menu_widget() {
    if (auto root = mb_shell::context_menu_hooks::active_root_menu_widget()) {
        return root;
    }

    auto current = mb_shell::menu_render::current;
    if (!current || !(*current) || !(*current)->rt || !(*current)->rt->root) {
        return nullptr;
    }

    auto root =
        (*current)->rt->root->get_child<mb_shell::mouse_menu_widget_main>();
    if (!root) {
        return nullptr;
    }

    mb_shell::context_menu_hooks::set_active_root_menu_widget(root->menu_wid);
    return root->menu_wid;
}

bool should_warn_menu_mutation(HMENU hMenu) {
    if (!current_live_menu()) {
        return false;
    }

    if (current_live_menu() == hMenu) {
        return true;
    }

    return find_menu_widget_by_handle(current_root_menu_widget(), hMenu) !=
           nullptr;
}

std::shared_ptr<mb_shell::menu_item_widget>
find_menu_item_widget(const std::shared_ptr<mb_shell::menu_widget> &menu,
                      HMENU hMenu, UINT item, BOOL fByPosition) {
    auto target_menu = find_menu_widget_by_handle(menu, hMenu);
    if (!target_menu) {
        return nullptr;
    }

    if (fByPosition) {
        auto identity = query_native_menu_item_identity(hMenu, item, TRUE);
        if (!identity) {
            return nullptr;
        }

        return find_menu_item_widget_by_identity(target_menu, *identity);
    }

    for (auto &child : target_menu->children) {
        auto item_widget = child->downcast<mb_shell::menu_item_widget>();
        if (item_widget && item_widget->item.wID &&
            item_widget->item.wID.value() == item) {
            return item_widget;
        }
    }

    return nullptr;
}

void sync_native_menu_item_update(HMENU hMenu, UINT item, BOOL fByPosition,
                                  LPCMENUITEMINFOW mii) {
    if (!mii || !should_warn_menu_mutation(hMenu)) {
        return;
    }

    auto render = mb_shell::menu_render::current;
    if (!render || !(*render) || !(*render)->rt) {
        return;
    }

    if (!(mii->fMask & MIIM_STATE)) {
        return;
    }

    auto copied_info = *mii;
    (*render)->rt->post_loop_thread_task(
        [hMenu, item, fByPosition, copied_info]() mutable {
            auto root_menu = current_root_menu_widget();
            if (!root_menu) {
                spdlog::warn(
                    "Late SetMenuItemInfoW state update could not be applied "
                    "because the active Breeze root menu is not ready "
                    "(current_menu={}, hMenu={}, item={}, by_position={}, "
                    "fMask=0x{:x})",
                    current_live_menu(), (void *)hMenu, item, (int)fByPosition,
                    copied_info.fMask);
                return;
            }

            auto target =
                find_menu_item_widget(root_menu, hMenu, item, fByPosition);
            if (!target) {
                spdlog::warn(
                    "Late SetMenuItemInfoW state update could not be mapped to "
                    "the current Breeze menu (current_menu={}, hMenu={}, "
                    "item={}, by_position={}, fMask=0x{:x})",
                    current_live_menu(), (void *)hMenu, item, (int)fByPosition,
                    copied_info.fMask);
                return;
            }

            spdlog::info(
                "Applying late SetMenuItemInfoW state update to Breeze menu "
                "(current_menu={}, hMenu={}, item={}, by_position={}, "
                "fMask=0x{:x}, disabled={})",
                current_live_menu(), (void *)hMenu, item, (int)fByPosition,
                copied_info.fMask,
                (copied_info.fState & (MFS_DISABLED | MFS_GRAYED)) != 0);
            target->item.disabled =
                (copied_info.fState & (MFS_DISABLED | MFS_GRAYED)) != 0;
        },
        true);
}
} // namespace

#define WARN_LATE_MENU_MUTATION(API_NAME, HMENU_VALUE, FMT, ...)               \
    do {                                                                       \
        if (should_warn_menu_mutation(HMENU_VALUE)) {                          \
            spdlog::warn("Late menu mutation via " API_NAME                    \
                         " (current_menu={}, hMenu={}, " FMT ")",              \
                         current_live_menu(), (void *)(HMENU_VALUE),           \
                         __VA_ARGS__);                                         \
        }                                                                      \
    } while (false)

#define INSTALL_USER32_MENU_HOOK(HOOK_NAME, RETTYPE, PARAMS, CALL_ARGS, BODY)  \
    do {                                                                       \
        auto HOOK_NAME##Func = user32.value()->exports(#HOOK_NAME);            \
        static auto HOOK_NAME##Hook = HOOK_NAME##Func->inline_hook();          \
        HOOK_NAME##Hook->install(+[] PARAMS -> RETTYPE {                       \
            auto result = HOOK_NAME##Hook->call_trampoline<RETTYPE> CALL_ARGS; \
            BODY;                                                              \
            return result;                                                     \
        });                                                                    \
    } while (false)

void mb_shell::context_menu_hooks::set_active_root_menu_widget(
    std::shared_ptr<menu_widget> menu) {
    std::lock_guard lock(active_root_menu_mutex);
    active_root_menu = std::move(menu);
}

std::shared_ptr<mb_shell::menu_widget>
mb_shell::context_menu_hooks::active_root_menu_widget() {
    std::lock_guard lock(active_root_menu_mutex);
    return active_root_menu.lock();
}

void mb_shell::context_menu_hooks::clear_active_root_menu_widget(
    const menu_widget *expected) {
    std::lock_guard lock(active_root_menu_mutex);
    auto current = active_root_menu.lock();
    if (!expected || !current || current.get() == expected) {
        active_root_menu.reset();
    }
}

std::optional<int>
mb_shell::track_popup_menu(mb_shell::menu menu, int x, int y,
                           std::function<void(menu_render &)> on_before_show,
                           bool run_js) {
    auto thread_id_orig = GetCurrentThreadId();
    auto selected_menu_future = renderer_thread.add_task([&]() {
        try {
            struct live_menu_guard {
                explicit live_menu_guard(HMENU hMenu) {
                    current_live_menu_handle.store(hMenu,
                                                   std::memory_order_relaxed);
                    mb_shell::context_menu_hooks::clear_active_root_menu_widget();
                }
                ~live_menu_guard() {
                    mb_shell::context_menu_hooks::clear_active_root_menu_widget();
                    current_live_menu_handle.store(nullptr,
                                                   std::memory_order_relaxed);
                }
            } guard((HMENU)menu.native_handle);

            set_thread_name("breeze::context_menu_renderer");
            perf_counter perf("mb_shell::track_popup_menu");

            bool shift_pressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

            auto menu_render = menu_render::create(x, y, menu, run_js);
            menu_render.rt->last_time = menu_render.rt->clock.now();
            perf.end("menu_render::create");

            if (shift_pressed && menu_render.rt->nvg) {
                spdlog::info("Resetting font atlas due to shift key pressed");
                nvgFonsResetAtlas(menu_render.rt->nvg);
            }

            static HWND window = nullptr;
            window = (HWND)menu_render.rt->hwnd();
            // set keyboard hook to handle keyboard input
            auto hook = SetWindowsHookExW(
                WH_KEYBOARD,
                [](int nCode, WPARAM wParam, LPARAM lParam) -> LRESULT {
                    if (nCode == HC_ACTION) {
                        PostMessageW(window,
                                     lParam == WM_KEYDOWN ? WM_KEYDOWN
                                                          : WM_KEYUP,
                                     wParam, lParam);
                        return 1;
                    }
                    return CallNextHookEx(NULL, nCode, wParam, lParam);
                },
                NULL, thread_id_orig);

            SetWindowLongPtrW(window, GWL_EXSTYLE,
                              GetWindowLongPtrW(window, GWL_EXSTYLE) |
                                  WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW);
            if (on_before_show)
                on_before_show(menu_render);
            menu_render.rt->start_loop();
            UnhookWindowsHookEx(hook);

            return menu_render.selected_menu;
        } catch (std::exception &e) {
            spdlog::error("Error in track_popup_menu: {}", e.what());
            return std::optional<int>{};
        }
    });
    qjs::wait_with_msgloop([&]() { selected_menu_future.wait(); });

    auto selected_menu = selected_menu_future.get();

    return selected_menu;
}

void mb_shell::context_menu_hooks::install_NtUserTrackPopupMenuEx_hook() {
    auto proc = blook::Process::self();
    auto win32u = proc->module("win32u.dll");
    auto user32 = proc->module("user32.dll");

    auto NtUserTrackPopupMenu =
        win32u.value()->exports("NtUserTrackPopupMenuEx");
    static auto NtUserTrackHook = NtUserTrackPopupMenu->inline_hook();

    NtUserTrackHook->install(+[](HMENU hMenu, int64_t uFlags, int64_t x,
                                 int64_t y, HWND hWnd, int64_t lptpm) {
        if (GetPropW(hWnd, L"COwnerDrawPopupMenu_This") &&
            config::current->context_menu.ignore_owner_draw) {
            return NtUserTrackHook->call_trampoline<int32_t>(hMenu, uFlags, x,
                                                             y, hWnd, lptpm);
        }

        entry::main_window_loop_hook.install(hWnd);
        block_js_reload.fetch_add(1);

        perf_counter perf("TrackPopupMenuEx");
        menu menu = menu::construct_with_hmenu(hMenu, hWnd);
        perf.end("construct_with_hmenu");

        auto selected_menu = track_popup_menu(menu, x, y);
        if (selected_menu && !(uFlags & TPM_NONOTIFY)) {
            PostMessageW(hWnd, WM_COMMAND, *selected_menu, 0);
            PostMessageW(hWnd, WM_NULL, 0, 0);
        }
        mb_shell::context_menu_hooks::block_js_reload.fetch_sub(1);
        return (int32_t)selected_menu.value_or(0);
    });
}

void mb_shell::context_menu_hooks::install_menu_mutation_hooks() {
    auto proc = blook::Process::self();
    auto user32 = proc->module("user32.dll");

    INSTALL_USER32_MENU_HOOK(
        SetMenuItemInfoW, BOOL,
        (HMENU hMenu, UINT item, BOOL fByPosition, LPCMENUITEMINFOW lpmii),
        (hMenu, item, fByPosition, lpmii), {
            WARN_LATE_MENU_MUTATION(
                "SetMenuItemInfoW", hMenu,
                "item={}, by_position={}, fMask=0x{:x}, fState=0x{:x}", item,
                (int)fByPosition, lpmii ? lpmii->fMask : 0,
                lpmii ? lpmii->fState : 0);

            sync_native_menu_item_update(hMenu, item, fByPosition, lpmii);
        });

    INSTALL_USER32_MENU_HOOK(
        SetMenuItemInfoA, BOOL,
        (HMENU hMenu, UINT item, BOOL fByPosition, LPCMENUITEMINFOA lpmii),
        (hMenu, item, fByPosition, lpmii), {
            WARN_LATE_MENU_MUTATION(
                "SetMenuItemInfoA", hMenu,
                "item={}, by_position={}, fMask=0x{:x}, fState=0x{:x}", item,
                (int)fByPosition, lpmii ? lpmii->fMask : 0,
                lpmii ? lpmii->fState : 0);
            if (lpmii) {
                MENUITEMINFOW info{sizeof(MENUITEMINFOW)};
                info.fMask = lpmii->fMask;
                info.fState = lpmii->fState;
                sync_native_menu_item_update(hMenu, item, fByPosition, &info);
            }
        });

    INSTALL_USER32_MENU_HOOK(
        InsertMenuItemW, BOOL,
        (HMENU hMenu, UINT item, BOOL fByPosition, LPCMENUITEMINFOW lpmii),
        (hMenu, item, fByPosition, lpmii), {
            WARN_LATE_MENU_MUTATION("InsertMenuItemW", hMenu,
                                    "item={}, by_position={}, fMask=0x{:x}",
                                    item, (int)fByPosition,
                                    lpmii ? lpmii->fMask : 0);
        });

    INSTALL_USER32_MENU_HOOK(
        InsertMenuItemA, BOOL,
        (HMENU hMenu, UINT item, BOOL fByPosition, LPCMENUITEMINFOA lpmii),
        (hMenu, item, fByPosition, lpmii), {
            WARN_LATE_MENU_MUTATION("InsertMenuItemA", hMenu,
                                    "item={}, by_position={}, fMask=0x{:x}",
                                    item, (int)fByPosition,
                                    lpmii ? lpmii->fMask : 0);
        });

    INSTALL_USER32_MENU_HOOK(
        ModifyMenuW, BOOL,
        (HMENU hMenu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem,
         LPCWSTR lpNewItem),
        (hMenu, uPosition, uFlags, uIDNewItem, lpNewItem), {
            WARN_LATE_MENU_MUTATION("ModifyMenuW", hMenu,
                                    "position={}, flags=0x{:x}, new_id={}",
                                    uPosition, uFlags, (size_t)uIDNewItem);
        });

    INSTALL_USER32_MENU_HOOK(
        ModifyMenuA, BOOL,
        (HMENU hMenu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem,
         LPCSTR lpNewItem),
        (hMenu, uPosition, uFlags, uIDNewItem, lpNewItem), {
            WARN_LATE_MENU_MUTATION("ModifyMenuA", hMenu,
                                    "position={}, flags=0x{:x}, new_id={}",
                                    uPosition, uFlags, (size_t)uIDNewItem);
        });

    INSTALL_USER32_MENU_HOOK(
        InsertMenuW, BOOL,
        (HMENU hMenu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem,
         LPCWSTR lpNewItem),
        (hMenu, uPosition, uFlags, uIDNewItem, lpNewItem), {
            WARN_LATE_MENU_MUTATION("InsertMenuW", hMenu,
                                    "position={}, flags=0x{:x}, new_id={}",
                                    uPosition, uFlags, (size_t)uIDNewItem);
        });

    INSTALL_USER32_MENU_HOOK(
        InsertMenuA, BOOL,
        (HMENU hMenu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem,
         LPCSTR lpNewItem),
        (hMenu, uPosition, uFlags, uIDNewItem, lpNewItem), {
            WARN_LATE_MENU_MUTATION("InsertMenuA", hMenu,
                                    "position={}, flags=0x{:x}, new_id={}",
                                    uPosition, uFlags, (size_t)uIDNewItem);
        });

    INSTALL_USER32_MENU_HOOK(
        AppendMenuW, BOOL,
        (HMENU hMenu, UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem),
        (hMenu, uFlags, uIDNewItem, lpNewItem), {
            WARN_LATE_MENU_MUTATION("AppendMenuW", hMenu,
                                    "flags=0x{:x}, new_id={}", uFlags,
                                    (size_t)uIDNewItem);
        });

    INSTALL_USER32_MENU_HOOK(
        AppendMenuA, BOOL,
        (HMENU hMenu, UINT uFlags, UINT_PTR uIDNewItem, LPCSTR lpNewItem),
        (hMenu, uFlags, uIDNewItem, lpNewItem), {
            WARN_LATE_MENU_MUTATION("AppendMenuA", hMenu,
                                    "flags=0x{:x}, new_id={}", uFlags,
                                    (size_t)uIDNewItem);
        });

    INSTALL_USER32_MENU_HOOK(
        EnableMenuItem, UINT, (HMENU hMenu, UINT uIDEnableItem, UINT uEnable),
        (hMenu, uIDEnableItem, uEnable), {
            WARN_LATE_MENU_MUTATION("EnableMenuItem", hMenu,
                                    "item={}, flags=0x{:x}", uIDEnableItem,
                                    uEnable);
            if (result != static_cast<UINT>(-1)) {
                MENUITEMINFOW info{sizeof(MENUITEMINFOW)};
                info.fMask = MIIM_STATE;
                if (uEnable & (MF_DISABLED | MF_GRAYED)) {
                    info.fState = uEnable & (MFS_DISABLED | MFS_GRAYED);
                } else {
                    info.fState = MFS_ENABLED;
                }
                sync_native_menu_item_update(hMenu, uIDEnableItem,
                                             (uEnable & MF_BYPOSITION) != 0,
                                             &info);
            }
        });

    INSTALL_USER32_MENU_HOOK(
        RemoveMenu, BOOL, (HMENU hMenu, UINT uPosition, UINT uFlags),
        (hMenu, uPosition, uFlags), {
            WARN_LATE_MENU_MUTATION("RemoveMenu", hMenu,
                                    "item={}, flags=0x{:x}", uPosition, uFlags);
        });

    INSTALL_USER32_MENU_HOOK(
        DeleteMenu, BOOL, (HMENU hMenu, UINT uPosition, UINT uFlags),
        (hMenu, uPosition, uFlags), {
            WARN_LATE_MENU_MUTATION("DeleteMenu", hMenu,
                                    "item={}, flags=0x{:x}", uPosition, uFlags);
        });

    INSTALL_USER32_MENU_HOOK(
        CheckMenuItem, DWORD, (HMENU hMenu, UINT uIDCheckItem, UINT uCheck),
        (hMenu, uIDCheckItem, uCheck), {
            WARN_LATE_MENU_MUTATION("CheckMenuItem", hMenu,
                                    "item={}, flags=0x{:x}", uIDCheckItem,
                                    uCheck);
        });

    INSTALL_USER32_MENU_HOOK(
        CheckMenuRadioItem, BOOL,
        (HMENU hMenu, UINT first, UINT last, UINT check, UINT flags),
        (hMenu, first, last, check, flags), {
            WARN_LATE_MENU_MUTATION("CheckMenuRadioItem", hMenu,
                                    "first={}, last={}, check={}, flags=0x{:x}",
                                    first, last, check, flags);
        });

    INSTALL_USER32_MENU_HOOK(
        SetMenuDefaultItem, BOOL, (HMENU hMenu, UINT uItem, UINT fByPos),
        (hMenu, uItem, fByPos), {
            WARN_LATE_MENU_MUTATION("SetMenuDefaultItem", hMenu,
                                    "item={}, by_position={}", uItem, fByPos);
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
    } else {              // only version 1 is supported
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
        spdlog::error("Failed to find CreateWindowExW export");
        return;
    }
    static auto CreateWindowExWHook = CreateWindowExWFunc->inline_hook();
    CreateWindowExWHook->install(
        +[](DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName,
            DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
            HWND hWndParent, HMENU hMenu, HINSTANCE hInstance,
            LPVOID lpParam) -> HWND {
            std::wstring class_name = [&] {
                if (!lpClassName) {
                    return std::wstring(L"");
                }
                if (auto class_name = blook::Pointer((void *)lpClassName)
                                          .try_read_utf16_string()) {
                    return class_name.value();
                } else {
                    // read as registered class
                    wchar_t class_name_buffer[256];
                    if (GetClassNameW((HWND)lpClassName, class_name_buffer,
                                      256) > 0) {
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
    SHCreateDefaultContextMenuHook->install(
        +[](DEFCONTEXTMENU *def, REFIID riid, void **ppv) -> HRESULT {
            SHCreateDefaultContextMenuHook->uninstall();
            auto res = SHCreateDefaultContextMenu(def, riid, ppv);
            SHCreateDefaultContextMenuHook->install();
            if (riid != IID_IContextMenu || !ppv || !*ppv || !def ||
                !def->hwnd) {
                return res;
            }

            IContextMenu *pdcm = (IContextMenu *)(*ppv);
            if (SUCCEEDED(res) && pdcm) {
                CComPtr<IContextMenu> pCM(pdcm);

                HMENU hmenu = CreatePopupMenu();
                pCM->QueryContextMenu(hmenu, 0, 1, 0x7FFF,
                                      CMF_EXPLORE | CMF_CANRENAME);

                CComPtr<IContextMenu2> pCM2 = NULL;
                if (SUCCEEDED(pCM->QueryInterface(&pCM2))) {
                    POINT pt;
                    GetCursorPos(&pt);
                    auto hwndOwner = def->hwnd;
                    entry::main_window_loop_hook.install(hwndOwner);
                    block_js_reload.fetch_add(1);
                    perf_counter perf("TrackPopupMenuEx");
                    menu menu = menu::construct_with_hmenu(
                        hmenu, hwndOwner, true,
                        [=](int message, WPARAM wParam, LPARAM lParam) {
                            pCM2->HandleMenuMsg(message, wParam, lParam);
                        });
                    perf.end("construct_with_hmenu");

                    auto selected_menu = track_popup_menu(menu, pt.x, pt.y);
                    mb_shell::context_menu_hooks::block_js_reload.fetch_sub(1);

                    if (selected_menu) {
                        CMINVOKECOMMANDINFOEX ici = {};
                        ici.cbSize = sizeof(CMINVOKECOMMANDINFOEX);
                        ici.hwnd = hwndOwner;
                        ici.fMask = 0x100000; /* CMIC_MASK_UNICODE */
                        ici.lpVerb = MAKEINTRESOURCEA(*selected_menu - 1);
                        ici.lpVerbW = MAKEINTRESOURCEW(*selected_menu - 1);
                        ici.nShow = SW_SHOWNORMAL;

                        pCM->InvokeCommand((LPCMINVOKECOMMANDINFO)&ici);
                    }

                    close_next_create_window_exw_window = true;
                }
            }

            return res;
        });
}

#pragma optimize("", off)
extern "C" __declspec(dllexport) size_t
functionToGetGetUIObjectOfVptr(IShellFolder2 *i) {
    return i->GetUIObjectOf(nullptr, 0, nullptr, IID_IContextMenu, nullptr,
                            nullptr);
}
#pragma optimize("", on)
void mb_shell::context_menu_hooks::install_GetUIObjectOf_hook() {
    // For OneCommander

    auto proc = blook::Process::self();

    static std::atomic_bool close_next_create_window_exw_window = false;

    IShellFolder *psfDesktop = NULL;
    IShellFolder2 *psf2Desktop = NULL;
    if (NOERROR != SHGetDesktopFolder(&psfDesktop)) {
        spdlog::error("Failed to get desktop shell folder");
        return;
    }
    psfDesktop->QueryInterface(IID_IShellFolder2, (void **)&psf2Desktop);
    psfDesktop->Release();

    // patch 'call' in functionToGetGetUIObjectOfVptr to 'mov rax, ptr' to get
    // the real function called
    auto disasmFuncGetVptr =
        blook::Pointer((void *)&functionToGetGetUIObjectOfVptr)
            .range_size(0x50)
            .disassembly();

    std::optional<size_t> stackSize;
    for (auto &inst : disasmFuncGetVptr) {
        if (inst->getMnemonic() == zasm::x86::Mnemonic::Sub && !stackSize)
            stackSize = inst->getOperand(1).get<zasm::Imm>().value<size_t>();

        if (inst->getMnemonic() == zasm::x86::Mnemonic::Call) {
            inst.ptr()
                .reassembly([&](zasm::x86::Assembler a) {
                    a.mov(zasm::x86::rax, inst->getOperand(0).get<zasm::Mem>());
                    a.add(zasm::x86::rsp, *stackSize);
                    a.ret();
                })
                .patch();
            break;
        }
    }

    auto pGetUIObjectOf =
        blook::Pointer((void *)functionToGetGetUIObjectOfVptr(psf2Desktop));
    spdlog::info("GetUIObjectOf ptr: {}", pGetUIObjectOf.data());

    /**
     prototype:
HRESULT GetUIObjectOf(
  [in]      HWND                  hwndOwner,
  [in]      UINT                  cidl,
  [in]      PCUITEMID_CHILD_ARRAY apidl,
  [in]      REFIID                riid,
  [in, out] UINT                  *rgfReserved,
  [out]     void                  **ppv
);

    https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-ishellfolder-getuiobjectof
     */
    static auto GetUIObjectOfHook = pGetUIObjectOf.as_function().inline_hook();
    GetUIObjectOfHook->install(+[](void *thiz, HWND hwndOwner, UINT cidl,
                                   PCUITEMID_CHILD_ARRAY apidl, REFIID riid,
                                   UINT *rgfReserved, void **ppv) -> HRESULT {
        spdlog::info("GetUIObjectOf called");
        auto res = GetUIObjectOfHook->call_trampoline<HRESULT>(
            thiz, hwndOwner, cidl, apidl, riid, rgfReserved, ppv);

        // only proceed if requesting IContextMenu
        if (riid != IID_IContextMenu || !ppv || !*ppv) {
            return res;
        }

        spdlog::info("Upgrading context menu");
        IContextMenu *pdcm = (IContextMenu *)(*ppv);
        if (SUCCEEDED(res) && pdcm) {
            CComPtr<IContextMenu> pCM(pdcm);

            HMENU hmenu = CreatePopupMenu();
            pCM->QueryContextMenu(hmenu, 0, 1, 0x7FFF,
                                  CMF_EXPLORE | CMF_CANRENAME);

            CComPtr<IContextMenu2> pCM2 = NULL;
            if (SUCCEEDED(pCM->QueryInterface(&pCM2))) {
                POINT pt;
                GetCursorPos(&pt);
                entry::main_window_loop_hook.install(hwndOwner);
                block_js_reload.fetch_add(1);
                perf_counter perf("TrackPopupMenuEx");
                menu menu = menu::construct_with_hmenu(
                    hmenu, hwndOwner, true,
                    [=](int message, WPARAM wParam, LPARAM lParam) {
                        pCM2->HandleMenuMsg(message, wParam, lParam);
                    });
                perf.end("construct_with_hmenu");

                auto selected_menu = track_popup_menu(menu, pt.x, pt.y);
                mb_shell::context_menu_hooks::block_js_reload.fetch_sub(1);

                if (selected_menu) {
                    CMINVOKECOMMANDINFOEX ici = {};
                    ici.cbSize = sizeof(CMINVOKECOMMANDINFOEX);
                    ici.hwnd = hwndOwner;
                    ici.fMask = 0x100000; /* CMIC_MASK_UNICODE */
                    ici.lpVerb = MAKEINTRESOURCEA(*selected_menu - 1);
                    ici.lpVerbW = MAKEINTRESOURCEW(*selected_menu - 1);
                    ici.nShow = SW_SHOWNORMAL;

                    pCM->InvokeCommand((LPCMINVOKECOMMANDINFO)&ici);
                }

                close_next_create_window_exw_window = true;
            }
        }

        return res;
    });
    spdlog::info("GetUIObjectOf hook installed");
}
