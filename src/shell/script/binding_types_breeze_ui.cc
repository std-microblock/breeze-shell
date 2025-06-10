#include "../contextmenu/menu_widget.h"
#include "animator.h"
#include "binding_types.hpp"
#include "ui.h"
#include "widget.h"
#include <memory>
#include <print>

namespace mb_shell::js {
std::vector<std::shared_ptr<breeze_ui::js_widget>>
breeze_ui::js_widget::children() const {
  std::vector<std::shared_ptr<js_widget>> result;
  if (!$widget)
    return result;

  for (const auto &child : $widget->children) {
    result.push_back(std::make_shared<js_widget>(child));
  }
  return result;
}
std::string breeze_ui::js_text_widget::get_text() const {
  if (!$widget)
    return "";
  auto text_widget = std::dynamic_pointer_cast<ui::text_widget>($widget);
  if (!text_widget)
    return "";
  return text_widget->text;
}

void breeze_ui::js_text_widget::set_text(std::string text) {
  if (!$widget)
    return;
  auto text_widget = std::dynamic_pointer_cast<ui::text_widget>($widget);
  if (!text_widget)
    return;
  text_widget->text = text;
  text_widget->needs_repaint = true;
}

int breeze_ui::js_text_widget::get_font_size() const {
  if (!$widget)
    return 0;
  auto text_widget = std::dynamic_pointer_cast<ui::text_widget>($widget);
  if (!text_widget)
    return 0;
  return text_widget->font_size;
}

void breeze_ui::js_text_widget::set_font_size(int size) {
  if (!$widget)
    return;
  auto text_widget = std::dynamic_pointer_cast<ui::text_widget>($widget);
  if (!text_widget)
    return;
  text_widget->font_size = size;
  text_widget->needs_repaint = true;
}

std::tuple<float, float, float, float>
breeze_ui::js_text_widget::get_color() const {
  if (!$widget)
    return {0.0f, 0.0f, 0.0f, 0.0f};
  auto text_widget = std::dynamic_pointer_cast<ui::text_widget>($widget);
  if (!text_widget)
    return {0.0f, 0.0f, 0.0f, 0.0f};
  auto color_array = *text_widget->color;
  return {color_array[0], color_array[1], color_array[2], color_array[3]};
}

void breeze_ui::js_text_widget::set_color(
    std::optional<std::tuple<float, float, float, float>> color) {
  if (!$widget)
    return;
  auto text_widget = std::dynamic_pointer_cast<ui::text_widget>($widget);
  if (!text_widget)
    return;
  if (color.has_value()) {
    text_widget->color.animate_to(
        {std::get<0>(color.value()), std::get<1>(color.value()),
         std::get<2>(color.value()), std::get<3>(color.value())});
  } else {
    text_widget->color.animate_to({0.0f, 0.0f, 0.0f, 0.0f});
  }
}

void breeze_ui::js_widget::append_child_after(std::shared_ptr<js_widget> child,
                                              int after_index) {
  if (child && child->$widget) {
    $widget->children_dirty = true;
    if (after_index < 0) {
      after_index = $widget->children.size() + after_index + 1;
    }
    $widget->children.insert(
        $widget->children.begin() + std::max(0, after_index),
        std::dynamic_pointer_cast<ui::widget>(child->$widget));
  }
}

void breeze_ui::js_widget::append_child(std::shared_ptr<js_widget> child) {
  append_child_after(child, -1);
}

void breeze_ui::js_widget::prepend_child(std::shared_ptr<js_widget> child) {
  append_child_after(child, 0);
}

void breeze_ui::js_widget::remove_child(std::shared_ptr<js_widget> child) {
  if (!$widget)
    return;

  if (child && child->$widget) {
    auto it = std::find($widget->children.begin(), $widget->children.end(),
                        child->$widget);
    if (it != $widget->children.end()) {
      $widget->children.erase(it);
      $widget->children_dirty = true;
    }
  }
}

std::shared_ptr<breeze_ui::js_text_widget>
breeze_ui::widgets_factory::create_text_widget() {
  auto text_widget = std::make_shared<ui::text_widget>();

  auto res = std::make_shared<js_text_widget>();
  res->$widget = std::dynamic_pointer_cast<ui::widget>(text_widget);
  return res;
}

struct widget_js_base : public ui::widget_flex {
  using super = ui::widget_flex;

  std::function<void(int)> on_click;
  std::function<void(float, float)> on_mouse_move;
  std::function<void()> on_mouse_enter;
  std::function<void(ui::update_context &ctx)> on_mouse_leave;
  std::function<void(ui::update_context &ctx)> on_mouse_down;
  std::function<void(ui::update_context &ctx)> on_mouse_up;
  std::function<void(ui::update_context &ctx)> on_mouse_wheel;
  std::function<void(ui::update_context &ctx)> on_update;

