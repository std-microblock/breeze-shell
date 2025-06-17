#include "menu_widget.h"
#include "../config.h"
#include "../utils.h"
#include "GLFW/glfw3.h"
#include "animator.h"
#include "contextmenu.h"
#include "hbitmap_utils.h"
#include "menu_render.h"
#include "nanovg.h"
#include "nanovg_wrapper.h"
#include "ui.h"
#include "widget.h"
#include <algorithm>
#include <iostream>
#include <print>
#include <ranges>
#include <vector>

#include "../logger.h"
/*
| padding | icon_padding | icon | icon_padding | text_padding | text |
text_padding | hotkey_padding | hotkey | hotkey_padding | right_icon_padding |
right_icon | right_icon_padding |
*/
void mb_shell::menu_item_normal_widget::render(ui::nanovg_context ctx) {
  super::render(ctx);

  auto icon_width = config::current->context_menu.theme.font_size + 2;
  auto has_icon = has_icon_padding || icon_img;
  auto c = menu_render::current.value()->light_color ? 0 : 1;

  if (item.type == menu_item::type::spacer) {
    ctx.fillColor(nvgRGBAf(c, c, c, 0.1 * *opacity / 255.f));
    ctx.fillRect(x->dest(), *y, *width, *height);
    return;
  }

  // Draw background
  ctx.fillColor(nvgRGBAf(c, c, c, *bg_opacity / 255.f));
  float roundcorner = std::min(height->dest() / 2,
                               config::current->context_menu.theme.item_radius);
  ctx.fillRoundedRect(*x + margin, *y, *width - margin * 2, *height,
                      roundcorner);

  // Draw focused border
  if (focused()) {
    ctx.strokeColor(nvgRGBAf(c, c, c, *opacity / 255.f * 0.5)); // Half opacity
    ctx.strokeWidth(1.0f);
    ctx.strokeRoundedRect(*x + margin, *y, *width - margin * 2, *height,
                          roundcorner);
  }

  // Draw left icon
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

  // Draw text
  ctx.fillColor(nvgRGBAf(c, c, c, *opacity / 255.f));
  ctx.fontFace("main");
  auto font_size = config::current->context_menu.theme.font_size;
  auto hotkey_padding = config::current->context_menu.theme.hotkey_padding;
  ctx.fontSize(font_size);
  ctx.textAlign(NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
  if (item.name) {
    ctx.text(round(*x + padding +
                   (has_icon ? (icon_width + icon_padding * 2) : 0) +
                   text_padding + margin),
             round(*y + *height / 2), item.name->c_str(), nullptr);
  }

  // Calculate right side positions
  auto right_x = *x + width->dest() - margin - padding;

  // Draw right icon (submenu indicator)
  if (item.submenu) {
    if (!icon_unfold_img) {
      auto icon_unfold = std::format(
          R"#(<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" viewBox="0 0 12 12"><path opacity="0.7" fill="{}" d="M4.646 2.146a.5.5 0 0 0 0 .708L7.793 6L4.646 9.146a.5.5 0 1 0 .708.708l3.5-3.5a.5.5 0 0 0 0-.708l-3.5-3.5a.5.5 0 0 0-.708 0"/></svg>)#",
          c ? "white" : "black");
      auto icon_unfold_img_svg = nsvgParse(icon_unfold.data(), "px", 96);
      this->icon_unfold_img =
          ctx.imageFromSVG(icon_unfold_img_svg, ctx.rt->dpi_scale);
    }

    auto paintY = floor(*y + (*height - icon_width) / 2);
    auto paintX = right_x - icon_width;
    auto paint = ctx.imagePattern(paintX, paintY, icon_width, icon_width, 0,
                                  icon_unfold_img->id, *opacity / 255.f);
    ctx.beginPath();
    ctx.rect(paintX, paintY, icon_width, icon_width);
    ctx.fillPaint(paint);
    ctx.fill();

    right_x = paintX;
  } else if (has_submenu_padding) {
    // Reserve space for right icon alignment even if this item doesn't have
    // submenu
    right_x -= icon_width;
  }

  // Draw hotkey
  if (item.hotkey && !item.hotkey->empty()) {
    auto t = ctx.transaction();
    ctx.fillColor(nvgRGBAf(c, c, c, *opacity / 255.f * 0.7));
    ctx.fontSize(font_size * 0.9);
    ctx.textAlign(NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
    ctx.fontFace("monospace");
    auto hotkey_x = right_x - hotkey_padding;
    ctx.text(round(hotkey_x), round(*y + *height / 2), item.hotkey->c_str(),
             nullptr);
  }
}

float mb_shell::menu_item_normal_widget::measure_width(
    ui::update_context &ctx) {
  if (item.type == menu_item::type::spacer) {
    return 1;
  }

  auto font_size = config::current->context_menu.theme.font_size;
  float width = 0;

  // Left padding
  width += padding;

  // Left icon
  if (has_icon_padding || icon_img)
    width += icon_padding * 2 + font_size + 2;

  // Text
  ctx.vg.fontSize(font_size);
  if (item.name)
    width += ctx.vg.measureText(item.name->c_str()).first + text_padding * 2;

  // Hotkey
  if (item.hotkey && !item.hotkey->empty()) {
    auto t = ctx.vg.transaction();
    ctx.vg.fontSize(font_size * 0.9);
    auto hotkey_padding = config::current->context_menu.theme.hotkey_padding;
    ctx.vg.fontFace("monospace");
    width +=
        ctx.vg.measureText(item.hotkey->c_str()).first + hotkey_padding * 2;
  }

  // Right icon space (always reserve if any item in menu has submenu)
  if (has_submenu_padding) {
    width += font_size + 2 + right_icon_padding;
  }

  // Right padding
  width += padding;

  return width + margin * 2;
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
    float show_submenu_timer_before = show_submenu_timer;
    if (ctx.hovered(this)) {
      show_submenu_timer = std::min(show_submenu_timer + ctx.delta_time, 300.f);
    } else if (ctx.within(parent).hovered(parent)) {
      show_submenu_timer = std::max(show_submenu_timer - ctx.delta_time, 0.f);
    }

    // only act if changed
    if (show_submenu_timer_before != show_submenu_timer) {
      if (show_submenu_timer >= 150.f) {
        show_submenu(ctx);
      } else {
        hide_submenu();
      }
    }
  } else {
    hide_submenu();
  }

  if (submenu_wid && submenu_wid->dying_time.has_value) {
    submenu_wid = nullptr;
  }
}
std::shared_ptr<ui::rect_widget>
mb_shell::menu_widget::create_bg(bool is_main) {
  std::shared_ptr<ui::rect_widget> bg;

  if (is_acrylic_available() && config::current->context_menu.theme.acrylic) {

    auto light_color = menu_render::current.value()->light_color;
    auto acrylic_color =
        light_color
            ? parse_color(
                  config::current->context_menu.theme.acrylic_color_light)
            : parse_color(
                  config::current->context_menu.theme.acrylic_color_dark);

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
  else {
    config::current->context_menu.theme.animation.submenu_bg.opacity(
        bg->opacity, 0);
    config::current->context_menu.theme.animation.submenu_bg.x(bg->x, 0);
    config::current->context_menu.theme.animation.submenu_bg.y(bg->y, 0);
    config::current->context_menu.theme.animation.submenu_bg.w(bg->width, 0);
    config::current->context_menu.theme.animation.submenu_bg.h(bg->height, 0);
  }
  bg->opacity->animate_to(
      255 * config::current->context_menu.theme.background_opacity);
  return bg;
}

mb_shell::menu_widget::menu_widget() : super() {
  gap = config::current->context_menu.theme.item_gap;
  width->set_easing(ui::easing_type::mutation);
  height->set_easing(ui::easing_type::mutation);
  config::current->context_menu.theme.animation.main.y(y);
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

  reverse = (direction == popup_direction::top_left ||
             direction == popup_direction::top_right) &&
            config::current->context_menu.reverse_if_open_to_up;

  auto forkctx_1 = ctx.with_offset(*x, *y);
  update_children(forkctx_1, rendering_submenus);

  if (bg_submenu) {
    bg_submenu->update(forkctx_1);
  }

  if (ctx.hovered(this)) {
    scroll_top->animate_to(std::clamp(scroll_top->dest() + ctx.scroll_y * 100,
                                      height->dest() - actual_height, 0.f));
  }
  widget::update(ctx);
  ctx.hovered_hit(this);
  auto forkctx = ctx.with_offset(*x, *y + *scroll_top);
  update_children(forkctx, item_widgets);
  reposition_children_flex(forkctx, item_widgets);
  actual_height = height->dest();
  height->reset_to(std::min(max_height, height->dest()));

  if (bg) {
    bg->update(ctx);
  }

  // process keyboard actions
  // 1. check if focused
  //    we should handle keyboard actions only if this menu is focused or
  //    nothing is focused and we don't have any submenus
  bool should_handle_keyboard =
      owner_rt &&
      (focused() ||
       std::ranges::any_of(
           item_widgets,
           [](const auto &item) { return item->focus_within(); }) ||
       (!owner_rt->focused_widget.has_value() && rendering_submenus.empty()));

  if (should_handle_keyboard) {
    auto move_key = [&](this auto &self, bool next, auto &items) -> void {
      auto focused_item = std::ranges::find_if(
          items, [](const auto &item) { return item->focused(); });
      if (focused_item != items.end()) {
        if (next) {
          focused_item++;
          if (focused_item == items.end()) {
            focused_item = items.begin();
          }
        } else {
          if (focused_item == items.begin()) {
            focused_item = items.end() - 1;
          } else {
            focused_item--;
          }
        }

        (*focused_item)->set_focus(true);

        if (auto wid =
                (*focused_item)->template downcast<menu_item_normal_widget>()) {
          if (wid->item.disabled ||
              wid->item.type == mb_shell::menu_item::type::spacer) {
            self(next, items);
            return;
          }
        } else {
          self(next, items);
        }
      } else {
        if (!items.empty()) {
          items.front()->set_focus(true);
        }
      }
    };
    if (ctx.key_pressed(GLFW_KEY_UP)) {
      move_key(false, item_widgets);
    } else if (ctx.key_pressed(GLFW_KEY_DOWN)) {
      move_key(true, item_widgets);
    } else if (ctx.key_pressed(GLFW_KEY_LEFT)) {
      // close submenu on left key
      if (parent_menu) {
        parent_menu->current_submenu = nullptr;
        close();
      }

      ctx.stop_key_propagation(GLFW_KEY_LEFT);

      if (parent_item_widget) {
        parent_item_widget->lock()->set_focus();
      } else {
        parent_menu->set_focus();
      }
    } else if (ctx.key_pressed(GLFW_KEY_RIGHT)) {
      auto focused_item = std::ranges::find_if(
          item_widgets, [](const auto &item) { return item->focused(); });
      if (focused_item != item_widgets.end()) {
        if (auto wid = (*focused_item)->downcast<menu_item_normal_widget>())
          if (wid->item.submenu && !wid->submenu_wid) {
            wid->show_submenu(ctx);
            current_submenu->set_focus();
          }
      }
    } else if (ctx.key_pressed(GLFW_KEY_ENTER) ||
               ctx.key_pressed(GLFW_KEY_SPACE)) { // Enter or Space key
      auto focused_item = std::ranges::find_if(
          item_widgets, [](const auto &item) { return item->focused(); });
      if (focused_item != item_widgets.end()) {
        if (auto wid = (*focused_item)->downcast<menu_item_normal_widget>()) {
          if (wid->item.action) {
            try {
              wid->item.action.value()();
            } catch (std::exception &e) {
              std::cerr << "Error in action: " << e.what() << std::endl;
            }
          } else if (wid->item.submenu) {
            wid->show_submenu(ctx);
          }
        }
      }
    } else {
      auto menus_matching_key =
          item_widgets | std::views::filter([&](const auto &item) {
            if (auto wid = item->template downcast<menu_item_normal_widget>()) {
              if (!wid->item.hotkey)
                return false;
              auto hotkey = *wid->item.hotkey | std::views::split('+') |
                            std::views::transform([](const auto &c) {
                              auto s = std::string(c.begin(), c.end());
                              // trim whitespace
                              s.erase(s.find_last_not_of(" \t\n\r") + 1);
                              s.erase(0, s.find_first_not_of(" \t\n\r"));
                              return s;
                            });

              static auto translate_map = std::unordered_map<std::string, int>{
                  {"ctrl", GLFW_KEY_LEFT_CONTROL},
                  {"shift", GLFW_KEY_LEFT_SHIFT},
                  {"alt", GLFW_KEY_LEFT_ALT},
                  {"win", GLFW_KEY_LEFT_SUPER},
                  {"a", GLFW_KEY_A},
                  {"b", GLFW_KEY_B},
                  {"c", GLFW_KEY_C},
                  {"d", GLFW_KEY_D},
                  {"e", GLFW_KEY_E},
                  {"f", GLFW_KEY_F},
                  {"g", GLFW_KEY_G},
                  {"h", GLFW_KEY_H},
                  {"i", GLFW_KEY_I},
                  {"j", GLFW_KEY_J},
                  {"k", GLFW_KEY_K},
                  {"l", GLFW_KEY_L},
                  {"m", GLFW_KEY_M},
                  {"n", GLFW_KEY_N},
                  {"o", GLFW_KEY_O},
                  {"p", GLFW_KEY_P},
                  {"q", GLFW_KEY_Q},
                  {"r", GLFW_KEY_R},
                  {"s", GLFW_KEY_S},
                  {"t", GLFW_KEY_T},
                  {"u", GLFW_KEY_U},
                  {"v", GLFW_KEY_V},
                  {"w", GLFW_KEY_W},
                  {"x", GLFW_KEY_X},
                  {"y", GLFW_KEY_Y},
                  {"z", GLFW_KEY_Z},
                  {"0", GLFW_KEY_0},
                  {"1", GLFW_KEY_1},
                  {"2", GLFW_KEY_2},
                  {"3", GLFW_KEY_3},
                  {"4", GLFW_KEY_4},
                  {"5", GLFW_KEY_5},
                  {"6", GLFW_KEY_6},
                  {"7", GLFW_KEY_7},
                  {"8", GLFW_KEY_8},
                  {"9", GLFW_KEY_9},
              };

              auto key_combination = std::vector<int>();
              for (const auto &key : hotkey) {
                if (auto it = translate_map.find(
                        std::string(key) | std::views::transform(::tolower) |
                        std::ranges::to<std::string>());
                    it != translate_map.end()) {
                  key_combination.push_back(it->second);
                } else {
                  // If the key is not found, we can ignore it
                  return false;
                }
              }

              return std::ranges::all_of(key_combination, [&](int key) {
                return ctx.key_pressed(key);
              });
            }
            return false;
          }) |
          std::ranges::to<std::vector>();

      if (menus_matching_key.size() == 1) {
        auto wid = menus_matching_key.front()
                       ->downcast<mb_shell::menu_item_normal_widget>();
        if (wid && wid->item.action) {
          try {
            wid->item.action.value()();
          } catch (std::exception &e) {
            std::cerr << "Error in action: " << e.what() << std::endl;
          }
        } else if (wid && wid->item.submenu && !wid->submenu_wid) {
          wid->show_submenu(ctx);
          current_submenu->set_focus();
        }
      } else if (menus_matching_key.size() > 1) {
        move_key(!ctx.key_pressed(GLFW_KEY_LEFT_SHIFT) &&
                     !ctx.key_pressed(GLFW_KEY_RIGHT_SHIFT),
                 menus_matching_key);
      }
    }
  }
}

bool mb_shell::menu_widget::check_hit(const ui::update_context &ctx) {
  auto hit = ui::widget::check_hit(ctx) || (bg && bg->check_hit(ctx));
  return hit;
}

void mb_shell::menu_widget::render(ui::nanovg_context ctx) {

  auto bg_filler_factory = [&](auto bg, ui::nanovg_context &ctx) {
    return [bg, ctx]() mutable {
      ctx.globalAlpha(*bg->opacity / 255.f);
      auto &theme = config::current->context_menu.theme;
      bool light = menu_render::current.value()->light_color;

      bool use_dwm = config::current->context_menu.theme.use_dwm_if_available
                         ? is_win11_or_later()
                         : false;
      bool use_self_drawn_border = theme.use_self_drawn_border && !use_dwm;

      float boarder_width = use_self_drawn_border ? theme.border_width : 0.0f;
      if (use_self_drawn_border) {
        float shadow_size = theme.shadow_size,
              shadow_offset_x = theme.shadow_offset_x,
              shadow_offset_y = theme.shadow_offset_y;
        float corner_radius = theme.radius;
        NVGcolor shadow_color_from =
                     parse_color(light ? theme.shadow_color_light_from
                                       : theme.shadow_color_dark_from),
                 shadow_color_to =
                     parse_color(light ? theme.shadow_color_light_to
                                       : theme.shadow_color_dark_to);

        ctx.beginPath();
        ctx.beginPath();

        ctx.roundedRect(*bg->x - shadow_size + shadow_offset_x,
                        *bg->y - shadow_size + shadow_offset_y,
                        *bg->width + shadow_size * 2,
                        *bg->height + shadow_size * 2,
                        corner_radius + shadow_size);
        ctx.fillPaint(ctx.boxGradient(*bg->x + shadow_offset_x,
                                      *bg->y + shadow_offset_y, *bg->width,
                                      *bg->height, corner_radius, shadow_size,
                                      shadow_color_from, shadow_color_to));
        ctx.fill();

        // Draw the border
        ctx.beginPath();

        if (theme.inset_border) {
          ctx.roundedRect(*bg->x + boarder_width / 2,
                          *bg->y + boarder_width / 2,
                          *bg->width - boarder_width,
                          *bg->height - boarder_width, corner_radius);
        } else {
          ctx.roundedRect(*bg->x, *bg->y, *bg->width, *bg->height,
                          corner_radius);
        }
        ctx.strokeWidth(boarder_width);
        auto border_color =
            light ? theme.border_color_light : theme.border_color_dark;
        border_color.apply_to_ctx(ctx, *bg->x, *bg->y, *bg->width, *bg->height);
        ctx.stroke();
      }

      ctx.globalCompositeOperation(NVG_DESTINATION_IN);
      ctx.globalAlpha(1);
      auto cl = nvgRGBAf(0, 0, 0, 1 - *bg->opacity / 255.f);
      ctx.fillColor(cl);
      if (theme.inset_border)
        ctx.fillRoundedRect(*bg->x + boarder_width, *bg->y + boarder_width,
                            *bg->width - boarder_width * 2,
                            *bg->height - boarder_width * 2, *bg->radius);
      else
        ctx.fillRoundedRect(*bg->x, *bg->y, *bg->width, *bg->height,
                            *bg->radius);
    };
  };
  if (bg) {
    ctx.transaction(bg_filler_factory(bg, ctx));
    bg->render(ctx);
  }

  ctx.transaction([&] {
    super::render(ctx);
    ctx.scissor(*x, *y, *width, *height);
    render_children(ctx.with_offset(*x, *y + *scroll_top), item_widgets);
  });

  auto ctx2 = ctx.with_offset(*x, *y);

  if (bg_submenu) {
    ctx2.transaction(bg_filler_factory(bg_submenu, ctx2));
    bg_submenu->render(ctx2);
  }
  render_children(ctx2, rendering_submenus);

  // scrollbar
  if (height->dest() < actual_height) {
    auto scrollbar_width = config::current->context_menu.theme.scrollbar_width;
    auto scrollbar_height = height->dest() * height->dest() / actual_height;
    auto scrollbar_x = width->dest() - scrollbar_width - 2 + *x;
    auto scrollbar_y = *y - *scroll_top / (actual_height - height->dest()) *
                                (height->dest() - scrollbar_height);

    float c = menu_render::current.value()->light_color ? 0 : 1;
    ctx.fillColor(nvgRGBAf(c, c, c, 0.1));
    ctx.fillRoundedRect(scrollbar_x, scrollbar_y, scrollbar_width,
                        scrollbar_height,
                        config::current->context_menu.theme.scrollbar_radius);
  }
}
void mb_shell::menu_item_normal_widget::reset_appear_animation(float delay) {
  this->opacity->after_animate = [this](float dest) {
    this->opacity->set_delay(0);
  };
  opacity->reset_to(0);
  this->x->reset_to(-20);

  config::current->context_menu.theme.animation.item.opacity(opacity, delay);
  config::current->context_menu.theme.animation.item.x(x, delay);
  config::current->context_menu.theme.animation.item.width(width);

  opacity->animate_to(255);
  this->y->progress = 1;
  this->y->easing = ui::easing_type::mutation;

  this->x->animate_to(0);
}

BOOL IsCursorVisible() {
  CURSORINFO ci = {sizeof(CURSORINFO)};
  if (GetCursorInfo(&ci)) {
    return (ci.flags & CURSOR_SHOWING) != 0;
  }
  return FALSE;
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

  auto using_touchscreen = !IsCursorVisible();

  if (ctx.hovered_widgets->empty()) {
    glfwSetWindowAttrib(ctx.rt.window, GLFW_MOUSE_PASSTHROUGH,
                        using_touchscreen ? GLFW_FALSE : GLFW_TRUE);

    if ((ctx.mouse_clicked || ctx.right_mouse_clicked) ||
        GetAsyncKeyState(VK_LBUTTON) & 0x8000 ||
        GetAsyncKeyState(VK_RBUTTON) & 0x8000) {
      ctx.rt.hide_as_close();
    }
  } else {
    glfwSetWindowAttrib(ctx.rt.window, GLFW_MOUSE_PASSTHROUGH, GLFW_FALSE);
  }

  // esc/alt to close
  if ((GetAsyncKeyState(VK_ESCAPE) & 0x8000) ||
      (GetAsyncKeyState(VK_MENU) & 0x8000) ||
      (GetAsyncKeyState(VK_LMENU) & 0x8000)) {
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

  if (config::current->context_menu.reverse_if_open_to_up && reverse)
    reverse = !reverse;

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
           config::current->context_menu.position.padding_horizontal *
           ctx.rt.dpi_scale,
       padding_horizontal =
           config::current->context_menu.position.padding_vertical *
           ctx.rt.dpi_scale;

  if (x < padding_vertical) {
    x = padding_vertical;
  } else if (x + menu_width * ctx.rt.dpi_scale >
             ctx.screen.width - padding_vertical) {
    x = ctx.screen.width - menu_width * ctx.rt.dpi_scale - padding_vertical;
  }
  auto top_overflow = y < padding_horizontal;
  auto bottom_overflow = y + menu_height * ctx.rt.dpi_scale >
                         ctx.screen.height - padding_horizontal;

  if (menu_height * ctx.rt.dpi_scale >
      ctx.screen.height - padding_horizontal * 2) {
    y = padding_horizontal;
  } else if (top_overflow) {
    y = padding_horizontal;
  } else if (bottom_overflow) {
    y = ctx.screen.height - menu_height * ctx.rt.dpi_scale - padding_horizontal;
  }

  menu_wid->max_height =
      (ctx.screen.height - y) / ctx.rt.dpi_scale - padding_horizontal;

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

  dbgout("Calibrated position: {} {} in screen {} {}", x, y, ctx.screen.width,
         ctx.screen.height);

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

  dbgout("Calibrated direction: {}",
         direction == popup_direction::top_left       ? "top_left"
         : direction == popup_direction::top_right    ? "top_right"
         : direction == popup_direction::bottom_left  ? "bottom_left"
         : direction == popup_direction::bottom_right ? "bottom_right"
                                                      : "unknown");
}

bool mb_shell::menu_item_normal_widget::check_hit(
    const ui::update_context &ctx) {
  return (ui::widget::check_hit(ctx)) ||
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
    if (item.owner_draw) {
      auto mi = std::make_shared<menu_item_ownerdraw_widget>(item);
      item_widgets.push_back(mi);
    } else {
      auto mi = std::make_shared<menu_item_normal_widget>(item);
      item_widgets.push_back(mi);
    }
  }

  dbgout("Menu widget init from data: {}", menu_data.items.size());

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

  bool has_submenu = std::ranges::any_of(item_widgets, [](auto &item) {
    if (!item->template downcast<menu_item_normal_widget>())
      return false;
    auto i = item->template downcast<menu_item_normal_widget>()->item;
    return i.submenu.has_value();
  });

  for (auto &item : item_widgets) {
    auto mi = item->template downcast<menu_item_normal_widget>();
    if (!mi)
      continue;
    mi->has_icon_padding = has_icon;
    mi->has_submenu_padding = has_submenu;
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

    for (auto &item : item_widgets) {
      auto mi = item->downcast<menu_item_normal_widget>();
      if (mi) {
        mi->hide_submenu();
      }
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
void mb_shell::menu_item_parent_widget::reset_appear_animation(float delay) {
  y->set_easing(ui::easing_type::mutation);
  x->reset_to(-20);
  x->animate_to(0);
  opacity->reset_to(0);
  opacity->animate_to(255);
}
void mb_shell::menu_item_normal_widget::hide_submenu() {
  if (submenu_wid != nullptr) {
    submenu_wid->close();
    submenu_wid = nullptr;
  }
}
void mb_shell::menu_item_normal_widget::show_submenu(ui::update_context &ctx) {
  if (submenu_wid != nullptr)
    return;
  submenu_wid = std::make_shared<menu_widget>();
  item.submenu.value()(submenu_wid);

  // We calculate the position of the submenu in
  // the screen space, then convert it to the
  // window space.

  auto anchor_x = width->dest() + *x + ctx.offset_x;
  auto anchor_y = **y + ctx.offset_y;

  anchor_x *= ctx.rt.dpi_scale;
  anchor_y *= ctx.rt.dpi_scale;

  ctx.mouse_clicked = false;
  ctx.mouse_down = false;
  submenu_wid->update(ctx);
  auto direction = mouse_menu_widget_main::calculate_direction(
      submenu_wid.get(), ctx, anchor_x, anchor_y,
      popup_direction::bottom_right);

  if (direction == popup_direction::top_left ||
      direction == popup_direction::bottom_left) {
    anchor_x -= *width * ctx.rt.dpi_scale;
  }

  if (direction == popup_direction::top_left ||
      direction == popup_direction::top_right) {
    anchor_y += *height * ctx.rt.dpi_scale;
  }

  auto [x, y] = mouse_menu_widget_main::calculate_position(
      submenu_wid.get(), ctx, anchor_x, anchor_y, direction);

  if (auto parent = search_parent<menu_widget>())
    y += *parent->scroll_top * ctx.rt.dpi_scale;

  x -= ctx.offset_x * ctx.rt.dpi_scale;
  y -= ctx.offset_y * ctx.rt.dpi_scale;

  submenu_wid->direction = direction;
  submenu_wid->x->reset_to(x / ctx.rt.dpi_scale);
  submenu_wid->y->reset_to(y / ctx.rt.dpi_scale);

  submenu_wid->reset_animation(direction == popup_direction::top_left ||
                               direction == popup_direction::top_right);
  submenu_wid->parent_item_widget = weak_from_this();
  auto parent_menu = parent->downcast<menu_widget>();
  if (!parent_menu)
    parent_menu = parent->parent->downcast<menu_widget>();
  if (parent_menu->current_submenu) {
    parent_menu->current_submenu->close();
    parent_menu->current_submenu = nullptr;
  }
  parent_menu->current_submenu = submenu_wid;
  parent_menu->rendering_submenus.push_back(submenu_wid);
  submenu_wid->parent_menu = parent_menu.get();
}
void mb_shell::menu_item_ownerdraw_widget::update(ui::update_context &ctx) {
  width->reset_to(owner_draw.width);
  height->reset_to(owner_draw.height);
}
void mb_shell::menu_item_ownerdraw_widget::render(ui::nanovg_context ctx) {
  if (!img)
    img = ui::LoadBitmapImage(ctx, owner_draw.bitmap);

  auto paint = ctx.imagePattern(*x, y->dest(), owner_draw.width,
                                owner_draw.height, 0, img->id, 1);

  ctx.beginPath();
  ctx.rect(*x, y->dest(), owner_draw.width, owner_draw.height);
  ctx.fillPaint(paint);
  ctx.fill();
}
void mb_shell::menu_item_ownerdraw_widget::reset_appear_animation(float delay) {
}
mb_shell::menu_item_ownerdraw_widget::menu_item_ownerdraw_widget(
    menu_item item) {
  this->item = item;
  if (item.owner_draw) {
    owner_draw = item.owner_draw.value();
    width->reset_to(owner_draw.width);
    height->reset_to(owner_draw.height);
  }
}
void mb_shell::menu_item_custom_widget::update(ui::update_context &ctx) {
  super::update(ctx);
  if (custom_widget) {
    auto ctx2 = ctx.with_offset(*x, *y);
    custom_widget->update(ctx2);
    custom_widget->parent = this;
  }
}
void mb_shell::menu_item_custom_widget::render(ui::nanovg_context ctx) {
  super::render(ctx);
  if (custom_widget) {
    custom_widget->render(ctx.with_offset(*x, *y));
  }
}
float mb_shell::menu_item_custom_widget::measure_width(
    ui::update_context &ctx) {
  return custom_widget->measure_width(ctx);
}
float mb_shell::menu_item_custom_widget::measure_height(
    ui::update_context &ctx) {
  return custom_widget->measure_height(ctx);
}
