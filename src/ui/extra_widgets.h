#pragma once
#include "animator.h"
#include "nanovg.h"
#include "widget.h"

namespace ui {
struct acrylic_background_widget : public widget {
  void *hwnd;
  acrylic_background_widget();
  ~acrylic_background_widget();
  sp_anim_float opacity = anim_float(0, 200);
  sp_anim_float radius = anim_float(0, 200);
  NVGcolor acrylic_bg_color = nvgRGBAf(0, 0, 0, 0);

  void update_color();

  void render(nanovg_context ctx) override;

  void update(UpdateContext &ctx) override;
};
} // namespace ui