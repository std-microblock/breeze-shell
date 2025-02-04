#include "menu_widget.h"
#include "../config.h"
#include "../utils.h"
#include "animator.h"
#include "contextmenu.h"
#include "hbitmap_utils.h"
#include "menu_render.h"
#include "nanovg.h"
#include "ui.h"
#include <algorithm>
#include <iostream>
#include <print>
#include <ranges>
#include <vector>

/*
| padding | icon_padding | icon | icon_padding | text_padding | text |
text_padding | right_icon_padding | right_icon | right_icon_padding |
*/
void mb_shell::menu_item_normal_widget::render(ui::nanovg_context ctx) {
  super::render(ctx);
  auto icon_width = config::current->context_menu.theme.font_size + 2;
  auto has_icon = has_icon_padding || icon_img;
  auto c = menu_render::current.value()->light_color ? 0 : 1;
  if (item.type == menu_item::type::spacer) {
    ctx.fillColor(nvgRGBAf(c, c, c, 0.1));
    ctx.fillRect(x->dest(), *y, *width, *height);
    return;
  }

  ctx.fillColor(nvgRGBAf(c, c, c, *bg_opacity / 255.f));

  float roundcorner = std::min(roundcorner = height->dest() / 2,
                               config::current->context_menu.theme.item_radius);

  ctx.fillRoundedRect(*x + margin, *y, *width - margin * 2, *height,
                      roundcorner);

  if (item.icon_bitmap.has_value() || item.icon_svg.has_value()) {
    if (!icon_img || item.icon_updated)
      reload_icon_img(ctx);
    item.icon_updated = false;

    auto paintY = floor(*y + (*height - icon_width) / 2);
    auto imageX = *x + padding + margin + icon_padding;
    auto paint = ctx.imagePattern(imageX, paintY, icon_width, icon_width, 0,
                                  icon_img->id, *opacity / 255.f);

    ctx.beginPath();
    ctx.rect(imageX, paintY, icon_width, icon_width);
    ctx.fillPaint(paint);
    ctx.fill();
  }

  ctx.fillColor(nvgRGBAf(c, c, c, *opacity / 255.f));
  ctx.fontFace("main");
  auto font_size = config::current->context_menu.theme.font_size;
  ctx.fontSize(font_size);
  ctx.textAlign(NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
  if (item.name)
    ctx.text(round(*x + padding +
                   (has_icon ? (icon_width + icon_padding * 2) : 0) +
                   text_padding + margin),
             round(*y + *height / 2), item.name->c_str(), nullptr);

  if (item.submenu) {
    static auto icon_unfold = std::format(
        // point to right
        R"#(<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" viewBox="0 0 12 12"><path opacity="0.7" fill="{}" d="M4.646 2.146a.5.5 0 0 0 0 .708L7.793 6L4.646 9.146a.5.5 0 1 0 .708.708l3.5-3.5a.5.5 0 0 0 0-.708l-3.5-3.5a.5.5 0 0 0-.708 0"/></svg>)#",
        c ? "white" : "black");

    if (!icon_unfold_img) {
      static auto icon_unfold_img_svg = nsvgParse(icon_unfold.data(), "px", 96);

      this->icon_unfold_img =
          ctx.imageFromSVG(icon_unfold_img_svg, ctx.rt->dpi_scale);
    }

    auto paintY = floor(*y + (*height - icon_width) / 2);
    auto paintX = *x + padding + *width - right_icon_padding - icon_width;
    auto paint2 = ctx.imagePattern(paintX, paintY, icon_width, icon_width, 0,
                                   icon_unfold_img->id, *opacity / 255.f);
    ctx.beginPath();
    ctx.rect(paintX, paintY, icon_width, icon_width);
    ctx.fillPaint(paint2);
    ctx.fill();
  }
}
void mb_shell::menu_item_normal_widget::update(ui::update_context &ctx) {
  super::update(ctx);

  if (parent->dying_time) {
    bg_opacity->animate_to(0);
    opacity->animate_to(0);
    return;
  }

  if (item.type == menu_item::type::spacer) {
    height->reset_to(1);
  } else {
    height->reset_to(config::current->context_menu.theme.item_height);
  }

  if (item.disabled) {
    opacity->animate_to(128);
    bg_opacity->animate_to(0);

    if (submenu_wid) {
      submenu_wid->close();
      submenu_wid = nullptr;
    }
    return;
  } else {
    opacity->animate_to(255);
  }

  if (ctx.mouse_down_on(this)) {
    bg_opacity->animate_to(40);
  } else if (ctx.hovered(this)) {
    bg_opacity->animate_to(20);
  } else {
    bg_opacity->animate_to(0);
  }

  if (ctx.mouse_clicked_on(this)) {
    if (item.action) {
      try {
        item.action.value()();
      } catch (std::exception &e) {
        std::cerr << "Error in action: " << e.what() << std::endl;
      }
    }
  }

  if (item.submenu) {
    if (ctx.hovered(this) || (submenu_wid && ctx.with_reset_offset().hovered(
                                                 submenu_wid.get(), false))) {
      show_submenu_timer = std::min(show_submenu_timer + ctx.delta_t, 500.f);
    } else {
      show_submenu_timer = std::max(show_submenu_timer - ctx.delta_t, 0.f);
    }

    if (show_submenu_timer >= 250.f) {
      if (!submenu_wid) {
        submenu_wid = std::make_shared<menu_widget>();
        item.submenu.value()(submenu_wid);

        auto anchor_x = *width + *x + ctx.offset_x;
        auto anchor_y = **y + ctx.offset_y;

        anchor_x *= ctx.rt.dpi_scale;
        anchor_y *= ctx.rt.dpi_scale;

        submenu_wid->update(ctx);
        auto direction = mouse_menu_widget_main::calculate_direction(
            submenu_wid.get(), ctx, anchor_x, anchor_y,
            popup_direction::bottom_right);

        if (direction == popup_direction::top_left ||
            direction == popup_direction::bottom_left) {
          anchor_x -= *width;
        }

        auto [x, y] = mouse_menu_widget_main::calculate_position(
            submenu_wid.get(), ctx, anchor_x, anchor_y, direction);

        submenu_wid->direction = direction;
        submenu_wid->x->reset_to(x / ctx.rt.dpi_scale);
        submenu_wid->y->reset_to(y / ctx.rt.dpi_scale);

        submenu_wid->reset_animation(direction == popup_direction::top_left ||
                                     direction == popup_direction::top_right);
        auto parent_menu = parent->downcast<menu_widget>();
        if (!parent_menu)
          parent_menu = parent->parent->downcast<menu_widget>();
        parent_menu->current_submenu = submenu_wid;
        parent_menu->rendering_submenus.push_back(submenu_wid);
        submenu_wid->parent_menu = parent_menu.get();
      }
    } else {
      if (submenu_wid) {
        submenu_wid->close();
        submenu_wid = nullptr;
      }
    }
  } else {
    if (submenu_wid) {
      submenu_wid->close();
      submenu_wid = nullptr;
    }
  }

  if (submenu_wid && submenu_wid->dying_time.has_value) {
    submenu_wid = nullptr;
  }
}
float mb_shell::menu_item_normal_widget::measure_width(
    ui::update_context &ctx) {
  if (item.type == menu_item::type::spacer) {
    return 1;
  }
  auto font_size = config::current->context_menu.theme.font_size;
  float width = 0;

  // left icon
  if (has_icon_padding || icon_img)
    width += icon_padding * 2 + font_size + 2;

  // text
  ctx.vg.fontSize(font_size);
  if (item.name)
    width += ctx.vg.measureText(item.name->c_str()).first + text_padding * 2;

  // right icon
  if (item.submenu) {
    width += font_size + 2 + right_icon_padding * 2;
  }

  return width + margin * 2 + padding * 2;
}

