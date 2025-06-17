#pragma once
#include "nanovg.h"
#include <array>
#include <cmath>
#include <functional>
#include <optional>
#include <print>

namespace ui {
struct widget;
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
                 easing_type easing = easing_type::mutation)
      : easing(easing), duration(duration), destination(destination) {}

  std::optional<std::function<void(float)>> before_animate = {};
  std::optional<std::function<void(float)>> after_animate = {};

  operator float() const { return var(); }
  float operator*() const { return var(); }
  void update(float delta_time);

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

  easing_type easing = easing_type::mutation;
  float progress = 0.f;

private:
  float duration = 200.f;
  float value = 0.f;
  float from = 0.f;
  float destination = value;
  float delay = 0.f, delay_timer = 0.f;
  bool _updated = true;
};

using sp_anim_float = std::shared_ptr<animated_float>;

struct animated_color {
  sp_anim_float r = nullptr;
  sp_anim_float g = nullptr;
  sp_anim_float b = nullptr;
  sp_anim_float a = nullptr;

  operator NVGcolor() {
    return nvgRGBAf(r->var(), g->var(), b->var(), a->var());
  }
  animated_color() = delete;
  animated_color(animated_color &&) = default;

  animated_color(ui::widget *thiz, float r = 0, float g = 0, float b = 0,
                 float a = 0);

  NVGcolor blend(const animated_color &other, float factor = 0.5f) const {
    return nvgRGBAf(r->var() * (1 - factor) + other.r->var() * factor,
                    g->var() * (1 - factor) + other.g->var() * factor,
                    b->var() * (1 - factor) + other.b->var() * factor,
                    a->var() * (1 - factor) + other.a->var() * factor);
  }

  std::array<float, 4> operator*() const;

  inline void animate_to(float r, float g, float b, float a) {
    this->r->animate_to(r);
    this->g->animate_to(g);
    this->b->animate_to(b);
    this->a->animate_to(a);
  }

  inline void animate_to(const std::array<float, 4> &color) {
    animate_to(color[0], color[1], color[2], color[3]);
  }

  inline void reset_to(float r, float g, float b, float a) {
    this->r->reset_to(r);
    this->g->reset_to(g);
    this->b->reset_to(b);
    this->a->reset_to(a);
  }

  inline void reset_to(const std::array<float, 4> &color) {
    reset_to(color[0], color[1], color[2], color[3]);
  }

  inline NVGcolor nvg() const {
    return nvgRGBAf(r->var(), g->var(), b->var(), a->var());
  }
};
} // namespace ui