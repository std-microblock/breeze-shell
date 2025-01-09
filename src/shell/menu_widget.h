#pragma once
#include "shell.h"
#include "ui.h"
#include "widget.h"
#include "extra_widgets.h"
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

  int icon_img_id = -1;

  ui::sp_anim_float bg_opacity = anim_float(0, 200);
  void render(ui::nanovg_context ctx) override;

  void update(ui::UpdateContext &ctx) override;

  float measure_width(ui::UpdateContext &ctx) override;
};

struct menu_widget : public ui::widget_parent_flex {
  using super = ui::widget_parent_flex;
  float bg_padding_vertical = 6;
  float anchor_x = 0, anchor_y = 0;
  std::unique_ptr<ui::acrylic_background_widget> bg;
  menu menu_data;
  menu_widget(menu menu_data, float wid_x, float wid_y);

  void update(ui::UpdateContext &ctx) override;

  void render(ui::nanovg_context ctx) override;
};
} // namespace mb_shell