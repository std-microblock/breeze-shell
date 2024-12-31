#include "animator.h"
void ui::animated_float::update(float delta_t) {
  if (easing == easing_type::mutation) {
    value = destination;
  } else if (easing == easing_type::linear) {
    progress += delta_t / duration;
    if (progress > 1.f) {
      progress = 1.f;
    }
    value = std::lerp(0.f, destination, progress);
  }
}
void ui::animated_float::animate_to(float destination) {
  this->destination = destination;
}
