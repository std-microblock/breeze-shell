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
#include <cctype>
#include <fmt/format.h>
#include <ranges>
#include <spdlog/spdlog.h>
#include <unordered_map>
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
    const mb_shell::menu_animation_rect &target,
    const mb_shell::config::context_menu::theme::animation::bg &anim,
    mb_shell::popup_direction direction =
        mb_shell::popup_direction::bottom_right) {
    if (target.width <= 0 || target.height <= 0) {
        return target;
    }

    auto width_scale = std::clamp(anim.appear_w_scale, 0.f, 1.f);
    auto height_scale = std::clamp(anim.appear_h_scale, 0.f, 1.f);
    auto start_width = std::max(1.f, target.width * width_scale);
    auto start_height = std::max(1.f, target.height * height_scale);
    const bool from_right =
        direction == mb_shell::popup_direction::top_left ||
        direction == mb_shell::popup_direction::bottom_left;
    const bool from_bottom =
        direction == mb_shell::popup_direction::top_left ||
        direction == mb_shell::popup_direction::top_right;

    return {.x = from_right ? target.x + (target.width - start_width)
                            : target.x,
            .y = from_bottom ? target.y + (target.height - start_height)
                             : target.y,
            .width = start_width,
            .height = start_height};
}

mb_shell::menu_animation_rect
make_bg_target_rect(const mb_shell::menu_widget *menu) {
    return {.x = 0,
            .y = -menu->bg_padding_vertical,
            .width = menu->width->dest(),
            .height = menu->height->dest() + menu->bg_padding_vertical * 2};
}

bool is_upward(mb_shell::popup_direction direction) {
    return direction == mb_shell::popup_direction::top_left ||
           direction == mb_shell::popup_direction::top_right;
}

float get_item_appear_offset_x(const mb_shell::menu_item_widget *item) {
    auto menu = item->owner_menu();
    const auto direction =
        menu ? menu->direction : mb_shell::popup_direction::bottom_right;
    return direction == mb_shell::popup_direction::top_left ||
                   direction == mb_shell::popup_direction::bottom_left
               ? 20.0f
               : -20.0f;
}

void run_item_action(mb_shell::menu_item &item) {
    if (!item.action)
        return;
    try {
        item.action.value()();
    } catch (std::exception &e) {
        spdlog::error("Error in menu item action: {}", e.what());
    }
}

bool hotkey_matches(const std::string &hotkey, const ui::key_event &e) {
    static const auto translate_map = [] {
        std::unordered_map<std::string, int> map{
            {"ctrl", -GLFW_MOD_CONTROL},
            {"shift", -GLFW_MOD_SHIFT},
            {"alt", -GLFW_MOD_ALT},
            {"win", -GLFW_MOD_SUPER},
        };
        for (char c = 'a'; c <= 'z'; ++c)
            map[std::string(1, c)] = GLFW_KEY_A + (c - 'a');
        for (char c = '0'; c <= '9'; ++c)
            map[std::string(1, c)] = GLFW_KEY_0 + (c - '0');
        return map;
    }();

    int required_mods = 0;
    bool key_matched = false;
    for (const auto part : hotkey | std::views::split('+')) {
        auto key = std::string(part.begin(), part.end());
        const auto first = key.find_first_not_of(" \t\n\r");
        if (first == std::string::npos)
            return false;
        key.erase(0, first);
        key.erase(key.find_last_not_of(" \t\n\r") + 1);
        std::ranges::transform(key, key.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        auto it = translate_map.find(key);
        if (it == translate_map.end())
            return false;
        if (it->second < 0)
            required_mods |= -it->second;
        else if (it->second == e.key)
            key_matched = true;
        else
            return false;
    }
    return key_matched && (e.mods & required_mods) == required_mods;
}
} // namespace

mb_shell::menu_item_widget::menu_item_widget() {}

mb_shell::menu_widget *mb_shell::menu_item_widget::owner_menu() const {
    return const_cast<menu_item_widget *>(this)->search_parent<menu_widget>();
}

void mb_shell::menu_item_widget::reset_appear_animation(float delay) {
    for (auto &child : get_children<menu_item_widget>())
        child->reset_appear_animation(delay);
}

