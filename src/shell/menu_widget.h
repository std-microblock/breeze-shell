#pragma once
#include "extra_widgets.h"
#include "shell.h"
#include "ui.h"
#include "widget.h"
#include <memory>
#include <optional>

namespace mb_shell {
struct menu_item_widget : public ui::widget {
  using super = ui::widget;
  menu_item item;
  ui::sp_anim_float opacity = anim_float(0, 200);
  float text_padding = 13;
  float margin = 5;
  float icon_width = 16;
  float icon_padding = 5;
  menu_item_widget(menu_item item);
  void set_index_for_animation(int index);

  int icon_img_id = -1;

  ui::sp_anim_float bg_opacity = anim_float(0, 200);
  void render(ui::nanovg_context ctx) override;

  void update(ui::UpdateContext &ctx) override;

  float measure_width(ui::UpdateContext &ctx) override;
};

struct menu_widget : public ui::widget_parent_flex {
  using super = ui::widget_parent_flex;
  float bg_padding_vertical = 6;
  std::shared_ptr<ui::rect_widget> bg;
  menu menu_data;
  menu_widget(menu menu_data);

  void update(ui::UpdateContext &ctx) override;

  void render(ui::nanovg_context ctx) override;
};

struct mouse_menu_widget_main : public ui::widget_parent {
  float anchor_x = 0, anchor_y = 0;
  mouse_menu_widget_main(menu menu_data, float x, float y);

  void update(ui::UpdateContext &ctx);

  void render(ui::nanovg_context ctx);
};

} // namespace mb_shell