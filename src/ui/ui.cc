#include "glad/glad.h"
#include "swcadef.h"
#include <dwmapi.h>
#include <future>
#include <print>
#include <stacktrace>
#include <thread>
#include <winuser.h>
#define GLFW_INCLUDE_GLEXT
#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"

#include "ui.h"

#include "widget.h"

#include "nanovg.h"
#define NANOVG_GL3_IMPLEMENTATION
#include "nanovg_gl.h"

namespace ui {
std::atomic_int render_target::view_cnt = 0;
void render_target::start_loop() {
  glfwMakeContextCurrent(window);
  while (!glfwWindowShouldClose(window)) {
    render();
  }
}
std::expected<bool, std::string> render_target::init() {
  root = std::make_shared<widget>();

  std::ignore = init_global();

  std::promise<void> p;

  render_target::post_main_thread_task([&]() {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_RESIZABLE, resizable);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, 1);
    if (transparent) {
      glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, 1);
      glfwWindowHint(GLFW_FLOATING, 1);
    } else {
      glfwWindowHint(GLFW_FLOATING, 0);
      glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, 0);
    }
    glfwWindowHint(GLFW_DECORATED, decorated);

    glfwWindowHint(GLFW_VISIBLE, 0);
    window = glfwCreateWindow(width, height, "UI", nullptr, nullptr);
    p.set_value();
  });

  p.get_future().get();

  if (!window) {
    return std::unexpected("Failed to create window");
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(0);
  gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

  auto h = glfwGetWin32Window(window);

  if (acrylic || extend) {
    MARGINS margins = {
        .cxLeftWidth = -1,
        .cxRightWidth = -1,
        .cyTopHeight = -1,
        .cyBottomHeight = -1,
    };
    DwmExtendFrameIntoClientArea(h, &margins);
  }

  if (acrylic) {
    DWM_BLURBEHIND bb = {0};
    bb.dwFlags = DWM_BB_ENABLE;
    bb.fEnable = true;
    DwmEnableBlurBehindWindow(h, &bb);

    ACCENT_POLICY accent = {ACCENT_ENABLE_ACRYLICBLURBEHIND,
                            Flags::AllowSetWindowRgn, 0x00000000, 0};
    WINDOWCOMPOSITIONATTRIBDATA data = {WCA_ACCENT_POLICY, &accent,
                                        sizeof(accent)};
    pSetWindowCompositionAttribute((HWND)h, &data);

    // dwm round corners
    auto round_value = DWMWCP_ROUND;
    DwmSetWindowAttribute((HWND)h, DWMWA_WINDOW_CORNER_PREFERENCE, &round_value,
                          sizeof(round_value));

  } else {
    DwmEnableBlurBehindWindow(h, nullptr);
  }

  if (no_focus) {
    ShowWindow(h, SW_SHOWNOACTIVATE);
    SetWindowLongPtr(h, GWL_EXSTYLE,
                     GetWindowLongPtr(h, GWL_EXSTYLE) | WS_EX_LAYERED |
                         WS_EX_NOACTIVATE);
  } else {
    ShowWindow(h, SW_SHOWNORMAL);
  }
  if (capture_all_input) {
    // retrieve all mouse messages
    SetCapture(h);
  }

  if (topmost) {
    SetWindowPos(h, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
  }

  glfwSetWindowUserPointer(window, this);
  glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int width,
                                            int height) {
    auto rt = static_cast<render_target *>(glfwGetWindowUserPointer(window));
    rt->width = width / rt->dpi_scale;
    rt->height = height / rt->dpi_scale;
    rt->reset_view();
    // rt->render();
  });

  glfwSetWindowFocusCallback(window, [](GLFWwindow *window, int focused) {
    auto thiz = static_cast<render_target *>(glfwGetWindowUserPointer(window));
    if (thiz->on_focus_changed) {
      thiz->on_focus_changed.value()(focused);
    }
  });

  glfwSetWindowContentScaleCallback(window, [](GLFWwindow *window, float x,
                                               float y) {
    auto rt = static_cast<render_target *>(glfwGetWindowUserPointer(window));
    rt->dpi_scale = x;
  });

  glfwGetWindowContentScale(window, &dpi_scale, nullptr);
  reset_view();

  if (!nvg) {
    return std::unexpected("Failed to create NanoVG context");
  }

  return true;
}

