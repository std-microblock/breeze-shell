#pragma once
#include "../config.h"
#include "animator.h"
#include "contextmenu.h"
#include "extra_widgets.h"
#include "nanovg_wrapper.h"
#include "ui.h"
#include "widget.h"
#include <algorithm>
#include <functional>
#include <memory>
#include <optional>

namespace mb_shell {

struct menu_widget;


struct menu_item_widget : public ui::widget {
  using super = ui::widget;
  menu_item item;
  ui::sp_anim_float opacity = anim_float(0, 200);
  menu_item_widget();
  virtual void reset_appear_animation(float delay);
};

struct menu_item_ownerdraw_widget : public menu_item_widget {
  using super = menu_item_widget;
  owner_draw_menu_info owner_draw;
  std::optional<ui::NVGImage> img{};
  menu_item_ownerdraw_widget(menu_item item);
  void update(ui::update_context &ctx) override;
  void render(ui::nanovg_context ctx) override;
  void reset_appear_animation(float delay) override;
};

struct menu_item_parent_widget : public menu_item_widget {
  using super = menu_item_widget;
  void update(ui::update_context &ctx) override;
  void reset_appear_animation(float delay) override;
};

struct menu_item_normal_widget : public menu_item_widget {
  using super = menu_item_widget;
  ui::sp_anim_float opacity = anim_float(0, 200);
  float text_padding = config::current->context_menu.theme.text_padding;
  float margin = config::current->context_menu.theme.margin;
  bool has_icon_padding = false;
  bool has_submenu_padding = false;
  float padding = config::current->context_menu.theme.padding;
  float icon_padding = config::current->context_menu.theme.icon_padding;
  float right_icon_padding =
      config::current->context_menu.theme.right_icon_padding;
  menu_item_normal_widget(menu_item item);
  void reset_appear_animation(float delay) override;

  std::optional<ui::NVGImage> icon_img{};
  std::optional<ui::NVGImage> icon_unfold_img{};

  std::shared_ptr<menu_widget> submenu_wid = nullptr;
  float show_submenu_timer = 0.f;

  ui::sp_anim_float bg_opacity = anim_float(0, 200);
  void render(ui::nanovg_context ctx) override;
  void update(ui::update_context &ctx) override;
  float measure_width(ui::update_context &ctx) override;
  bool check_hit(const ui::update_context &ctx) override;

  void hide_submenu();
  void show_submenu(ui::update_context &ctx);
  void reload_icon_img(ui::nanovg_context ctx);
};

struct menu_item_custom_widget : public menu_item_widget {
  using super = menu_item_widget;
  std::shared_ptr<ui::widget> custom_widget;
  menu_item_custom_widget(
      std::shared_ptr<ui::widget> custom_widget)
      : custom_widget(custom_widget) {}
  void render(ui::nanovg_context ctx) override;
  void update(ui::update_context &ctx) override;
  float measure_width(ui::update_context &ctx) override;
  float measure_height(ui::update_context &ctx) override;
};

enum class popup_direction {
  // 第一象限 ~ 第四象限
  top_left,
  top_right,
  bottom_left,
  bottom_right,
};
struct menu_item_widget;
struct menu_widget : public ui::widget_flex {
  using super = ui::widget_flex;
  float bg_padding_vertical = 6;

  float max_height = 99999;
  float actual_height = 0;
  ui::sp_anim_float scroll_top =
      anim_float(0, 200, ui::easing_type::ease_in_out);
  std::shared_ptr<ui::rect_widget> bg;

  std::shared_ptr<ui::rect_widget> bg_submenu;
  std::shared_ptr<menu_widget> current_submenu;
  std::optional<std::weak_ptr<ui::widget>> parent_item_widget;
  std::vector<std::shared_ptr<widget>> rendering_submenus;
  std::vector<std::shared_ptr<widget>> item_widgets;
  menu_widget *parent_menu = nullptr;

  std::shared_ptr<ui::rect_widget> create_bg(bool is_main);
  menu menu_data;
  menu_widget();
  popup_direction direction = popup_direction::bottom_right;
  void init_from_data(menu menu_data);
  bool animate_appear_started = false;
  void reset_animation(bool reverse = false);
  void update(ui::update_context &ctx) override;

  void update_icon_width();

  void render(ui::nanovg_context ctx) override;

  bool check_hit(const ui::update_context &ctx) override;
  void close();
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