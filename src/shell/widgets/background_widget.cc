#include "background_widget.h"
#include "breeze_ui/extra_widgets.h"
#include "nanovg.h"
#include "shell/config.h"
#include "shell/contextmenu/menu_render.h"
#include "shell/utils.h"

namespace mb_shell {

background_widget::background_widget(bool is_main) {
    auto light_color = is_light_mode();
    if (is_acrylic_available() && config::current->context_menu.theme.acrylic) {
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
        bg_impl = acrylic;

        if (light_color)
            bg_impl->bg_color = nvgRGBAf(1, 1, 1, 0);
        else
            bg_impl->bg_color = nvgRGBAf(0, 0, 0, 0);
    } else {
        bg_impl = std::make_shared<ui::rect_widget>();
        auto c = light_color ? 1 : 25 / 255.f;
        bg_impl->bg_color = nvgRGBAf(c, c, c, 1);
    }

    bg_impl->radius->reset_to(config::current->context_menu.theme.radius);
    bg_impl->opacity->reset_to(0);

    if (is_main)
        config::current->context_menu.theme.animation.main_bg.opacity(
            bg_impl->opacity, 0);
    else {
        config::current->context_menu.theme.animation.submenu_bg.opacity(
            bg_impl->opacity, 0);
        config::current->context_menu.theme.animation.submenu_bg.x(bg_impl->x,
                                                                   0);
        config::current->context_menu.theme.animation.submenu_bg.y(bg_impl->y,
                                                                   0);
        config::current->context_menu.theme.animation.submenu_bg.w(
            bg_impl->width, 0);
        config::current->context_menu.theme.animation.submenu_bg.h(
            bg_impl->height, 0);
    }
    bg_impl->opacity->animate_to(
        255 * config::current->context_menu.theme.background_opacity);

    opacity = bg_impl->opacity;
    x = bg_impl->x;
    y = bg_impl->y;
    width = bg_impl->width;
    height = bg_impl->height;
    radius = bg_impl->radius;
    bg_color = bg_impl->bg_color;
}

void background_widget::update(ui::update_context &ctx) {
    bg_impl->x->reset_to(x->dest());
    bg_impl->y->reset_to(y->dest());
    bg_impl->width->reset_to(width->dest());
    bg_impl->height->reset_to(height->dest());
    bg_impl->bg_color = bg_color;
    bg_impl->update(ctx);

    super::update(ctx);
}

void background_widget::render(ui::nanovg_context ctx) {
    {
        auto t = ctx.transaction();
        ctx.globalAlpha(*opacity / 255.f);
        auto &theme = config::current->context_menu.theme;
        bool light = is_light_mode();

        bool use_dwm = theme.use_dwm_if_available ? is_win11_or_later() : false;
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
            ctx.roundedRect(*x - shadow_size + shadow_offset_x,
                            *y - shadow_size + shadow_offset_y,
                            *width + shadow_size * 2, *height + shadow_size * 2,
                            corner_radius + shadow_size);
            ctx.fillPaint(ctx.boxGradient(*x + shadow_offset_x,
                                          *y + shadow_offset_y, *width, *height,
                                          corner_radius, shadow_size,
                                          shadow_color_from, shadow_color_to));
            ctx.fill();

            ctx.beginPath();
            if (theme.inset_border) {
                ctx.roundedRect(*x + boarder_width / 2, *y + boarder_width / 2,
                                *width - boarder_width, *height - boarder_width,
                                corner_radius);
            } else {
                ctx.roundedRect(*x, *y, *width, *height, corner_radius);
            }
            ctx.strokeWidth(boarder_width);
            auto border_color =
                light ? theme.border_color_light : theme.border_color_dark;
            border_color.apply_to_ctx(ctx, *x, *y, *width, *height);
            ctx.stroke();
        }

        ctx.globalCompositeOperation(NVG_DESTINATION_IN);
        ctx.globalAlpha(1);
        auto cl = nvgRGBAf(0, 0, 0, 1 - *opacity / 255.f);
        ctx.fillColor(cl);
        if (theme.inset_border)
            ctx.fillRoundedRect(*x + boarder_width, *y + boarder_width,
                                *width - boarder_width * 2,
                                *height - boarder_width * 2, *radius);
        else
            ctx.fillRoundedRect(*x, *y, *width, *height, *radius);
    }
    bg_impl->render(ctx);
    super::render(ctx);
}

} // namespace mb_shell
