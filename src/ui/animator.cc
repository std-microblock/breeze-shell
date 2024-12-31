#include "animator.h"
#include <numbers>
#include <print>
void ui::animated_float::update(float delta_t) {
  if (easing == easing_type::mutation) {
    value = destination;
  } else if (easing == easing_type::linear) {
    progress += delta_t / duration;
    if (progress > 1.f) {
      progress = 1.f;
    }

    value = std::lerp(value, destination, progress);
  } else if (easing == easing_type::ease_in) {
    progress += delta_t / duration;
    if (progress > 1.f) {
      progress = 1.f;
    }

    value = std::lerp(value, destination, progress * progress);
  } else if (easing == easing_type::ease_out) {
    progress += delta_t / duration;
    if (progress > 1.f) {
      progress = 1.f;
    }

    value = std::lerp(value, destination, 1 - std::sqrt(1 - progress));
  } else if (easing == easing_type::ease_in_out) {
    progress += delta_t / duration;
    if (progress > 1.f) {
      progress = 1.f;
    }

    value = std::lerp(
        value, destination,
        (0.5f * std::sin(progress * std::numbers::pi - std::numbers::pi / 2) +
         0.5f));
  }
}
void ui::animated_float::animate_to(float destination) {
  if (this->destination == destination)
    return;
  this->destination = destination;
  progress = 0.f;
}
float ui::animated_float::var() const { return value; }
float ui::animated_float::prog() const { return progress; }
void ui::animated_float::reset_to(float destination) {
  value = destination;
  this->destination = destination;
  progress = 1.f;
}
float ui::animated_float::dest() const { return destination; }
