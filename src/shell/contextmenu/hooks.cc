#include "hooks.h"
#include "blook/memo.h"
#include "contextmenu.h"
#include "menu_render.h"
#include "nanovg.h"
#include "shell/config.h"
#include "shell/entry.h"
#include "breeze-js/quickjspp.hpp"

#include "blook/blook.h"
#include <atlcomcli.h>
#include <atomic>
#include <spdlog/spdlog.h>
#include <shobjidl_core.h>
#include <thread>

#include "Windows.h"
#include "shlobj_core.h"

std::atomic_int mb_shell::context_menu_hooks::block_js_reload = 0;

static auto renderer_thread = mb_shell::task_queue{};

std::optional<int>
mb_shell::track_popup_menu(mb_shell::menu menu, int x, int y,
                           std::function<void(menu_render &)> on_before_show,
                           bool run_js) {
    auto thread_id_orig = GetCurrentThreadId();
    auto selected_menu_future = renderer_thread.add_task([&]() {
        try {
            set_thread_name("breeze::context_menu_renderer");
            perf_counter perf("mb_shell::track_popup_menu");

            bool shift_pressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

            auto menu_render = menu_render::create(x, y, menu, run_js);
            menu_render.rt->last_time = menu_render.rt->clock.now();
            perf.end("menu_render::create");

            if (shift_pressed && menu_render.rt->nvg) {
                spdlog::info( "Resetting font atlas due to shift key pressed");
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
                if (auto class_name = blook::Pointer((void *)lpClassName).try_read_utf16_string()) {
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
            if (riid != IID_IContextMenu || !ppv || !*ppv || !def || !def->hwnd) {
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
    spdlog::info( "GetUIObjectOf ptr: {}", pGetUIObjectOf.data());

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
        spdlog::info( "GetUIObjectOf called");
        auto res = GetUIObjectOfHook->call_trampoline<HRESULT>(
            thiz, hwndOwner, cidl, apidl, riid, rgfReserved, ppv);

        // only proceed if requesting IContextMenu
        if (riid != IID_IContextMenu || !ppv || !*ppv) {
            return res;
        }

        spdlog::info( "Upgrading context menu");
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
    spdlog::info( "GetUIObjectOf hook installed");
}
