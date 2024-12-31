#pragma once
#include <cmath>

namespace ui {
struct animated_float {
  void update(float delta_t);

  void animate_to(float destination);
  // current value
  float var() const;
  // progress, if have any
  float prog() const;

private:
  float duration = 200.f;
  float value = 0.f;
  float destination = value;
  float progress = 0.f;

  enum class easing_type {
    mutation,
    linear,
  } easing = easing_type::linear;
};
} // namespace ui