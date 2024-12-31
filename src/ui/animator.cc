#include "animator.h"
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