mb_shell::menu_item_normal_widget::menu_item_normal_widget(menu_item item)
    : super() {
    opacity->reset_to(0);
    text_blur->reset_to(0.f);
    this->item = item;
}

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

    ctx.fillColor(nvgRGBAf(c, c, c, *bg_opacity / 255.f));
    float roundcorner = std::min(
        height->dest() / 2, config::current->context_menu.theme.item_radius);
    ctx.fillRoundedRect(*x + margin, *y, *width - margin * 2, *height,
                        roundcorner);

    if (focused()) {
        ctx.strokeColor(nvgRGBAf(c, c, c, *opacity / 255.f * 0.5));
        constexpr auto border_width = 1.0f;
        ctx.strokeWidth(border_width);
        ctx.strokeRoundedRect(*x + margin + border_width / 2,
                              *y + border_width / 2,
                              *width - margin * 2 - border_width,
                              *height - border_width, roundcorner);
    }

    if (item.icon_bitmap.has_value() || item.icon_svg.has_value()) {
        if (!icon_img || item.icon_updated)
            reload_icon_img(ctx);
        item.icon_updated = false;

        if (icon_img && icon_img->id >= 0) {
            auto paintY = floor(*y + (*height - icon_width) / 2);
            auto imageX = *x + padding + margin + icon_padding;
            auto paint = ctx.imagePattern(imageX, paintY, icon_width,
                                          icon_width, 0, icon_img->id,
                                          *opacity / 255.f);

            ctx.beginPath();
            ctx.rect(imageX, paintY, icon_width, icon_width);
            ctx.fillPaint(paint);
            ctx.fill();
        }
    }

    ctx.fillColor(nvgRGBAf(c, c, c, *opacity / 255.f));
    ctx.fontFaceId(ui::resolve_font(ctx.ctx, "main"));
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

    auto right_x = *x + width->dest() - margin - padding;

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
        right_x -= icon_width;
    }

    if (item.hotkey && !item.hotkey->empty()) {
        auto t = ctx.transaction();
        ctx.fillColor(nvgRGBAf(c, c, c, *opacity / 255.f * 0.7));
        ctx.fontSize(font_size * 0.9);
        ctx.textAlign(NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
        ctx.fontFaceId(ui::resolve_font(ctx.ctx, "monospace"));
        auto hotkey_x = right_x - hotkey_padding;
        auto hotkey_y = *y + *height / 2;
        ctx.fontBlur(*text_blur);
        ctx.text(round(hotkey_x), round(hotkey_y), item.hotkey->c_str(),
                 nullptr);
    }
}

void mb_shell::menu_item_normal_widget::before_layout() {
    super::before_layout();
    auto key = fmt::format(
        "{}|{}|{}|{}|{}|{}|{}", static_cast<int>(item.type),
        item.name.value_or(""), item.hotkey.value_or(""),
        has_icon_padding || icon_img.has_value(), has_submenu_padding,
        config::current->context_menu.theme.font_size,
        config::current->context_menu.theme.item_height);
    if (key != measure_key) {
        measure_key = std::move(key);
        invalidate_measure();
    }
}

YGSize mb_shell::menu_item_normal_widget::measure(float, YGMeasureMode, float,
                                                  YGMeasureMode) {
    if (item.type == menu_item::type::spacer) {
        return {1, 1};
    }

    const auto item_height = config::current->context_menu.theme.item_height;
    ui::text_measure_scope scope(*this);
    if (!scope) {
        return {0, item_height};
    }
    auto &vg = scope.vg;
    auto font_size = config::current->context_menu.theme.font_size;
    float width = padding;

    if (has_icon_padding || icon_img)
        width += icon_padding * 2 + font_size + 2;

    vg.fontFaceId(ui::resolve_font(vg.ctx, "main"));
    vg.fontSize(font_size);
    if (item.name)
        width += vg.measureText(item.name->c_str()).first + text_padding * 2;

    if (item.hotkey && !item.hotkey->empty()) {
        vg.fontSize(font_size * 0.9);
        vg.fontFaceId(ui::resolve_font(vg.ctx, "monospace"));
        width += vg.measureText(item.hotkey->c_str()).first +
                 config::current->context_menu.theme.hotkey_padding * 2;
    }

    if (has_submenu_padding) {
        width += font_size + 2 + right_icon_padding;
    }

    width += padding;
    return {width + margin * 2, item_height};
}

