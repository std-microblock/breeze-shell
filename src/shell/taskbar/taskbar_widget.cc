#include "taskbar_widget.h"
#include "async_simple/Promise.h"
#include "breeze_ui/nanovg_wrapper.h"
#include <atlcomcli.h>
#include <thread>
#include <uiautomation.h> // Added for UI Automation
#include <unordered_map>

#include <shellapi.h>

#include <shobjidl.h>
#include <unordered_set>
#include <windows.h>

#include "cinatra/coro_http_client.hpp"
#include "shell/entry.h"

namespace mb_shell::taskbar {

// https://stackoverflow.com/questions/2397578/how-to-get-the-executable-name-of-a-window
static HWND GetLastVisibleActivePopUpOfWindow(HWND window) {
    HWND lastPopUp = GetLastActivePopup(window);

    if (IsWindowVisible(lastPopUp))
        return lastPopUp;
    else if (lastPopUp == window)
        return nullptr;
    else
        return GetLastVisibleActivePopUpOfWindow(lastPopUp);
}

static bool KeepWindowHandleInAltTabList(HWND window) {
    if (window == GetShellWindow()) // Desktop
        return false;

    if (!IsWindowVisible(window))
        return false;

    // Walk up owner chain to find root owner
    HWND root = GetAncestor(window, GA_ROOTOWNER);

    if (GetLastVisibleActivePopUpOfWindow(root) == window) {
        // Get window class name for filtering
        wchar_t class_name[256];
        GetClassNameW(window, class_name, sizeof(class_name) / sizeof(wchar_t));
        std::wstring className(class_name);

        // Filter out system windows
        if (className == L"Shell_TrayWnd" ||              // Windows taskbar
            className == L"DV2ControlHost" ||             // Windows startmenu
            className == L"MsgrIMEWindowClass" ||         // Live messenger
            className == L"SysShadow" ||                  // Shadow windows
            className.find(L"WMP9MediaBarFlyout") == 0) { // WMP toolbar
            return false;
        }

        // Check for Start button
        if (className == L"Button") {
            wchar_t window_text[256];
            GetWindowTextW(window, window_text,
                           sizeof(window_text) / sizeof(wchar_t));
            if (std::wstring(window_text) == L"Start") {
                return false;
            }
        }

        // Check window style
        LONG_PTR style = GetWindowLongPtr(window, GWL_STYLE);
        if (style & WS_EX_TOOLWINDOW) {
            return false;
        }

        return true;
    }

    return false;
}

namespace IconExtractor {
static auto GetIconBitmapInfo(HICON hIcon)
    -> std::expected<std::tuple<ICONINFO, BITMAP, BITMAP>, DWORD> {
    ICONINFO iconInfo;
    if (!GetIconInfo(hIcon, &iconInfo)) {
        return std::unexpected(GetLastError());
    }

    BITMAP bmpColor;
    BITMAP bmpMask;

    if (!GetObject(iconInfo.hbmColor, sizeof(BITMAP), &bmpColor) ||
        !GetObject(iconInfo.hbmMask, sizeof(BITMAP), &bmpMask)) {
        return std::unexpected(GetLastError());
    }

    return std::make_tuple(iconInfo, bmpColor, bmpMask);
}

static auto GetBitmapBytes(const BITMAP &bmp) -> size_t {
    auto widthBytes = bmp.bmWidthBytes;
    if (widthBytes & 3) {
        widthBytes = (widthBytes + 4) & ~3;
    }
    return widthBytes * bmp.bmHeight;
}

static auto ExtractBitmapData(HBITMAP hBitmap) -> std::vector<std::uint8_t> {
    BITMAP bmp;
    if (!GetObject(hBitmap, sizeof(BITMAP), &bmp)) {
        auto error = GetLastError();
        std::print("Failed to get bitmap object: {}\n", error);
        throw std::runtime_error("Failed to get bitmap object");
    }

    const auto bitmapBytes = GetBitmapBytes(bmp);
    std::vector<std::uint8_t> iconData(bitmapBytes);

    GetBitmapBits(hBitmap, bitmapBytes, iconData.data());
    return iconData;
}

struct IconData {
    std::vector<std::uint8_t> rgbaData;
    int width, height;
};

static auto GetIcon(HICON hIcon) -> IconData {
    auto [iconInfo, bmpColor, bmpMask] = GetIconBitmapInfo(hIcon).value();

    std::vector<std::uint8_t> result;

    auto colorData = ExtractBitmapData(iconInfo.hbmColor);

    result.resize(bmpColor.bmWidth * bmpColor.bmHeight * 4);

    for (int y = 0; y < bmpColor.bmHeight; ++y) {
        for (int x = 0; x < bmpColor.bmWidth; ++x) {
            int colorIndex = (y * bmpColor.bmWidth + x) * 4;
            int resultIndex = (y * bmpColor.bmWidth + x) * 4;

            uint8_t r = colorData[colorIndex + 2];
            uint8_t g = colorData[colorIndex + 1];
            uint8_t b = colorData[colorIndex + 0];
            uint8_t a = colorData[colorIndex + 3];
            result[resultIndex + 0] = r;
            result[resultIndex + 1] = g;
            result[resultIndex + 2] = b;
            result[resultIndex + 3] = a;
        }
    }

    DeleteObject(iconInfo.hbmColor);
    DeleteObject(iconInfo.hbmMask);
    return IconData{
        .rgbaData = std::move(result),
        .width = bmpColor.bmWidth,
        .height = bmpColor.bmHeight,
    };
}
}; // namespace IconExtractor

struct window_info {
    HWND hwnd;
    std::string title;
    std::string class_name;
    HICON icon_handle;