  bool previous_hovered = false;

  float prev_mouse_x = 0, prev_mouse_y = 0;

  void update(ui::update_context &ctx) override {
    super::update(ctx);

    try {
      if (on_update) {
        on_update(ctx);
      }

      auto weak = weak_from_this();
      if (ctx.hovered(this) && ctx.mouse_clicked && on_click) {
        on_click(0);
        if (weak.expired())
          return;
      }

      if (ctx.hovered(this) && !previous_hovered && on_mouse_enter) {
        on_mouse_enter();
        if (weak.expired())
          return;
      } else if (!ctx.hovered(this) && previous_hovered && on_mouse_leave) {
        on_mouse_leave(ctx);
        if (weak.expired())
          return;
      }

      previous_hovered = ctx.hovered(this);
      if (ctx.mouse_down_on(this) && on_mouse_down) {
        on_mouse_down(ctx);
        if (weak.expired())
          return;
      }

      if (ctx.mouse_up && on_mouse_up) {
        on_mouse_up(ctx);
        if (weak.expired())
          return;
      }

      if (ctx.mouse_x != prev_mouse_x || ctx.mouse_y != prev_mouse_y) {
        prev_mouse_x = ctx.mouse_x;
        prev_mouse_y = ctx.mouse_y;
        if (on_mouse_move && ctx.hovered(this)) {
          on_mouse_move(ctx.mouse_x, ctx.mouse_y);
          if (weak.expired())
            return;
        }
      }

      if (ctx.scroll_y != 0 && on_mouse_wheel) {
        on_mouse_wheel(ctx);
        if (weak.expired())
          return;
      }
    } catch (const std::exception &e) {
      std::cerr << "Exception in widget update: " << e.what() << std::endl;
    }
  }

  ui::sp_anim_float opacity = anim_float(255), border_radius = anim_float(0),
                    border_width = anim_float(0);
  ui::animated_color background_color = {this, 0.2f, 0.2f, 0.2f, 0.6f},
                     border_color = {this, 0.0f, 0.0f, 0.0f, 1.0f};
  bool inset_border = false;

  std::optional<paint_color> background_paint, border_paint;

