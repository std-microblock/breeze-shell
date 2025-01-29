#include "extra_widgets.h"
#include "GLFW/glfw3.h"
#include "widget.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"
#include <print>

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

namespace ui {
void acrylic_background_widget::update(update_context &ctx) {
  rect_widget::update(ctx);
  dpi_scale = ctx.rt.dpi_scale;
  cv.notify_all();
  last_hwnd = 0;
  if (use_dwm) {
    radius->reset_to(8.f);
  }
}

int GetWindowZOrder(HWND hwnd) {
  int z = 1;
  for (HWND h = hwnd; h; h = GetWindow(h, GW_HWNDNEXT)) {
    z++;
  }
  return z;
}
thread_local size_t acrylic_background_widget::last_hwnd = 0;
void acrylic_background_widget::render(nanovg_context ctx) {
  widget::render(ctx);
  cv.notify_all();

  auto bg_color_tmp = bg_color;
  bg_color_tmp.a *= *opacity / 255.f;
  ctx.fillColor(bg_color_tmp);
  ctx.fillRoundedRect(*x, *y, *width, *height, *radius);
  // we should be higher than the last hwnd
  // first check if we are already higher
  if (last_hwnd && hwnd) {
    auto last_z = GetWindowZOrder((HWND)last_hwnd);
    auto z = GetWindowZOrder((HWND)hwnd);
    if (z < last_z) {
      SetWindowPos((HWND)hwnd, (HWND)last_hwnd, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW |
                       SWP_NOSENDCHANGING | SWP_NOCOPYBITS | SWP_NOREPOSITION |
                       SWP_NOZORDER);
    }
  }

  last_hwnd = (size_t)hwnd;

  offset_x = ctx.offset_x;
  offset_y = ctx.offset_y;
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

  auto win = glfwGetCurrentContext();
  auto handle = glfwGetWin32Window(win);
  render_thread = std::thread([=, this]() {
    hwnd = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT |
                               WS_EX_NOACTIVATE | WS_EX_LAYERED,
                           L"mbui-acrylic-bg", L"", WS_POPUP, *x, *y, 0, 0,
                           nullptr, NULL, GetModuleHandleW(nullptr), NULL);

    if (!hwnd) {
      std::println("Failed to create window: {}", GetLastError());
    }

    SetWindowLongPtrW((HWND)hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);

    update_color();

    if (use_dwm) {
      // dwm round corners
      auto round_value = DWMWCP_ROUND;
      DwmSetWindowAttribute((HWND)hwnd, DWMWA_WINDOW_CORNER_PREFERENCE,
                            &round_value, sizeof(round_value));
    }

    std::unique_lock<std::mutex> lk(cv_m);

    ShowWindow((HWND)hwnd, SW_SHOW);

    SetWindowPos((HWND)hwnd, handle, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW |
                     SWP_NOSENDCHANGING | SWP_NOCOPYBITS);

    while (true) {

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

      SetWindowPos((HWND)hwnd, nullptr, winx + (*x + offset_x) * dpi_scale,
                   winy + (*y + offset_y) * dpi_scale, *width * dpi_scale,
                   *height * dpi_scale,
                   SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOOWNERZORDER |
                       SWP_NOSENDCHANGING | SWP_NOCOPYBITS | SWP_NOREPOSITION |
                       SWP_NOZORDER);

      SetLayeredWindowAttributes((HWND)hwnd, 0, *opacity, LWA_ALPHA);

      cv.wait_for(lk, std::chrono::milliseconds(200));

      // limit to 60fps
      std::this_thread::sleep_for(std::chrono::milliseconds(16));

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
  std::println("Closing acrylic bg");
  cv.notify_all();
  render_thread.join();
}
void acrylic_background_widget::update_color() {
  ACCENT_POLICY accent = {
      ACCENT_ENABLE_ACRYLICBLURBEHIND,
      Flags::GradientColor | Flags::AllBorder | Flags::AllowSetWindowRgn,
      RGB(acrylic_bg_color.r * 255, acrylic_bg_color.g * 255,
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