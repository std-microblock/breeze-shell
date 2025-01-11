#include "widget.h"
#include "ui.h"
#include <chrono>
#include <thread>
void ui::widget_parent::render(ui::render_context& ctx) {
  widget::render(ctx);
  auto new_ctx = ctx.with_offset(*x, *y);
  for (auto &child : children) {
     child->render(new_ctx);
  }
  ctx->push(new_ctx.vg);
}
void ui::widget_parent::update(update_context &ctx) {
  widget::update(ctx);
  float orig_offset_x = ctx.offset_x, orig_offset_y = ctx.offset_y;

  for (auto &child : children) {
    ctx.offset_x = *x + orig_offset_x;
    ctx.offset_y = *y + orig_offset_y;
    child->update(ctx);
  }

  ctx.offset_x = orig_offset_x;
  ctx.offset_y = orig_offset_y;
}
void ui::widget_parent::add_child(std::shared_ptr<widget> child) {
  children.push_back(std::move(child));
}

void ui::widget::update(update_context &ctx) {
  for (auto anim : anim_floats) {
    anim->update(ctx.delta_t);
  }
}
bool ui::update_context::hovered(widget *w) const {
  return mouse_x >= (w->x->dest() + offset_x) &&
         mouse_x <= (w->x->dest() + w->width->dest() + offset_x) &&
         mouse_y >= (w->y->dest() + offset_y) &&
         mouse_y <= (w->y->dest() + w->height->dest() + offset_y);
}
float ui::widget::measure_height(update_context &ctx) { return height->dest(); }
float ui::widget::measure_width(update_context &ctx) { return width->dest(); }
void ui::widget_parent_flex::update(update_context &ctx) {
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
void ui::update_context::set_hit_hovered(widget *w) {
  hovered_widgets.push_back(w->shared_from_this());
}
bool ui::update_context::mouse_clicked_on(widget *w) const {
  return mouse_clicked && hovered(w);
}
bool ui::update_context::mouse_down_on(widget *w) const {
  return mouse_down && hovered(w);
}
void ui::update_context::set_hit_clicked(widget *w) {
  clicked_widgets.push_back(w->shared_from_this());
}
bool ui::update_context::mouse_clicked_on_hit(widget *w) {
  if (mouse_clicked_on(w)) {
    set_hit_clicked(w);
    return true;
  }
  return false;
}
bool ui::update_context::hovered_hit(widget *w) {
  if (hovered(w)) {
    set_hit_hovered(w);
    return true;
  } else {
    return false;
  }
}
ui::render_context ui::render_context::with_offset(float x, float y) {
  render_context copy = *this;
  copy.offset_x = x + offset_x;
  copy.offset_y = y + offset_y;
  copy.vg = tvg::Scene::gen();
  copy.vg->translate(x, y);
  return copy;
}
