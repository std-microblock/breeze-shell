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
  float text_padding = 8;
  float margin = 5;
  float icon_width = 16;
  float icon_padding = 10;
  menu_item_widget(menu_item item);
  void reset_appear_animation(float delay);

  std::optional<tvg::Picture> icon_img_bmp{};

  ui::sp_anim_float bg_opacity = anim_float(0, 200);
  void render(ui::render_context& ctx) override;
  void update(ui::update_context &ctx) override;
  float measure_width(ui::update_context &ctx) override;
};

enum class popup_direction {
  // 第一象限 ~ 第四象限
  top_left,
  top_right,
  bottom_left,
  bottom_right,
};

struct menu_widget : public ui::widget_parent_flex {
  using super = ui::widget_parent_flex;
  float bg_padding_vertical = 6;
  std::shared_ptr<ui::rect_widget> bg;
  menu menu_data;
  menu_widget(menu menu_data);

  std::mutex data_lock;

  void reset_animation(bool reverse = false);
  void update(ui::update_context &ctx) override;

  void render(ui::render_context& ctx) override;
};

struct mouse_menu_widget_main : public ui::widget {
  float anchor_x = 0, anchor_y = 0;
  mouse_menu_widget_main(menu menu_data, float x, float y);
  bool position_calibrated = false, direction_calibrated = false;
  popup_direction direction;
  std::shared_ptr<menu_widget> menu_wid;

  void update(ui::update_context &ctx);

  void render(ui::render_context& ctx);

  void calibrate_position(ui::update_context &ctx, bool animated = true);
  void calibrate_direction(ui::update_context &ctx);
};

} // namespace mb_shell