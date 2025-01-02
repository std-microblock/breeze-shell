#include "widget.h"
#include "ui.h"
#include <chrono>
#include <thread>
void ui::widget_parent::render(nanovg_context ctx) {
  widget::render(ctx);
  float orig_offset_x = ctx.offset_x, orig_offset_y = ctx.offset_y;
  for (auto &child : children) {
    ctx.offset_x = *x;
    ctx.offset_y = *y;
    ctx.save();
    child->render(ctx);
    ctx.restore();
  }

  ctx.offset_x = orig_offset_x;
  ctx.offset_y = orig_offset_y;
}
void ui::widget_parent::update(UpdateContext &ctx) {
  widget::update(ctx);
  float orig_offset_x = ctx.offset_x, orig_offset_y = ctx.offset_y;

  for (auto &child : children) {
    ctx.offset_x = *x;
    ctx.offset_y = *y;
    child->update(ctx);
  }

  ctx.offset_x = orig_offset_x;
  ctx.offset_y = orig_offset_y;
}
void ui::widget_parent::add_child(std::unique_ptr<widget> child) {
  children.push_back(std::move(child));
}

void ui::widget::update(UpdateContext &ctx) {
  for (auto anim : anim_floats) {
    anim->update(ctx.delta_t);
  }
}
bool ui::UpdateContext::hovered(widget *w) const {
  return mouse_x >= (w->x->dest() + offset_x) &&
         mouse_x <= (w->x->dest() + w->width->dest() + offset_x) &&
         mouse_y >= (w->y->dest() + offset_y) &&
         mouse_y <= (w->y->dest() + w->height->dest() + offset_y);
}
float ui::widget::measure_height(UpdateContext &ctx) { return height->dest(); }
float ui::widget::measure_width(UpdateContext &ctx) { return width->dest(); }
void ui::widget_parent_flex::update(UpdateContext &ctx) {
  widget_parent::update(ctx);
  float x = 0, y = 0;
  float target_width = 0, target_height = 0;

  for (auto &child : children) {
    if (horizontal) {
      child->x->animate_to(x);
      x += child->measure_width(ctx) + gap;
      target_height = std::max(target_height, child->measure_height(ctx));
    } else {
      child->y->animate_to(y);
      y += child->measure_height(ctx) + gap;
      target_width = std::max(target_width, child->measure_width(ctx));
    }
  }

  if (!auto_size) {
    if (horizontal) {
      target_width = *width;
    } else {
      target_height = *height;
    }
  }

  if (horizontal) {
    width->animate_to(x - gap);
    height->animate_to(target_height);

    for (auto &child : children) {
      child->height->animate_to(target_height);
    }
  } else {
    width->animate_to(target_width);
    height->animate_to(y - gap);

    for (auto &child : children) {
      child->width->animate_to(target_width);
    }
  }
}
