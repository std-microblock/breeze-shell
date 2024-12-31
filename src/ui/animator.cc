#include "animator.h"
#include <numbers>
#include <print>
void ui::animated_float::update(float delta_t) {
  if (easing == easing_type::mutation) {
    if (destination != value) {
      value = destination;
      _updated = true;
    } else {
      _updated = false;
    }
    return;
  }

  progress += delta_t / duration;

  if (easing == easing_type::linear) {
    value = std::lerp(value, destination, progress);
  } else if (easing == easing_type::ease_in) {
    value = std::lerp(value, destination, progress * progress);
  } else if (easing == easing_type::ease_out) {
    value = std::lerp(value, destination, 1 - std::sqrt(1 - progress));
  } else if (easing == easing_type::ease_in_out) {
    value = std::lerp(
        value, destination,
        (0.5f * std::sin(progress * std::numbers::pi - std::numbers::pi / 2) +
         0.5f));
  }

  if (progress >= 1.f) {
    progress = 1.f;
    _updated = false;
    return;
  }

  _updated = true;
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
void ui::animated_float::set_easing(easing_type easing) {
  this->easing = easing;
}
void ui::animated_float::set_duration(float duration) {
  this->duration = duration;
}
bool ui::animated_float::updated() const { return _updated; }