    bool operator==(const window_info &other) const {
        return hwnd == other.hwnd;
    }

    async_simple::coro::Lazy<HICON> get_async_icon_cached();
    async_simple::coro::Lazy<HICON> get_async_icon();
};

struct window_stack_info {
    std::vector<window_info> windows;
    bool active() {
        return !windows.empty() &&
               std::ranges::any_of(windows, [](const window_info &win) {
                   return win.hwnd == GetForegroundWindow();
               });
    }

    // if there are at least one window same in both stacks, they are same stack
    bool is_same(const window_stack_info &other) const {
        if (windows.empty() || other.windows.empty()) {
            return false;
        }

        for (const auto &win : windows) {
            if (std::find(other.windows.begin(), other.windows.end(), win) !=
                other.windows.end()) {
                return true;
            }
        }
        return false;
    }
};

std::vector<window_info> get_window_list();
std::vector<window_stack_info> get_window_stacks();

struct app_list_stack_widget : public ui::widget {
    window_stack_info stack;
    bool active = false;
    std::optional<ui::NVGImage> icon;

    ui::sp_anim_float active_indicator_width = anim_float(),
                      active_indicator_opacity = anim_float();
    ui::animated_color bg_color = {this, 0.1f, 0.1f, 0.1f, 0.8f};
    app_list_stack_widget(const window_stack_info &stack) : stack(stack) {
        config::current->taskbar.theme.animation.bg_color.apply_to(bg_color);
        config::current->taskbar.theme.animation.active_indicator.apply_to(
            active_indicator_width);
        config::current->taskbar.theme.animation.active_indicator.apply_to(
            active_indicator_opacity);
    }

    void render(ui::nanovg_context ctx) override {
        if (stack.windows.empty()) {
            return;
        }

        ctx.fillColor(bg_color);
        static constexpr auto margin = 4;
        ctx.fillRoundedRect(*x + margin, *y + margin, *width - margin * 2,
                            *height - margin * 2, 6);

        auto &first_window = stack.windows.front();
        if (first_window.icon_handle) {
            if (!icon) {
                if (!IconExtractor::GetIconBitmapInfo(
                        first_window.icon_handle)) {
                    first_window.icon_handle = nullptr;
                    return;
                }
                auto rgba = IconExtractor::GetIcon(first_window.icon_handle);
                icon =
                    ui::NVGImage{ctx.createImageRGBA(rgba.width, rgba.height, 0,
                                                     rgba.rgbaData.data()),
                                 rgba.width, rgba.height, ctx};
            }

            if (icon) {
                ctx.drawImage(*icon, *x + 8, *y + 8, *width - 16, *height - 16);
            }
        }

        // active indicator
        ctx.fillColor({1.0f, 1.0f, 1.0f, *active_indicator_opacity});
        ctx.fillRoundedRect(*x + (*width - *active_indicator_width) / 2,
                            *y + *height - 4, *active_indicator_width, 3, 1.5f);
    }

    void update_stack(const window_stack_info &new_stack) {
        stack = new_stack;

        // Reset icon to reload it next time
        icon.reset();
    }

