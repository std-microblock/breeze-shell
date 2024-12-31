#pragma once
#include <chrono>
#include <expected>
#include <memory>
#include <string>


#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"
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

    if (!window) {
  std::expected<bool, std::string> init();
  void start_loop();
};
} // namespace ui