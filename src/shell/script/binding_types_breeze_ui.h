#pragma once
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

#include "../paint_color.h"

namespace ui {
struct widget;
struct render_target;
} // namespace ui

namespace mb_shell::js {
struct breeze_ui {
    struct js_text_widget;
    struct js_flex_layout_widget;
    struct breeze_paint;
    struct js_image_widget;
    struct js_spacer_widget;
    struct js_widget : public std::enable_shared_from_this<js_widget> {
        std::shared_ptr<ui::widget> $widget;

        js_widget() = default;
        js_widget(std::shared_ptr<ui::widget> widget) : $widget(widget) {}
        virtual ~js_widget() = default;

        std::vector<std::shared_ptr<js_widget>> children() const;
        void append_child(std::shared_ptr<js_widget> child);
        void prepend_child(std::shared_ptr<js_widget> child);
        void remove_child(std::shared_ptr<js_widget> child);
        void append_child_after(std::shared_ptr<js_widget> child,
                                int after_index);

        void set_animation(std::string variable_name, bool enabled);

        float get_x() const;
        void set_x(float x);
        float get_y() const;
        void set_y(float y);
        float get_width() const;
        void set_width(float width);
        float get_height() const;
        void set_height(float height);

        std::variant<std::shared_ptr<js_widget>,
                     std::shared_ptr<js_text_widget>,
                     std::shared_ptr<js_flex_layout_widget>,
                     std::shared_ptr<js_image_widget>,
                     std::shared_ptr<js_spacer_widget>>
        downcast();
        // // Note: You can only certain widgets that can be loaded with
        // `downcast()`.
        // std::shared_ptr<js_widget> clone(bool with_children = true) const;
    };

    struct js_text_widget : public js_widget {
        std::string get_text() const;
        void set_text(std::string text);
        int get_font_size() const;
        void set_font_size(int size);
        std::tuple<float, float, float, float> get_color() const;
        void
        set_color(std::optional<std::tuple<float, float, float, float>> color);
    };

    struct js_flex_layout_widget : public js_widget {
#define DEFINE_PROP(type, name)                                                \
    type get_##name() const;                                                   \
    void set_##name(type);

        DEFINE_PROP(bool, auto_size)

        DEFINE_PROP(bool, horizontal)
        DEFINE_PROP(float, padding_left)
        DEFINE_PROP(float, padding_right)
        DEFINE_PROP(float, padding_top)
        DEFINE_PROP(float, padding_bottom)
        std::tuple<float, float, float, float> get_padding() const;
        void set_padding(float left, float right, float top, float bottom);

        DEFINE_PROP(std::function<void(int)>, on_click)
        DEFINE_PROP(std::function<void(float, float)>, on_mouse_move)
        DEFINE_PROP(std::function<void()>, on_mouse_enter)
        DEFINE_PROP(std::function<void()>, on_mouse_leave)
        DEFINE_PROP(std::function<void()>, on_mouse_down)
        DEFINE_PROP(std::function<void()>, on_mouse_up)

        std::string get_justify_content() const;
        void set_justify_content(std::string justify);
        std::string get_align_items() const;
        void set_align_items(std::string align);

        void set_background_color(
            std::optional<std::tuple<float, float, float, float>> color);
        std::optional<std::tuple<float, float, float, float>>
        get_background_color() const;

        DEFINE_PROP(std::shared_ptr<breeze_paint>, background_paint)
        DEFINE_PROP(std::shared_ptr<breeze_paint>, border_paint)
        DEFINE_PROP(float, border_radius)
        DEFINE_PROP(float, gap)
        void set_border_color(
            std::optional<std::tuple<float, float, float, float>> color);
        std::optional<std::tuple<float, float, float, float>>
        get_border_color() const;
        DEFINE_PROP(float, border_width)

#undef DEFINE_PROP
    };

    struct js_image_widget : public js_widget {
        std::string get_svg() const;
        void set_svg(std::string svg);
    };

    struct js_spacer_widget : public js_widget {
        float get_size() const;
        void set_size(float size);
    };

    struct widgets_factory {
        static std::shared_ptr<js_text_widget> create_text_widget();
        static std::shared_ptr<js_flex_layout_widget>
        create_flex_layout_widget();
        static std::shared_ptr<js_image_widget> create_image_widget();
        static std::shared_ptr<js_spacer_widget> create_spacer_widget();
    };

    struct breeze_paint {
        paint_color $paint;
        static std::shared_ptr<breeze_paint> from_color(std::string color);
    };

    struct window {
        static std::shared_ptr<window> create(std::string title, int width,
                                              int height);

        std::shared_ptr<ui::render_target> $render_target;
        void set_root_widget(
            std::shared_ptr<mb_shell::js::breeze_ui::js_widget> widget);
        void close();
    };
};
} // namespace mb_shell::js