std::shared_ptr<ui::rect_widget>
mb_shell::menu_widget::create_bg(bool is_main) {
  std::shared_ptr<ui::rect_widget> bg;

  if (is_acrylic_available() && config::current->context_menu.theme.acrylic) {
    auto op = config::current->context_menu.theme.acrylic_opacity;
    auto acrylic_color = nvgRGBAf(op, op, op, 0.5);
    auto light_color = menu_render::current.value()->light_color;

    if (light_color) {
      acrylic_color = nvgRGBAf(1 - op, 1 - op, 1 - op, 0.5);
    }
    auto acrylic = std::make_shared<ui::acrylic_background_widget>(
        config::current->context_menu.theme.use_dwm_if_available
            ? is_win11_or_later()
            : false);
    acrylic->acrylic_bg_color = acrylic_color;
    acrylic->update_color();
    bg = acrylic;

    if (menu_render::current.value()->light_color)
      bg->bg_color = nvgRGBAf(1, 1, 1, 0);
    else
      bg->bg_color = nvgRGBAf(0, 0, 0, 0);
  } else {
    bg = std::make_shared<ui::rect_widget>();
    auto c = menu_render::current.value()->light_color ? 1 : 25 / 255.f;
    bg->bg_color = nvgRGBAf(c, c, c, 1);
  }

  bg->radius->reset_to(config::current->context_menu.theme.radius);

  bg->opacity->reset_to(0);
  if (is_main)
    config::current->context_menu.theme.animation.main_bg.opacity(bg->opacity,
                                                                  0);
  else
    config::current->context_menu.theme.animation.submenu_bg.opacity(
        bg->opacity, 0);
  bg->opacity->animate_to(
      255 * config::current->context_menu.theme.background_opacity);
  return bg;
}

