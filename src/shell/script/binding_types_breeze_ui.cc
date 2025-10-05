#include "binding_types.hpp"
#include "breeze_ui/animator.h"
#include "breeze_ui/ui.h"
#include "breeze_ui/widget.h"
#include "shell/config.h"
#include "shell/contextmenu/menu_widget.h"
#include <memory>
#include <print>

#include "../utils.h"

namespace mb_shell::js {

// Macro for getter/setter pairs with animation support
#define IMPL_ANIMATED_PROP(class_name, widget_type, prop_name, prop_type)      \
    prop_type class_name::get_##prop_name() const {                            \
        auto widget = std::dynamic_pointer_cast<widget_type>($widget);         \
        if (!widget)                                                           \
            return prop_type{};                                                \
        return widget->prop_name->dest();                                      \
    }                                                                          \
    void class_name::set_##prop_name(prop_type value) {                        \
        auto widget = std::dynamic_pointer_cast<widget_type>($widget);         \
        if (!widget)                                                           \
            return;                                                            \
        widget->prop_name->animate_to(value);                                  \
    }

// Macro for simple getter/setter pairs
#define IMPL_SIMPLE_PROP(class_name, widget_type, prop_name, prop_type)        \
    prop_type class_name::get_##prop_name() const {                            \
        auto widget = std::dynamic_pointer_cast<widget_type>($widget);         \
        if (!widget)                                                           \
            return prop_type{};                                                \
        return widget->prop_name;                                              \
    }                                                                          \
    void class_name::set_##prop_name(prop_type value) {                        \
        auto widget = std::dynamic_pointer_cast<widget_type>($widget);         \
        if (!widget)                                                           \
            return;                                                            \
        widget->prop_name = value;                                             \
    }

// Macro for callback function getter/setter pairs
#define IMPL_CALLBACK_PROP(class_name, widget_type, prop_name, callback_type)  \
    callback_type class_name::get_##prop_name() const {                        \
        auto widget = std::dynamic_pointer_cast<widget_type>($widget);         \
        if (!widget)                                                           \
            return nullptr;                                                    \
        return widget->prop_name;                                              \
    }                                                                          \
    void class_name::set_##prop_name(callback_type callback) {                 \
        auto widget = std::dynamic_pointer_cast<widget_type>($widget);         \
        if (!widget)                                                           \
            return;                                                            \
        widget->prop_name = callback;                                          \
    }

// Macro for color getter/setter pairs with animation
#define IMPL_COLOR_PROP(class_name, widget_type, prop_name)                    \
    std::optional<std::tuple<float, float, float, float>>                      \
        class_name::get_##prop_name() const {                                  \
        auto widget = std::dynamic_pointer_cast<widget_type>($widget);         \
        if (!widget)                                                           \
            return std::nullopt;                                               \
        auto color = *widget->prop_name;                                       \
        return std::make_tuple(color[0], color[1], color[2], color[3]);        \
    }                                                                          \
    void class_name::set_##prop_name(                                          \
        std::optional<std::tuple<float, float, float, float>> color) {         \
        auto widget = std::dynamic_pointer_cast<widget_type>($widget);         \
        if (!widget)                                                           \
            return;                                                            \
        if (color.has_value()) {                                               \
            widget->prop_name.animate_to(                                      \
                {std::get<0>(color.value()), std::get<1>(color.value()),       \
                 std::get<2>(color.value()), std::get<3>(color.value())});     \
        } else {                                                               \
            widget->prop_name.animate_to({0.0f, 0.0f, 0.0f, 0.0f});            \
        }                                                                      \
    }

// Macro for paint getter/setter pairs
#define IMPL_PAINT_PROP(class_name, widget_type, prop_name)                    \
    std::shared_ptr<breeze_ui::breeze_paint> class_name::get_##prop_name()     \
        auto widget = std::dynamic_pointer_cast<widget_type>($widget);         \
        if (!widget || !widget->prop_name)                                     \
            return nullptr;                                                    \
        return std::make_shared<breeze_paint>(*widget->prop_name);             \
    }                                                                          \
    void class_name::set_##prop_name(std::shared_ptr<breeze_paint> paint) {    \
        auto widget = std::dynamic_pointer_cast<widget_type>($widget);         \
        if (!widget || !paint)                                                 \
            return;                                                            \
        widget->prop_name = paint->$paint;                                     \
    }