    void update(ui::update_context &ctx) override {
        ui::widget::update(ctx);
        width->reset_to(40);
        height->reset_to(40);

        if (ctx.mouse_down_on(this)) {
            bg_color.animate_to({0.2f, 0.2f, 0.2f, 0.5f});
        } else if (ctx.hovered(this)) {
            bg_color.animate_to({0.3f, 0.3f, 0.3f, 0.5f});
        } else {
            bg_color.animate_to({0.1f, 0.1f, 0.1f, 0});
        }
        // active predicator
        if (active) {
            active_indicator_width->animate_to(15);
            active_indicator_opacity->animate_to(0.7);
        } else {
            active_indicator_width->animate_to(5);
            active_indicator_opacity->animate_to(0.3);
        }

        if (ctx.mouse_clicked_on(this)) {
            HWND hWnd = stack.windows.front().hwnd;
            bool isForeground = active;
            bool isMinimized = IsIconic(hWnd);

            if (isMinimized) {
                ShowWindow(hWnd, SW_RESTORE);
                SetForegroundWindow(hWnd);
            } else if (isForeground) {
                ShowWindow(hWnd, SW_MINIMIZE);
            } else {
                SetForegroundWindow(hWnd);
                ShowWindow(hWnd, SW_SHOW);
            }
        }
    }
};

struct app_list_widget : public ui::widget_flex {
    using super = ui::widget_flex;
    std::vector<
        std::pair<std::shared_ptr<app_list_stack_widget>, window_stack_info>>
        stacks;

    background_widget bg;

    app_list_widget() : super(), bg(true) {
        horizontal = true;
        gap = 2;
    }

    void update_stacks() {
        auto new_stacks = get_window_stacks();

        // Mark which existing stacks have been matched
        std::vector<bool> matched(stacks.size(), false);

        // firstly, try to match existing stacks with new ones
        for (auto &new_stack : new_stacks) {
            auto it = std::find_if(stacks.begin(), stacks.end(),
                                   [&new_stack](const auto &pair) {
                                       return pair.second.is_same(new_stack);
                                   });
            if (it != stacks.end()) {
                // Mark this existing stack as matched
                matched[std::distance(stacks.begin(), it)] = true;
                it->second = new_stack;
                it->first->update_stack(new_stack);
            } else {
                auto widget =
                    std::make_shared<app_list_stack_widget>(new_stack);
                stacks.emplace_back(widget, new_stack);
                matched.push_back(true); // New stack is automatically matched
                if (!new_stack.windows.empty()) {
                    new_stack.windows[0].get_async_icon_cached().start(
                        [=](async_simple::Try<HICON> ico) mutable {
                            if (ico.available() && ico.value() != nullptr) {
                                widget->stack.windows[0].icon_handle = ico.value();
                                widget->icon.reset();
                            }
                        });
                }
                add_child(widget);
            }
        }

        // Remove unmatched stacks
        for (int i = matched.size() - 1; i >= 0; --i) {
            if (!matched[i]) {
                remove_child(stacks[i].first);
                stacks.erase(stacks.begin() + i);
            }
        }
    }

    void update_active_stacks() {
        if (GetForegroundWindow() == owner_rt->hwnd() ||
            GetForegroundWindow() == 0)
            return;
        for (auto &pair : stacks) {
            pair.first->active = pair.second.active();
        }
    }

    void update(ui::update_context &ctx) override {
        super::update(ctx);
        update_active_stacks();
        bg.width->reset_to(*width);
        bg.height->reset_to(*height);
        bg.x->reset_to(*x);
        bg.y->reset_to(*y);
        bg.update(ctx);
    }

    void render(ui::nanovg_context ctx) override {
        bg.render(ctx);
        super::render(ctx);
    }
};

struct windows_button_widget : public background_widget {
    using super = background_widget;
    windows_button_widget() : background_widget(false) {}
    int is_windows_menu_open = false;
    bool should_ignore_next_click = false;
    std::optional<ui::NVGImage> icon;
    ui::animated_color bg_color = {this, 0.1f, 0.1f, 0.1f, 0.8f};
    void render(ui::nanovg_context ctx) override {
        super::render(ctx);
        constexpr auto padding = 10;

        if (!icon) {
            static auto svg_icon_windows =
                R"#(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 4875 4875"><path fill="#fff" d="M0 0h2311v2310H0zm2564 0h2311v2310H2564zM0 2564h2311v2311H0zm2564 0h2311v2311H2564"/></svg>)#";
            ui::nanovg_context::NSVGimageRAII svg =
                nsvgParse(std::string(svg_icon_windows).data(), "px", 96);
            icon = ctx.imageFromSVG(svg.image, ctx.rt->dpi_scale);
        }