mb_shell::menu_widget::menu_widget() : super() {
  gap = config::current->context_menu.theme.item_gap;
  width->set_easing(ui::easing_type::mutation);
  height->set_easing(ui::easing_type::mutation);
}
void mb_shell::menu_widget::update(ui::update_context &ctx) {
  if (dying_time) {
    if (dying_time.changed()) {
      y->animate_to(*y - 10);
    }
    if (bg_submenu)
      bg_submenu->opacity->animate_to(0);
    if (bg)
      bg->opacity->animate_to(0);
  }

  if (bg) {
    bg->x->reset_to(x->dest());
    bg->y->reset_to(y->dest() - bg_padding_vertical);
    bg->width->reset_to(width->dest());
    bg->height->reset_to(height->dest() + bg_padding_vertical * 2);
    bg->update(ctx);
  }

  if (current_submenu) {
    if (!bg_submenu) {
      bg_submenu = create_bg(false);
      bg_submenu->x->reset_to(current_submenu->x->dest());
      bg_submenu->y->reset_to(current_submenu->y->dest() - bg_padding_vertical);
      bg_submenu->width->reset_to(current_submenu->width->dest());
      bg_submenu->height->reset_to(current_submenu->height->dest() +
                                   bg_padding_vertical * 2);
    }

    bg_submenu->dying_time = std::nullopt;
    bg_submenu->opacity->animate_to(
        config::current->context_menu.theme.background_opacity * 255.f);
    bg_submenu->x->animate_to(current_submenu->x->dest());
    bg_submenu->y->animate_to(current_submenu->y->dest() - bg_padding_vertical);
    bg_submenu->width->animate_to(current_submenu->width->dest());
    bg_submenu->height->animate_to(current_submenu->height->dest() +
                                   bg_padding_vertical * 2);
  } else {
    if (bg_submenu) {
      if (!bg_submenu->dying_time)
        bg_submenu->dying_time = 200;
      bg_submenu->opacity->animate_to(0);

      if (bg_submenu->dying_time.time <= 0) {
        bg_submenu = nullptr;
      }
    }
  }

  auto rst = ctx.with_reset_offset();
  update_children(rst, rendering_submenus);

  if (bg_submenu) {
    auto c = ctx.with_reset_offset();
    c.mouse_clicked_on_hit(bg_submenu.get());
    c.hovered_hit(bg_submenu.get());
    bg_submenu->update(c);
  }

  if (ctx.hovered(this)) {
    scroll_top->animate_to(std::clamp(scroll_top->dest() + ctx.scroll_y * 100,
                                      height->dest() - actual_height, 0.f));
  }

  super::update(ctx);
  auto forkctx = ctx.with_offset(*x, *y + *scroll_top);
  update_children(forkctx, item_widgets);
  reposition_children_flex(forkctx, item_widgets);
  actual_height = height->dest();
  height->reset_to(std::min(max_height, height->dest()));

  if (bg) {
    ctx.mouse_clicked_on_hit(bg.get());
    ctx.hovered_hit(bg.get());
  }
}