std::vector<std::shared_ptr<breeze_ui::js_widget>>
breeze_ui::js_widget::children() const {
    std::vector<std::shared_ptr<js_widget>> result;
    if (!$widget)
        return result;

    for (const auto &child : $widget->children) {
        result.push_back(std::make_shared<js_widget>(child));
    }
    return result;
}

std::string breeze_ui::js_text_widget::get_text() const {
    auto text_widget = std::dynamic_pointer_cast<ui::text_widget>($widget);
    if (!text_widget)
        return "";
    return text_widget->text;
}

void breeze_ui::js_text_widget::set_text(std::string text) {
    auto text_widget = std::dynamic_pointer_cast<ui::text_widget>($widget);
    if (!text_widget)
        return;
    text_widget->text = text;
    text_widget->needs_repaint = true;
}

int breeze_ui::js_text_widget::get_font_size() const {
    auto text_widget = std::dynamic_pointer_cast<ui::text_widget>($widget);
    if (!text_widget)
        return 0;
    return text_widget->font_size;
}

void breeze_ui::js_text_widget::set_font_size(int size) {
    auto text_widget = std::dynamic_pointer_cast<ui::text_widget>($widget);
    if (!text_widget)
        return;
    text_widget->font_size = size;
    text_widget->needs_repaint = true;
}

std::tuple<float, float, float, float>
breeze_ui::js_text_widget::get_color() const {
    auto text_widget = std::dynamic_pointer_cast<ui::text_widget>($widget);
    if (!text_widget)
        return {0.0f, 0.0f, 0.0f, 0.0f};
    auto color_array = *text_widget->color;
    return {color_array[0], color_array[1], color_array[2], color_array[3]};
}

void breeze_ui::js_text_widget::set_color(
    std::optional<std::tuple<float, float, float, float>> color) {
    auto text_widget = std::dynamic_pointer_cast<ui::text_widget>($widget);
    if (!text_widget)
        return;
    if (color.has_value()) {
        text_widget->color.animate_to(
            {std::get<0>(color.value()), std::get<1>(color.value()),
             std::get<2>(color.value()), std::get<3>(color.value())});
    } else {
        text_widget->color.animate_to({0.0f, 0.0f, 0.0f, 0.0f});
    }
}

void breeze_ui::js_widget::append_child_after(std::shared_ptr<js_widget> child,
                                              int after_index) {
    if (child && child->$widget) {
        $widget->children_dirty = true;
        $widget->needs_repaint = true;
        if (after_index < 0) {
            after_index = $widget->children.size() + after_index + 1;
        }
        $widget->children.insert(
            $widget->children.begin() + std::max(0, after_index),
            std::dynamic_pointer_cast<ui::widget>(child->$widget));
    }
}

void breeze_ui::js_widget::append_child(std::shared_ptr<js_widget> child) {
    append_child_after(child, -1);
}

void breeze_ui::js_widget::prepend_child(std::shared_ptr<js_widget> child) {
    append_child_after(child, 0);
}

void breeze_ui::js_widget::remove_child(std::shared_ptr<js_widget> child) {
    if (!$widget)
        return;

    if (child && child->$widget) {
        auto it = std::find($widget->children.begin(), $widget->children.end(),
                            child->$widget);
        if (it != $widget->children.end()) {
            $widget->children.erase(it);
            $widget->children_dirty = true;
            $widget->needs_repaint = true;
        }
    }
}

IMPL_ANIMATED_PROP(breeze_ui::js_widget, ui::widget, x, float)
IMPL_ANIMATED_PROP(breeze_ui::js_widget, ui::widget, y, float)
IMPL_ANIMATED_PROP(breeze_ui::js_widget, ui::widget, width, float)
IMPL_ANIMATED_PROP(breeze_ui::js_widget, ui::widget, height, float)

std::shared_ptr<breeze_ui::js_text_widget>
breeze_ui::widgets_factory::create_text_widget() {
    auto text_widget = std::make_shared<ui::text_widget>();

    auto res = std::make_shared<js_text_widget>();
    res->$widget = std::dynamic_pointer_cast<ui::widget>(text_widget);
    return res;
}

struct image_widget : public ui::widget {
    struct data_svg {
        std::string svg;
    };

