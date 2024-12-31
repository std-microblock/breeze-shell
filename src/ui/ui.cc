#include "ui.h"
#include "GLFW/glfw3.h"

#include "bgfx/bgfx.h"
#include "bgfx/platform.h"
#include "gl/GL.h"
#include "widget.h"

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
  if (!window) {
    return std::unexpected("Failed to create window");
  }

  bgfx::Init init;

  init.type = bgfx::RendererType::Count;
  init.resolution.width = width;
  init.resolution.height = height;
  init.resolution.reset = BGFX_RESET_VSYNC | BGFX_RESET_TRANSPARENT_BACKBUFFER;
  init.platformData.nwh = glfwGetWin32Window(window);
  init.platformData.type = bgfx::NativeWindowHandleType::Default;
  bgfx::init(init);

  bgfx::setViewClear(view_id, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x00000000,
                     1.0f, 0);

  bgfx::setViewMode(view_id, bgfx::ViewMode::Sequential);

  nvg = nvgCreate(1, view_id);
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
  bgfx::shutdown();
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

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  return true;
}
void render_target::render() {
  int fb_width, fb_height;
  glfwGetFramebufferSize(window, &fb_width, &fb_height);

  if (fb_width != width || fb_height != height) {
    width = fb_width;
    height = fb_height;
    bgfx::reset(width, height,
                BGFX_RESET_VSYNC | BGFX_RESET_TRANSPARENT_BACKBUFFER);
    nvg = nvgCreate(1, view_id);
  }

  bgfx::setViewRect(view_id, 0, 0, width, height);
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
  root->render(view_id, vg);
  vg.endFrame();
  bgfx::frame();
}
} // namespace ui