bool mb_shell::menu_widget::check_hit(const ui::update_context &ctx) {
  auto hit =
      ui::widget::check_hit(ctx) || (bg && bg->check_hit(ctx)) ||
      (current_submenu && current_submenu->check_hit(ctx.with_reset_offset()));

  return hit;
}

void mb_shell::menu_widget::render(ui::nanovg_context ctx) {
  if (bg) {
    ctx.transaction([&]() {
      ctx.globalCompositeOperation(NVG_DESTINATION_IN);
      auto cl = nvgRGBAf(0, 0, 0, 1 - *bg->opacity / 255.f);
      ctx.fillColor(cl);
      ctx.fillRoundedRect(*bg->x, *bg->y, *bg->width, *bg->height, *bg->radius);
    });
    bg->render(ctx);
  }

  ctx.transaction([&] {
    super::render(ctx);
    ctx.scissor(*x, *y, *width, *height);
    render_children(ctx.with_offset(*x, *y + *scroll_top), item_widgets);
  });

  if (bg_submenu) {
    ctx.transaction([&]() {
      ctx.globalCompositeOperation(NVG_DESTINATION_IN);
      ctx.resetScissor();
      auto cl = nvgRGBAf(0, 0, 0, 1 - *bg_submenu->opacity / 255.f);
      ctx.fillColor(cl);
      ctx.with_reset_offset().fillRoundedRect(
          *bg_submenu->x, *bg_submenu->y, *bg_submenu->width,
          *bg_submenu->height, *bg_submenu->radius);
    });
    bg_submenu->render(ctx.with_reset_offset());
  }
  auto rst = ctx.with_reset_offset();
  render_children(rst, rendering_submenus);
}
void mb_shell::menu_item_normal_widget::reset_appear_animation(float delay) {
  this->opacity->after_animate = [this](float dest) {
    this->opacity->set_delay(0);
  };
  config::current->context_menu.theme.animation.item.opacity(opacity, delay);
  config::current->context_menu.theme.animation.item.x(x, delay);

  opacity->reset_to(0);
  opacity->animate_to(255);
  this->y->progress = 1;
  this->x->reset_to(-20);
  this->x->animate_to(0);
}
mb_shell::mouse_menu_widget_main::mouse_menu_widget_main(menu menu_data,
                                                         float x, float y)
    : widget(), anchor_x(x), anchor_y(y) {
  menu_wid = std::make_shared<menu_widget>();
  menu_wid->init_from_data(menu_data);
}
void mb_shell::mouse_menu_widget_main::update(ui::update_context &ctx) {
  ui::widget::update(ctx);

  // process events of parents
  PeekMessage(nullptr, nullptr, 0, 0, PM_REMOVE);

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

  if (!ctx.hovered(menu_wid.get(), false)) {
    glfwSetWindowAttrib(ctx.rt.window, GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE);
    if ((ctx.mouse_clicked || ctx.right_mouse_clicked) ||
        GetAsyncKeyState(VK_LBUTTON) & 0x8000 ||
        GetAsyncKeyState(VK_RBUTTON) & 0x8000) {
      ctx.rt.hide_as_close();
    }
  } else {
    glfwSetWindowAttrib(ctx.rt.window, GLFW_MOUSE_PASSTHROUGH, GLFW_FALSE);
  }

  // esc to close
  if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
    ctx.rt.hide_as_close();
  }
}
void mb_shell::mouse_menu_widget_main::render(ui::nanovg_context ctx) {
  ui::widget::render(ctx);
  menu_wid->render(ctx);
}
void mb_shell::menu_widget::reset_animation(bool reverse) {
  if (animate_appear_started)
    return;

  animate_appear_started = true;
  auto children = item_widgets | std::ranges::views::transform([](auto &w) {
                    return std::dynamic_pointer_cast<menu_item_widget>(w);
                  });

  // the show duration for the menu should be within 200ms
  float delay = std::min(200.f / children.size(), 30.f);

  for (size_t i = 0; i < children.size(); i++) {
    auto child = children[i];
    child->reset_appear_animation(delay * (reverse ? children.size() - i : i));
  }
}
std::pair<float, float> mb_shell::mouse_menu_widget_main::calculate_position(
    menu_widget *menu_wid, ui::update_context &ctx, float anchor_x,
    float anchor_y, popup_direction direction) {

  menu_wid->update(ctx);
  auto menu_width = menu_wid->measure_width(ctx);
  auto menu_height = menu_wid->measure_height(ctx);

  float x, y;

  // avoid the menu to be out of the screen
  constexpr auto mouse_padding = 1.f;
  if (direction == popup_direction::top_left) {
    x = anchor_x - menu_width * ctx.rt.dpi_scale - mouse_padding;
    y = anchor_y - menu_height * ctx.rt.dpi_scale;
  } else if (direction == popup_direction::top_right) {
    x = anchor_x + mouse_padding;
    y = anchor_y - menu_height * ctx.rt.dpi_scale;
  } else if (direction == popup_direction::bottom_left) {
    x = anchor_x - menu_width * ctx.rt.dpi_scale - mouse_padding;
    y = anchor_y;
  } else {
    x = anchor_x + mouse_padding;
    y = anchor_y;
  }

  auto padding_vertical =
           config::current->context_menu.position.padding_horizontal,
       padding_horizontal =
           config::current->context_menu.position.padding_vertical;

  if (x < padding_vertical) {
    x = padding_vertical;
  } else if (x + menu_width * ctx.rt.dpi_scale >
             ctx.screen.width - padding_vertical) {
    x = ctx.screen.width - menu_width * ctx.rt.dpi_scale - padding_vertical;
  }
  auto top_overflow = y < padding_horizontal;
  auto bottom_overflow = y + menu_height * ctx.rt.dpi_scale >
                         ctx.screen.height - padding_horizontal;

  if (top_overflow && bottom_overflow) {
    y = padding_horizontal;
    menu_wid->max_height = ctx.screen.height - padding_horizontal * 2;
  } else if (top_overflow) {
    y = padding_horizontal;
  } else if (bottom_overflow) {
    y = ctx.screen.height - menu_height * ctx.rt.dpi_scale - padding_horizontal;
  }

  return {x, y};
}

