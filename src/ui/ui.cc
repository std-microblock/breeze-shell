#include "glad/glad.h"
#include "thorvg.h"
#include <dwmapi.h>
#include <winuser.h>
#define GLFW_INCLUDE_GLEXT
#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"

#include "ui.h"

#include "widget.h"

// #include "nanovg.h"
// #define NANOVG_GL3_IMPLEMENTATION
// #include "nanovg_gl.h"

namespace ui {
void render_target::start_loop() {
  glfwMakeContextCurrent(window);
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    render();
  }
}
std::expected<bool, std::string> render_target::init() {
  root = std::make_shared<widget_parent>();

  if (auto res = init_global(); !res) {
    return res;
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  // glfwWindowHint(GLFW_DECORATED, 0);
  glfwWindowHint(GLFW_RESIZABLE, 1);
  glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, 1);
  glfwWindowHint(GLFW_FLOATING, 1);
  // glfwWindowHint(GLFW_VISIBLE, 0);
  window = glfwCreateWindow(width, height, "UI", nullptr, nullptr);
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  auto h = glfwGetWin32Window(window);
  DwmEnableBlurBehindWindow(h, nullptr);

  ShowWindow(h, SW_SHOWNOACTIVATE);
  // topmost & focused
  SetWindowPos(h, HWND_TOPMOST, 0, 0, 0, 0,
               SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
  // retrieve all mouse messages
  SetCapture(h);

  SetWindowLongPtr(h, GWL_EXSTYLE,
                   GetWindowLongPtr(h, GWL_EXSTYLE) | WS_EX_LAYERED |
                       WS_EX_NOACTIVATE);

  if (!window) {
    return std::unexpected("Failed to create window");
  }

  int version = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
  if (version == 0) {
    return std::unexpected("Failed to load OpenGL");
  }

  if (!window) {
    return std::unexpected("Failed to create window");
  }

  glfwSetWindowUserPointer(window, this);
  glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int width,
                                            int height) {
    auto rt = static_cast<render_target *>(glfwGetWindowUserPointer(window));
    rt->width = width;
    rt->height = height;
    rt->reset_view();
    rt->render();
  });

  glfwSetWindowFocusCallback(window, [](GLFWwindow *window, int focused) {
    auto thiz = static_cast<render_target *>(glfwGetWindowUserPointer(window));
    if (thiz->on_focus_changed) {
      thiz->on_focus_changed.value()(focused);
    }
  });

  reset_view();

  if (!canvas) {
    return std::unexpected("Failed to create NanoVG context");
  }

  return true;
}

render_target::~render_target() {
  if (canvas) {
    delete canvas;
  }

  glfwDestroyWindow(window);
}
std::expected<bool, std::string> render_target::init_global() {
  std::atomic_bool initialized = false;
  if (initialized.exchange(true)) {
    return true;
  }

  if (!glfwInit()) {
    return std::unexpected("Failed to initialize GLFW");
  }
  tvg::Initializer::init(0);
  tvg::Text::load("C:\\WINDOWS\\FONTS\\msyh.ttc");
  return true;
}
void render_target::render() {

  int fb_width, fb_height;
  glfwGetFramebufferSize(window, &fb_width, &fb_height);
  glViewport(0, 0, fb_width, fb_height);
  glClearColor(0, 0, 0, 0.0f);
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
      .rt = *this
  };
  ctx.mouse_clicked = !ctx.mouse_down && mouse_down;
  ctx.right_mouse_clicked = !ctx.right_mouse_down && right_mouse_down;
  ctx.mouse_up = !ctx.mouse_down && mouse_down;
  mouse_down = ctx.mouse_down;
  right_mouse_down = ctx.right_mouse_down;
  root->update(ctx);
  render_context render_ctx{
      .vg = tvg::Scene::gen(),
      .offset_x = 0,
      .offset_y = 0,
  };
  render_ctx.vg->scale(dpi_scale);
  root->render(render_ctx);
  canvas->push(render_ctx.vg);

  if (canvas->draw() == tvg::Result::Success) {
    canvas->sync();
  } else {
    std::println("Failed to draw");
  }

  glfwSwapBuffers(window);
  canvas->remove();
}
void render_target::reset_view() {
  if (!canvas) {
    canvas = tvg::GlCanvas::gen();
    if (!canvas) {
      std::println("Failed to create canvas");
      return;
    }

    if (canvas->target(glfwGetCurrentContext(), 0, width, height,
                       tvg::ColorSpace::ABGR8888S) != tvg::Result::Success) {
      std::println("Failed to set target");
    }
  }

  if (canvas->viewport(0, 0, width, height) != tvg::Result::Success) {
    std::println("Failed to set viewport");
  }

  glfwGetWindowContentScale(window, &this->dpi_scale, nullptr);
  glfwSetWindowSize(window, width, height);
}
void render_target::set_position(int x, int y) {
  glfwSetWindowPos(window, x, y);
}
void render_target::resize(int width, int height) {
  this->width = width;
  this->height = height;
  reset_view();
}
void render_target::close() {
  ShowWindow(glfwGetWin32Window(window), SW_HIDE);
  glfwSetWindowShouldClose(window, true);
}
} // namespace ui