void mb_shell::menu_item_normal_widget::tick(float delta_time) {
    auto menu = owner_menu();
    if (menu && menu->dying_time) {
        bg_opacity->animate_to(0);
        opacity->animate_to(0);
        return;
    }

    if (item.disabled) {
        opacity->animate_to(128);
        bg_opacity->animate_to(0);
        if (submenu_wid) {
            submenu_wid->close();
            submenu_wid = nullptr;
        }
        return;
    }
    opacity->animate_to(255);

    if (pressed()) {
        bg_opacity->animate_to(40);
    } else if (hovered()) {
        bg_opacity->animate_to(20);
    } else {
        bg_opacity->animate_to(0);
    }

    if (item.submenu) {
        float before = show_submenu_timer;
        const float step = std::min(delta_time, 50.f);
        if (hovered()) {
            show_submenu_timer = std::min(show_submenu_timer + step, 300.f);
        } else if (menu && menu->directly_hovered()) {
            show_submenu_timer = std::max(show_submenu_timer - step, 0.f);
        }

        if (before != show_submenu_timer) {
            if (show_submenu_timer >= 150.f) {
                show_submenu();
            } else {
                hide_submenu();
            }
            if (owner_rt && show_submenu_timer > 0.f &&
                show_submenu_timer < 300.f) {
                owner_rt->schedule_frame(16);
            }
        }
    } else {
        hide_submenu();
    }

    if (submenu_wid && submenu_wid->dying_time.has_value) {
        submenu_wid = nullptr;
    }
}

void mb_shell::menu_item_normal_widget::activate() { run_item_action(item); }

