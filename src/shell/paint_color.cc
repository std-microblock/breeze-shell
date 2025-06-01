#include "paint_color.h"
#include "nanovg.h"
#include "ui.h"
#include "utils.h"
#include <numbers>
#include <vector>

namespace mb_shell {
paint_color paint_color::from_string(const std::string &str) {
  paint_color res;
  // Trim whitespace
  std::string trimmed = str;
  trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
  trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);

  if (trimmed.starts_with("solid(") && trimmed.ends_with(")")) {
    std::string color_str = trimmed.substr(6, trimmed.length() - 7);
    res.type = type::solid;
    res.color = rgba_color::from_string(color_str);
  } else if (trimmed.starts_with("linear-gradient(") &&
             trimmed.ends_with(")")) {
    std::string params = trimmed.substr(16, trimmed.length() - 17);

    // Split by commas
    std::vector<std::string> parts;
    size_t start = 0, end = 0;
    while ((end = params.find(',', start)) != std::string::npos) {
      std::string part = params.substr(start, end - start);
      part.erase(0, part.find_first_not_of(" \t"));
      part.erase(part.find_last_not_of(" \t") + 1);
      parts.push_back(part);
      start = end + 1;
    }
    std::string last_part = params.substr(start);
    last_part.erase(0, last_part.find_first_not_of(" \t"));
    last_part.erase(last_part.find_last_not_of(" \t") + 1);
    parts.push_back(last_part);

    if (parts.size() >= 3) {
      res.type = type::linear_gradient;
      res.angle = std::stof(parts[0]) * std::numbers::pi /
                  180.0f; // Convert degrees to radians
      res.color = parse_color(parts[1]);
      res.color2 = parse_color(parts[2]);
    }
  } else if (trimmed.starts_with("radial-gradient(") &&
             trimmed.ends_with(")")) {
    std::string params = trimmed.substr(16, trimmed.length() - 17);

    // Split by commas
    std::vector<std::string> parts;
    size_t start = 0, end = 0;
    while ((end = params.find(',', start)) != std::string::npos) {
      std::string part = params.substr(start, end - start);
      part.erase(0, part.find_first_not_of(" \t"));
      part.erase(part.find_last_not_of(" \t") + 1);
      parts.push_back(part);
      start = end + 1;
    }
    std::string last_part = params.substr(start);
    last_part.erase(0, last_part.find_first_not_of(" \t"));
    last_part.erase(last_part.find_last_not_of(" \t") + 1);
    parts.push_back(last_part);

    if (parts.size() >= 3) {
      res.type = type::radial_gradient;
      res.radius = std::stof(parts[0]);
      res.color = parse_color(parts[1]);
      res.color2 = parse_color(parts[2]);
      res.radius2 = res.radius * 2; // Default outer radius
    }
  } else {
    // Default to solid color
    res.type = type::solid;
    res.color = parse_color(trimmed);
  }

  return res;
}
std::string paint_color::to_string() const {
  switch (type) {
  case type::solid:
    return "solid(" + format_color(color) + ")";
  case type::linear_gradient:
    return "linear-gradient(" +
           std::to_string(angle * 180.0f / std::numbers::pi) + ", " +
           format_color(color) + ", " + format_color(color2) + ")";
  case type::radial_gradient:
    return "radial-gradient(" + std::to_string(radius) + ", " +
           format_color(color) + ", " + format_color(color2) + ")";
  }
  return "";
}
void paint_color::apply_to_ctx(ui::nanovg_context &ctx, float x, float y,
                               float width, float height) const {

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
rgba_color::operator NVGcolor() const {
  return nvgRGBAf(r, g, b, a);
}
rgba_color::rgba_color(const NVGcolor &color)
    : r(color.r), g(color.g), b(color.b), a(color.a) {}
NVGcolor rgba_color::nvg() const { return nvgRGBAf(r, g, b, a); }
rgba_color rgba_color::from_string(const std::string &str) {
    return parse_color(str);
}
std::string rgba_color::to_string() const {
  return std::format("#{0:02x}{1:02x}{2:02x}{3:02x}", static_cast<int>(r * 255),
                     static_cast<int>(g * 255), static_cast<int>(b * 255),
                     static_cast<int>(a * 255));
}
} // namespace mb_shell