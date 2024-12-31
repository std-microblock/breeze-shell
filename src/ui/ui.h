#pragma once
#include <expected>
#include <memory>
#include <string>

#include "GLFW/glfw3.h"
#include "bgfx/bgfx.h"
#include "nanovg/nanovg.h"

#include "widget.h"

namespace ui {
struct render_target {
  std::unique_ptr<widget_parent> root;
  GLFWwindow *window;
  NVGcontext *nvg;
  int width = 1280;
  int height = 720;

  std::expected<bool, std::string> init() {
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
    bgfx::init(init);
    nvg = nvgCreate(1, 0);
    if (!nvg) {
      return std::unexpected("Failed to create NanoVG context");
    }

    return true;
  }

  void start_loop() {
    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();
      bgfx::setViewRect(0, 0, 0, width, height);
      bgfx::touch(0);
      bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff,
                         1.0f, 0);
      bgfx::setViewRect(0, 0, 0, width, height);
      bgfx::touch(0);
      bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff,
                         1.0f, 0);
      bgfx::touch(0);

      root->update({0.016f});
      root->render(0, nvg);
      bgfx::frame();
    }
  }
};
} // namespace ui