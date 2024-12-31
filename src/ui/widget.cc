#include "widget.h"
void ui::widget_parent::render(nanovg_context ctx) {
  for (auto &child : children) {
    ctx.save();
    child->render(ctx);
    ctx.restore();
  }
}
void ui::widget_parent::update(const UpdateContext &ctx) {
  widget::update(ctx);
  for (auto &child : children) {
    child->update(ctx);
  }
}
void ui::widget_parent::add_child(std::unique_ptr<widget> child) {
  children.push_back(std::move(child));
}

void ui::widget::update(const UpdateContext &ctx) {
  for (auto anim : anim_floats) {
    anim->update(ctx.delta_t);
  }
}
bool ui::UpdateContext::hovered(widget *w) const {
  return mouse_x >= *w->x && mouse_x <= *w->x + *w->width && mouse_y >= *w->y &&
         mouse_y <= *w->y + *w->height;
}