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
struct tree_lock;
} // namespace ui

namespace mb_shell::js {
struct breeze_ui {
    using rgba_prop = std::optional<std::tuple<float, float, float, float>>;
    struct js_text_widget;
    struct js_textbox_widget;
    struct js_flex_layout_widget;
    struct breeze_paint;
    struct js_image_widget;
    struct js_icon_widget;
    struct js_spacer_widget;
    struct js_widget : public std::enable_shared_from_this<js_widget> {
        std::shared_ptr<ui::widget> $widget;

        js_widget() = default;
        js_widget(std::shared_ptr<ui::widget> widget) : $widget(widget) {}
        virtual ~js_widget() = default;

        std::optional<std::unique_lock<ui::tree_lock>> $rt_lock();

        // 想自己处理键盘的自绘控件: 菜单的上下导航会停在它上面, 按键先交给它
        std::function<bool(int, bool, bool, bool, bool)> on_key_down;
        std::function<bool(int, bool, bool, bool, bool)> get_on_key_down() const {
            return on_key_down;
        }
        void
        set_on_key_down(std::function<bool(int, bool, bool, bool, bool)> cb) {
            on_key_down = std::move(cb);
        }
        std::function<void()> on_focus;
        std::function<void()> get_on_focus() const { return on_focus; }
        void set_on_focus(std::function<void()> cb) { on_focus = std::move(cb); }
        std::function<void()> on_blur;
        std::function<void()> get_on_blur() const { return on_blur; }
        void set_on_blur(std::function<void()> cb) { on_blur = std::move(cb); }

        std::vector<std::shared_ptr<js_widget>> children() const;
        void append_child(std::shared_ptr<js_widget> child);
        void prepend_child(std::shared_ptr<js_widget> child);
        void remove_child(std::shared_ptr<js_widget> child);
        void append_child_after(std::shared_ptr<js_widget> child,
                                int after_index);

        void set_animation(std::string variable_name, bool enabled);
        void set_animation_curve(std::string variable_name, float duration,
                                 std::string easing);

        // 任何 widget 都能参与 flex 布局: 这几项在 ui::widget::before_layout
        // 里会写进 Yoga。
        float get_flex_grow() const;
        void set_flex_grow(float value);
        float get_flex_shrink() const;
        void set_flex_shrink(float value);
        float get_min_width() const;
        void set_min_width(float value);
        float get_min_height() const;
        void set_min_height(float value);
        std::string get_align_self() const;
        void set_align_self(std::string align);

        float get_x() const;
        void set_x(float x);
        float get_y() const;
        void set_y(float y);
        float get_width() const;
        void set_width(float width);
        float get_height() const;
        void set_height(float height);

        std::optional<std::string> get_cursor() const;
        void set_cursor(std::optional<std::string> value);

        std::variant<
            std::shared_ptr<js_widget>, std::shared_ptr<js_text_widget>,
            std::shared_ptr<js_textbox_widget>,
            std::shared_ptr<js_flex_layout_widget>,
            std::shared_ptr<js_image_widget>, std::shared_ptr<js_icon_widget>,
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
        int get_font_weight() const;
        void set_font_weight(int weight);
        float get_max_width() const;
        void set_max_width(float w);
        float get_line_height_multiplier() const;
        void set_line_height_multiplier(float value);
        std::string get_text_overflow() const;
        void set_text_overflow(std::string mode);
        std::string get_font_family() const;
        void set_font_family(std::string family);
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
        DEFINE_PROP(bool, reverse)
        DEFINE_PROP(float, padding_left)
        DEFINE_PROP(float, padding_right)
        DEFINE_PROP(float, padding_top)
        DEFINE_PROP(float, padding_bottom)
        DEFINE_PROP(float, max_height)
        DEFINE_PROP(bool, enable_scrolling)
        DEFINE_PROP(bool, enable_child_clipping)
        DEFINE_PROP(bool, crop_overflow)
        DEFINE_PROP(float, scroll_bar_width)
        DEFINE_PROP(float, scroll_bar_radius)
        DEFINE_PROP(float, scroll_bar_margin)
        void set_scroll_bar_color(
            std::optional<std::tuple<float, float, float, float>> color);
        std::optional<std::tuple<float, float, float, float>>
        get_scroll_bar_color() const;
        std::tuple<float, float, float, float> get_padding() const;
        void set_padding(float left, float right, float top, float bottom);

        DEFINE_PROP(std::function<void(int)>, on_click)
        DEFINE_PROP(std::function<void(float, float)>, on_mouse_move)
        DEFINE_PROP(std::function<void()>, on_mouse_enter)
        DEFINE_PROP(std::function<void()>, on_mouse_leave)
        DEFINE_PROP(std::function<void()>, on_mouse_down)
        DEFINE_PROP(std::function<void()>, on_mouse_up)
        // (key, shift, ctrl, alt, super), key 是 GLFW 键值; 异步回调, 吞不掉按键
        // 焦点链上没人处理的键会广播给所有设了 on_key_down 的 flex
        std::function<void(int, bool, bool, bool, bool)> get_on_key_down() const;
        void set_on_key_down(std::function<void(int, bool, bool, bool, bool)>);

        std::string get_justify_content() const;
        void set_justify_content(std::string justify);
        std::string get_align_items() const;
        void set_align_items(std::string align);
        std::string get_flex_wrap() const;
        void set_flex_wrap(std::string wrap);

        void set_background_color(
            std::optional<std::tuple<float, float, float, float>> color);
        std::optional<std::tuple<float, float, float, float>>
        get_background_color() const;

        DEFINE_PROP(std::shared_ptr<breeze_paint>, background_paint)
        DEFINE_PROP(std::shared_ptr<breeze_paint>, border_paint)
        DEFINE_PROP(float, border_radius)
        DEFINE_PROP(float, opacity)
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

    // 画一个位图图标 (load_file_icon 给的 HBITMAP), 大小由控件宽高决定
    // 控件接管设进来的位图, 换图或销毁时释放
    struct js_icon_widget : public js_widget {
        size_t get_icon() const;
        void set_icon(size_t icon);
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
        static std::shared_ptr<js_icon_widget> create_icon_widget();
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
