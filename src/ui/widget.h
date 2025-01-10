#pragma once
#include "animator.h"
#include "nanovg_wrapper.h"

#include <functional>
#include <memory>
#include <vector>

namespace ui {
struct render_target;
struct widget;
struct UpdateContext {
  float delta_t;
  // mouse position in window coordinates
  double mouse_x, mouse_y;
  bool mouse_down;
  // only true for one frame
  bool mouse_clicked;
  bool mouse_up;

  // hit test
  std::vector<std::shared_ptr<widget>> hovered_widgets;
  std::vector<std::shared_ptr<widget>> clicked_widgets;
  void set_hit_hovered(widget *w);
  void set_hit_clicked(widget *w);

  bool hovered(widget *w) const;
  bool mouse_clicked_on(widget *w) const;
  bool mouse_down_on(widget *w) const;

  bool mouse_clicked_on_hit(widget *w);
  bool hovered_hit(widget *w);

  float offset_x = 0, offset_y = 0;
  render_target &rt;
  nanovg_context vg;

  UpdateContext with_offset(float x, float y) {
    auto copy = *this;
    copy.offset_x = x;
    copy.offset_y = y;
    return copy;
  }
};

/*
All the widgets in the tree should be wrapped in a shared_ptr.
If you want to use a widget in multiple places, you should create a new instance
for each place.
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
  virtual void render(nanovg_context ctx) {}
  virtual void update(UpdateContext &ctx);
  virtual ~widget() = default;
  virtual float measure_height(UpdateContext &ctx);
  virtual float measure_width(UpdateContext &ctx);

  template <typename T> inline T *downcast() { return dynamic_cast<T *>(this); }
};

// A widget that renders its children
// It is responsible for updating and rendering its children
// It also sets the offset for the children
// It's like `posision: relative` in CSS
// While all other widgets are like `position: absolute`
struct widget_parent : public widget {
  std::vector<std::shared_ptr<widget>> children;
  void render(nanovg_context ctx) override;
  void update(UpdateContext &ctx) override;
  void add_child(std::shared_ptr<widget> child);
  template <typename T, typename... Args>
  inline void emplace_child(Args &&...args) {
    children.emplace_back(std::make_shared<T>(std::forward<Args>(args)...));
  }

  template <typename T> inline T *get_child() {
    for (auto &child : children) {
      if (auto c = child->downcast<T>()) {
        return c;
      }
    }
    return nullptr;
  }

  template <typename T> inline std::vector<T *> get_children() {
    std::vector<T *> res;
    for (auto &child : children) {
      if (auto c = child->downcast<T>()) {
        res.push_back(c);
      }
    }
    return res;
  }
};

// A widget with child which lays out children in a row or column
struct widget_parent_flex : public widget_parent {
  float gap = 0;
  bool horizontal = false;
  bool auto_size = true;

  void update(UpdateContext &ctx) override;
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
  void update(UpdateContext &ctx) override;

  void render(nanovg_context ctx) override;
};

} // namespace ui