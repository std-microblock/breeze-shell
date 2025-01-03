#pragma once
#include <atomic>
#include <chrono>
#include <expected>
#include <memory>
#include <string>
#include <utility>

#include "GLFW/glfw3.h"
#include "nanovg.h"

#include "widget.h"

namespace ui {
struct render_target {
  std::unique_ptr<widget_parent> root;
  GLFWwindow *window;

  NVGcontext *nvg;
  int width = 1280;
  int height = 720;
  static std::atomic_int view_cnt;
  int view_id = view_cnt++;
  float dpi_scale = 1;
  std::expected<bool, std::string> init();
  static std::expected<bool, std::string> init_global();
  void start_loop();
  void render();
  void resize(int width, int height);
  void set_position(int x, int y);
  void reset_view();
  void close();
  std::optional<std::function<void(bool)>> on_focus_changed;
  std::chrono::high_resolution_clock clock{};
  decltype(clock.now()) last_time = clock.now();
  bool mouse_down = false;

  ~render_target();
};
} // namespace ui