    std::variant<data_svg> image_data;
    std::optional<ui::NVGImage> image;
    void render(ui::nanovg_context ctx) override {
        if (!image) {
            if (std::get_if<data_svg>(&image_data)) {
                const auto &data = std::get<data_svg>(image_data);
                auto svg = data.svg;

                image = ctx.imageFromSVG(nsvgParse(svg.data(), "px", 96));
            }
        }

        if (image) {
            ctx.drawImage(*image, *x, *y, *width, *height);
        }
    }
};

std::string breeze_ui::js_image_widget::get_svg() const {
    auto w = $widget->downcast<image_widget>();
    if (w) {
        const auto &data = std::get<image_widget::data_svg>(w->image_data);
        return data.svg;
    }
    return {};
}
void breeze_ui::js_image_widget::set_svg(std::string svg) {
    auto w = $widget->downcast<image_widget>();
    if (w) {
        w->image_data = image_widget::data_svg{std::move(svg)};
    }
}
std::shared_ptr<breeze_ui::js_image_widget>
breeze_ui::widgets_factory::create_image_widget() {
    auto iw = std::make_shared<image_widget>();

    auto res = std::make_shared<js_image_widget>();
    res->$widget = std::dynamic_pointer_cast<ui::widget>(iw);
    return res;
}

std::shared_ptr<breeze_ui::js_spacer_widget>
breeze_ui::widgets_factory::create_spacer_widget() {
    auto iw = std::make_shared<ui::widget_flex::spacer>();

    auto res = std::make_shared<js_spacer_widget>();
    res->$widget = std::dynamic_pointer_cast<ui::widget>(iw);
    return res;
}

void breeze_ui::js_spacer_widget::set_size(float size) {
    auto w = $widget->downcast<ui::widget_flex::spacer>();
    if (w) {
        w->size = size;
    }
}

float breeze_ui::js_spacer_widget::get_size() const {
    auto w = $widget->downcast<ui::widget_flex::spacer>();
    if (w) {
        return w->size;
    }
    return 0;
}

struct widget_js_base : public ui::widget_flex {
    using super = ui::widget_flex;

    std::function<void(int)> on_click;
    std::function<void(float, float)> on_mouse_move;
    std::function<void()> on_mouse_enter;
    std::function<void()> on_mouse_leave;
    std::function<void()> on_mouse_down;
    std::function<void()> on_mouse_up;
    std::function<void(int)> on_mouse_wheel;
    std::function<void(ui::update_context &ctx)> on_update;

    bool previous_hovered = false;

    float prev_mouse_x = 0, prev_mouse_y = 0;

    void update(ui::update_context &ctx) override {
        super::update(ctx);

        try {
            if (on_update) {
                on_update(ctx);
            }

            auto weak = weak_from_this();
            if (ctx.hovered(this) && ctx.mouse_clicked && on_click) {
                on_click(0);
                if (weak.expired())
                    return;
            }

            if (ctx.hovered(this) && !previous_hovered && on_mouse_enter) {
                on_mouse_enter();
                if (weak.expired())
                    return;
            } else if (!ctx.hovered(this) && previous_hovered &&
                       on_mouse_leave) {
                on_mouse_leave();
                if (weak.expired())
                    return;
            }

            previous_hovered = ctx.hovered(this);
            if (ctx.mouse_down_on(this) && on_mouse_down) {
                on_mouse_down();
                if (weak.expired())
                    return;
            }

            if (ctx.mouse_up && on_mouse_up) {
                on_mouse_up();
                if (weak.expired())
                    return;
            }

            if (ctx.mouse_x != prev_mouse_x || ctx.mouse_y != prev_mouse_y) {
                prev_mouse_x = ctx.mouse_x;
                prev_mouse_y = ctx.mouse_y;
                if (on_mouse_move && ctx.hovered(this)) {
                    on_mouse_move(ctx.mouse_x, ctx.mouse_y);
                    if (weak.expired())
                        return;
                }
            }

            if (ctx.scroll_y != 0 && on_mouse_wheel) {
                on_mouse_wheel(ctx.scroll_y);
                if (weak.expired())
                    return;
            }
        } catch (const std::exception &e) {
            std::cerr << "Exception in widget update: " << e.what()
                      << std::endl;
        }
    }