  void render(ui::nanovg_context ctx) override {
    float rx = *x, ry = *y, rw = *width, rh = *height;
    if (inset_border) {
      rx += *border_width / 2;
      ry += *border_width / 2;
      rw -= *border_width;
      rh -= *border_width;
    }

    auto scope = ctx.transaction();

    ctx.globalAlpha(*opacity / 255.f);
    if (background_paint) {
      background_paint->apply_to_ctx(ctx, rx, ry, rw, rh);
    } else {
      ctx.fillColor(background_color);
    }

    ctx.fillRoundedRect(rx, ry, rw, rh, *border_radius);

    if (border_paint) {
      border_paint->apply_to_ctx(ctx, rx, ry, rw, rh);
    } else {
      ctx.strokeColor(border_color);
    }

    if (*border_width > 0) {
      ctx.strokeWidth(*border_width);
      ctx.strokeRoundedRect(rx, ry, rw, rh, *border_radius);
    }

    super::render(ctx);
  }
};

std::shared_ptr<breeze_ui::js_flex_layout_widget>
breeze_ui::widgets_factory::create_flex_layout_widget() {
  auto layout_widget = std::make_shared<widget_js_base>();
  auto res = std::make_shared<js_flex_layout_widget>();
  res->$widget = std::dynamic_pointer_cast<ui::widget>(layout_widget);
  return res;
}

float breeze_ui::js_flex_layout_widget::get_padding_left() const {
  if (!$widget)
    return 0.0f;
  auto flex_widget = std::dynamic_pointer_cast<widget_js_base>($widget);
  if (!flex_widget)
    return 0.0f;
  return flex_widget->padding_left->dest();
}
void breeze_ui::js_flex_layout_widget::set_padding_left(float padding) {
  if (!$widget)
    return;
  auto flex_widget = std::dynamic_pointer_cast<widget_js_base>($widget);
  if (!flex_widget)
    return;
  flex_widget->padding_left->animate_to(padding);
}
float breeze_ui::js_flex_layout_widget::get_padding_right() const {
  if (!$widget)
    return 0.0f;
  auto flex_widget = std::dynamic_pointer_cast<widget_js_base>($widget);
  if (!flex_widget)
    return 0.0f;
  return flex_widget->padding_right->dest();
}
void breeze_ui::js_flex_layout_widget::set_padding_right(float padding) {
  if (!$widget)
    return;
  auto flex_widget = std::dynamic_pointer_cast<widget_js_base>($widget);
  if (!flex_widget)
    return;
  flex_widget->padding_right->animate_to(padding);
}
float breeze_ui::js_flex_layout_widget::get_padding_top() const {
  if (!$widget)
    return 0.0f;
  auto flex_widget = std::dynamic_pointer_cast<widget_js_base>($widget);
  if (!flex_widget)
    return 0.0f;
  return flex_widget->padding_top->dest();
}
void breeze_ui::js_flex_layout_widget::set_padding_top(float padding) {
  if (!$widget)
    return;
  auto flex_widget = std::dynamic_pointer_cast<widget_js_base>($widget);
  if (!flex_widget)
    return;
  flex_widget->padding_top->animate_to(padding);
}
float breeze_ui::js_flex_layout_widget::get_padding_bottom() const {
  if (!$widget)
    return 0.0f;
  auto flex_widget = std::dynamic_pointer_cast<widget_js_base>($widget);
  if (!flex_widget)
    return 0.0f;
  return flex_widget->padding_bottom->dest();
}
void breeze_ui::js_flex_layout_widget::set_padding_bottom(float padding) {
  if (!$widget)
    return;
  auto flex_widget = std::dynamic_pointer_cast<widget_js_base>($widget);
  if (!flex_widget)
    return;
  flex_widget->padding_bottom->animate_to(padding);
}
std::tuple<float, float, float, float>
breeze_ui::js_flex_layout_widget::get_padding() const {
  return {get_padding_left(), get_padding_right(), get_padding_top(),
          get_padding_bottom()};
}
bool breeze_ui::js_flex_layout_widget::get_horizontal() const {
  return $widget && std::dynamic_pointer_cast<widget_js_base>($widget)
             ? std::dynamic_pointer_cast<widget_js_base>($widget)->horizontal
             : false;
}
void breeze_ui::js_flex_layout_widget::set_horizontal(bool horizontal) {
  if ($widget) {
    auto flex_widget = std::dynamic_pointer_cast<widget_js_base>($widget);
    if (flex_widget) {
      flex_widget->horizontal = horizontal;
    }
  }
}

void breeze_ui::js_flex_layout_widget::set_padding(float left, float right,
                                                   float top, float bottom) {
  set_padding_left(left);
  set_padding_right(right);
  set_padding_top(top);
  set_padding_bottom(bottom);
}
std::variant<std::shared_ptr<breeze_ui::js_widget>,
             std::shared_ptr<breeze_ui::js_text_widget>,
             std::shared_ptr<breeze_ui::js_flex_layout_widget>>
breeze_ui::js_widget::downcast() {
#define TRY_DOWNCAST(type)                                                     \
  if (auto casted =                                                            \
          std::dynamic_pointer_cast<type>(this->shared_from_this())) {         \
    return casted;                                                             \
  }
  TRY_DOWNCAST(js_text_widget);
  TRY_DOWNCAST(js_flex_layout_widget);
#undef TRY_DOWNCAST

  return this->shared_from_this();
}
std::shared_ptr<breeze_ui::breeze_paint>
breeze_ui::breeze_paint::from_color(std::string color) {
  auto paint = std::make_shared<breeze_paint>();
  paint->$paint = paint_color::from_string(color);
  return paint;
}
std::function<void(int)>
breeze_ui::js_flex_layout_widget::get_on_click() const {
  if (!$widget)
    return nullptr;
  auto flex_widget = std::dynamic_pointer_cast<widget_js_base>($widget);
  if (!flex_widget)
    return nullptr;
  return flex_widget->on_click;
}
void breeze_ui::js_flex_layout_widget::set_on_click(
    std::function<void(int)> on_click) {
  if ($widget) {
    auto flex_widget = std::dynamic_pointer_cast<widget_js_base>($widget);
    if (flex_widget) {
      flex_widget->on_click = on_click;
    }
  }
}
std::function<void(float, float)>
breeze_ui::js_flex_layout_widget::get_on_mouse_move() const {
  if ($widget) {
    auto flex_widget = std::dynamic_pointer_cast<widget_js_base>($widget);
    if (flex_widget) {
      return flex_widget->on_mouse_move;
    }
  }
  return nullptr;
}
void breeze_ui::js_flex_layout_widget::set_on_mouse_move(
    std::function<void(float, float)> on_mouse_move) {
  if ($widget) {
    auto flex_widget = std::dynamic_pointer_cast<widget_js_base>($widget);
    if (flex_widget) {
      flex_widget->on_mouse_move = on_mouse_move;
    }
  }
}
std::function<void()>
breeze_ui::js_flex_layout_widget::get_on_mouse_enter() const {
  if ($widget) {
    auto flex_widget = std::dynamic_pointer_cast<widget_js_base>($widget);
    if (flex_widget) {
      return flex_widget->on_mouse_enter;
    }
  }
  return nullptr;
}
void breeze_ui::js_flex_layout_widget::set_on_mouse_enter(
    std::function<void()> on_mouse_enter) {
  if ($widget) {
    auto flex_widget = std::dynamic_pointer_cast<widget_js_base>($widget);
    if (flex_widget) {
      flex_widget->on_mouse_enter = on_mouse_enter;
    }
  }
}
void breeze_ui::js_flex_layout_widget::set_background_color(
    std::optional<std::tuple<float, float, float, float>> color) {
  if ($widget) {
    auto flex_widget = std::dynamic_pointer_cast<widget_js_base>($widget);
    if (flex_widget) {
      if (color.has_value()) {
        flex_widget->background_color.animate_to(
            {std::get<0>(color.value()), std::get<1>(color.value()),
             std::get<2>(color.value()), std::get<3>(color.value())});
      } else {
        flex_widget->background_color.animate_to({0.0f, 0.0f, 0.0f, 0.0f});
      }
    }
  }
}
std::optional<std::tuple<float, float, float, float>>
breeze_ui::js_flex_layout_widget::get_background_color() const {
  if ($widget) {
    auto flex_widget = std::dynamic_pointer_cast<widget_js_base>($widget);
    if (flex_widget) {
      auto color = *flex_widget->background_color;
      return std::make_tuple(color[0], color[1], color[2], color[3]);
    }
  }
  return std::nullopt;
}
void breeze_ui::js_flex_layout_widget::set_background_paint(
    std::shared_ptr<breeze_ui::breeze_paint> paint) {
  if ($widget) {
    auto flex_widget = std::dynamic_pointer_cast<widget_js_base>($widget);
    if (flex_widget && paint) {
      flex_widget->background_paint = paint->$paint;
    }
  }
}
std::shared_ptr<breeze_ui::breeze_paint>
breeze_ui::js_flex_layout_widget::get_background_paint() const {
  if ($widget) {
    auto flex_widget = std::dynamic_pointer_cast<widget_js_base>($widget);
    if (flex_widget) {
      return std::make_shared<breeze_paint>(*flex_widget->background_paint);
    }
  }
  return nullptr;
}
void breeze_ui::js_flex_layout_widget::set_border_radius(float radius) {
  if ($widget) {
    auto flex_widget = std::dynamic_pointer_cast<widget_js_base>($widget);
    if (flex_widget) {
      flex_widget->border_radius->animate_to(radius);
    }
  }
}
float breeze_ui::js_flex_layout_widget::get_border_radius() const {
  if ($widget) {
    auto flex_widget = std::dynamic_pointer_cast<widget_js_base>($widget);
    if (flex_widget) {
      return flex_widget->border_radius->dest();
    }
  }
  return 0.0f;
}
void breeze_ui::js_flex_layout_widget::set_border_color(
    std::optional<std::tuple<float, float, float, float>> color) {
  if ($widget) {
    auto flex_widget = std::dynamic_pointer_cast<widget_js_base>($widget);
    if (flex_widget) {
      if (color.has_value()) {
        flex_widget->border_color.animate_to(
            {std::get<0>(color.value()), std::get<1>(color.value()),
             std::get<2>(color.value()), std::get<3>(color.value())});
      }
    }
  }
}
std::optional<std::tuple<float, float, float, float>>
breeze_ui::js_flex_layout_widget::get_border_color() const {
  if ($widget) {
    auto flex_widget = std::dynamic_pointer_cast<widget_js_base>($widget);
    if (flex_widget) {
      auto color = *flex_widget->border_color;
      return std::make_tuple(color[0], color[1], color[2], color[3]);
    }
  }
  return std::nullopt;
}
void breeze_ui::js_flex_layout_widget::set_border_width(float width) {
  if ($widget) {
    auto flex_widget = std::dynamic_pointer_cast<widget_js_base>($widget);
    if (flex_widget) {
      flex_widget->border_width->animate_to(width);
    }
  }
}
float breeze_ui::js_flex_layout_widget::get_border_width() const {
  if ($widget) {
    auto flex_widget = std::dynamic_pointer_cast<widget_js_base>($widget);
    if (flex_widget) {
      return flex_widget->border_width->dest();
    }
  }
  return 0.0f;
}
void breeze_ui::js_flex_layout_widget::set_border_paint(
    std::shared_ptr<breeze_paint> paint) {
  if ($widget) {
    auto flex_widget = std::dynamic_pointer_cast<widget_js_base>($widget);
    if (flex_widget) {
      flex_widget->border_paint = paint->$paint;
    }
  }
}
std::shared_ptr<breeze_ui::breeze_paint>
breeze_ui::js_flex_layout_widget::get_border_paint() const {
  if ($widget) {
    auto flex_widget = std::dynamic_pointer_cast<widget_js_base>($widget);
    if (flex_widget && flex_widget->border_paint) {
      return std::make_shared<breeze_paint>(*flex_widget->border_paint);
    }
  }
  return nullptr;
}
} // namespace mb_shell::js
