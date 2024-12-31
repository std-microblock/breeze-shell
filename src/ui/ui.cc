#include "glad/glad.h"
#include <dwmapi.h>
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
    glfwPollEvents();
    render();
  }
}
std::expected<bool, std::string> render_target::init() {
  root = std::make_unique<widget_parent>();

  if (auto res = init_global(); !res) {
    return res;
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  // glfwWindowHint(GLFW_DECORATED, 0);
  glfwWindowHint(GLFW_RESIZABLE, 1);
  glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, 1);
  window = glfwCreateWindow(width, height, "UI", nullptr, nullptr);
  glfwMakeContextCurrent(window);
  glfwSwapInterval(0);

  auto h = glfwGetWin32Window(window);
  DwmEnableBlurBehindWindow(h, nullptr);

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

  nvg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);

  if (!nvg) {
    return std::unexpected("Failed to create NanoVG context");
  }

  glfwSetWindowUserPointer(window, this);
  glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int width,
                                            int height) {
    auto rt = static_cast<render_target *>(glfwGetWindowUserPointer(window));
    rt->width = width;
    rt->height = height;
    rt->render();
  });

  return true;
}

render_target::~render_target() {
  glfwDestroyWindow(window);
  glfwTerminate();
}
std::expected<bool, std::string> render_target::init_global() {
  std::atomic_bool initialized = false;
  if (initialized.exchange(true)) {
    return true;
  }

  if (!glfwInit()) {
    return std::unexpected("Failed to initialize GLFW");
  }
  return true;
}
void render_target::render() {

  int fb_width, fb_height;
  glfwGetFramebufferSize(window, &fb_width, &fb_height);
  glViewport(0, 0, fb_width, fb_height);
  glClearColor(0, 0, 0, 0.1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  if (fb_width != width || fb_height != height) {
    width = fb_width;
    height = fb_height;
    nvg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
  }

  nanovg_context vg{nvg};

  vg.beginFrame(width, height, 1);
  auto now = clock.now();
  auto delta_t = 1000 * std::chrono::duration<float>(now - last_time).count();
  last_time = now;
  double mouse_x, mouse_y;
  glfwGetCursorPos(window, &mouse_x, &mouse_y);
  int window_x, window_y;
  glfwGetWindowPos(window, &window_x, &window_y);

  UpdateContext ctx{
      .delta_t = delta_t,
      .mouse_x = mouse_x,
      .mouse_y = mouse_y,
      .mouse_down =
          glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS,
  };
  ctx.mouse_clicked = !ctx.mouse_down && mouse_down;
  ctx.mouse_up = !ctx.mouse_down && mouse_down;
  mouse_down = ctx.mouse_down;

  root->update(ctx);
  root->render(vg);
  vg.endFrame();
  glfwSwapBuffers(window);
}
} // namespace ui