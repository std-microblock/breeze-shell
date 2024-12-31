#include "animator.h"
#include "bgfx/bgfx.h"
#include "ui.h"
#include "widget.h"
#include <iostream>
#include <print>
#include <thread>

struct test_widget : public ui::widget {
  test_widget() : ui::widget() {
    x->animate_to(100);
    y->animate_to(100);
    width->animate_to(100);
    height->animate_to(100);
  }

  ui::sp_anim_float color_transition = anim_float(256, 3000);
  void render(bgfx::ViewId view, ui::nanovg_context ctx) override {
    ctx.text(1, 1, "Hello, World!", nullptr);
    ctx.beginPath();
    ctx.rect(*x, *y, *width, *height);
    ctx.fillColor(
        nvgRGBA(*color_transition, 255 - (*color_transition), 0, 255));
    ctx.fill();
  }

  void update(const ui::UpdateContext &ctx) override {
    ui::widget::update(ctx);
    if (ctx.mouse_down_on(this)) {
      color_transition->animate_to(255);
    } else if (ctx.hovered(this)) {
      color_transition->animate_to(0);
    } else {
      color_transition->animate_to(128);
    }
  }
};

int main() {
  if (auto res = ui::render_target::init_global(); !res) {
    std::println("Failed to initialize global render target: {}", res.error());
    return 1;
  }

  ui::render_target rt;

  if (auto res = rt.init(); !res) {
    std::println("Failed to initialize render target: {}", res.error());
    return 1;
  }

  rt.root->emplace_child<test_widget>();
  rt.start_loop();
  return 0;

  return 0;
}