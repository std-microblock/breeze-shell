#include "extra_widgets.h"
#include "widget.h"
#include <future>
#include <iostream>
#include <print>
#include <thread>

#include "swcadef.h"
#include "ui.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"
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

int GetWindowZOrder(HWND hwnd) {
  int z = 1;
  for (HWND h = hwnd; h; h = GetWindow(h, GW_HWNDNEXT)) {
    z++;
  }
  return z;
}

namespace ui {
void acrylic_background_widget::update(update_context &ctx) {
  if (!render_thread) {
    auto win = glfwGetCurrentContext();
    if (!win) {
      std::cerr << "[acrylic window] Failed to get current context"
                << std::endl;
      return;
    }
    auto handle = glfwGetWin32Window(win);
    render_thread = std::thread([=, this]() {
      hwnd =
          CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT |
                              WS_EX_NOACTIVATE | WS_EX_LAYERED | WS_EX_TOPMOST,
                          L"mbui-acrylic-bg", L"", WS_POPUP, *x, *y, 0, 0,
                          nullptr, NULL, GetModuleHandleW(nullptr), NULL);

      if (!hwnd) {
        std::printf("Failed to create window %d\n", GetLastError());
      }

      SetWindowLongPtrW((HWND)hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);

      update_color();

      if (use_dwm) {
        // dwm round corners
        auto round_value = radius > 0 ? DWMWCP_ROUND : DWMWCP_DONOTROUND;
        DwmSetWindowAttribute((HWND)hwnd, DWMWA_WINDOW_CORNER_PREFERENCE,
                              &round_value, sizeof(round_value));
      }

      std::unique_lock<std::mutex> lk(cv_m);

      ShowWindow((HWND)hwnd, SW_SHOW);

      SetWindowPos((HWND)hwnd, handle, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW |
                       SWP_NOSENDCHANGING | SWP_NOCOPYBITS);

      bool rgn_set = false;
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
                         SWP_NOSENDCHANGING | SWP_NOCOPYBITS |
                         SWP_NOREPOSITION | SWP_NOZORDER);

        auto zorder_this = GetWindowZOrder((HWND)hwnd);
        auto zorder_last = GetWindowZOrder((HWND)last_hwnd_self);

        if (zorder_this < zorder_last && last_hwnd_self && hwnd) {
          SetWindowPos((HWND)hwnd, handle, 0, 0, 0, 0,
                       SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW |
                           SWP_NOSENDCHANGING | SWP_NOCOPYBITS);
        }

        SetLayeredWindowAttributes((HWND)hwnd, 0, *opacity, LWA_ALPHA);

        if (!use_dwm && should_update) {
          rgn_set = true;
          should_update = false;
          auto rgn = CreateRoundRectRgn(
              0, 0, *width * dpi_scale, *height * dpi_scale,
              *radius * 2 * dpi_scale, *radius * 2 * dpi_scale);
          SetWindowRgn((HWND)hwnd, rgn, 1);

          if (rgn) {
            DeleteObject(rgn);
          }
        }

        cv.wait_for(lk, std::chrono::milliseconds(200));

        // limit to 60fps
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
      }
    });
  }

  rect_widget::update(ctx);
  dpi_scale = ctx.rt.dpi_scale;
  should_update = should_update ||
                  (width->updated() || height->updated() || radius->updated() ||
                   x->updated() || y->updated() || opacity->updated());
  last_hwnd = nullptr;
  if (use_dwm) {
    radius->reset_to(8.f);
  }
}
thread_local void *acrylic_background_widget::last_hwnd = 0;
void acrylic_background_widget::render(nanovg_context ctx) {
  widget::render(ctx);
  cv.notify_all();

  auto bg_color_tmp = bg_color;
  bg_color_tmp.a *= *opacity / 255.f;
  ctx.fillColor(bg_color_tmp);
  ctx.fillRoundedRect(*x, *y, *width, *height, *radius);
  last_hwnd_self = last_hwnd;
  last_hwnd = hwnd;

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
}
acrylic_background_widget::~acrylic_background_widget() {
  to_close = true;
  cv.notify_all();
  if (render_thread)
    render_thread->join();
}

// b a r g
// r g b a

void acrylic_background_widget::update_color() {
  ACCENT_POLICY accent = {
      ACCENT_ENABLE_ACRYLICBLURBEHIND,
      Flags::GradientColor | Flags::AllBorder | Flags::AllowSetWindowRgn,
      // GradientColor uses BGRA
      ARGB(acrylic_bg_color.a * 255, acrylic_bg_color.b * 255,
           acrylic_bg_color.g * 255, acrylic_bg_color.r * 255),
      0};
  WINDOWCOMPOSITIONATTRIBDATA data = {WCA_ACCENT_POLICY, &accent,
                                      sizeof(accent)};
  pSetWindowCompositionAttribute((HWND)hwnd, &data);
}
void rect_widget::render(nanovg_context ctx) {
  bg_color.a = *opacity / 255.f;
  ctx.fillColor(bg_color);
  ctx.fillRoundedRect(*x, *y, *width, *height, *radius);
}
rect_widget::rect_widget() : widget() {}
rect_widget::~rect_widget() {}
} // namespace ui