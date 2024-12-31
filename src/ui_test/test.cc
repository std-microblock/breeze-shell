#include "animator.h"
#include "extra_widgets.h"
#include "ui.h"
#include "widget.h"
#include <functional>
#include <iostream>
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
    {.type = menu_item::type::button, .name = "终端(I)"},
    {.type = menu_item::type::button, .name = "桌面"},
    {.type = menu_item::type::spacer},
    {.type = menu_item::type::button, .name = "Button 3"},
    {.type = menu_item::type::button, .name = "Button 4"},
};

struct menu_item_widget : public ui::widget {
  using super = ui::widget;
  menu_item item;
  ui::sp_anim_float opacity = anim_float(0, 200);
  menu_item_widget(menu_item item, float y, size_t index) : super() {
    opacity->reset_to(0);

    this->y->animate_to(y);
    opacity->animate_to(255);
    width->animate_to(100);

    this->y->set_delay(index * 50);
    opacity->set_delay(index * 50);
    if (item.type == menu_item::type::spacer) {
      height->animate_to(1);
    } else {
      height->animate_to(25);
    }
    this->item = item;
  }

  float measure_height() {
    if (item.type == menu_item::type::spacer) {
      return 1;
    }
    return 20;
  }

  ui::sp_anim_float hover = anim_float(0, 200);
  void render(ui::nanovg_context ctx) override {
    super::render(ctx);
    if (item.type == menu_item::type::spacer) {
      ctx.fillColor(nvgRGBAf(1, 1, 1, 0.4));
      ctx.fillRect(*x, *y, *width, *height);
      return;
    }

    ctx.fillColor(nvgRGBAf(1, 1, 1, *opacity / 255.f));

    ctx.fontFace("Segui");
    ctx.fontSize(13);
    ctx.text(floor(*x + 10), floor(*y + 2 + 14), item.name->c_str(), nullptr);
  }

  void update(ui::UpdateContext &ctx) override {
    super::update(ctx);
    if (ctx.hovered(this)) {
      hover->animate_to(255);
    } else {
      hover->animate_to(0);
    }
  }
};

struct menu_widget : public ui::widget_parent {
  using super = ui::widget_parent;

  menu_widget(std::vector<menu_item> init_items, float wid_x, float wid_y) : super() {
    width->reset_to(1000);
    height->reset_to(1000);
    this->x->reset_to(wid_x);
    this->y->reset_to(wid_y);

    auto bg = new ui::acrylic_background_widget();
    children.push_back(std::unique_ptr<ui::acrylic_background_widget>(bg));

    bg->width->animate_to(100);
    bg->radius->reset_to(10);

    constexpr float margin = 3;
    float y = margin;
    for (size_t i = 0; i < init_items.size(); i++) {
      auto &item = init_items[i];
      auto mi = new menu_item_widget(item, y, i);
      children.push_back(std::unique_ptr<menu_item_widget>(mi));
      y += mi->measure_height() + margin;
    }

    bg->height->animate_to(y);
  }

  void update(ui::UpdateContext &ctx) override {
    super::update(ctx);
    if (ctx.mouse_clicked) {
      std::println("Clicked on menu");
      ctx.rt.root->children.clear();
      ctx.rt.root->emplace_child<menu_widget>(items, ctx.mouse_x, ctx.mouse_y);
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

  rt.root->emplace_child<menu_widget>(items, 20, 20);

  nvgCreateFont(rt.nvg, "Segui",
                "C:\\Users\\MicroBlock\\Downloads\\Noto_Sans_"
                "SC\\static\\NotoSansSC-Regular.ttf");

  rt.start_loop();
  return 0;
}