void mb_shell::menu_item_normal_widget::handle_mouse_down(ui::mouse_event &e) {
    if (e.button != ui::mouse_button::left)
        return;
    e.handled = true;
    if (item.disabled || item.type == menu_item::type::spacer)
        return;
    auto menu = owner_menu();
    if (menu && menu->dying_time)
        return;
    activate();
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

void mb_shell::menu_item_normal_widget::reload_icon_img(
    ui::nanovg_context ctx) {
    if (item.icon_bitmap)
        icon_img = ui::LoadBitmapImage(ctx, (HBITMAP)item.icon_bitmap.value());
    else if (item.icon_svg) {
        std::string copy = item.icon_svg.value();
        ui::nanovg_context::NSVGimageRAII svg(nsvgParse(copy.data(), "px", 96));
        icon_img = ctx.imageFromSVG(svg.image, ctx.rt->dpi_scale);
    } else {
        icon_img = std::nullopt;
    }

    if (auto menu = owner_menu()) {
        menu->update_icon_width();
    }
}

void mb_shell::menu_item_normal_widget::hide_submenu() {
    if (submenu_wid != nullptr) {
        submenu_wid->close();
        submenu_wid = nullptr;
    }
}

void mb_shell::menu_item_normal_widget::show_submenu() {
    if (submenu_wid != nullptr || !item.submenu || !owner_rt)
        return;
    auto menu = owner_menu();
    if (!menu)
        return;
    auto &rt = *owner_rt;

    submenu_wid = std::make_shared<menu_widget>(false);
    item.submenu.value()(submenu_wid);
    submenu_wid->compute_layout_now(owner_rt);

    const float dpi = rt.dpi_scale;
    float anchor_x = (abs_x() + width->dest()) * dpi;
    float anchor_y = abs_y() * dpi;

    auto direction = mouse_menu_widget_main::calculate_direction(
        submenu_wid.get(), rt, anchor_x, anchor_y,
        popup_direction::bottom_right);

    if (direction == popup_direction::top_left ||
        direction == popup_direction::bottom_left) {
        anchor_x -= *width * dpi;
    }
    if (is_upward(direction)) {
        anchor_y += *height * dpi;
    }

    auto [x, y] = mouse_menu_widget_main::calculate_position(
        submenu_wid.get(), rt, anchor_x, anchor_y, direction);

    auto target_x = x / dpi - menu->abs_x();
    auto target_y = y / dpi - menu->abs_y();

    submenu_wid->direction = direction;
    submenu_wid->parent_item_widget = weak_from_this();

    config::current->context_menu.theme.animation.submenu_bg.x(submenu_wid->x,
                                                               0);
    config::current->context_menu.theme.animation.submenu_bg.y(submenu_wid->y,
                                                               0);
    submenu_wid->x->reset_to(target_x);
    submenu_wid->y->reset_to(target_y);

    submenu_wid->arm_background_animation(make_collapsed_rect(
        make_bg_target_rect(submenu_wid.get()),
        config::current->context_menu.theme.animation.submenu_bg, direction));
    submenu_wid->reset_animation(is_upward(direction));
    if (menu->current_submenu) {
        menu->current_submenu->close();
        menu->current_submenu = nullptr;
    }
    menu->current_submenu = submenu_wid;
    submenu_wid->parent_menu = menu;
    menu->add_submenu(submenu_wid);
}

void mb_shell::menu_item_parent_widget::before_layout() {
    super::before_layout();
    YGNodeStyleSetFlexDirection(node, YGFlexDirectionRow);
    YGNodeStyleSetAlignItems(node, YGAlignFlexStart);
    YGNodeStyleSetGap(node, YGGutterColumn,
                      config::current->context_menu.theme.multibutton_line_gap);
}

void mb_shell::menu_item_parent_widget::reset_appear_animation(float delay) {
    y->set_easing(ui::easing_type::mutation);
    x->reset_to(get_item_appear_offset_x(this));
    x->animate_to(0);
    opacity->reset_to(0);
    opacity->animate_to(255);
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

mb_shell::menu_item_custom_widget::menu_item_custom_widget(
    std::shared_ptr<ui::widget> custom_widget)
    : custom_widget(custom_widget) {
    if (custom_widget)
        add_child(custom_widget);
}

void mb_shell::menu_item_custom_widget::before_layout() {
    super::before_layout();
    YGNodeStyleSetFlexDirection(node, YGFlexDirectionColumn);
    YGNodeStyleSetAlignItems(node, YGAlignFlexStart);
}

mb_shell::menu_widget::menu_widget(bool is_main) : super() {
    gap = config::current->context_menu.theme.item_gap;
    align_items = align::stretch;
    width->set_easing(ui::easing_type::mutation);
    height->set_easing(ui::easing_type::mutation);
    config::current->context_menu.theme.animation.main.y(y);
    is_top_level_menu = is_main;
    bg = std::make_shared<background_widget>(is_main);
    add_floating(bg);
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
    request_repaint();
}

void mb_shell::menu_widget::add_submenu(std::shared_ptr<menu_widget> submenu) {
    add_floating(std::move(submenu));
}

std::vector<std::shared_ptr<mb_shell::menu_widget>>
mb_shell::menu_widget::submenus() const {
    std::vector<std::shared_ptr<menu_widget>> res;
    for (auto &w : floating) {
        if (auto m = std::dynamic_pointer_cast<menu_widget>(w))
            res.push_back(std::move(m));
    }
    return res;
}

bool mb_shell::menu_widget::directly_hovered() const {
    if (!owner_rt)
        return false;
    for (auto w = owner_rt->hovered_widget(); w; w = w->parent) {
        if (auto m = dynamic_cast<const menu_widget *>(w))
            return m == this;
    }
    return false;
}

void mb_shell::menu_widget::tick(float delta_time) {
    if (native_content_dirty) {
        native_content_dirty = false;
        resync_native_content();
    }

    if (dying_time) {
        if (!closing_seen) {
            closing_seen = true;
            if (is_top_level_menu)
                y->animate_to(y->dest() - 10);
            scroll_bar_color.a = 0;
        }
        if (bg)
            bg->opacity->animate_to(0);
    }
}

void mb_shell::menu_widget::before_layout() {
    reverse = is_upward(direction) &&
              config::current->context_menu.reverse_if_open_to_up;
    super::before_layout();
}

void mb_shell::menu_widget::after_layout() {
    super::after_layout();
    if (!bg)
        return;

    auto target = make_bg_target_rect(this);
    if (bg_animation_armed && !bg_appear_initialized && target.width > 0 &&
        target.height > 0) {
        auto start = bg_start_rect.value_or(
            is_top_level_menu
                ? make_collapsed_rect(target, get_menu_bg_animation(this),
                                      direction)
                : target);
        bg->x->reset_to(start.x);
        bg->y->reset_to(start.y);
        bg->width->reset_to(start.width);
        bg->height->reset_to(start.height);
        bg_appear_initialized = true;
    }

    if (bg_animation_armed) {
        bg->x->animate_to(target.x);
        bg->y->animate_to(target.y);
        bg->width->animate_to(target.width);
        bg->height->animate_to(target.height);
    } else {
        bg->x->reset_to(target.x);
        bg->y->reset_to(target.y);
        bg->width->reset_to(target.width);
        bg->height->reset_to(target.height);
    }
}

bool mb_shell::menu_widget::keyboard_owner() const {
    if (!owner_rt || dying_time)
        return false;
    auto self = const_cast<menu_widget *>(this);
    return self->focused() ||
           std::ranges::any_of(children,
                               [](const auto &item) {
                                   return item && item->focus_within();
                               }) ||
           (!owner_rt->focused_widget.has_value() && submenus().empty());
}

void mb_shell::menu_widget::handle_key(ui::key_event &e) {
    if (!keyboard_owner())
        return;

    auto move_key = [](bool next, auto &items) {
        if (items.empty()) {
            return;
        }

        auto focused_item = std::ranges::find_if(
            items, [](const auto &item) { return item->focused(); });
        auto index = focused_item == items.end()
                         ? (next ? items.size() - 1 : size_t{0})
                         : static_cast<size_t>(
                               std::distance(items.begin(), focused_item));

        for (size_t attempts = 0; attempts < items.size(); ++attempts) {
            index = next ? (index + 1) % items.size()
                         : (index + items.size() - 1) % items.size();
            auto wid =
                items[index]->template downcast<menu_item_normal_widget>();
            if (wid && wid->visible && !wid->item.disabled &&
                wid->item.type != mb_shell::menu_item::type::spacer) {
                items[index]->set_focus(true);
                return;
            }
        }
    };

    auto focused_item = [&]() -> std::shared_ptr<menu_item_normal_widget> {
        auto it = std::ranges::find_if(
            children, [](const auto &item) { return item->focused(); });
        return it == children.end()
                   ? nullptr
                   : (*it)->template downcast<menu_item_normal_widget>();
    };

    auto embedded_widget_focused = [&] {
        return std::ranges::any_of(children, [](const auto &item) {
            return !item->template downcast<menu_item_normal_widget>() &&
                   item->focus_within();
        });
    };

    auto first_visible_item = [&]() -> std::shared_ptr<menu_item_normal_widget> {
        for (auto &item : children) {
            auto wid = item->template downcast<menu_item_normal_widget>();
            if (wid && wid->visible && !wid->item.disabled &&
                wid->item.type != mb_shell::menu_item::type::spacer &&
                (wid->item.action || wid->item.submenu))
                return wid;
        }
        return nullptr;
    };

    switch (e.key) {
    case GLFW_KEY_UP:
        move_key(false, children);
        break;
    case GLFW_KEY_DOWN:
        move_key(true, children);
        break;
    case GLFW_KEY_LEFT:
        if (parent_item_widget) {
            if (auto item = parent_item_widget->lock())
                item->set_focus();
        } else if (parent_menu) {
            parent_menu->set_focus();
            parent_menu->current_submenu = nullptr;
        }
        close();
        break;
    case GLFW_KEY_RIGHT:
        if (auto wid = focused_item(); wid && wid->item.submenu) {
            if (!wid->submenu_wid)
                wid->show_submenu();
            if (current_submenu && !current_submenu->focus_within()) {
                if (auto child =
                        current_submenu->get_child<menu_item_normal_widget>())
                    child->set_focus();
                else
                    current_submenu->set_focus();
            }
        }
        break;
    case GLFW_KEY_ENTER:
    case GLFW_KEY_SPACE: {
        if (e.repeat)
            return;
        auto wid = focused_item();
        if (!wid && e.key == GLFW_KEY_ENTER && embedded_widget_focused())
            wid = first_visible_item();
        if (wid) {
            if (wid->item.action)
                wid->activate();
            else if (wid->item.submenu)
                wid->show_submenu();
        }
        break;
    }
    default: {
        if (e.repeat)
            return;
        auto matching =
            children | std::views::filter([&](const auto &item) {
                auto wid = item->template downcast<menu_item_normal_widget>();
                return wid && wid->visible && wid->item.hotkey &&
                       hotkey_matches(*wid->item.hotkey, e);
            }) |
            std::ranges::to<std::vector>();

        if (matching.empty())
            return;
        if (matching.size() == 1) {
            auto wid =
                matching.front()->downcast<mb_shell::menu_item_normal_widget>();
            if (wid && wid->item.action) {
                wid->activate();
            } else if (wid && wid->item.submenu && !wid->submenu_wid) {
                wid->show_submenu();
                if (current_submenu)
                    current_submenu->set_focus();
            }
        } else {
            move_key(!(e.mods & GLFW_MOD_SHIFT), matching);
        }
        break;
    }
    }
    e.handled = true;
    request_repaint();
}

bool mb_shell::menu_widget::hit_test(float px, float py) const {
    if (ui::widget::hit_test(px, py))
        return true;
    if (!bg)
        return false;
    const float lx = px - x->var(), ly = py - y->var();
    return lx >= bg->x->var() && lx <= bg->x->var() + bg->width->var() &&
           ly >= bg->y->var() && ly <= bg->y->var() + bg->height->var();
}

ui::widget *mb_shell::menu_widget::hit_test_tree(float px, float py) {
    if (!visible || dying_time)
        return nullptr;
    const float lx = px - x->var(), ly = py - y->var();
    auto subs = submenus();
    for (auto it = subs.rbegin(); it != subs.rend(); ++it) {
        if (auto hit = (*it)->hit_test_tree(lx, ly))
            return hit;
    }
    if (ui::widget::hit_test(px, py)) {
        const float cy = ly - child_offset_y();
        for (auto it = children.rbegin(); it != children.rend(); ++it) {
            if (*it)
                if (auto hit = (*it)->hit_test_tree(lx, cy))
                    return hit;
        }
    }
    return hit_test(px, py) ? this : nullptr;
}

void mb_shell::menu_widget::render(ui::nanovg_context ctx) {
    if (bg) {
        auto local = ctx.with_offset(*x, *y);
        auto t = local.transaction();
        bg->render(local);
    }

    {
        auto scope = ctx.transaction();
        if (bg) {
            ctx.scissor(*x + bg->x->var(),
                        *y + bg->y->var() + bg_padding_vertical,
                        std::max(0.f, bg->width->var()),
                        std::max(0.f, bg->height->var() -
                                          bg_padding_vertical * 2));
            ctx.intersectScissor(*x, *y, *width, *height);
        } else {
            ctx.scissor(*x, *y, *width, *height);
        }

        render_children(ctx.with_offset(*x, *y + *scroll_top), children);
        render_scrollbar(ctx);
    }

    std::vector<std::shared_ptr<ui::widget>> subs;
    for (auto &s : submenus())
        subs.push_back(s);
    render_children(ctx.with_offset(*x, *y), subs);
}

void mb_shell::menu_widget::reset_animation(bool reverse) {
    if (animate_appear_started)
        return;

    animate_appear_started = true;
    std::vector<std::shared_ptr<menu_item_widget>> items;
    for (auto &w : children) {
        if (auto item = std::dynamic_pointer_cast<menu_item_widget>(w))
            items.push_back(item);
    }
    if (items.empty())
        return;

    float delay = std::min(200.f / items.size(), 30.f);
    auto should_reverse =
        config::current->context_menu.reverse_if_open_to_up ? false : reverse;

    for (size_t i = 0; i < items.size(); i++) {
        items[i]->reset_appear_animation(
            delay * (should_reverse ? items.size() - i : i));
    }
}

void mb_shell::menu_widget::init_from_data(menu menu_data) {
    is_top_level_menu = menu_data.is_top_level;
    for (auto &item : menu_data.items) {
        if (item.owner_draw) {
            add_child(std::make_shared<menu_item_ownerdraw_widget>(item));
        } else {
            add_child(std::make_shared<menu_item_normal_widget>(item));
        }
    }

    spdlog::info("Menu widget init from data: {}", menu_data.items.size());

    update_icon_width();
    this->menu_data = menu_data;
}

namespace {
bool same_native_menu_layout(const std::vector<mb_shell::menu_item> &a,
                             const std::vector<mb_shell::menu_item> &b) {
    if (a.size() != b.size())
        return false;

    for (size_t i = 0; i < a.size(); i++) {
        if (a[i].type != b[i].type || a[i].name != b[i].name ||
            a[i].origin_name != b[i].origin_name || a[i].wID != b[i].wID ||
            a[i].submenu.has_value() != b[i].submenu.has_value())
            return false;
    }

    return true;
}
} // namespace

void mb_shell::menu_widget::resync_native_content() {
    if (is_top_level_menu || !menu_data.native_handle)
        return;

    auto hMenu = (HMENU)menu_data.native_handle;
    auto fresh = menu::construct_with_hmenu(
        hMenu, (HWND)menu_data.parent_window, false, {}, 0, false);

    if (same_native_menu_layout(fresh.items, menu_data.items))
        return;

    spdlog::info("Native submenu {} changed after being shown ({} -> {} items)",
                 (void *)hMenu, menu_data.items.size(), fresh.items.size());

    for (auto &child : children) {
        if (auto item = child->downcast<menu_item_normal_widget>())
            item->hide_submenu();
    }

    if (current_submenu) {
        current_submenu->close();
        current_submenu = nullptr;
    }
    std::erase_if(floating, [this](auto &w) { return w != bg; });
    children.clear();
    children_dirty = true;

    init_from_data(fresh);
    request_repaint();
}

void mb_shell::menu_widget::update_icon_width() {
    bool has_icon = false, has_submenu = false;
    for (auto &child : children) {
        if (auto mi = child->downcast<menu_item_normal_widget>()) {
            has_icon |= mi->item.icon_bitmap.has_value() ||
                        mi->item.icon_svg.has_value();
            has_submenu |= mi->item.submenu.has_value();
        }
    }

    for (auto &child : children) {
        if (auto mi = child->downcast<menu_item_normal_widget>()) {
            mi->has_icon_padding = has_icon;
            mi->has_submenu_padding = has_submenu;
        }
    }
    request_repaint();
}

void mb_shell::menu_widget::close() {
    if (menu_data.is_top_level) {
        auto current = menu_render::current;
        if (current) {
            (*current)->rt->hide_as_close();
        }
        return;
    }

    dying_time = 200;
    if (parent_menu && parent_menu->current_submenu.get() == this) {
        parent_menu->current_submenu = nullptr;
    }

    for (auto &item : children) {
        if (auto mi = item->downcast<menu_item_normal_widget>()) {
            mi->hide_submenu();
        }
    }
    request_repaint();
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
    hit_self = false;
    menu_wid = std::make_shared<menu_widget>(true);
    menu_wid->init_from_data(menu_data);

    emplace_child<screenside_button_group_widget>();
    add_child(menu_wid);
}

void mb_shell::mouse_menu_widget_main::tick(float delta_time) {
    PeekMessage(nullptr, nullptr, 0, 0, PM_REMOVE);
    if (!owner_rt)
        return;
    auto &rt = *owner_rt;

    if (!direction_calibrated) {
        calibrate_direction();
        direction_calibrated = true;
        calibrate_position(false);
        position_calibrated = true;
    }

    if (!position_calibrated) {
        calibrate_position();
        position_calibrated = true;
    }

    auto using_touchscreen = !IsCursorVisible();
    auto has_pressed_mouse_button = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) ||
                                    (GetAsyncKeyState(VK_RBUTTON) & 0x8000);

    if (ignore_outside_click_until_mouse_release && !has_pressed_mouse_button) {
        ignore_outside_click_until_mouse_release = false;
    }

    const bool nothing_hovered =
        !rt.root || rt.root->hit_test_tree(rt.mouse_x, rt.mouse_y) == nullptr;
    const bool passthrough = nothing_hovered && !using_touchscreen;
    if (last_passthrough != passthrough) {
        glfwSetWindowAttrib(rt.window, GLFW_MOUSE_PASSTHROUGH,
                            passthrough ? GLFW_TRUE : GLFW_FALSE);
        last_passthrough = passthrough;
    }

    if (nothing_hovered && !ignore_outside_click_until_mouse_release &&
        has_pressed_mouse_button) {
        rt.hide_as_close();
    }

    if ((GetAsyncKeyState(VK_ESCAPE) & 0x8000) ||
        (GetAsyncKeyState(VK_MENU) & 0x8000) ||
        (GetAsyncKeyState(VK_LMENU) & 0x8000)) {
        rt.hide_as_close();
    }
}

std::pair<float, float> mb_shell::mouse_menu_widget_main::calculate_position(
    menu_widget *menu_wid, ui::render_target &rt, float anchor_x,
    float anchor_y, popup_direction direction) {
    menu_wid->compute_layout_now(&rt);
    const float dpi = rt.dpi_scale;
    auto menu_width = menu_wid->width->dest() * dpi;
    auto menu_height = menu_wid->height->dest() * dpi;

    float x, y;
    constexpr auto mouse_padding = 1.f;
    if (direction == popup_direction::top_left) {
        x = anchor_x - menu_width - mouse_padding;
        y = anchor_y - menu_height;
    } else if (direction == popup_direction::top_right) {
        x = anchor_x + mouse_padding;
        y = anchor_y - menu_height;
    } else if (direction == popup_direction::bottom_left) {
        x = anchor_x - menu_width - mouse_padding;
        y = anchor_y;
    } else {
        x = anchor_x + mouse_padding;
        y = anchor_y;
    }

    auto padding_x =
        config::current->context_menu.position.padding_horizontal * dpi;
    auto padding_y =
        config::current->context_menu.position.padding_vertical * dpi;

    if (x < padding_x) {
        x = padding_x;
    } else if (x + menu_width > rt.screen.width - padding_x) {
        x = rt.screen.width - menu_width - padding_x;
    }

    if (menu_height > rt.screen.height - padding_y * 2 || y < padding_y) {
        y = padding_y;
    } else if (y + menu_height > rt.screen.height - padding_y) {
        y = rt.screen.height - menu_height - padding_y;
    }

    menu_wid->max_height = (rt.screen.height - y - padding_y) / dpi;
    return {x, y};
}

mb_shell::popup_direction mb_shell::mouse_menu_widget_main::calculate_direction(
    menu_widget *menu_wid, ui::render_target &rt, float anchor_x,
    float anchor_y, popup_direction prefer_direction) {
    menu_wid->compute_layout_now(&rt);
    const float dpi = rt.dpi_scale;
    auto menu_width = menu_wid->width->dest() * dpi;
    auto menu_height = menu_wid->height->dest() * dpi;

    auto padding_y = config::current->context_menu.position.padding_horizontal,
         padding_x = config::current->context_menu.position.padding_vertical;

    bool bottom_overflow = anchor_y + menu_height > rt.screen.height - padding_y;
    bool top_overflow = anchor_y - menu_height < padding_y;
    bool right_overflow = anchor_x + menu_width > rt.screen.width - padding_x;
    bool left_overflow = anchor_x - menu_width < padding_x;

    bool prefer_top = is_upward(prefer_direction);
    bool prefer_left = prefer_direction == popup_direction::top_left ||
                       prefer_direction == popup_direction::bottom_left;

    bool top_revert = prefer_top ? (top_overflow && !bottom_overflow)
                                 : (bottom_overflow && !top_overflow);
    bool left_revert = prefer_left ? (left_overflow && !right_overflow)
                                   : (right_overflow && !left_overflow);

    if (top_revert && left_revert) {
        return popup_direction::top_left;
    } else if (top_revert) {
        return popup_direction::top_right;
    } else if (left_revert) {
        return popup_direction::bottom_left;
    }
    return popup_direction::bottom_right;
}

void mb_shell::mouse_menu_widget_main::calibrate_position(bool animated) {
    if (!owner_rt)
        return;
    auto &rt = *owner_rt;
    auto [x, y] =
        calculate_position(menu_wid.get(), rt, anchor_x, anchor_y, direction);

    spdlog::info("Calibrated position: {} {} in screen {} {}", x, y,
                 rt.screen.width, rt.screen.height);

    if (animated) {
        menu_wid->x->animate_to(x / rt.dpi_scale);
        menu_wid->y->animate_to(y / rt.dpi_scale);
    } else {
        menu_wid->x->reset_to(x / rt.dpi_scale);
        menu_wid->y->reset_to(y / rt.dpi_scale);
    }

    menu_wid->arm_background_animation();
}

void mb_shell::mouse_menu_widget_main::calibrate_direction() {
    if (!owner_rt)
        return;
    direction =
        calculate_direction(menu_wid.get(), *owner_rt, anchor_x, anchor_y);
    menu_wid->direction = direction;
    menu_wid->reset_animation(is_upward(direction));

    spdlog::info("Calibrated direction: {}",
                 direction == popup_direction::top_left      ? "top_left"
                 : direction == popup_direction::top_right   ? "top_right"
                 : direction == popup_direction::bottom_left ? "bottom_left"
                                                             : "bottom_right");
}

mb_shell::screenside_button_group_widget::screenside_button_group_widget()
    : super() {
    padding_left->reset_to(8);
    padding_right->reset_to(8);
    padding_top->reset_to(8);
    padding_bottom->reset_to(8);

    gap = 4;
    horizontal = true;
    hit_self = false;
}

mb_shell::screenside_button_group_widget::button_widget::button_widget(
    std::string icon_svg)
    : icon_svg(std::move(icon_svg)) {
    bg_opacity->reset_to(0);

    width->reset_to(30);
    height->reset_to(30);
    config::current->context_menu.theme.animation.item.opacity(bg_opacity, 0);
}

void mb_shell::screenside_button_group_widget::button_widget::tick(float) {
    if (pressed()) {
        bg_opacity->animate_to(0.6f);
    } else if (hovered()) {
        bg_opacity->animate_to(0.8f);
    } else {
        bg_opacity->animate_to(1);
    }
}

void mb_shell::screenside_button_group_widget::button_widget::handle_mouse_down(
    ui::mouse_event &e) {
    if (e.button != ui::mouse_button::left)
        return;
    e.handled = true;
    if (on_click) {
        on_click();
    }
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
