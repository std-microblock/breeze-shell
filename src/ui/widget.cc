#include "widget.h"
void ui::widget_parent::render(bgfx::ViewId view, NVGcontext *ctx) {
  for (auto &child : children) {
    child->render(view, ctx);
  }
}
void ui::widget_parent::update(const UpdateContext &ctx) {
  for (auto &child : children) {
    child->update(ctx);
  }
}
