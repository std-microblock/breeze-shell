#pragma once
#include "nanovg.h"
#include "utils.h"
#include "ui.h"

namespace mb_shell {
struct paint_color {
  enum class type {
    solid,
    linear_gradient,
    radial_gradient
  } type = type::solid;
  NVGcolor color = parse_color("#000000");
  NVGcolor color2 = parse_color("#000000");
  float radius = 0;
  float radius2 = 0;
  float angle = 0;
  void apply_to_ctx(ui::nanovg_context &ctx, float x, float y, float width,
                    float height) const {

    switch (type) {
    case type::solid: {
      ctx.fillColor(color);
      ctx.strokeColor(color);
      return;
    }
    case type::linear_gradient: {
      auto paint = ctx.linearGradient(x, y, x + width * cos(angle),
                                      y + height * sin(angle), color, color2);
      ctx.fillPaint(paint);
      ctx.strokePaint(paint);
      return;
    }
    case type::radial_gradient: {
      auto paint = ctx.radialGradient(x + width / 2, y + height / 2, radius,
                                      radius2, color, color2);
      ctx.fillPaint(paint);
      ctx.strokePaint(paint);
      return;
    }
    }
    throw std::runtime_error("Unknown paint color type");
  }

  // supported patterns:
  // #RRGGBBAA
  // #RRGGBB
  // #RRGGBB00
  // rgba(R, G, B, A)
  // rgb(R, G, B)
  // linear-gradient(angle, color1, color2)
  // radial-gradient(radius, color1, color2)
  // solid(color)
  static paint_color from_string(const std::string &str);
  std::string to_string() const;
};
} // namespace mb_shell