        ctx.fillColor(bg_color.nvg());
        ctx.fillRoundedRect(*x, *y, *width, *height, 6);
        ctx.fillColor(nvgRGBAf(1, 1, 1, 1));
        ctx.drawImage(*icon, *x + padding, *y + padding, *width - padding * 2,
                      *height - padding * 2);
    }

    void update(ui::update_context &ctx) override {
        super::update(ctx);
        bool last_is_windows_menu_open = is_windows_menu_open;
        static ATL::CComPtr<IAppVisibility> appVisibility = nullptr;
        if (!appVisibility) {
            HRESULT hr = CoCreateInstance(CLSID_AppVisibility, nullptr,
                                          CLSCTX_INPROC_SERVER,
                                          IID_PPV_ARGS(&appVisibility));
            if (FAILED(hr)) {
                std::cerr << "Failed to create AppVisibility instance: "
                          << std::hex << hr << std::endl;
                return;
            }
        }
        appVisibility->IsLauncherVisible(&is_windows_menu_open);

        should_ignore_next_click =
            (last_is_windows_menu_open && ctx.mouse_down_on(this)) ||
            should_ignore_next_click;

        if (should_ignore_next_click && ctx.mouse_clicked) {
            should_ignore_next_click = false;
            return;
        }

        if (last_is_windows_menu_open) {
            bg_color.animate_to({0.2f, 0.2f, 0.2f, 0.5f});
            return;
        }

        if (ctx.mouse_down_on(this)) {
            bg_color.animate_to({0.3f, 0.3f, 0.3f, 0.5f});
        } else if (ctx.hovered(this)) {
            bg_color.animate_to({0.2f, 0.2f, 0.2f, 0.5f});
        } else {
            bg_color.animate_to({0.1f, 0.1f, 0.1f, 0});
        }

        if (ctx.mouse_clicked_on(this)) {
            INPUT input = {};
            input.type = INPUT_KEYBOARD;
            input.ki.wVk = VK_LWIN; // Left Windows key
            SendInput(1, &input, sizeof(INPUT));
            input.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &input, sizeof(INPUT));
        }
    }
};

