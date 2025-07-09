#pragma once
#include <atomic>
#include <chrono>
#include <expected>
#include <future>
#include <memory>
#include <mutex>
#include <print>
#include <queue>
#include <string>
#include <type_traits>
#include <utility>

#include "GLFW/glfw3.h"
#include "nanovg.h"

#include "widget.h"

namespace ui {

template <typename T> struct flip_buffer {
  T buffer1{}, buffer2{};
  std::atomic_bool use_buffer2 = false;
  mutable std::mutex buffer1_lock{}, buffer2_lock{};

  T &get() { return use_buffer2 ? buffer2 : buffer1; }
  T &get_back() { return use_buffer2 ? buffer1 : buffer2; }

  std::unique_lock<std::mutex> get_front_lock() const {
    return std::unique_lock<std::mutex>(use_buffer2 ? buffer2_lock
                                                    : buffer1_lock);
  }

  std::unique_lock<std::mutex> get_back_lock() const {
    return std::unique_lock<std::mutex>(use_buffer2 ? buffer1_lock
                                                    : buffer2_lock);
  }

  template <typename U = T,
            typename = std::enable_if_t<std::is_default_constructible_v<U>>>
  void flip() {
    flip(T{});
  }

  void flip(T value) {
    {
      std::unique_lock<std::mutex> lock(use_buffer2 ? buffer1_lock
                                                    : buffer2_lock);
      use_buffer2 = !use_buffer2;
    }

    std::unique_lock<std::mutex> lock(use_buffer2 ? buffer1_lock
                                                  : buffer2_lock);
    if (use_buffer2) {
      buffer1 = value;
    } else {
      buffer2 = value;
    }
  }
};
enum class key_state : char {
  none = 0,
  pressed = 1 << 1,  // Pressed
  released = 1 << 2, // Released
  repeated = 1 << 3, // Repeated
};
inline constexpr key_state &operator|=(key_state &a, key_state b) {
  a = static_cast<key_state>(static_cast<char>(a) | static_cast<char>(b));
  return a;
}
inline constexpr key_state operator&(key_state a, key_state b) {
  return static_cast<key_state>(static_cast<char>(a) & static_cast<char>(b));
}
inline constexpr key_state operator|(key_state a, key_state b) {
  return static_cast<key_state>(static_cast<char>(a) | static_cast<char>(b));
}

constexpr key_state test_pressed = key_state::pressed | key_state::repeated;
static_assert((bool)(test_pressed & key_state::pressed),
              "test_pressed should contain pressed state");

struct render_target {
  std::shared_ptr<widget> root;
  GLFWwindow *window;

  // float: darkness of the acrylic effect, 0~1
  std::optional<float> acrylic = {};
  bool extend = false;
  bool transparent = false;
  bool no_activate = false;
  bool capture_all_input = false;
  bool decorated = true;
  bool topmost = false;
  bool resizable = false;
  bool vsync = true;
  std::string title = "Window";
  NVGcontext *nvg = nullptr;
  int width = 1280;
  int height = 720;
  static std::atomic_int view_cnt;
  int view_id = view_cnt++;
  float dpi_scale = 1;
  float scroll_y = 0;
  flip_buffer<std::array<key_state, GLFW_KEY_LAST + 1>> key_states;
  int64_t last_repaint = 0;
  std::expected<bool, std::string> init();

  std::optional<std::weak_ptr<widget>> focused_widget = {};

  static std::queue<std::function<void()>> main_thread_tasks;
  static std::mutex main_thread_tasks_mutex;
  static void post_main_thread_task(std::function<void()> task);

  static std::expected<bool, std::string> init_global();
  void start_loop();
  void render();
  void resize(int width, int height);
  void set_position(int x, int y);
  void reset_view();
  void close();
  void hide();
  void show();
  void focus();
  void hide_as_close();
  void *hwnd() const;
  bool should_loop_stop_hide_as_close = false;
  std::optional<std::function<void(bool)>> on_focus_changed;
  std::chrono::steady_clock clock{};
  std::recursive_mutex rt_lock{};
  std::mutex loop_thread_tasks_lock{};
  std::queue<std::function<void()>> loop_thread_tasks{};
  void post_loop_thread_task(std::function<void()> task);
  template <typename T>
  T inline post_loop_thread_task(std::function<T()> task) {
    std::promise<T> p;
    post_loop_thread_task([&]() { p.set_value(task()); });
    return p.get_future().get();
  }
  decltype(clock.now()) last_time = clock.now();
  bool mouse_down = false, right_mouse_down = false;
  void *parent = nullptr;

  render_target() = default;
  ~render_target();
  render_target operator=(const render_target &) = delete;
  render_target(const render_target &) = delete;
};
} // namespace ui