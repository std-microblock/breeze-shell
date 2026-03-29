#pragma once
#include <functional>
#include <memory>
#include <mutex>
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
    using rgba_prop = std::optional<std::tuple<float, float, float, float>>;
    struct js_text_widget;
    struct js_textbox_widget;
    struct js_flex_layout_widget;
    struct breeze_paint;
    struct js_image_widget;
    struct js_spacer_widget;
    struct js_widget : public std::enable_shared_from_this<js_widget> {
        std::shared_ptr<ui::widget> $widget;

        js_widget() = default;
        js_widget(std::shared_ptr<ui::widget> widget) : $widget(widget) {}
        virtual ~js_widget() = default;

        std::optional<std::unique_lock<std::recursive_mutex>> $rt_lock();

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

        std::variant<
            std::shared_ptr<js_widget>, std::shared_ptr<js_text_widget>,
            std::shared_ptr<js_textbox_widget>,
            std::shared_ptr<js_flex_layout_widget>,
            std::shared_ptr<js_image_widget>, std::shared_ptr<js_spacer_widget>>
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
        int get_font_weight() const;
        void set_font_weight(int weight);
        float get_max_width() const;
        void set_max_width(float w);
        std::optional<std::tuple<float, float, float, float>> get_color() const;
        void
        set_color(std::optional<std::tuple<float, float, float, float>> color);
    };

    struct js_textbox_widget : public js_widget {
#define DEFINE_PROP(type, name)                                                \
    type get_##name() const;                                                   \
    void set_##name(type);

        DEFINE_PROP(std::string, text)
        DEFINE_PROP(std::string, placeholder)
        DEFINE_PROP(int, font_size)
        DEFINE_PROP(int, font_weight)
        DEFINE_PROP(float, padding_x)
        DEFINE_PROP(float, padding_y)
        DEFINE_PROP(float, border_radius)
        DEFINE_PROP(float, min_height)
        DEFINE_PROP(float, preferred_multiline_height)
        DEFINE_PROP(float, line_height_multiplier)
        DEFINE_PROP(bool, multiline)
        DEFINE_PROP(bool, readonly)
        DEFINE_PROP(bool, disabled)
        DEFINE_PROP(rgba_prop, background_color)
        DEFINE_PROP(rgba_prop, readonly_background_color)
        DEFINE_PROP(rgba_prop, disabled_background_color)
        DEFINE_PROP(rgba_prop, border_color)
        DEFINE_PROP(rgba_prop, focus_border_color)
        DEFINE_PROP(rgba_prop, text_color)
        DEFINE_PROP(rgba_prop, disabled_text_color)
        DEFINE_PROP(rgba_prop, placeholder_color)
        DEFINE_PROP(rgba_prop, selection_color)
        DEFINE_PROP(rgba_prop, caret_color)
        DEFINE_PROP(rgba_prop, composition_underline_color)
        DEFINE_PROP(std::function<void(std::string)>, on_change)
        DEFINE_PROP(std::function<void()>, on_focus)
        DEFINE_PROP(std::function<void()>, on_blur)
        DEFINE_PROP(std::function<bool(int, bool, bool, bool, bool)>,
                    on_key_down)

        void focus();
        void blur();
        void select_all();
        void select_range(int start, int end);
        int get_selection_start() const;
        int get_selection_end() const;
        void set_selection(int start, int end);
        void insert_text(std::string new_text);
        void delete_text(int start, int end);
        void clear();
        void copy();
        void cut();
        void paste();

#undef DEFINE_PROP
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
        DEFINE_PROP(float, flex_grow)
        DEFINE_PROP(float, flex_shrink)
        DEFINE_PROP(float, max_height)
        DEFINE_PROP(bool, enable_scrolling)
        DEFINE_PROP(bool, enable_child_clipping)
        DEFINE_PROP(bool, crop_overflow)
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
        static std::shared_ptr<js_textbox_widget> create_textbox_widget();
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
                                              int height) {
            return create_ex(title, width, height, nullptr);
        }

        static std::shared_ptr<window>
        create_ex(std::string title, int width, int height,
                  std::function<void()> on_close);

        std::shared_ptr<ui::render_target> $render_target;
        std::shared_ptr<mb_shell::js::breeze_ui::js_widget> get_root_widget()
            const;
        void set_root_widget(
            std::shared_ptr<mb_shell::js::breeze_ui::js_widget> widget);
        void close();
    };
};
} // namespace mb_shell::js
