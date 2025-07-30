#include "glad/glad.h"
#include "swcadef.h"
#include <dwmapi.h>
#include <future>
#include <print>
#include <stacktrace>
#include <thread>
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
thread_local static bool is_in_loop_thread = false;

void render_target::start_loop() {
  is_in_loop_thread = true;
  glfwMakeContextCurrent(window);
  while (!glfwWindowShouldClose(window) && !should_loop_stop_hide_as_close) {
    render();
    {
      std::lock_guard lock(loop_thread_tasks_lock);
      while (!loop_thread_tasks.empty()) {
        loop_thread_tasks.front()();
        loop_thread_tasks.pop();
      }
    }
  }
  if (should_loop_stop_hide_as_close) {
    should_loop_stop_hide_as_close = false;
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glFlush();
    glfwSwapBuffers(window);
    resize(0, 0);
    hide();
    {
      std::lock_guard lock(rt_lock);
      root->children.clear();
    }
    glfwMakeContextCurrent(nullptr);
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
    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    p.set_value();
  });

  p.get_future().get();

  if (!window) {
    return std::unexpected("Failed to create window");
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(vsync);
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

    ACCENT_POLICY accent = {
        ACCENT_ENABLE_ACRYLICBLURBEHIND,
        Flags::AllowSetWindowRgn | Flags::AllBorder | Flags::GradientColor,
        RGB(*acrylic * 255, *acrylic * 255, *acrylic * 255), 0};
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

  if (no_activate) {
    SetWindowLongPtr(h, GWL_EXSTYLE,
                     GetWindowLongPtr(h, GWL_EXSTYLE) | WS_EX_LAYERED |
                         WS_EX_NOACTIVATE);
    ShowWindow(h, SW_SHOWNOACTIVATE);
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

  glfwSetScrollCallback(window, [](GLFWwindow *window, double xoffset,
                                   double yoffset) {
    auto rt = static_cast<render_target *>(glfwGetWindowUserPointer(window));
    rt->scroll_y += yoffset;
  });

  glfwSetKeyCallback(window, [](GLFWwindow *window, int key, int scancode,
                                int action, int mods) {
    auto rt = static_cast<render_target *>(glfwGetWindowUserPointer(window));
    if (key >= 0 && key <= GLFW_KEY_LAST) {
      auto lock = rt->key_states.get_back_lock();
      auto &back = rt->key_states.get_back();
      if (action == GLFW_PRESS) {
        back[key] |= key_state::pressed;
      } else if (action == GLFW_RELEASE) {
        back[key] |= key_state::released;
      } else if (action == GLFW_REPEAT) {
        back[key] |= key_state::repeated;
      }
    }
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
          auto task = std::move(main_thread_tasks.front());
          main_thread_tasks.pop();
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

  auto now = clock.now();
  auto delta_time =
      1000 * std::chrono::duration<float>(now - last_time).count();
  last_time = now;
  if constexpr (true) {
    static float counter = 0, time_ctr = 0;
    counter++;
    time_ctr += delta_time;
    if (time_ctr > 1000) {
      time_ctr = 0;
      std::printf("FPS: %f\n", counter);
      counter = 0;
    }
  }

  auto begin = clock.now();
  auto ms_steady =
      duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
  auto time_checkpoints = [&](const char *name) {
    if constexpr (false) {
      auto end = clock.now();
      auto delta = std::chrono::duration<float>(end - begin).count();
      std::printf("%s: %fms\n", name, delta);
      begin = end;
    }
  };

  nanovg_context vg{nvg, this};
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
  bool need_repaint = false;
  update_context ctx{
      .delta_time = delta_time,
      .mouse_x = mouse_x / dpi_scale,
      .mouse_y = mouse_y / dpi_scale,
      .mouse_down =
          glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS,
      .right_mouse_down =
          glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS,
      .window = window,
      .screen =
          {
              .width =
                  monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
              .height =
                  monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
              .dpi_scale = dpi_scale,
          },
      .scroll_y = scroll_y,
      .need_repaint = need_repaint,
      .rt = *this,
      .vg = vg,
  };
  scroll_y = 0;
  ctx.mouse_clicked = !ctx.mouse_down && mouse_down;
  ctx.right_mouse_clicked = !ctx.right_mouse_down && right_mouse_down;
  ctx.mouse_up = !ctx.mouse_down && mouse_down;
  mouse_down = ctx.mouse_down;
  right_mouse_down = ctx.right_mouse_down;
  glfwMakeContextCurrent(window);
  {
    time_checkpoints("Update context");
    {
      std::lock_guard lock(rt_lock);
      root->owner_rt = this;
      root->update(ctx);
      key_states.flip();
    }
    time_checkpoints("Update root");
    if (need_repaint || (ms_steady - last_repaint) > 1000) {
      glClearColor(0, 0, 0, 0);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
              GL_STENCIL_BUFFER_BIT);
      last_repaint = ms_steady;
      {
        std::lock_guard lock(rt_lock);
        root->render(vg);
      }
      vg.endFrame();
      glFlush();
      glfwSwapBuffers(window);

    } else {
      if (vsync)
        Sleep(5);
    }
    time_checkpoints("Render root");
  }
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
  post_main_thread_task([this] {
    glfwSetWindowSize(window, this->width, this->height);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glFlush();
    glfwSwapBuffers(window);
  });
}
void render_target::close() {
  ShowWindow(glfwGetWin32Window(window), SW_HIDE);
  glfwSetWindowShouldClose(window, true);
}

std::queue<std::function<void()>> render_target::main_thread_tasks = {};
std::mutex render_target::main_thread_tasks_mutex = {};
void render_target::post_main_thread_task(std::function<void()> task) {
  std::lock_guard lock(main_thread_tasks_mutex);
  main_thread_tasks.push(std::move(task));
  glfwPostEmptyEvent();
}
void render_target::show() {
  if (no_activate) {
    ShowWindow(glfwGetWin32Window(window), SW_SHOWNOACTIVATE);
  } else {
    ShowWindow(glfwGetWin32Window(window), SW_SHOWNORMAL);
  }

  if (this->parent) {
    SetWindowLongPtr(glfwGetWin32Window(window), GWLP_HWNDPARENT,
                     (LONG_PTR)this->parent);
  }

  if (topmost)
    SetWindowPos(glfwGetWin32Window(window), HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}
void render_target::hide() { ShowWindow(glfwGetWin32Window(window), SW_HIDE); }
void render_target::hide_as_close() {
  glfwMakeContextCurrent(nullptr);
  should_loop_stop_hide_as_close = true;
  focused_widget = std::nullopt;
  // reset owner widget
  SetWindowLong(glfwGetWin32Window(window), GWLP_HWNDPARENT, 0);
}
void render_target::post_loop_thread_task(std::function<void()> task) {
  if (is_in_loop_thread) {
    task();
    return;
  }
  std::lock_guard lock(loop_thread_tasks_lock);
  loop_thread_tasks.push(std::move(task));
}
void render_target::focus() {
  if (this->window) {
    if (!no_activate) {
      glfwFocusWindow(this->window);
      SetActiveWindow(glfwGetWin32Window(this->window));
    }
    SetFocus(glfwGetWin32Window(this->window));
  }
}
void *render_target::hwnd() const {
  return window ? glfwGetWin32Window(window) : nullptr;
}
} // namespace ui