    ui::sp_anim_float opacity = anim_float(255), border_radius = anim_float(0),
                      border_width = anim_float(0);
    ui::animated_color background_color = {this, 0.f, 0.f, 0.f, 0.f},
                       border_color = {this, 0.0f, 0.0f, 0.0f, 1.0f};
    bool inset_border = false;

    std::optional<paint_color> background_paint, border_paint;

    void render(ui::nanovg_context ctx) override {
        float rx = *x, ry = *y, rw = *width, rh = *height;
        if (inset_border) {
            rx += *border_width / 2;
            ry += *border_width / 2;
            rw -= *border_width;
            rh -= *border_width;
        }

        auto scope = ctx.transaction();

        ctx.globalAlpha(*opacity / 255.f);
        if (background_paint) {
            background_paint->apply_to_ctx(ctx, rx, ry, rw, rh);
        } else {
            ctx.fillColor(background_color);
        }

        ctx.fillRoundedRect(rx, ry, rw, rh, *border_radius);

        if (border_paint) {
            border_paint->apply_to_ctx(ctx, rx, ry, rw, rh);
        } else {
            ctx.strokeColor(border_color);
        }

        if (*border_width > 0) {
            ctx.strokeWidth(*border_width);
            ctx.strokeRoundedRect(rx, ry, rw, rh, *border_radius);
        }

        super::render(ctx);
    }
};

std::shared_ptr<breeze_ui::js_flex_layout_widget>
breeze_ui::widgets_factory::create_flex_layout_widget() {
    auto layout_widget = std::make_shared<widget_js_base>();
    auto res = std::make_shared<js_flex_layout_widget>();
    res->$widget = std::dynamic_pointer_cast<ui::widget>(layout_widget);
    return res;
}

IMPL_ANIMATED_PROP(breeze_ui::js_flex_layout_widget, widget_js_base,
                   padding_left, float)
IMPL_ANIMATED_PROP(breeze_ui::js_flex_layout_widget, widget_js_base,
                   padding_right, float)
IMPL_ANIMATED_PROP(breeze_ui::js_flex_layout_widget, widget_js_base,
                   padding_top, float)
IMPL_ANIMATED_PROP(breeze_ui::js_flex_layout_widget, widget_js_base,
                   padding_bottom, float)

std::tuple<float, float, float, float>
breeze_ui::js_flex_layout_widget::get_padding() const {
    return {get_padding_left(), get_padding_right(), get_padding_top(),
            get_padding_bottom()};
}

IMPL_SIMPLE_PROP(breeze_ui::js_flex_layout_widget, widget_js_base, horizontal,
                 bool)

void breeze_ui::js_flex_layout_widget::set_padding(float left, float right,
                                                   float top, float bottom) {
    set_padding_left(left);
    set_padding_right(right);
    set_padding_top(top);
    set_padding_bottom(bottom);
}

std::variant<std::shared_ptr<breeze_ui::js_widget>,
             std::shared_ptr<breeze_ui::js_text_widget>,
             std::shared_ptr<breeze_ui::js_flex_layout_widget>,
             std::shared_ptr<breeze_ui::js_image_widget>,
             std::shared_ptr<breeze_ui::js_spacer_widget>>
breeze_ui::js_widget::downcast() {
#define TRY_DOWNCAST(type)                                                     \
    if (auto casted =                                                          \
            std::dynamic_pointer_cast<type>(this->shared_from_this())) {       \
        return casted;                                                         \
    }
    TRY_DOWNCAST(js_text_widget);
    TRY_DOWNCAST(js_flex_layout_widget);
    TRY_DOWNCAST(js_image_widget);
    TRY_DOWNCAST(js_spacer_widget);
#undef TRY_DOWNCAST

    return this->shared_from_this();
}

std::shared_ptr<breeze_ui::breeze_paint>
breeze_ui::breeze_paint::from_color(std::string color) {
    auto paint = std::make_shared<breeze_paint>();
    paint->$paint = paint_color::from_string(color);
    return paint;
}

IMPL_CALLBACK_PROP(breeze_ui::js_flex_layout_widget, widget_js_base, on_click,
                   std::function<void(int)>)
IMPL_CALLBACK_PROP(breeze_ui::js_flex_layout_widget, widget_js_base,
                   on_mouse_move, std::function<void(float, float)>)
IMPL_CALLBACK_PROP(breeze_ui::js_flex_layout_widget, widget_js_base,
                   on_mouse_enter, std::function<void()>)
