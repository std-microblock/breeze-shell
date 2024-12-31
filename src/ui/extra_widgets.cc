#include "extra_widgets.h"
#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"
#include <print>
#define NOMINMAX
#include "windows.h"
#include <dwmapi.h>

#include "swcadef.h"

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  return DefWindowProc(hwnd, msg, wParam, lParam);
}

namespace ui {
void acrylic_background_widget::update(const UpdateContext &ctx) {
  widget::update(ctx);
}
void acrylic_background_widget::render(nanovg_context ctx) {
  widget::render(ctx);

  auto win = glfwGetCurrentContext();
  auto handle = glfwGetWin32Window(win);

  int winx, winy;
  glfwGetWindowPos(win, &winx, &winy);

  SetWindowPos((HWND)hwnd, handle, winx + *x, winy + *y, *width, *height,
               SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOOWNERZORDER |
                   SWP_NOCOPYBITS);
  SetLayeredWindowAttributes((HWND)hwnd, 0, *opacity, LWA_ALPHA);

  if (width->updated() || height->updated() || radius->updated()) {
    auto rgn = CreateRoundRectRgn(0, 0, *width, *height, *radius, *radius);
    SetWindowRgn((HWND)hwnd, rgn, 0);

    if (rgn) {
      DeleteObject(rgn);
    }
  }
}

acrylic_background_widget::acrylic_background_widget() : widget() {
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

  hwnd = CreateWindowExW(
      WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_LAYERED,
      L"mbui-acrylic-bg", L"", WS_POPUP, *x, *y, *width, *height, nullptr, NULL,
      GetModuleHandleW(nullptr), NULL);

  if (!hwnd) {
    std::println("Failed to create window: {}", GetLastError());
  }

  SetWindowLongPtrW((HWND)hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);

  auto pSetWindowCompositionAttribute =
      (PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE)GetProcAddress(
          GetModuleHandleW(L"user32.dll"), "SetWindowCompositionAttribute");

  ACCENT_POLICY accent = {ACCENT_ENABLE_ACRYLICBLURBEHIND,
                          Flags::AllowSetWindowRgn, 0, 0};
  WINDOWCOMPOSITIONATTRIBDATA data = {WCA_ACCENT_POLICY, &accent,
                                      sizeof(accent)};
  pSetWindowCompositionAttribute((HWND)hwnd, &data);
  ShowWindow((HWND)hwnd, SW_SHOW);
}
acrylic_background_widget::~acrylic_background_widget() {
  DestroyWindow((HWND)hwnd);
}
} // namespace ui