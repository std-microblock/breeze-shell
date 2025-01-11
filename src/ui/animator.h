#pragma once
#include <array>
#include <cmath>
#include <functional>
#include <optional>
#include <print>

#include "thorvg.h"


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

struct animated_color {
  sp_anim_float r = nullptr;
  sp_anim_float g = nullptr;
  sp_anim_float b = nullptr;
  sp_anim_float a = nullptr;

  animated_color() = delete;
  animated_color(animated_color &&) = default;

  animated_color(ui::widget *thiz, float r = 0, float g = 0, float b = 0,
                 float a = 0);

  std::array<float, 4> operator*() const;

  inline void fill_shape(tvg::Shape* shape) {
    shape->fill(r->var(), g->var(), b->var(), a->var());
  }

  inline void color_text(tvg::Text* text) {
    text->opacity(a->var());
    text->fill(r->var(), g->var(), b->var());
  }
};
} // namespace ui