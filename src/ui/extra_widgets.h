#pragma once

#include "animator.h"
#include "widget.h"

namespace ui {
struct acrylic_background_widget : public widget {
  void *hwnd;

  acrylic_background_widget();
  ~acrylic_background_widget();
  sp_anim_float opacity = anim_float(0, 200);

  void render(nanovg_context ctx) override;

  void update(const UpdateContext &ctx) override;
};
} // namespace ui