std::unordered_set<taskbar_widget *> taskbar_widgets;
void init_polling_thread() {
    std::thread([]() {
        while (true) {
            int cnt = 0;
            EnumWindows(
                [](HWND hwnd, LPARAM lParam) -> BOOL {
                    auto *count = reinterpret_cast<int *>(lParam);
                    (*count)++;
                    return TRUE;
                },
                reinterpret_cast<LPARAM>(&cnt));

            static auto last_win_cnt = 0;
            if (cnt != last_win_cnt) {
                last_win_cnt = cnt;
                for (auto *widget : taskbar_widgets) {
                    widget->get_child<app_list_widget>()->update_stacks();
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }).detach();
}

void ensure_polling_thread_initialized() {
    static bool initialized = false;
    if (!initialized) {
        init_polling_thread();
    }
}

taskbar_widget::taskbar_widget() {
    horizontal = true;
    gap = 7;
    auto left_padding = emplace_child<ui::rect_widget>();
    left_padding->width->reset_to(5);
    left_padding->height->reset_to(0);

    auto btn_windows = emplace_child<windows_button_widget>();
    btn_windows->width->reset_to(40);
    btn_windows->height->reset_to(40);

    auto app_list = emplace_child<app_list_widget>();
    app_list->update_stacks();
    taskbar_widgets.insert(this);
    ensure_polling_thread_initialized();
}

taskbar_widget::~taskbar_widget() { taskbar_widgets.erase(this); }

std::vector<window_info> get_window_list() {
    static std::mutex lock;
    std::lock_guard<std::mutex> guard(lock);
    std::vector<window_info> windows;

    EnumWindows(
        [](HWND hwnd, LPARAM lParam) -> BOOL {
            auto *window_list =
                reinterpret_cast<std::vector<window_info> *>(lParam);

            if (KeepWindowHandleInAltTabList(hwnd)) {
                window_info info;
                info.hwnd = hwnd;
                int title_length = GetWindowTextLengthW(hwnd);
                if (title_length > 0) {
                    std::wstring title(title_length + 1, L'\0');
                    GetWindowTextW(hwnd, title.data(), title_length + 1);
                    info.title = wstring_to_utf8(title);
                }

                wchar_t class_name[256];
                GetClassNameW(hwnd, class_name,
                              sizeof(class_name) / sizeof(wchar_t));
                info.class_name = wstring_to_utf8(class_name);

                info.icon_handle = (HICON)GetClassLongPtrW(hwnd, GCLP_HICON);
                if (info.icon_handle == nullptr) {
                    info.icon_handle =
                        (HICON)GetClassLongPtrW(hwnd, GCLP_HICONSM);
                }

                if (info.icon_handle == nullptr) {
                    // use the exe icon if nothing else is available
                    int pid;

                    GetWindowThreadProcessId(hwnd, (LPDWORD)&pid);
                    HANDLE hProcess =
                        OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                                    FALSE, pid);
                    if (hProcess) {
                        DWORD size = MAX_PATH;
                        std::wstring exe_path(MAX_PATH + 2, L'\0');
                        if (QueryFullProcessImageNameW(
                                (HMODULE)hProcess, 0, exe_path.data(), &size)) {
                            info.icon_handle =
                                ExtractIconW(nullptr, exe_path.c_str(), 0);
                        }
                        CloseHandle(hProcess);
                    }
                }

                if (info.icon_handle == nullptr) {
                    // if still no icon, use default icon
                    info.icon_handle = LoadIcon(nullptr, IDI_APPLICATION);
                }

                window_list->push_back(info);
            }

            return TRUE;
        },
        reinterpret_cast<LPARAM>(&windows));

    return windows;
}

std::vector<window_stack_info> get_window_stacks() {
    std::vector<window_stack_info> stacks;
    auto windows = get_window_list();

    std::unordered_map<std::string, window_stack_info> stack_map;
    for (const auto &win : windows) {
        int pid;
        if (GetWindowThreadProcessId(win.hwnd, (LPDWORD)&pid)) {

            HANDLE hProcess = OpenProcess(
                PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
            if (hProcess) {
                DWORD size = MAX_PATH;
                std::wstring exe_path(MAX_PATH + 2, L'\0');
                if (QueryFullProcessImageNameW((HMODULE)hProcess, 0,
                                               exe_path.data(), &size)) {
                    std::string exe_path_str = wstring_to_utf8(exe_path);
                    auto &stack = stack_map[exe_path_str];

                    stack.windows.push_back(win);
                }
                CloseHandle(hProcess);
            }
        }
    }

    for (const auto &[exe_path, stack] : stack_map) {
        stacks.push_back(stack);
    }

    return stacks;
}

async_simple::coro::Lazy<HICON> window_info::get_async_icon() {
    auto sendMessageAsync =
        [](HWND hwnd, UINT msg, WPARAM wParam,
           LPARAM lParam) -> async_simple::coro::Lazy<HICON> {
        async_simple::Promise<HICON> promise;
        auto future = promise.getFuture();
        std::thread([hwnd, msg, wParam, lParam,
                     promise = std::move(promise)]() mutable {
            HICON result = (HICON)SendMessageW(hwnd, msg, wParam, lParam);
            promise.setValue(result);
        }).detach();

        co_return co_await std::move(future);
    };

    HICON hIcon = co_await sendMessageAsync(hwnd, WM_GETICON, ICON_BIG, 192);
    if (hIcon == NULL) {
        hIcon = co_await sendMessageAsync(hwnd, WM_GETICON, ICON_SMALL, 192);
    }
    if (hIcon == NULL) {
        hIcon = co_await sendMessageAsync(hwnd, WM_GETICON, ICON_SMALL2, 192);
    }

    co_return hIcon;
}

static std::unordered_map<HWND, HICON> large_icon_cache;
async_simple::coro::Lazy<HICON> window_info::get_async_icon_cached() {
    if (large_icon_cache.find(hwnd) != large_icon_cache.end() &&
        large_icon_cache[hwnd] != nullptr) {
        co_return large_icon_cache[hwnd];
    }

    auto icon = co_await get_async_icon();
    large_icon_cache[hwnd] = icon;
    co_return large_icon_cache[hwnd];
}
} // namespace mb_shell::taskbar
