#include "ui.h"
#include "widget.h"
namespace ui {
void render_target::start_loop() {
  auto clock = std::chrono::high_resolution_clock();
  auto last_time = clock.now();
  bool mouse_down = false;
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    bgfx::setViewRect(0, 0, 0, width, height);
    bgfx::touch(0);
    nvgBeginFrame(nvg, width, height, 1);
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
        .mouse_down = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS,
    };
    ctx.mouse_clicked = ctx.mouse_down && !mouse_down;
    ctx.mouse_up = !ctx.mouse_down && mouse_down;
    mouse_down = ctx.mouse_down;

    root->update(ctx);
    root->render(0, nanovg_context{nvg});
    nvgEndFrame(nvg);
    bgfx::frame();
  }
}
std::expected<bool, std::string> render_target::init() {
  root = std::make_unique<widget_parent>();
  if (!glfwInit()) {
    return std::unexpected("Failed to initialize GLFW");
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window = glfwCreateWindow(width, height, "UI", nullptr, nullptr);
  if (!window) {
    return std::unexpected("Failed to create window");
  }

  bgfx::Init init;
  init.type = bgfx::RendererType::Count;
  init.resolution.width = width;
  init.resolution.height = height;
  init.resolution.reset = BGFX_RESET_VSYNC;
  init.platformData.nwh = glfwGetWin32Window(window);
  init.platformData.type = bgfx::NativeWindowHandleType::Default;
  bgfx::init(init);

  bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f,
                     0);

  nvg = nvgCreate(1, 0);
  if (!nvg) {
    return std::unexpected("Failed to create NanoVG context");
  }

  return true;
}

} // namespace ui