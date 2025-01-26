#pragma once
#include "animator.h"
#include "nanovg_wrapper.h"

#include <functional>
#include <memory>
#include <vector>

namespace ui {
struct render_target;
struct widget;
struct screen_info {
  int width, height;
  float dpi_scale;
};
struct update_context {
  // time since last frame, in milliseconds
  float delta_t;
  // mouse position in window coordinates
  double mouse_x, mouse_y;
  bool mouse_down, right_mouse_down;
  // only true for one frame
  bool mouse_clicked, right_mouse_clicked;
  bool mouse_up;

  screen_info screen;

  // hit test
  std::vector<std::shared_ptr<widget>> hovered_widgets;
  std::vector<std::shared_ptr<widget>> clicked_widgets;
  void set_hit_hovered(widget *w);
  void set_hit_clicked(widget *w);

  bool hovered(widget *w, bool hittest = true) const;
  bool mouse_clicked_on(widget *w, bool hittest = true) const;
  bool mouse_down_on(widget *w, bool hittest = true) const;

  bool mouse_clicked_on_hit(widget *w);
  bool hovered_hit(widget *w);

  float offset_x = 0, offset_y = 0;
  render_target &rt;
  nanovg_context vg;

  update_context with_offset(float x, float y) const {
    auto copy = *this;
    copy.offset_x = x + offset_x;
    copy.offset_y = y + offset_y;
    return copy;
  }
};

struct dying_time {
  float time;
  bool _last_has_value = false;
  bool _changed = false;
  inline bool changed() const {
    return _last_has_value != has_value || _changed;
  }
  bool has_value;

  operator bool() const { return has_value; }
  inline float operator=(float t) {
    time = t;
    has_value = true;
    return t;
  }
  inline float operator-=(float t) {
    time -= t;
    has_value = true;
    return time;
  }
  inline void operator=(std::nullopt_t) { has_value = false; }
  inline void reset() {
    has_value = false;
    time = 0;
  }

  inline void update(float dt) {
    if (has_value && time > 0) {
      time -= dt;
    }

    if (_last_has_value != has_value) {
      _changed = true;
      _last_has_value = has_value;
    } else {
      _changed = false;
    }
  }
};

/*
All the widgets in the tree should be wrapped in a shared_ptr.
If you want to use a widget in multiple places, you should create a new instance
for each place.

It is responsible for updating and rendering its children
It also sets the offset for the children
It's like `posision: relative` in CSS
While all other widgets are like `position: absolute`
*/
struct widget : std::enable_shared_from_this<widget> {
  std::vector<sp_anim_float> anim_floats{};
  sp_anim_float anim_float(auto &&...args) {
    auto anim =
        std::make_shared<animated_float>(std::forward<decltype(args)>(args)...);
    anim_floats.push_back(anim);
    return anim;
  }

  sp_anim_float x = anim_float(), y = anim_float(), width = anim_float(),
                height = anim_float();
  virtual void render(nanovg_context ctx);
  virtual void update(update_context &ctx);
  virtual ~widget() = default;
  virtual float measure_height(update_context &ctx);
  virtual float measure_width(update_context &ctx);
  // Update children with the offset.
  // Also deal with the dying time. (If the widget is died, it will be set to
  // nullptr)
  void update_child_basic(update_context &ctx, std::shared_ptr<widget> &w);
  // Render children with the offset.
  void render_child_basic(nanovg_context ctx, std::shared_ptr<widget> &w);

  // Update children list in the widget manner
  // It will remove the dead children
  // It will also update the dying time
  // It will also update the children with the offset
  void update_children(update_context &ctx,
                       std::vector<std::shared_ptr<widget>> &children);
  // Render children list in the widget manner
  void render_children(nanovg_context ctx,
                       std::vector<std::shared_ptr<widget>> &children);

  template <typename T> inline auto downcast() {
    return std::dynamic_pointer_cast<T>(this->shared_from_this());
  }

  virtual bool check_hit(const update_context &ctx);

  void add_child(std::shared_ptr<widget> child);
  std::vector<std::shared_ptr<widget>> children;
  template <typename T, typename... Args>
  inline std::shared_ptr<T> emplace_child(Args &&...args) {
    auto child = std::make_shared<T>(std::forward<Args>(args)...);
    children.emplace_back(child);
    return child;
  }

  template <typename T> inline std::shared_ptr<T> get_child() {
    for (auto &child : children) {
      if (auto c = child->downcast<T>()) {
        return c;
      }
    }
    return nullptr;
  }

  template <typename T> inline std::vector<std::shared_ptr<T>> get_children() {
    std::vector<std::shared_ptr<T>> res;
    for (auto &child : children) {
      if (auto c = child->downcast<T>()) {
        res.push_back(c);
      }
    }
    return res;
  }

  // Time until the widget is removed from the tree
  // in milliseconds
  // Widget itself will update this value
  // And its parent is responsible for removing it
  // when the time is up
  dying_time dying_time;
};

// A widget with child which lays out children in a row or column
struct widget_flex : public widget {
  float gap = 0;
  bool horizontal = false;
  bool auto_size = true;

  void update(update_context &ctx) override;
};

// A widget with padding and margin
struct widget_padding : public widget {
  float padding_left = 0, padding_right = 0, padding_top = 0,
        padding_bottom = 0;
  float margin_left = 0, margin_right = 0, margin_top = 0, margin_bottom = 0;
  inline void set_padding(float padding) {
    padding_left = padding_right = padding_top = padding_bottom = padding;
  }

  inline void set_margin(float margin) {
    margin_left = margin_right = margin_top = margin_bottom = margin;
  }

  inline void set_padding(float vertical, float horizontal) {
    padding_top = padding_bottom = vertical;
    padding_left = padding_right = horizontal;
  }

  inline void set_margin(float vertical, float horizontal) {
    margin_top = margin_bottom = vertical;
    margin_left = margin_right = horizontal;
  }

  std::shared_ptr<widget> child;
  void update(update_context &ctx) override;

  void render(nanovg_context ctx) override;
};

// A widget that renders text
struct text_widget : public widget {
  std::string text;
  float font_size = 14;
  std::string font_family = "Yahei";
  animated_color color = {this, 0, 0, 0, 1};

  void render(nanovg_context ctx) override;

  float measure_width(update_context &ctx) override;

  float measure_height(update_context &ctx) override;
};
} // namespace ui