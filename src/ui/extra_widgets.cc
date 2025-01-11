#include "extra_widgets.h"
#include "GLFW/glfw3.h"
#include "widget.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"
#include <print>
#define NOMINMAX
#include "windows.h"
#include <dwmapi.h>

#include "swcadef.h"
#include "ui.h"
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  if (msg == WM_MOUSEACTIVATE) {
    return MA_NOACTIVATE;
  } else if (msg == WM_PAINT) {
    PAINTSTRUCT ps;
    BeginPaint(hwnd, &ps);
    EndPaint(hwnd, &ps);
    return 0;
  }
  return DefWindowProc(hwnd, msg, wParam, lParam);
}

static auto pSetWindowCompositionAttribute =
    (PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE)GetProcAddress(
        GetModuleHandleW(L"user32.dll"), "SetWindowCompositionAttribute");

namespace ui {
void acrylic_background_widget::update(update_context &ctx) {
  rect_widget::update(ctx);

  dpi_scale = ctx.rt.dpi_scale;
}
void acrylic_background_widget::render(nanovg_context ctx) {
  rect_widget::render(ctx);
  offset_x = ctx.offset_x;
  offset_y = ctx.offset_y;

  cv.notify_all();
}

acrylic_background_widget::acrylic_background_widget(bool use_dwm)
    : rect_widget(), use_dwm(use_dwm) {
  static bool registered = false;
  if (!registered) {
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"mbui-acrylic-bg";
    RegisterClassW(&wc);
    registered = true;
  }

  opacity->reset_to(255);
  auto win = glfwGetCurrentContext();
  auto handle = glfwGetWin32Window(win);
  render_thread = std::thread([=, this]() {
    hwnd = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_LAYERED,
        L"mbui-acrylic-bg", L"", WS_POPUP, *x, *y, *width, *height, nullptr,
        NULL, GetModuleHandleW(nullptr), NULL);

    if (!hwnd) {
      std::println("Failed to create window: {}", GetLastError());
    }

    SetWindowLongPtrW((HWND)hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);

    ACCENT_POLICY accent = {ACCENT_ENABLE_ACRYLICBLURBEHIND,
                            Flags::AllowSetWindowRgn, 0x01000000, 0};
    WINDOWCOMPOSITIONATTRIBDATA data = {WCA_ACCENT_POLICY, &accent,
                                        sizeof(accent)};
    pSetWindowCompositionAttribute((HWND)hwnd, &data);

    if (use_dwm) {
      // dwm round corners
      auto round_value = DWMWCP_ROUND;
      DwmSetWindowAttribute((HWND)hwnd, DWMWA_WINDOW_CORNER_PREFERENCE,
                            &round_value, sizeof(round_value));
    }

    ShowWindow((HWND)hwnd, SW_SHOW);
    while (true) {
      std::unique_lock<std::mutex> lk(cv_m);
      cv.wait(lk);

      if (to_close) {
        ShowWindow((HWND)hwnd, SW_HIDE);
        DestroyWindow((HWND)hwnd);
        break;
      }

      int winx, winy;
      RECT rect;
      GetWindowRect(handle, &rect);
      winx = rect.left;
      winy = rect.top;

      SetWindowPos((HWND)hwnd, handle, winx + (*x + offset_x) * dpi_scale,
                   winy + (*y + offset_y) * dpi_scale, *width * dpi_scale,
                   *height * dpi_scale,
                   SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOOWNERZORDER |
                       SWP_NOSENDCHANGING | SWP_NOCOPYBITS);
      SetLayeredWindowAttributes((HWND)hwnd, 0, *opacity, LWA_ALPHA);

      if ((width->updated() || height->updated() || radius->updated()) &&
          !use_dwm) {
        auto rgn = CreateRoundRectRgn(
            0, 0, *width * dpi_scale, *height * dpi_scale,
            *radius * 2 * dpi_scale, *radius * 2 * dpi_scale);
        SetWindowRgn((HWND)hwnd, rgn, 0);

        if (rgn) {
          DeleteObject(rgn);
        }
      }
    }
  });
}
acrylic_background_widget::~acrylic_background_widget() {
  to_close = true;
  cv.notify_all();
  render_thread.join();
}
void acrylic_background_widget::update_color() {
  ACCENT_POLICY accent = {ACCENT_ENABLE_ACRYLICBLURBEHIND,
                          Flags::AllowSetWindowRgn | Flags::AllBorder,
                          RGB(acrylic_bg_color.r * 255,
                              acrylic_bg_color.g * 255,
                              acrylic_bg_color.b * 255),
                          0};
  WINDOWCOMPOSITIONATTRIBDATA data = {WCA_ACCENT_POLICY, &accent,
                                      sizeof(accent)};
  pSetWindowCompositionAttribute((HWND)hwnd, &data);
}
void rect_widget::render(nanovg_context ctx) {
  ctx.fillColor(bg_color);
  ctx.fillRoundedRect(*x, *y, *width, *height, *radius);
}
rect_widget::rect_widget() : widget() {}
rect_widget::~rect_widget() {}
} // namespace ui