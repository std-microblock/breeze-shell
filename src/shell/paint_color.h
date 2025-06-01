#pragma once
#include <string>
#include <array>

struct NVGcolor;

namespace ui {
struct nanovg_context;
}

namespace mb_shell {

struct rgba_color {
    float r = 0;
    float g = 0;
    float b = 0;
    float a = 1;
    
    rgba_color() = default;
    rgba_color(float r, float g, float b, float a = 1)
        : r(r), g(g), b(b), a(a) {}

    static rgba_color from_string(const std::string &str);
    std::string to_string() const;

    rgba_color(const NVGcolor &color);
    NVGcolor nvg() const;
    // we want to pass this directly into fillColor( NVGcolor color)
    operator NVGcolor() const;

};

struct paint_color {
  enum class type {
    solid,
    linear_gradient,
    radial_gradient
  } type = type::solid;
  rgba_color color = {0, 0, 0, 1};  // RGBA
  rgba_color color2 = {0, 0, 0, 1}; // RGBA for gradients
  float radius = 0;
  float radius2 = 0;
  float angle = 0;
  void apply_to_ctx(ui::nanovg_context &ctx, float x, float y, float width,
                    float height) const;

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