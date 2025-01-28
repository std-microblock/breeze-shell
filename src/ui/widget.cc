#include "widget.h"
#include "ui.h"
#include <algorithm>
#include <chrono>
#include <print>
#include <thread>

void ui::widget::update_child_basic(update_context &ctx,
                                    std::shared_ptr<widget> &w) {
  if (!w)
    return;
  // handle dying time
  if (w->dying_time && w->dying_time.time <= 0) {
    w = nullptr;
    return;
  }
  update_context upd = ctx.with_offset(*x, *y);
  w->update(upd);
}

void ui::widget::render_child_basic(nanovg_context ctx,
                                    std::shared_ptr<widget> &w) {
  if (!w)
    return;
  ctx.save();
  w->render(ctx.with_offset(*x, *y));
  ctx.restore();
}

void ui::widget::render(nanovg_context ctx) {
  if constexpr (false)
    if (_debug_offset_cache[0] != ctx.offset_x ||
        _debug_offset_cache[1] != ctx.offset_y) {
      if (_debug_offset_cache[0] != -1 || _debug_offset_cache[1] != -1) {
        std::println(
            "[Warning] The offset during render is different from the "
            "offset during update: (update) {} {} vs (render) {} {} ({}, {})",
            _debug_offset_cache[0], _debug_offset_cache[1], ctx.offset_x,
            ctx.offset_y, (void *)this, typeid(*this).name());
      } else {
        std::println(
            "[Warning] The update function is not called before render: {}",
            (void *)this);
      }
    }

  _debug_offset_cache[0] = -1;
  _debug_offset_cache[1] = -1;

  render_children(ctx, children);
}
void ui::widget::update(update_context &ctx) {
  for (auto anim : anim_floats) {
    anim->update(ctx.delta_t);
  }

  dying_time.update(ctx.delta_t);

  update_children(ctx, children);
  if constexpr (false)
    if (_debug_offset_cache[0] != -1 || _debug_offset_cache[1] != -1) {
      std::println(
          "[Warning] The update function is called twice with different "
          "offsets: {} {} vs {} {} ({})",
          _debug_offset_cache[0], _debug_offset_cache[1], ctx.offset_x,
          ctx.offset_y, (void *)this);
    }
  _debug_offset_cache[0] = ctx.offset_x;
  _debug_offset_cache[1] = ctx.offset_y;
}
void ui::widget::add_child(std::shared_ptr<widget> child) {
  children.push_back(std::move(child));
}

bool ui::update_context::hovered(widget *w, bool hittest) const {
  if (hittest && (!hovered_widgets->empty())) {
    return false;
  }

  return w->check_hit(*this);
}
float ui::widget::measure_height(update_context &ctx) { return height->dest(); }
float ui::widget::measure_width(update_context &ctx) { return width->dest(); }
void ui::widget_flex::update(update_context &ctx) {
  widget::update(ctx);
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
  hovered_widgets->push_back(w);
}
bool ui::update_context::mouse_clicked_on(widget *w, bool hittest) const {
  return mouse_clicked && hovered(w, hittest);
}
bool ui::update_context::mouse_down_on(widget *w, bool hittest) const {
  return mouse_down && hovered(w, hittest);
}
bool ui::update_context::mouse_clicked_on_hit(widget *w) {
  if (mouse_clicked_on(w)) {
    set_hit_hovered(w);
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
bool ui::widget::check_hit(const update_context &ctx) {
  return ctx.mouse_x >= (x->dest() + ctx.offset_x) &&
         ctx.mouse_x <= (x->dest() + width->dest() + ctx.offset_x) &&
         ctx.mouse_y >= (y->dest() + ctx.offset_y) &&
         ctx.mouse_y <= (y->dest() + height->dest() + ctx.offset_y);
}
void ui::widget::update_children(
    update_context &ctx, std::vector<std::shared_ptr<widget>> &children) {
  for (auto &child : children) {
    update_child_basic(ctx, child);
  }

  // Remove dead children
  std::erase_if(children, [](auto &child) { return !child; });
}
void ui::widget::render_children(
    nanovg_context ctx, std::vector<std::shared_ptr<widget>> &children) {
  for (auto &child : children) {
    render_child_basic(ctx, child);
  }
}
void ui::text_widget::render(nanovg_context ctx) {
  widget::render(ctx);
  ctx.fontSize(font_size);
  ctx.fillColor(color.nvg());
  ctx.text(x->dest(), y->dest(), text.c_str(), nullptr);
}
float ui::text_widget::measure_width(update_context &ctx) {
  return ctx.vg.measureText(text.c_str()).first;
}
float ui::text_widget::measure_height(update_context &ctx) {
  return ctx.vg.measureText(text.c_str()).second;
}
