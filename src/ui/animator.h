#pragma once
#include <cmath>
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

  operator float() const { return var(); }
  float operator*() const { return var(); }
  void update(float delta_t);

  void animate_to(float destination);
  void reset_to(float destination);
  // current value
  float var() const;
  // progress, if have any
  float prog() const;
  float dest() const;

private:
  float duration = 200.f;
  float value = 0.f;
  float destination = value;
  float progress = 0.f;

  easing_type easing = easing_type::ease_in_out;
};

using sp_anim_float = std::shared_ptr<animated_float>;
} // namespace ui