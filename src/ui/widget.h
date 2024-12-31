#pragma once
#include "animator.h"
#include "bgfx/bgfx.h"
#include "nanovg/nanovg.h"
#include <memory>
#include <vector>

namespace ui {

struct UpdateContext {
  float delta_t;
};

struct widget {
  animated_float x, y, width, height;
  virtual void render(bgfx::ViewId view, NVGcontext *ctx) = 0;
  virtual void update(const UpdateContext &ctx) = 0;
  virtual ~widget() = default;
};

struct widget_parent : public widget {
  std::vector<std::unique_ptr<widget>> children;
  void render(bgfx::ViewId view, NVGcontext *ctx) override;
  void update(const UpdateContext &ctx) override;
};
} // namespace ui