render_target::~render_target() {
  if (nvg) {
    nvgDeleteGL3(nvg);
  }

  glfwDestroyWindow(window);
}
std::expected<bool, std::string> render_target::init_global() {
  static std::atomic_bool initialized = false;
  if (initialized.exchange(true)) {
    return false;
  }

  std::promise<std::expected<bool, std::string>> res;
  auto future = res.get_future();
  std::thread([&]() {
    if (!glfwInit()) {
      res.set_value(std::unexpected(std::string("Failed to initialize GLFW")));
      return;
    }

    glfwPollEvents();
    res.set_value(true);

    while (true) {
      {
        std::lock_guard lock(main_thread_tasks_mutex);
        if (!main_thread_tasks.empty()) {
          auto task = std::move(main_thread_tasks.back());
          main_thread_tasks.pop_back();
          task();
        }
      }

      glfwWaitEvents();
    }
  }).detach();

  return future.get();
}
void render_target::render() {
  int fb_width, fb_height;
  glfwGetFramebufferSize(window, &fb_width, &fb_height);
  glViewport(0, 0, fb_width, fb_height);
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  auto now = clock.now();
  auto delta_t = 1000 * std::chrono::duration<float>(now - last_time).count();
  last_time = now;
  if constexpr (true) {
    static float counter = 0, time_ctr = 0;
    counter++;
    time_ctr += delta_t;
    if (time_ctr > 1000) {
      time_ctr = 0;
      std::println("FPS: {}", counter);
      counter = 0;
    }
  }

  auto begin = clock.now();

  auto time_checkpoints = [&](const char *name) {
    if constexpr (false) {
      auto end = clock.now();
      auto delta = std::chrono::duration<float>(end - begin).count();
      std::println("{}: {}ms", name, delta * 1000);
      begin = end;
    }
  };

  nanovg_context vg{nvg};
  time_checkpoints("NanoVG context");

  vg.beginFrame(fb_width, fb_height, dpi_scale);
  vg.scale(dpi_scale, dpi_scale);

  double mouse_x, mouse_y;
  glfwGetCursorPos(window, &mouse_x, &mouse_y);
  int window_x, window_y;
  glfwGetWindowPos(window, &window_x, &window_y);

  auto monitor =
      MonitorFromWindow(glfwGetWin32Window(window), MONITOR_DEFAULTTONEAREST);
  MONITORINFOEX monitor_info;
  monitor_info.cbSize = sizeof(MONITORINFOEX);
  GetMonitorInfo(monitor, &monitor_info);

  update_context ctx{
      .delta_t = delta_t,
      .mouse_x = mouse_x / dpi_scale,
      .mouse_y = mouse_y / dpi_scale,
      .mouse_down =
          glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS,
      .right_mouse_down =
          glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS,
      .screen =
          {
              .width =
                  monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
              .height =
                  monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
              .dpi_scale = dpi_scale,
          },
      .rt = *this,
      .vg = vg,
  };
  ctx.mouse_clicked = !ctx.mouse_down && mouse_down;
  ctx.right_mouse_clicked = !ctx.right_mouse_down && right_mouse_down;
  ctx.mouse_up = !ctx.mouse_down && mouse_down;
  mouse_down = ctx.mouse_down;
  right_mouse_down = ctx.right_mouse_down;
  time_checkpoints("Update context");
  root->update(ctx);
  time_checkpoints("Update root");
  root->render(vg);
  time_checkpoints("Render root");
  vg.endFrame();
  glFlush();
  glfwSwapBuffers(window);
  time_checkpoints("Swap buffers");
}
void render_target::reset_view() {
  if (!nvg)
    nvg = nvgCreateGL3(NVG_STENCIL_STROKES | NVG_ANTIALIAS);
}
void render_target::set_position(int x, int y) {
  glfwSetWindowPos(window, x, y);
}
void render_target::resize(int width, int height) {
  this->width = width;
  this->height = height;
  glfwSetWindowSize(window, this->width, this->height);
}
void render_target::close() {
  ShowWindow(glfwGetWin32Window(window), SW_HIDE);
  glfwSetWindowShouldClose(window, true);
}

std::vector<std::function<void()>> render_target::main_thread_tasks = {};
std::mutex render_target::main_thread_tasks_mutex = {};
void render_target::post_main_thread_task(std::function<void()> task) {
  std::lock_guard lock(main_thread_tasks_mutex);
  main_thread_tasks.push_back(std::move(task));
  glfwPostEmptyEvent();
}
} // namespace ui