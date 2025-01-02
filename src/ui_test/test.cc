#include "animator.h"
#include "extra_widgets.h"
#include "ui.h"
#include "widget.h"
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <print>
#include <thread>
#include <vector>

struct test_widget : public ui::acrylic_background_widget {
  using super = ui::acrylic_background_widget;
  test_widget() : super() {
    x->animate_to(100);
    y->animate_to(100);
    width->animate_to(100);
    height->animate_to(100);

    acrylic_bg_color = nvgRGBAf(0, 0.5, 0.5, 0.5);
    update_color();
    radius->animate_to(10);
  }

  ui::sp_anim_float color_transition = anim_float(256, 3000);
  void render(ui::nanovg_context ctx) override {
    super::render(ctx);
    ctx.fontFace("Segui");
    ctx.fontSize(24);
    ctx.text(*x + 10, *y + 30, "Button", nullptr);
  }

  void update(ui::UpdateContext &ctx) override {
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

struct menu_item {
  enum class type {
    button,
    toggle,
    spacer,
  } type = type::button;

  std::optional<std::string> name;
  std::optional<std::function<void()>> action;

  bool expandable = false;
};

std::vector<menu_item> items{
    {.type = menu_item::type::button, .name = "层叠窗口(D)"},
    {.type = menu_item::type::button, .name = "堆叠显示窗口(E)"},
    {.type = menu_item::type::button, .name = "并排显示窗口(I)"},
    {.type = menu_item::type::spacer},
    {.type = menu_item::type::button, .name = "最小化所有窗口(M)"},
    {.type = menu_item::type::button, .name = "还原所有窗口(R)"},
};

struct menu_item_widget : public ui::widget {
  using super = ui::widget;
  menu_item item;
  ui::sp_anim_float opacity = anim_float(0, 200);
  float text_padding = 13;
  float margin = 5;
  menu_item_widget(menu_item item, size_t index) : super() {
    opacity->reset_to(0);

    this->y->before_animate = [this, index](float dest) {
      this->y->from = std::max(0.f, dest - 40 - index * 10);
    };
    opacity->animate_to(255);

    auto delay = std::min(index * 10.f, 100.f);
    this->y->set_delay(delay);
    opacity->set_delay(delay);
    if (item.type == menu_item::type::spacer) {
      height->animate_to(1);
    } else {
      height->animate_to(25);
    }
    this->item = item;
  }

  ui::sp_anim_float bg_opacity = anim_float(0, 200);
  void render(ui::nanovg_context ctx) override {
    super::render(ctx);
    if (item.type == menu_item::type::spacer) {
      ctx.fillColor(nvgRGBAf(1, 1, 1, 0.1));
      ctx.fillRect(*x, *y, *width, *height);
      return;
    }

    ctx.fillColor(nvgRGBAf(1, 1, 1, *bg_opacity / 255.f));
    ctx.fillRoundedRect(*x + margin, *y, *width - margin * 2, *height, 4);

    ctx.fillColor(nvgRGBAf(1, 1, 1, *opacity / 255.f));
    ctx.fontFace("Segui");
    ctx.fontSize(14);
    ctx.text(floor(*x + text_padding), floor(*y + 2 + 14), item.name->c_str(),
             nullptr);
  }

  void update(ui::UpdateContext &ctx) override {
    super::update(ctx);
    if (ctx.mouse_down_on(this)) {
      bg_opacity->animate_to(30);
    } else if (ctx.hovered(this)) {
      bg_opacity->animate_to(20);
    } else {
      bg_opacity->animate_to(0);
    }
  }

  float measure_width(ui::UpdateContext &ctx) override {
    if (item.type == menu_item::type::spacer) {
      return 1;
    }
    return ctx.vg.measureText(item.name->c_str()).first + text_padding * 2 +
           margin * 2;
  }
};

struct menu_widget : public ui::widget_parent_flex {
  using super = ui::widget_parent_flex;
  float bg_padding_vertical = 6;
  float anchor_x = 0, anchor_y = 0;
  std::unique_ptr<ui::acrylic_background_widget> bg;
  menu_widget(std::vector<menu_item> init_items, float wid_x, float wid_y)
      : super() {
    gap = 5;
    anchor_x = wid_x;
    anchor_y = wid_y;
    bg = std::make_unique<ui::acrylic_background_widget>();
    bg->acrylic_bg_color = nvgRGBAf(0, 0, 0, 0.5);
    bg->update_color();
    bg->radius->reset_to(6);

    bg->opacity->reset_to(0);
    bg->opacity->animate_to(255);

    for (size_t i = 0; i < init_items.size(); i++) {
      auto &item = init_items[i];
      auto mi = new menu_item_widget(item, i);
      children.push_back(std::unique_ptr<menu_item_widget>(mi));
    }
  }

  void update(ui::UpdateContext &ctx) override {
    super::update(ctx);

    x->reset_to(anchor_x / ctx.rt.dpi_scale);
    y->reset_to(anchor_y / ctx.rt.dpi_scale);

    y->animate_to(anchor_y - height->dest() - 10);
    bg->width->reset_to(width->dest());
    bg->height->reset_to(height->dest() + bg_padding_vertical * 2);
    bg->x->reset_to(x->dest());
    bg->y->reset_to(y->dest() - bg_padding_vertical);
    bg->update(ctx);

    if (ctx.mouse_clicked) {
      std::println("Clicked on menu");
      auto i = items;
      ctx.rt.root->children.clear();
      ctx.rt.root->emplace_child<menu_widget>(i, ctx.mouse_x, ctx.mouse_y);
    }
  }

  void render(ui::nanovg_context ctx) override {
    bg->render(ctx);
    super::render(ctx);
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

  rt.root->emplace_child<menu_widget>(items, 20, 20);

  nvgCreateFont(rt.nvg, "Segui", "C:\\WINDOWS\\FONTS\\msyh.ttc");

  rt.start_loop();
  return 0;
}