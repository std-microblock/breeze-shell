#include "taskbar_widget.h"
#include "async_simple/Promise.h"
#include "nanovg_wrapper.h"
#include <unordered_map>

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
            // default icon if no icon is found
            info.icon_handle = LoadIcon(NULL, IDI_APPLICATION);
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
          if (stack.windows.empty() || stack.windows.back().active) {
            stack.active = true;
          }

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
  ctx.fillRoundedRect(*x, *y, *width, *height, 6);
  ctx.strokeColor({0.2f, 0.2f, 0.2f, 0.8f});
  ctx.strokeRoundedRect(*x, *y, *width, *height, 6);

  auto &first_window = stack.windows.front();
  if (first_window.icon_handle) {
    if (!icon) {
      auto rgba = IconExtractor::GetIcon(first_window.icon_handle);
      icon = ui::NVGImage{
          ctx.createImageRGBA(rgba.width, rgba.height, 0, rgba.rgbaData.data()),
          rgba.width, rgba.height, ctx};
    }

    if (icon) {
      ctx.drawImage(*icon, *x + 8, *y + 8, *width - 16,
                    *height - 16);
    }
  }
}

void app_list_stack_widget::update(ui::update_context &ctx) {
  ui::widget::update(ctx);
  width->reset_to(40);
  height->reset_to(40);

  if (ctx.mouse_down_on(this)) {
    bg_color.animate_to({0.2f, 0.2f, 0.2f, 0.8f});
  } else if (ctx.hovered(this)) {
    bg_color.animate_to({0.3f, 0.3f, 0.3f, 0.8f});
  } else {
    bg_color.animate_to({0.1f, 0.1f, 0.1f, 0.8f});
  }
}
} // namespace mb_shell::taskbar
