#include "taskbar_widget.h"
#include "async_simple/Promise.h"
#include "breeze_ui/nanovg_wrapper.h"
#include <atlcomcli.h>
#include <unordered_map>

#include <shellapi.h>

#include <shobjidl.h>
#include <windows.h>

#include "cinatra/coro_http_client.hpp"
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

std::vector<window_info> get_window_list() {
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
          GetClassNameW(hwnd, class_name, sizeof(class_name) / sizeof(wchar_t));
          info.class_name = wstring_to_utf8(class_name);

          info.icon_handle = (HICON)GetClassLongPtrW(hwnd, GCLP_HICON);
          if (info.icon_handle == nullptr) {
            info.icon_handle = (HICON)GetClassLongPtrW(hwnd, GCLP_HICONSM);
          }

          if (info.icon_handle == nullptr) {
            // use the exe icon if nothing else is available
            int pid;

            GetWindowThreadProcessId(hwnd, (LPDWORD)&pid);
            HANDLE hProcess = OpenProcess(
                PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
            if (hProcess) {
              DWORD size = MAX_PATH;
              std::wstring exe_path(MAX_PATH + 2, L'\0');
              if (QueryFullProcessImageNameW((HMODULE)hProcess, 0,
                                             exe_path.data(), &size)) {
                info.icon_handle = ExtractIconW(nullptr, exe_path.c_str(), 0);
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

      HANDLE hProcess =
          OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
      if (hProcess) {
        DWORD size = MAX_PATH;
        std::wstring exe_path(MAX_PATH + 2, L'\0');
        if (QueryFullProcessImageNameW((HMODULE)hProcess, 0, exe_path.data(),
                                       &size)) {
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
  auto sendMessageAsync = [](HWND hwnd, UINT msg, WPARAM wParam,
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

void app_list_stack_widget::render(ui::nanovg_context ctx) {
  if (stack.windows.empty()) {
    return;
  }

  ctx.fillColor(bg_color);
  static constexpr auto margin = 4;
  ctx.fillRoundedRect(*x + margin, *y + margin, *width - margin * 2, *height - margin * 2, 6);

  auto &first_window = stack.windows.front();
  if (first_window.icon_handle) {
    if (!icon) {
      auto rgba = IconExtractor::GetIcon(first_window.icon_handle);
      icon = ui::NVGImage{
          ctx.createImageRGBA(rgba.width, rgba.height, 0, rgba.rgbaData.data()),
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
void app_list_stack_widget::update(ui::update_context &ctx) {
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
void windows_button_widget::render(ui::nanovg_context ctx) {
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
void windows_button_widget::update(ui::update_context &ctx) {
  super::update(ctx);
  bool last_is_windows_menu_open = is_windows_menu_open;
  static ATL::CComPtr<IAppVisibility> appVisibility = nullptr;
  if (!appVisibility) {
    HRESULT hr =
        CoCreateInstance(CLSID_AppVisibility, nullptr, CLSCTX_INPROC_SERVER,
                         IID_PPV_ARGS(&appVisibility));
    if (FAILED(hr)) {
      std::cerr << "Failed to create AppVisibility instance: " << std::hex << hr
                << std::endl;
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
} // namespace mb_shell::taskbar
