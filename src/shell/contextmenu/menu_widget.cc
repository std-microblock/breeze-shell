#include "menu_widget.h"
#include "GLFW/glfw3.h"
#include "breeze_ui/animator.h"
#include "breeze_ui/hbitmap_utils.h"
#include "breeze_ui/nanovg_wrapper.h"
#include "breeze_ui/ui.h"
#include "breeze_ui/widget.h"
#include "contextmenu.h"
#include "menu_render.h"
#include "nanovg.h"
#include "shell/config.h"
#include "shell/utils.h"
#include <algorithm>
#include <fmt/format.h>
#include <iostream>
#include <ranges>
#include <spdlog/spdlog.h>
#include <vector>

#include "shell/logger.h"

namespace {
const mb_shell::config::context_menu::theme::animation::bg &
get_menu_bg_animation(const mb_shell::menu_widget *menu) {
    return menu->is_top_level_menu
               ? mb_shell::config::current->context_menu.theme.animation.main_bg
               : mb_shell::config::current->context_menu.theme.animation
                     .submenu_bg;
}

mb_shell::menu_animation_rect make_collapsed_rect(
    float target_x, float target_y, float target_width, float target_height,
    const mb_shell::config::context_menu::theme::animation::bg &anim,
    mb_shell::popup_direction direction =
        mb_shell::popup_direction::bottom_right) {
    if (target_width <= 0 || target_height <= 0) {
        return {.x = target_x,
                .y = target_y,
                .width = target_width,
                .height = target_height};
    }

    auto width_scale = std::clamp(anim.appear_w_scale, 0.f, 1.f);
    auto height_scale = std::clamp(anim.appear_h_scale, 0.f, 1.f);
    auto start_width = std::max(1.f, target_width * width_scale);
    auto start_height = std::max(1.f, target_height * height_scale);
    const auto start_x =
        direction == mb_shell::popup_direction::top_left ||
                direction == mb_shell::popup_direction::bottom_left
            ? target_x + (target_width - start_width)
            : target_x;
    const auto start_y =
        direction == mb_shell::popup_direction::top_left ||
                direction == mb_shell::popup_direction::top_right
            ? target_y + (target_height - start_height)
            : target_y;

    return {.x = start_x,
            .y = start_y,
            .width = start_width,
            .height = start_height};
}

mb_shell::menu_animation_rect
make_bg_target_rect(const mb_shell::menu_widget *menu, float target_x,
                    float target_y, float target_width, float target_height) {
    return {.x = target_x,
            .y = target_y - menu->bg_padding_vertical,
            .width = target_width,
            .height = target_height + menu->bg_padding_vertical * 2};
}

mb_shell::popup_direction get_parent_menu_direction(
    const mb_shell::menu_item_widget *item) {
    if (auto menu =
            const_cast<mb_shell::menu_item_widget *>(item)
                ->search_parent<mb_shell::menu_widget>()) {
        return menu->direction;
    }
    return mb_shell::popup_direction::bottom_right;
}

float get_item_appear_offset_x(const mb_shell::menu_item_widget *item) {
    const auto direction = get_parent_menu_direction(item);
    return direction == mb_shell::popup_direction::top_left ||
                   direction == mb_shell::popup_direction::bottom_left
               ? 20.0f
               : -20.0f;
}

} // namespace
/*
| padding | icon_padding | icon | icon_padding | text_padding | text |
text_padding | hotkey_padding | hotkey | hotkey_padding | right_icon_padding |
right_icon | right_icon_padding |
*/
void mb_shell::menu_item_normal_widget::render(ui::nanovg_context ctx) {
    super::render(ctx);

    auto icon_width = config::current->context_menu.theme.font_size + 2;
    auto has_icon = has_icon_padding || icon_img;
    auto c = mb_shell::is_light_mode() ? 0 : 1;

    if (item.type == menu_item::type::spacer) {
        ctx.fillColor(nvgRGBAf(c, c, c, 0.1 * *opacity / 255.f));
        ctx.fillRect(x->dest(), *y, *width, *height);
        return;
    }

    // Draw background
    ctx.fillColor(nvgRGBAf(c, c, c, *bg_opacity / 255.f));
    float roundcorner = std::min(
        height->dest() / 2, config::current->context_menu.theme.item_radius);
    ctx.fillRoundedRect(*x + margin, *y, *width - margin * 2, *height,
                        roundcorner);

    // Draw focused border
    if (focused()) {
        ctx.strokeColor(
            nvgRGBAf(c, c, c, *opacity / 255.f * 0.5)); // Half opacity
        constexpr auto border_width = 1.0f;
        ctx.strokeWidth(border_width);
        ctx.strokeRoundedRect(*x + margin + border_width / 2,
                              *y + border_width / 2,
                              *width - margin * 2 - border_width,
                              *height - border_width, roundcorner);
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
        auto text_x = *x + padding +
                      (has_icon ? (icon_width + icon_padding * 2) : 0) +
                      text_padding + margin;
        auto text_y = *y + *height / 2;
        ctx.fontBlur(*text_blur);
        ctx.text(round(text_x), round(text_y), item.name->c_str(), nullptr);
    }

    // Calculate right side positions
    auto right_x = *x + width->dest() - margin - padding;

    // Draw right icon (submenu indicator)
    if (item.submenu) {
        if (!icon_unfold_img) {
            auto icon_unfold = fmt::format(
                R"#(<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" viewBox="0 0 12 12"><path opacity="0.7" fill="{}" d="M4.646 2.146a.5.5 0 0 0 0 .708L7.793 6L4.646 9.146a.5.5 0 1 0 .708.708l3.5-3.5a.5.5 0 0 0 0-.708l-3.5-3.5a.5.5 0 0 0-.708 0"/></svg>)#",
                c ? "white" : "black");
            ui::nanovg_context::NSVGimageRAII icon_unfold_img_svg(
                nsvgParse(icon_unfold.data(), "px", 96));
            this->icon_unfold_img =
                ctx.imageFromSVG(icon_unfold_img_svg.image, ctx.rt->dpi_scale);
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
        auto hotkey_y = *y + *height / 2;
        ctx.fontBlur(*text_blur);
        ctx.text(round(hotkey_x), round(hotkey_y), item.hotkey->c_str(),
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
        width +=
            ctx.vg.measureText(item.name->c_str()).first + text_padding * 2;

    // Hotkey
    if (item.hotkey && !item.hotkey->empty()) {
        auto t = ctx.vg.transaction();
        ctx.vg.fontSize(font_size * 0.9);
        auto hotkey_padding =
            config::current->context_menu.theme.hotkey_padding;
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
                spdlog::error("Error in menu item action: {}", e.what());
            }
        }
    }

    if (item.submenu) {
        float show_submenu_timer_before = show_submenu_timer;
        if (ctx.hovered(this)) {
            show_submenu_timer =
                std::min(show_submenu_timer + ctx.delta_time, 300.f);
        } else if (ctx.within(parent).hovered(parent)) {
            show_submenu_timer =
                std::max(show_submenu_timer - ctx.delta_time, 0.f);
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

mb_shell::menu_widget::menu_widget(bool is_main) : super() {
    gap = config::current->context_menu.theme.item_gap;
    width->set_easing(ui::easing_type::mutation);
    height->set_easing(ui::easing_type::mutation);
    config::current->context_menu.theme.animation.main.y(y);
    is_top_level_menu = is_main;
    bg = std::make_shared<background_widget>(is_main);
    enable_scrolling = true;
    crop_overflow = false;
    int c = mb_shell::is_light_mode() ? 0 : 1;
    scroll_bar_color = nvgRGBAf(c, c, c, 0.3);
    scroll_bar_width = config::current->context_menu.theme.scrollbar_width;
    scroll_bar_radius = config::current->context_menu.theme.scrollbar_radius;
}

void mb_shell::menu_widget::arm_background_animation(
    std::optional<menu_animation_rect> initial_rect) {
    if (bg_animation_armed) {
        return;
    }

    bg_animation_armed = true;
    bg_start_rect = initial_rect;
}

void mb_shell::menu_widget::update(ui::update_context &ctx) {
    if (dying_time) {
        if (dying_time.changed() && is_top_level_menu) {
            y->animate_to(*y - 10);
        }
        if (dying_time.changed()) {
            int c = mb_shell::is_light_mode() ? 0 : 1;
            scroll_bar_color =
                nvgRGBAf(c, c, c, std::min(0.3 * dying_time.time / 200.f, 0.3));
        }
        if (bg)
            bg->opacity->animate_to(0);
    }

    reverse = (direction == popup_direction::top_left ||
               direction == popup_direction::top_right) &&
              config::current->context_menu.reverse_if_open_to_up;

    auto forkctx_1 = ctx.with_offset(*x, *y);
    update_children(forkctx_1, rendering_submenus);

    ui::flex_widget::update(ctx);

    for (auto &item : children) {
        item->width->reset_to(*width);
    }

    if (bg) {
        auto target_rect = make_bg_target_rect(this, x->dest(), y->dest(),
                                               width->dest(), height->dest());

        if (bg_animation_armed && !bg_appear_initialized &&
            target_rect.width > 0 && target_rect.height > 0) {
            auto start_rect = bg_start_rect.value_or(
                is_top_level_menu
                    ? make_collapsed_rect(target_rect.x, target_rect.y,
                                          target_rect.width, target_rect.height,
                                          get_menu_bg_animation(this), direction)
                    : target_rect);
            bg->x->reset_to(start_rect.x);
            bg->y->reset_to(start_rect.y);
            bg->width->reset_to(start_rect.width);
            bg->height->reset_to(start_rect.height);
            bg_appear_initialized = true;
        }

        if (bg_animation_armed) {
            bg->x->animate_to(target_rect.x);
            bg->y->animate_to(target_rect.y);
            bg->width->animate_to(target_rect.width);
            bg->height->animate_to(target_rect.height);
        } else {
            bg->x->reset_to(target_rect.x);
            bg->y->reset_to(target_rect.y);
            bg->width->reset_to(target_rect.width);
            bg->height->reset_to(target_rect.height);
        }
        bg->update(ctx);
        ctx.hovered_hit(bg.get(), true);
    }

    // process keyboard actions
    // 1. check if focused
    //    we should handle keyboard actions only if this menu is focused or
    //    nothing is focused and we don't have any submenus
    bool should_handle_keyboard =
        owner_rt &&
        (focused() ||
         std::ranges::any_of(
             children, [](const auto &item) { return item->focus_within(); }) ||
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
                        (*focused_item)
                            ->template downcast<menu_item_normal_widget>()) {
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
            move_key(false, children);
            ctx.need_repaint = true;
        } else if (ctx.key_pressed(GLFW_KEY_DOWN)) {
            move_key(true, children);
            ctx.need_repaint = true;
        } else if (ctx.key_pressed(GLFW_KEY_LEFT)) {
            ctx.stop_key_propagation(GLFW_KEY_LEFT);

            if (parent_item_widget) {
                parent_item_widget->lock()->set_focus();
            } else if (parent_menu) {
                parent_menu->set_focus();
                parent_menu->current_submenu = nullptr;
                close();
            } else {
                close();
            }

            ctx.need_repaint = true;
        } else if (ctx.key_pressed(GLFW_KEY_RIGHT)) {
            auto focused_item = std::ranges::find_if(
                children, [](const auto &item) { return item->focused(); });
            if (focused_item != children.end()) {
                if (auto wid =
                        (*focused_item)->downcast<menu_item_normal_widget>())
                    if (wid->item.submenu) {
                        if (!wid->submenu_wid)
                            wid->show_submenu(ctx);
                        if (!current_submenu->focus_within()) {
                            if (auto child = current_submenu->get_child<
                                             menu_item_normal_widget>()) {
                                child->set_focus();
                            } else {
                                current_submenu->set_focus();
                            }
                        }
                    }
            }
            ctx.need_repaint = true;
        } else if (ctx.key_pressed(GLFW_KEY_ENTER) ||
                   ctx.key_pressed(GLFW_KEY_SPACE)) { // Enter or Space key
            auto focused_item = std::ranges::find_if(
                children, [](const auto &item) { return item->focused(); });
            if (focused_item != children.end()) {
                if (auto wid =
                        (*focused_item)->downcast<menu_item_normal_widget>()) {
                    if (wid->item.action) {
                        try {
                            wid->item.action.value()();
                        } catch (std::exception &e) {
                            spdlog::error("Error in menu item action: {}", e.what());
                        }
                    } else if (wid->item.submenu) {
                        wid->show_submenu(ctx);
                    }
                }
            }
            ctx.need_repaint = true;
        } else {
            auto menus_matching_key =
                children | std::views::filter([&](const auto &item) {
                    if (auto wid = item->template downcast<
                                   menu_item_normal_widget>()) {
                        if (!wid->item.hotkey)
                            return false;
                        auto hotkey =
                            *wid->item.hotkey | std::views::split('+') |
                            std::views::transform([](const auto &c) {
                                auto s = std::string(c.begin(), c.end());
                                // trim whitespace
                                s.erase(s.find_last_not_of(" \t\n\r") + 1);
                                s.erase(0, s.find_first_not_of(" \t\n\r"));
                                return s;
                            });

                        static auto translate_map =
                            std::unordered_map<std::string, int>{
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
                                    std::string(key) |
                                    std::views::transform(::tolower) |
                                    std::ranges::to<std::string>());
                                it != translate_map.end()) {
                                key_combination.push_back(it->second);
                            } else {
                                // If the key is not found, we can ignore it
                                return false;
                            }
                        }

                        return std::ranges::all_of(
                            key_combination,
                            [&](int key) { return ctx.key_pressed(key); });
                    }
                    return false;
                }) |
                std::ranges::to<std::vector>();

            if (menus_matching_key.size() > 0)
                ctx.need_repaint = true;

            if (menus_matching_key.size() == 1) {
                auto wid = menus_matching_key.front()
                               ->downcast<mb_shell::menu_item_normal_widget>();
                if (wid && wid->item.action) {
                    try {
                        wid->item.action.value()();
                    } catch (std::exception &e) {
                        spdlog::error("Error in menu item action: {}", e.what());
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
    if (bg) {
        bg->render(ctx);
    }

    {
        auto scope = ctx.transaction();
        auto has_clip = false;

        if (bg) {
            float clip_x = *bg->x;
            auto clip_y = *bg->y + bg_padding_vertical;
            float clip_width = *bg->width;
            auto clip_height =
                std::max(0.f, *bg->height - bg_padding_vertical * 2);

            ctx.scissor(clip_x, clip_y, std::max(0.f, clip_width), clip_height);
            has_clip = true;
        }

        if (crop_overflow || enable_scrolling) {
            if (has_clip) {
                ctx.intersectScissor(*x, *y, *width, *height);
            } else {
                ctx.scissor(*x, *y, *width, *height);
            }
        }

        ui::widget::render(ctx.with_offset(0, *scroll_top));

        if (enable_scrolling && actual_height > height->dest()) {
            auto scrollbar_height =
                height->dest() * height->dest() / actual_height;
            auto scrollbar_x = width->dest() - scroll_bar_width - 2 + *x;
            auto scrollbar_y = *y - *scroll_top /
                                        (actual_height - height->dest()) *
                                        (height->dest() - scrollbar_height);

            ctx.fillColor(scroll_bar_color);
            ctx.fillRoundedRect(scrollbar_x, scrollbar_y, scroll_bar_width,
                                scrollbar_height, scroll_bar_radius);
        }
    }

    auto ctx2 = ctx.with_offset(*x, *y);
    render_children(ctx2, rendering_submenus);
}
void mb_shell::menu_item_normal_widget::reset_appear_animation(float delay) {
    this->opacity->after_animate = [this](float dest) {
        this->opacity->set_delay(0);
    };
    opacity->reset_to(0);
    this->x->reset_to(get_item_appear_offset_x(this));
    text_blur->reset_to(
        config::current->context_menu.theme.animation.item.appear_blur);

    config::current->context_menu.theme.animation.item.opacity(opacity, delay);
    config::current->context_menu.theme.animation.item.x(x, delay);
    config::current->context_menu.theme.animation.item.width(width);
    config::current->context_menu.theme.animation.item.blur(text_blur, delay);

    opacity->animate_to(255);
    this->y->progress = 1;
    this->y->easing = ui::easing_type::mutation;

    this->x->animate_to(0);
    text_blur->animate_to(0.f);
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
    : widget(), anchor_x(x), anchor_y(y),
      ignore_outside_click_until_mouse_release(true) {
    menu_wid = std::make_shared<menu_widget>(true);
    menu_wid->init_from_data(menu_data);

    emplace_child<screenside_button_group_widget>();
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

    // Override mouse state with GetAsyncKeyState for reliable detection
    // in WS_EX_NOACTIVATE windows where GLFW may miss mouse messages
    bool lmb_down = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    bool rmb_down = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
    ctx.mouse_down = lmb_down;
    ctx.right_mouse_down = rmb_down;
    ctx.mouse_clicked = lmb_down && !prev_lmb_down;
    ctx.right_mouse_clicked = rmb_down && !prev_rmb_down;
    ctx.mouse_up = !lmb_down && prev_lmb_down;
    prev_lmb_down = lmb_down;
    prev_rmb_down = rmb_down;

    menu_wid->update(ctx);

    auto using_touchscreen = !IsCursorVisible();
    auto has_pressed_mouse_button = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) ||
                                    (GetAsyncKeyState(VK_RBUTTON) & 0x8000);

    if (ignore_outside_click_until_mouse_release && !has_pressed_mouse_button) {
        ignore_outside_click_until_mouse_release = false;
    }

    if (ctx.hovered_widgets->empty()) {
        glfwSetWindowAttrib(ctx.rt.window, GLFW_MOUSE_PASSTHROUGH,
                            using_touchscreen ? GLFW_FALSE : GLFW_TRUE);

        if (!ignore_outside_click_until_mouse_release &&
            ((ctx.mouse_clicked || ctx.right_mouse_clicked) ||
             has_pressed_mouse_button)) {
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
    auto children = this->children | std::ranges::views::transform([](auto &w) {
                        return std::dynamic_pointer_cast<menu_item_widget>(w);
                    });

    // the show duration for the menu should be within 200ms
    float delay = std::min(200.f / children.size(), 30.f);

    auto should_reverse = config::current->context_menu.reverse_if_open_to_up ? false : reverse;

    for (size_t i = 0; i < children.size(); i++) {
        auto child = children[i];
        child->reset_appear_animation(delay *
                                      (should_reverse ? children.size() - i : i));
    }
}
std::pair<float, float> mb_shell::mouse_menu_widget_main::calculate_position(
    menu_widget *menu_wid, ui::update_context &ctx, float anchor_x,
    float anchor_y, popup_direction direction) {

    menu_wid->update(ctx);
    auto menu_width = menu_wid->width->dest();
    auto menu_height = menu_wid->height->dest();

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
        y = ctx.screen.height - menu_height * ctx.rt.dpi_scale -
            padding_horizontal;
    }

    menu_wid->max_height =
        (ctx.screen.height - y - padding_horizontal) / ctx.rt.dpi_scale;

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

    spdlog::info("Calibrated position: {} {} in screen {} {}", x, y,
                 ctx.screen.width, ctx.screen.height);

    if (animated) {
        this->menu_wid->x->animate_to(x / ctx.rt.dpi_scale);
        this->menu_wid->y->animate_to(y / ctx.rt.dpi_scale);
    } else {
        this->menu_wid->x->reset_to(x / ctx.rt.dpi_scale);
        this->menu_wid->y->reset_to(y / ctx.rt.dpi_scale);
    }

    this->menu_wid->arm_background_animation();
}

void mb_shell::mouse_menu_widget_main::calibrate_direction(
    ui::update_context &ctx) {
    menu_wid->update(ctx);
    direction = calculate_direction(menu_wid.get(), ctx, anchor_x, anchor_y);
    menu_wid->direction = direction;
    menu_wid->reset_animation(direction == popup_direction::top_left ||
                              direction == popup_direction::top_right);

    spdlog::info("Calibrated direction: {}",
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
    text_blur->reset_to(0.f);
    this->item = item;
}

void mb_shell::menu_widget::init_from_data(menu menu_data) {
    is_top_level_menu = menu_data.is_top_level;
    auto init_items = menu_data.items;

    for (size_t i = 0; i < init_items.size(); i++) {
        auto &item = init_items[i];
        if (item.owner_draw) {
            auto mi = std::make_shared<menu_item_ownerdraw_widget>(item);
            children.push_back(mi);
        } else {
            auto mi = std::make_shared<menu_item_normal_widget>(item);
            children.push_back(mi);
        }
    }

    spdlog::info("Menu widget init from data: {}", menu_data.items.size());

    update_icon_width();
    this->menu_data = menu_data;
}
void mb_shell::menu_widget::update_icon_width() {
    bool has_icon = std::ranges::any_of(children, [](auto &item) {
        if (!item->template downcast<menu_item_normal_widget>())
            return false;
        auto i = item->template downcast<menu_item_normal_widget>()->item;
        return i.icon_bitmap.has_value() || i.icon_svg.has_value();
    });

    bool has_submenu = std::ranges::any_of(children, [](auto &item) {
        if (!item->template downcast<menu_item_normal_widget>())
            return false;
        auto i = item->template downcast<menu_item_normal_widget>()->item;
        return i.submenu.has_value();
    });

    for (auto &item : children) {
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
        ui::nanovg_context::NSVGimageRAII svg(
            nsvgParse(copy.data(), "px", 96));
        icon_img = ctx.imageFromSVG(svg.image, ctx.rt->dpi_scale);
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

        for (auto &item : children) {
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
    x->reset_to(get_item_appear_offset_x(this));
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
    submenu_wid = std::make_shared<menu_widget>(false);
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

    auto owner_menu = search_parent<menu_widget>();
    if (owner_menu)
        y += *owner_menu->scroll_top * ctx.rt.dpi_scale;

    x -= ctx.offset_x * ctx.rt.dpi_scale;
    y -= ctx.offset_y * ctx.rt.dpi_scale;

    submenu_wid->direction = direction;
    submenu_wid->parent_item_widget = weak_from_this();
    auto parent_menu = parent->downcast<menu_widget>();
    if (!parent_menu)
        parent_menu = parent->parent->downcast<menu_widget>();

    auto target_x = x / ctx.rt.dpi_scale;
    auto target_y = y / ctx.rt.dpi_scale;

    config::current->context_menu.theme.animation.submenu_bg.x(submenu_wid->x,
                                                               0);
    config::current->context_menu.theme.animation.submenu_bg.y(submenu_wid->y,
                                                               0);

    auto target_bg = make_bg_target_rect(submenu_wid.get(), target_x, target_y,
                                         submenu_wid->width->dest(),
                                         submenu_wid->height->dest());
    auto start_bg = make_collapsed_rect(
        target_bg.x, target_bg.y, target_bg.width, target_bg.height,
        config::current->context_menu.theme.animation.submenu_bg, direction);
    submenu_wid->x->reset_to(target_x);
    submenu_wid->y->reset_to(target_y);

    submenu_wid->arm_background_animation(start_bg);
    submenu_wid->reset_animation(direction == popup_direction::top_left ||
                                 direction == popup_direction::top_right);
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
mb_shell::screenside_button_group_widget::screenside_button_group_widget()
    : super() {
    padding_left->reset_to(8);
    padding_right->reset_to(8);
    padding_top->reset_to(8);
    padding_bottom->reset_to(8);

    gap = 4;
    horizontal = true;
}
mb_shell::screenside_button_group_widget::button_widget::button_widget(
    std::string icon_svg)
    : icon_svg(std::move(icon_svg)) {
    bg_opacity->reset_to(0);

    width->reset_to(30);
    height->reset_to(30);
    config::current->context_menu.theme.animation.item.opacity(bg_opacity, 0);
}
void mb_shell::screenside_button_group_widget::button_widget::update(
    ui::update_context &ctx) {
    super::update(ctx);
    if (ctx.mouse_down_on(this)) {
        bg_opacity->animate_to(0.6f);
    } else if (ctx.hovered(this)) {
        bg_opacity->animate_to(0.8f);
    } else {
        bg_opacity->animate_to(1);
    }

    if (ctx.mouse_clicked_on(this)) {
        if (on_click) {
            on_click();
        }
    }

    ctx.hovered_hit(this);
}
void mb_shell::screenside_button_group_widget::button_widget::render(
    ui::nanovg_context ctx) {
    super::render(ctx);

    float icon_size = std::min(*width, *height) - 8.f;
    if (!icon) {
        ui::nanovg_context::NSVGimageRAII svg(
            nsvgParse(icon_svg.data(), "px", 96));
        icon = ctx.imageFromSVG(svg.image);
    }

    if (icon) {
        float c = is_light_mode() ? 1 : 0;
        ctx.fillColor(nvgRGBAf(c, c, c, *bg_opacity));
        ctx.fillCircle(*x + *width / 2, *y + *height / 2, icon_size / 2 + 4);
        ctx.drawImage(*icon, *x + (*width - icon_size) / 2,
                      *y + (*height - icon_size) / 2, icon_size, icon_size);
    }
}
