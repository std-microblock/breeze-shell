#include "animator.h"
#include "ui.h"
#include "widget.h"
#include "extra_widgets.h"
#include <iostream>
#include <print>
#include <thread>

struct test_widget : public ui::acrylic_background_widget {
  using super = ui::acrylic_background_widget;  
  test_widget() : super() {
    x->animate_to(100);
    y->animate_to(100);
    width->animate_to(100);
    height->animate_to(100);
  }

  ui::sp_anim_float color_transition = anim_float(256, 3000);
  void render(ui::nanovg_context ctx) override {
    super::render(ctx);
    ctx.text(1, 1, "Hello, World!", nullptr);
    ctx.beginPath();
    ctx.rect(*x, *y, *width, *height);
    ctx.fillColor(
        nvgRGBA(*color_transition, 255 - (*color_transition), 0, 255));
    ctx.fill();
  }

  void update(const ui::UpdateContext &ctx) override {
    super::update(ctx);
    if (ctx.mouse_down_on(this)) {
      color_transition->animate_to(255);
      width->animate_to(200);
      height->animate_to(200);
    } else if (ctx.hovered(this)) {
      color_transition->animate_to(0);
      width->animate_to(150);
      height->animate_to(150);
    } else {
      color_transition->animate_to(128);
      width->animate_to(100);
      height->animate_to(100);
    }

    if (ctx.mouse_clicked_on(this)) {
      if (x->dest() == 100) {
        x->animate_to(200);
        y->animate_to(200);
      } else {
        x->animate_to(100);
        y->animate_to(100);
      }
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
  // rt.root->emplace_child<ui::acrylic_background_widget>();

  rt.start_loop();
  return 0;
}