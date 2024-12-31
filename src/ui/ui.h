#pragma once
#include <atomic>
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
  bgfx::FrameBufferHandle fbh;

  bgfx::TextureHandle color_texture;
  bgfx::ProgramHandle program;
  bgfx::ShaderHandle vshader;
  bgfx::ShaderHandle fshader;

  NVGcontext *nvg;
  int width = 1280;
  int height = 720;
  static std::atomic_int view_cnt;
  int view_id = view_cnt++;
  std::expected<bool, std::string> init();
  static std::expected<bool, std::string> init_global();
  void start_loop();

  ~render_target();
};
} // namespace ui