mb_shell::popup_direction mb_shell::mouse_menu_widget_main::calculate_direction(
    menu_widget *menu_wid, ui::update_context &ctx, float anchor_x,
    float anchor_y, popup_direction prefer_direction) {
  auto menu_width = menu_wid->measure_width(ctx);
  auto menu_height = menu_wid->measure_height(ctx);

  auto padding_vertical =
           config::current->context_menu.position.padding_horizontal,
       padding_horizontal =
           config::current->context_menu.position.padding_vertical;

  bool bottom_overflow = (anchor_y + menu_height * ctx.rt.dpi_scale >
                          ctx.screen.height - padding_vertical);
  bool top_overflow =
      (anchor_y - menu_height * ctx.rt.dpi_scale < padding_vertical);

  bool right_overflow = (anchor_x + menu_width * ctx.rt.dpi_scale >
                         ctx.screen.width - padding_horizontal);
  bool left_overflow =
      (anchor_x - menu_width * ctx.rt.dpi_scale < padding_horizontal);

  bool top_revert = false;
  bool left_revert = false;

  if (prefer_direction == popup_direction::bottom_right) {
    if (bottom_overflow && !top_overflow)
      top_revert = true;
    if (right_overflow && !left_overflow)
      left_revert = true;
  } else if (prefer_direction == popup_direction::bottom_left) {
    if (bottom_overflow && !top_overflow)
      top_revert = true;
    if (left_overflow && !right_overflow)
      left_revert = true;
  } else if (prefer_direction == popup_direction::top_right) {
    if (top_overflow && !bottom_overflow)
      top_revert = true;
    if (right_overflow && !left_overflow)
      left_revert = true;
  } else if (prefer_direction == popup_direction::top_left) {
    if (top_overflow && !bottom_overflow)
      top_revert = true;
    if (left_overflow && !right_overflow)
      left_revert = true;
  }

  if (top_revert && left_revert) {
    return popup_direction::top_left;
  } else if (top_revert && !left_revert) {
    return popup_direction::top_right;
  } else if (!top_revert && left_revert) {
    return popup_direction::bottom_left;
  } else {
    return popup_direction::bottom_right;
  }
}

