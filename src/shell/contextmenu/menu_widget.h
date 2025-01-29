#pragma once
#include "animator.h"
#include "extra_widgets.h"
#include "nanovg_wrapper.h"
#include "contextmenu.h"
#include "ui.h"
#include "widget.h"
#include <algorithm>
#include <memory>
#include <optional>

namespace mb_shell {

struct menu_widget;
struct menu_item_widget : public ui::widget {
  using super = ui::widget;
  menu_item item;
  ui::sp_anim_float opacity = anim_float(0, 200);
  float text_padding = 8;
  float margin = 5;
  float icon_width = 16;
  float icon_padding = 10;
  menu_widget *parent_menu;
  menu_item_widget(menu_item item, menu_widget *parent_menu);
  void reset_appear_animation(float delay);

  std::optional<ui::NVGImage> icon_img{};

  std::shared_ptr<menu_widget> submenu_wid = nullptr;
  float show_submenu_timer = 0.f;

  ui::sp_anim_float bg_opacity = anim_float(0, 200);
  void render(ui::nanovg_context ctx) override;
  void update(ui::update_context &ctx) override;
  float measure_width(ui::update_context &ctx) override;
  bool check_hit(const ui::update_context &ctx) override;

  void reload_icon_img(ui::nanovg_context ctx);
};

enum class popup_direction {
  // 第一象限 ~ 第四象限
  top_left,
  top_right,
  bottom_left,
  bottom_right,
};

struct menu_widget : public ui::widget_flex {
  using super = ui::widget_flex;
  float bg_padding_vertical = 6;
  std::shared_ptr<ui::rect_widget> bg;

  std::shared_ptr<ui::rect_widget> bg_submenu;
  std::shared_ptr<menu_widget> current_submenu;
  std::vector<std::shared_ptr<widget>> rendering_submenus;

  std::shared_ptr<ui::rect_widget> create_bg();
  menu menu_data;
  menu_widget();
  popup_direction direction = popup_direction::bottom_right;
  std::mutex data_lock;
  void init_from_data(menu menu_data);
  bool animate_appear_started = false;
  void reset_animation(bool reverse = false);
  void update(ui::update_context &ctx) override;

  void update_icon_width();

  void render(ui::nanovg_context ctx) override;

  bool check_hit(const ui::update_context &ctx) override;
};

struct mouse_menu_widget_main : public ui::widget {
  float anchor_x = 0, anchor_y = 0;
  mouse_menu_widget_main(menu menu_data, float x, float y);
  bool position_calibrated = false, direction_calibrated = false;
  popup_direction direction;
  std::shared_ptr<menu_widget> menu_wid;

  void update(ui::update_context &ctx);

  void render(ui::nanovg_context ctx);

  static std::pair<float, float>
  calculate_position(menu_widget *menu_wid, ui::update_context &ctx,
                     float anchor_x, float anchor_y, popup_direction direction);

  static popup_direction calculate_direction(
      menu_widget *menu_wid, ui::update_context &ctx, float anchor_x,
      float anchor_y,
      popup_direction prefer_direction = popup_direction::bottom_right);

  void calibrate_position(ui::update_context &ctx, bool animated = true);
  void calibrate_direction(ui::update_context &ctx);
};

} // namespace mb_shell