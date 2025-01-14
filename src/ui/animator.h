#pragma once
#include <cmath>
#include <functional>
#include <optional>
#include <print>

namespace ui {
enum class easing_type {
  mutation,
  linear,
  ease_in,
  ease_out,
  ease_in_out,
};
struct animated_float {
  animated_float() = default;
  animated_float(animated_float &&) = default;
  animated_float &operator=(animated_float &&) = default;
  animated_float(const animated_float &) = delete;
  animated_float &operator=(const animated_float &) = delete;

  animated_float(float destination, float duration = 200.f,
                 easing_type easing = easing_type::ease_in_out)
      : duration(duration), destination(destination), easing(easing) {}

  std::optional<std::function<void(float)>> before_animate = {};
  std::optional<std::function<void(float)>> after_animate = {};

  operator float() const { return var(); }
  float operator*() const { return var(); }
  void update(float delta_t);

  void animate_to(float destination);
  void reset_to(float destination);
  void set_duration(float duration);
  void set_easing(easing_type easing);
  void set_delay(float delay);
  // current value
  float var() const;
  // progress, if have any
  float prog() const;
  float dest() const;
  bool updated() const;

  float duration = 200.f;
  float value = 0.f;
  float from = 0.f;
  float destination = value;
  float progress = 0.f;
  float delay = 0.f, delay_timer = 0.f;
  bool _updated = true;
  easing_type easing = easing_type::ease_in_out;
};

using sp_anim_float = std::shared_ptr<animated_float>;
} // namespace ui