void mb_shell::mouse_menu_widget_main::calibrate_position(
    ui::update_context &ctx, bool animated) {
  menu_wid->update(ctx);
  auto [x, y] =
      calculate_position(menu_wid.get(), ctx, anchor_x, anchor_y, direction);

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
  menu_wid->update(ctx);
  direction = calculate_direction(menu_wid.get(), ctx, anchor_x, anchor_y);
  menu_wid->direction = direction;
  menu_wid->reset_animation(direction == popup_direction::top_left ||
                            direction == popup_direction::top_right);

  std::println("Calibrated direction: {}",
               direction == popup_direction::top_left       ? "top_left"
               : direction == popup_direction::top_right    ? "top_right"
               : direction == popup_direction::bottom_left  ? "bottom_left"
               : direction == popup_direction::bottom_right ? "bottom_right"
                                                            : "unknown");
}

bool mb_shell::menu_item_normal_widget::check_hit(
    const ui::update_context &ctx) {
  return (ui::widget::check_hit(ctx) &&
          (ctx.mouse_x > ctx.offset_x + *x + margin &&
           ctx.mouse_x < ctx.offset_x + *x + *width - margin)) ||
         (submenu_wid && submenu_wid->check_hit(ctx));
}

mb_shell::menu_item_normal_widget::menu_item_normal_widget(menu_item item)
    : super() {
  opacity->reset_to(0);
  this->item = item;
}

void mb_shell::menu_widget::init_from_data(menu menu_data) {
  if (menu_data.is_top_level && !bg) {
    bg = create_bg(true);
  }
  auto init_items = menu_data.items;

  for (size_t i = 0; i < init_items.size(); i++) {
    auto &item = init_items[i];
    auto mi = std::make_shared<menu_item_normal_widget>(item);
    item_widgets.push_back(mi);
  }

  std::println("Menu widget init from data: {}", menu_data.items.size());

  update_icon_width();
  this->menu_data = menu_data;
}
void mb_shell::menu_widget::update_icon_width() {
  bool has_icon = std::ranges::any_of(item_widgets, [](auto &item) {
    if (!item->template downcast<menu_item_normal_widget>())
      return false;
    auto i = item->template downcast<menu_item_normal_widget>()->item;
    return i.icon_bitmap.has_value() || i.icon_svg.has_value();
  });

  for (auto &item : item_widgets) {
    auto mi = item->template downcast<menu_item_normal_widget>();
    if (!mi)
      continue;
    if (!has_icon) {
      mi->has_icon_padding = 0;
    } else {
      mi->has_icon_padding = 1;
    }
  }
};
void mb_shell::menu_item_normal_widget::reload_icon_img(
    ui::nanovg_context ctx) {
  if (item.icon_bitmap)
    icon_img = ui::LoadBitmapImage(ctx, (HBITMAP)item.icon_bitmap.value());
  else if (item.icon_svg) {
    std::string copy = item.icon_svg.value();
    auto svg = nsvgParse(copy.data(), "px", 96);
    auto icon_width = config::current->context_menu.theme.font_size + 2;
    icon_img = ctx.imageFromSVG(svg, ctx.rt->dpi_scale);
  } else {
    icon_img = std::nullopt;
  }

  if (auto pa = parent->downcast<menu_widget>()) {
    pa->update_icon_width();
  }
}
void mb_shell::menu_widget::close() {
  if (menu_data.is_top_level) {
    auto current = menu_render::current;
    if (current) {
      (*current)->rt->hide_as_close();
    }
  } else {
    dying_time = 200;
    if (parent_menu->current_submenu.get() == this) {
      parent_menu->current_submenu = nullptr;
    }
  }
}
mb_shell::menu_item_widget::menu_item_widget() {}
void mb_shell::menu_item_widget::reset_appear_animation(float delay) {
  for (auto &child : get_children<menu_item_widget>())
    child->reset_appear_animation(delay);
}
void mb_shell::menu_item_parent_widget::update(ui::update_context &ctx) {
  super::update(ctx);
  float x = 0;
  float gap = config::current->context_menu.theme.multibutton_line_gap;
  float max_height = 0;
  for (auto &item : children) {
    item->x->reset_to(x);
    item->y->reset_to(0);
    auto item_width = item->measure_width(ctx);
    item->width->reset_to(item_width);
    x += item_width + gap;
    max_height = std::max(max_height, item->measure_height(ctx));
  }

  width->reset_to(x - gap);
  height->reset_to(max_height);
}
