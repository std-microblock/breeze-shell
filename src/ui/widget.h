#pragma once
#include "animator.h"
#include "nanovg_wrapper.h"

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

  bool hovered(widget *w) const;
  bool mouse_clicked_on(widget *w) const { return mouse_clicked && hovered(w); }
  bool mouse_down_on(widget *w) const { return mouse_down && hovered(w); }

  float offset_x = 0, offset_y = 0;
  render_target& rt;
};

struct widget {
  std::vector<sp_anim_float> anim_floats{};
  sp_anim_float anim_float(auto &&...args) {
    auto anim = std::make_shared<animated_float>(std::forward<decltype(args)>(args)...);
    anim_floats.push_back(anim);
    return anim;
  }

  sp_anim_float x = anim_float(), y = anim_float(), width = anim_float(),
                height = anim_float();
  virtual void render(nanovg_context ctx) {}
  virtual void update(UpdateContext &ctx);
  virtual ~widget() = default;
};

struct widget_parent : public widget {
  std::vector<std::unique_ptr<widget>> children;
  void render(nanovg_context ctx) override;
  void update(UpdateContext &ctx) override;
  void add_child(std::unique_ptr<widget> child);
  template <typename T, typename... Args>
  inline void emplace_child(Args &&...args) {
    children.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
  }
};
} // namespace ui