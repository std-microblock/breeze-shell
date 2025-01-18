#include "menu_widget.h"
#include "../utils.h"
#include "animator.h"
#include "hbitmap_utils.h"
#include "menu_render.h"
#include "nanovg.h"
#include "shell.h"
#include "ui.h"
#include <print>
#include <vector>

void mb_shell::menu_item_widget::render(ui::nanovg_context ctx) {
  super::render(ctx);
  if (item.type == menu_item::type::spacer) {
    ctx.fillColor(nvgRGBAf(1, 1, 1, 0.1));
    ctx.fillRect(x->dest(), *y, *width, *height);
    return;
  }

  ctx.fillColor(nvgRGBAf(1, 1, 1, *bg_opacity / 255.f));

  float roundcorner = 4;

  if (menu_render::current.value()->style ==
      menu_render::menu_style::materialyou) {
    roundcorner = height->dest() / 2;
  }

  ctx.fillRoundedRect(*x + margin, *y, *width - margin * 2, *height,
                      roundcorner);

  ctx.fillColor(nvgRGBAf(1, 1, 1, *opacity / 255.f));
  ctx.fontFace("Yahei");
  ctx.fontSize(14);

  if (item.icon_bitmap.has_value()) {
    auto paintY = floor(*y + (*height - icon_width) / 2);
    if (!icon_img_bmp) {
      icon_img_bmp = ui::LoadBitmapImage(ctx, item.icon_bitmap.value());
    }

    auto paint =
        nvgImagePattern(ctx.ctx, *x + icon_padding + margin + ctx.offset_x,
                        paintY + ctx.offset_y, icon_width, icon_width, 0,
                        icon_img_bmp->id, *opacity / 255.f);

    ctx.beginPath();
    ctx.rect(*x + icon_padding + margin, paintY, icon_width, icon_width);
    ctx.fillPaint(paint);
    ctx.fill();
  }

  ctx.text(floor(*x + text_padding + icon_width + icon_padding * 2), *y + 16,
           item.name->c_str(), nullptr);

  if (item.submenu) {
    // draw arrow
    ctx.beginPath();
    ctx.moveTo(*x + *width - 20, *y + 8);
    ctx.lineTo(*x + *width - 15, *y + 12);
    ctx.lineTo(*x + *width - 20, *y + 16);
    ctx.strokeColor(nvgRGBAf(1, 1, 1, *opacity * 0.6 / 255.f));
    ctx.strokeWidth(1);
    ctx.stroke();
  }

  if (item.type == menu_item::type::toggle) {
    if (!item.icon_bitmap && item.checked) {
      ctx.beginPath();
      ctx.moveTo(*x + icon_padding + margin + 2, *y + 4 + 8);
      ctx.lineTo(*x + icon_padding + margin + 6, *y + 8 + 8);
      ctx.lineTo(*x + icon_padding + margin + 14, *y + 0 + 8);
      ctx.strokeColor(nvgRGBAf(1, 1, 1, *opacity * 0.6 / 255.f));
      ctx.strokeWidth(2);
      ctx.stroke();
    }
  }

  if (submenu_wid) {
    submenu_wid->render(ctx);
  }
}
void mb_shell::menu_item_widget::update(ui::update_context &ctx) {
  super::update(ctx);
  if (ctx.mouse_down_on(this)) {
    bg_opacity->animate_to(40);
  } else if (ctx.hovered(this)) {
    bg_opacity->animate_to(20);
  } else {
    bg_opacity->animate_to(0);
  }

  if (ctx.mouse_clicked_on(this)) {
    if (item.action) {
      item.action.value()();
    }
  }

  if (item.submenu) {
    if (ctx.hovered(this) || (submenu_wid && ctx.hovered(submenu_wid.get()))) {
      show_submenu_timer = std::min(show_submenu_timer + ctx.delta_t, 500.f);
    } else {
      show_submenu_timer = std::max(show_submenu_timer - ctx.delta_t, 0.f);
    }

    if (show_submenu_timer >= 250.f) {
      if (!submenu_wid) {
        submenu_wid = std::make_shared<menu_widget>(item.submenu.value()());

        submenu_wid->x->reset_to(*width + *x);
        submenu_wid->y->reset_to(*y);
        submenu_wid->reset_animation();
      }

      submenu_wid->update(ctx);
    } else {
      if (submenu_wid) {
        submenu_wid = nullptr;
      }
    }
  }
}
float mb_shell::menu_item_widget::measure_width(ui::update_context &ctx) {
  if (item.type == menu_item::type::spacer) {
    return 1;
  }
  return ctx.vg.measureText(item.name->c_str()).first + text_padding * 2 +
         margin * 2 + icon_width + icon_padding * 2;
}
mb_shell::menu_widget::menu_widget(menu menu) : super(), menu_data(menu) {
  gap = 5;

  if (menu_render::current.value()->style ==
      menu_render::menu_style::fluentui) {
    auto acrylic =
        std::make_shared<ui::acrylic_background_widget>(is_win11_or_later());
    acrylic->acrylic_bg_color = nvgRGBAf(0, 0, 0, 0.5);
    acrylic->update_color();
    bg = acrylic;
  } else {
    // bg = std::make_shared<ui::rect_widget>();
    // bg->bg_color = nvgRGBAf(0, 0, 0, 0.8);
    auto acrylic =
        std::make_shared<ui::acrylic_background_widget>(is_win11_or_later());
    acrylic->acrylic_bg_color = nvgRGBAf(0, 0, 0, 0.5);
    acrylic->update_color();
    bg = acrylic;
  }
  if (menu_render::current.value()->style ==
      menu_render::menu_style::materialyou) {
    bg->radius->reset_to(18);
  } else {
    bg->radius->reset_to(6);
  }

  bg->opacity->reset_to(0);
  bg->opacity->set_delay(80);
  bg->opacity->animate_to(255);

  auto init_items = menu_data.items;

  bool has_icon = std::ranges::any_of(init_items, [](auto &item) {
    return item.icon_bitmap.has_value() || item.type == menu_item::type::toggle;
  });

  for (size_t i = 0; i < init_items.size(); i++) {
    auto &item = init_items[i];
    auto mi = std::make_shared<menu_item_widget>(item, this);
    if (!has_icon) {
      mi->icon_width = 0;
    }
    children.push_back(mi);
  }
}
void mb_shell::menu_widget::update(ui::update_context &ctx) {
  std::lock_guard lock(data_lock);
  super::update(ctx);
  bg->x->reset_to(x->dest());
  bg->y->reset_to(y->dest() - bg_padding_vertical);
  bg->width->reset_to(width->dest());
  bg->height->reset_to(height->dest() + bg_padding_vertical * 2);
  bg->update(ctx);

  ctx.mouse_clicked_on_hit(bg.get());
}
void mb_shell::menu_widget::render(ui::nanovg_context ctx) {
  std::lock_guard lock(data_lock);
  bg->render(ctx);
  super::render(ctx);
}
void mb_shell::menu_item_widget::reset_appear_animation(float delay) {
  this->opacity->after_animate = [this](float dest) {
    this->opacity->set_delay(0);
  };
  opacity->set_delay(delay);
  opacity->set_duration(200);
  opacity->reset_to(0);
  opacity->animate_to(255);
  this->y->before_animate = [this](float dest) {
    if (this->y->dest() == 0) {
      this->y->set_easing(ui::easing_type::mutation);
    } else {
      this->y->set_easing(ui::easing_type::ease_in_out);
    }
  };
  this->x->set_delay(delay);
  this->x->set_duration(200);
  this->x->reset_to(-20);
  this->x->animate_to(0);
}
mb_shell::mouse_menu_widget_main::mouse_menu_widget_main(menu menu_data,
                                                         float x, float y)
    : widget(), anchor_x(x), anchor_y(y) {
  menu_wid = std::make_shared<menu_widget>(menu_data);
}
void mb_shell::mouse_menu_widget_main::update(ui::update_context &ctx) {
  ui::widget::update(ctx);
  x->reset_to(anchor_x / ctx.rt.dpi_scale);
  y->reset_to(anchor_y / ctx.rt.dpi_scale);

  if (!direction_calibrated) {
    calibrate_direction(ctx);
    direction_calibrated = true;
    calibrate_position(ctx, false);
    position_calibrated = true;
  }

  if (!position_calibrated) {
    calibrate_position(ctx);
    position_calibrated = true;
  }

  menu_wid->update(ctx);

  // static bool to_close = false;
  // auto mask = /*get if pressed after first call*/ 0x8000;
  // if (ctx.clicked_widgets.size() == 0) {
  //   if ((GetAsyncKeyState(VK_LBUTTON) & mask) ||
  //       (GetAsyncKeyState(VK_RBUTTON) & mask))
  //     to_close = true;
  //   else if (to_close) {
  //     ctx.rt.close();
  //   }
  // } else {
  //   to_close = false;
  // }
  if (!ctx.hovered(menu_wid.get())) {
    std::println("Not hovered");
    glfwSetWindowAttrib(ctx.rt.window, GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE);
    if ((ctx.mouse_clicked || ctx.right_mouse_clicked))
      ctx.rt.close();
  } else {
    std::println("Hovered");
    glfwSetWindowAttrib(ctx.rt.window, GLFW_MOUSE_PASSTHROUGH, GLFW_FALSE);
  }

  // esc to close
  if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
    ctx.rt.close();
  }
}
void mb_shell::mouse_menu_widget_main::render(ui::nanovg_context ctx) {
  ui::widget::render(ctx);
  menu_wid->render(ctx);
}
void mb_shell::menu_widget::reset_animation(bool reverse) {
  auto children = get_children<menu_item_widget>();

  // the show duration for the menu should be within 200ms
  float delay = std::min(400.f / children.size(), 50.f);

  for (size_t i = 0; i < children.size(); i++) {
    auto &child = children[i];
    child->reset_appear_animation(delay * (reverse ? children.size() - i : i));
  }
}
void mb_shell::mouse_menu_widget_main::calibrate_position(
    ui::update_context &ctx, bool animated) {
  menu_wid->update(ctx);
  auto menu_width = menu_wid->measure_width(ctx);
  auto menu_height = menu_wid->measure_height(ctx);

  int x, y;

  // avoid the menu to be out of the screen
  // if the menu is out of the screen, move it to the edge
  if (direction == popup_direction::top_left) {
    x = anchor_x - menu_width * ctx.rt.dpi_scale;
    y = anchor_y - menu_height * ctx.rt.dpi_scale;
  } else if (direction == popup_direction::top_right) {
    x = anchor_x;
    y = anchor_y - menu_height * ctx.rt.dpi_scale;
  } else if (direction == popup_direction::bottom_left) {
    x = anchor_x - menu_width * ctx.rt.dpi_scale;
    y = anchor_y;
  } else {
    x = anchor_x;
    y = anchor_y;
  }

  constexpr auto padding_vertical = 50, padding_horizontal = 10;

  if (x < padding_vertical) {
    x = padding_vertical;
  } else if (x + menu_width * ctx.rt.dpi_scale >
             ctx.screen.width - padding_vertical) {
    x = ctx.screen.width - menu_width * ctx.rt.dpi_scale - padding_vertical;
  }

  if (y < padding_horizontal) {
    y = padding_horizontal;
  } else if (y + menu_height * ctx.rt.dpi_scale >
             ctx.screen.height - padding_horizontal) {
    y = ctx.screen.height - menu_height * ctx.rt.dpi_scale - padding_horizontal;
  }

  std::println("Calibrated position: {} {} in screen {} {}", x, y,
               ctx.screen.width, ctx.screen.height);

  if (animated) {
    this->menu_wid->x->animate_to(x / ctx.rt.dpi_scale);
    this->menu_wid->y->animate_to(y / ctx.rt.dpi_scale);
  } else {
    this->menu_wid->x->reset_to(x / ctx.rt.dpi_scale);
    this->menu_wid->y->reset_to(y / ctx.rt.dpi_scale);
  }
}
void mb_shell::mouse_menu_widget_main::calibrate_direction(
    ui::update_context &ctx) {
  auto monitor = MonitorFromPoint({(LONG)anchor_x, (LONG)anchor_y},
                                  MONITOR_DEFAULTTONEAREST);
  MONITORINFOEX monitor_info;
  monitor_info.cbSize = sizeof(MONITORINFOEX);
  GetMonitorInfo(monitor, &monitor_info);

  menu_wid->update(ctx);
  auto menu_width = menu_wid->measure_width(ctx);
  auto menu_height = menu_wid->measure_height(ctx);

  direction = popup_direction::bottom_right;

  bool top =
      (/*the space is not enough*/ anchor_y + menu_height * ctx.rt.dpi_scale >
       monitor_info.rcMonitor.bottom) &&
      (
          /*and reversing can solve the problem*/
          anchor_y - menu_height * ctx.rt.dpi_scale >
          monitor_info.rcMonitor.top);

  bool left =
      (/*the space is not enough*/ anchor_x + menu_width * ctx.rt.dpi_scale >
       monitor_info.rcMonitor.right) &&
      (
          /*and reversing can solve the problem*/
          anchor_x - menu_width * ctx.rt.dpi_scale >
          monitor_info.rcMonitor.left);

  if (top && left) {
    direction = popup_direction::top_left;
  } else if (top) {
    direction = popup_direction::top_right;
  } else if (left) {
    direction = popup_direction::bottom_left;
  } else {
    direction = popup_direction::bottom_right;
  }

  menu_wid->reset_animation(direction == popup_direction::top_left ||
                            direction == popup_direction::top_right);

  std::println("Calibrated direction: {}",
               direction == popup_direction::top_left       ? "top_left"
               : direction == popup_direction::top_right    ? "top_right"
               : direction == popup_direction::bottom_left  ? "bottom_left"
               : direction == popup_direction::bottom_right ? "bottom_right"
                                                            : "unknown");
}
bool mb_shell::menu_item_widget::check_hit(const ui::update_context &ctx) {
  return ui::widget::check_hit(ctx) ||
         (submenu_wid && submenu_wid->check_hit(ctx));
}
mb_shell::menu_item_widget::menu_item_widget(menu_item item,
                                             menu_widget *parent_menu)
    : super(), parent_menu(parent_menu) {
  opacity->reset_to(0);

  if (item.type == menu_item::type::spacer) {
    height->animate_to(1);
  } else {
    height->animate_to(25);
  }
  this->item = item;
}
bool mb_shell::menu_widget::check_hit(const ui::update_context &ctx) {
  auto hit = ui::widget::check_hit(ctx) || bg->check_hit(ctx) ||
             std::ranges::any_of(
                 get_children<menu_item_widget>(), [&ctx, this](auto &child) {
                   return child->check_hit(ctx.with_offset(*x, *y));
                 });

  return hit;
}