IMPL_CALLBACK_PROP(breeze_ui::js_flex_layout_widget, widget_js_base,
                   on_mouse_leave, std::function<void()>)
IMPL_CALLBACK_PROP(breeze_ui::js_flex_layout_widget, widget_js_base,
                   on_mouse_down, std::function<void()>)

IMPL_COLOR_PROP(breeze_ui::js_flex_layout_widget, widget_js_base,
                background_color)
IMPL_COLOR_PROP(breeze_ui::js_flex_layout_widget, widget_js_base, border_color)
IMPL_PAINT_PROP(breeze_ui::js_flex_layout_widget, widget_js_base,
                background_paint)
IMPL_PAINT_PROP(breeze_ui::js_flex_layout_widget, widget_js_base, border_paint)
IMPL_ANIMATED_PROP(breeze_ui::js_flex_layout_widget, widget_js_base,
                   border_radius, float)
IMPL_ANIMATED_PROP(breeze_ui::js_flex_layout_widget, widget_js_base,
                   border_width, float)

IMPL_SIMPLE_PROP(breeze_ui::js_flex_layout_widget, widget_js_base, auto_size,
                 bool)

IMPL_SIMPLE_PROP(breeze_ui::js_flex_layout_widget, widget_js_base, gap,
                 float)
std::string breeze_ui::js_flex_layout_widget::get_justify_content() const {
    auto widget = std::dynamic_pointer_cast<ui::widget_flex>($widget);
    if (!widget)
        return "";
    return std::string(reflect::enum_name(widget->justify_content));
}

void breeze_ui::js_flex_layout_widget::set_justify_content(
    std::string justify) {
    auto widget = std::dynamic_pointer_cast<ui::widget_flex>($widget);
    if (!widget)
        return;

    if (auto val =
            mb_shell::enum_from_string<ui::widget_flex::justify>(justify)) {
        widget->justify_content = *val;
    }
}

std::string breeze_ui::js_flex_layout_widget::get_align_items() const {
    auto widget = std::dynamic_pointer_cast<ui::widget_flex>($widget);
    if (!widget)
        return "";
    return std::string(reflect::enum_name(widget->align_items));
}

void breeze_ui::js_flex_layout_widget::set_align_items(std::string align) {
    auto widget = std::dynamic_pointer_cast<ui::widget_flex>($widget);
    if (!widget)
        return;

    if (auto val =
            mb_shell::enum_from_string<ui::widget_flex::align>(align)) {
        widget->align_items = *val;
    }
}

void breeze_ui::window::set_root_widget(
    std::shared_ptr<mb_shell::js::breeze_ui::js_widget> widget) {
    if (!$render_target)
        return;
    std::lock_guard l($render_target->rt_lock);
    $render_target->root = widget->$widget;
}

void breeze_ui::window::close() {
    if (!$render_target)
        return;
    $render_target->close();
}

std::shared_ptr<breeze_ui::window>
breeze_ui::window::create(std::string title, int width, int height) {
    auto rt = std::make_shared<ui::render_target>();
    rt->acrylic = 0.1;
    rt->transparent = true;
    rt->width = width;
    rt->height = height;
    rt->title = title;

    auto win = std::make_shared<breeze_ui::window>();
    win->$render_target = std::move(rt);
    std::thread([win]() {
        if (auto res = win->$render_target->init(); res) {
            config::current->apply_fonts_to_nvg(win->$render_target->nvg);
            win->$render_target->show();
            win->$render_target->start_loop();
        }

        win->$render_target->close();
    }).detach();
    return win;
}

void breeze_ui::js_widget::set_animation(std::string variable_name,
                                         bool enabled) {
    if (!$widget)
        return;
    for (auto &anim_float : $widget->anim_floats) {
        if (anim_float->name == variable_name) {
            if (enabled) {
                anim_float->set_duration(100);
                anim_float->set_easing(ui::easing_type::ease_in_out);
            } else {
                anim_float->set_easing(ui::easing_type::mutation);
            }
        }
    }
}

// Clean up macros
#undef IMPL_ANIMATED_PROP
#undef IMPL_SIMPLE_PROP
#undef IMPL_CALLBACK_PROP
#undef IMPL_COLOR_PROP
#undef IMPL_PAINT_PROP

} // namespace mb_shell::js