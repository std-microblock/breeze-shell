#include "extra_widgets.h"
#include "widget.h"
#include <iostream>
#include <print>
#include <mutex>
#include <vector>

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

// Window pool to pre-create acrylic background windows
class AcrylicWindowPool {
public:
  static AcrylicWindowPool& getInstance() {
    static AcrylicWindowPool instance;
    return instance;
  }

  HWND acquireWindow() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!pool_.empty()) {
      HWND hwnd = pool_.back();
      pool_.pop_back();
      return hwnd;
    }
    
    return createWindow();
  }

  void releaseWindow(HWND hwnd) {
    if (!hwnd) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    SetLayeredWindowAttributes(hwnd, 0, 0, LWA_ALPHA);
    ShowWindow(hwnd, SW_HIDE);
    pool_.push_back(hwnd);
  }

  void initialize(size_t initialSize = 2) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Register window class if not already registered
    static bool registered = false;
    if (!registered) {
      WNDCLASSW wc = {0};
      wc.lpfnWndProc = WndProc;
      wc.hInstance = GetModuleHandleW(nullptr);
      wc.lpszClassName = L"mbui-acrylic-bg";
      RegisterClassW(&wc);
      registered = true;
    }
    
    // Pre-create windows
    for (size_t i = 0; i < initialSize; i++) {
      HWND hwnd = createWindow();
      if (hwnd) {
        pool_.push_back(hwnd);
      }
    }
  }

  ~AcrylicWindowPool() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (HWND hwnd : pool_) {
      DestroyWindow(hwnd);
    }
    pool_.clear();
  }

private:
  AcrylicWindowPool() {
    initialize();
  }
  
  HWND createWindow() {
    HWND hwnd = CreateWindowExW(
      WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_LAYERED | WS_EX_TOPMOST,
      L"mbui-acrylic-bg", L"", WS_POPUP, 0, 0, 0, 0,
      nullptr, NULL, GetModuleHandleW(nullptr), NULL);
      
    if (!hwnd) {
      std::printf("Failed to create window %d\n", GetLastError());
    } else {
      SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
    }
    
    return hwnd;
  }
  
  std::vector<HWND> pool_;
  std::mutex mutex_;
};

void acrylic_background_widget::update(update_context &ctx) {
  if (!hwnd) {
    // Get a window from the pool
    auto win = glfwGetCurrentContext();
    if (!win) {
      std::cerr << "[acrylic window] Failed to get current context" << std::endl;
      return;
    }
    
    hwnd = AcrylicWindowPool::getInstance().acquireWindow();
    if (!hwnd) {
      std::cerr << "[acrylic window] Failed to acquire window from pool" << std::endl;
      return;
    }
    
    update_color();
    
    if (use_dwm) {
      // dwm round corners
      auto round_value = radius > 0 ? DWMWCP_ROUND : DWMWCP_DONOTROUND;
      DwmSetWindowAttribute((HWND)hwnd, DWMWA_WINDOW_CORNER_PREFERENCE,
                          &round_value, sizeof(round_value));
    }
    
    ShowWindow((HWND)hwnd, SW_SHOW);
    
    auto handle = glfwGetWin32Window(win);
    SetWindowPos((HWND)hwnd, handle, 0, 0, 0, 0,
               SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW |
               SWP_NOSENDCHANGING | SWP_NOCOPYBITS);
  }
  
  // Update window position and attributes
  if (hwnd) {
    auto win = glfwGetCurrentContext();
    auto handle = glfwGetWin32Window(win);
    
    RECT rect;
    GetWindowRect(handle, &rect);
    int winx = rect.left;
    int winy = rect.top;
    
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
    
    if ((width->updated() || height->updated() || radius->updated()) && !use_dwm) {
      auto rgn = CreateRoundRectRgn(
          0, 0, *width * dpi_scale, *height * dpi_scale,
          *radius * 2 * dpi_scale, *radius * 2 * dpi_scale);
      SetWindowRgn((HWND)hwnd, rgn, 1);
      
      if (rgn) {
        DeleteObject(rgn);
      }
    }
  }
  
  rect_widget::update(ctx);
  dpi_scale = ctx.rt.dpi_scale;
  last_hwnd = nullptr;
  if (use_dwm) {
    radius->reset_to(8.f);
  }
}

thread_local void *acrylic_background_widget::last_hwnd = 0;

void acrylic_background_widget::render(nanovg_context ctx) {
  widget::render(ctx);
  
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
  // Window class registration is handled by the pool now
}

acrylic_background_widget::~acrylic_background_widget() {
  if (hwnd) {
    AcrylicWindowPool::getInstance().releaseWindow((HWND)hwnd);
    hwnd = nullptr;
  }
}

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