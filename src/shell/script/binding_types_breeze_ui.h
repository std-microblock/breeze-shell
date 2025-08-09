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
}

namespace mb_shell::js {
struct breeze_ui {
  struct js_text_widget;
  struct js_flex_layout_widget;
  struct breeze_paint;
  struct js_widget : public std::enable_shared_from_this<js_widget> {
    std::shared_ptr<ui::widget> $widget;

    js_widget() = default;
    js_widget(std::shared_ptr<ui::widget> widget) : $widget(widget) {}
    virtual ~js_widget() = default;

    std::vector<std::shared_ptr<js_widget>> children() const;
    void append_child(std::shared_ptr<js_widget> child);
    void prepend_child(std::shared_ptr<js_widget> child);
    void remove_child(std::shared_ptr<js_widget> child);
    void append_child_after(std::shared_ptr<js_widget> child, int after_index);

    std::variant<std::shared_ptr<js_widget>, std::shared_ptr<js_text_widget>,
                 std::shared_ptr<js_flex_layout_widget>>
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
    void set_color(std::optional<std::tuple<float, float, float, float>> color);
  };

  struct js_flex_layout_widget : public js_widget {
    bool get_horizontal() const;
    void set_horizontal(bool horizontal);

    float get_padding_left() const;
    void set_padding_left(float padding);
    float get_padding_right() const;
    void set_padding_right(float padding);
    float get_padding_top() const;
    void set_padding_top(float padding);
    float get_padding_bottom() const;
    void set_padding_bottom(float padding);
    std::tuple<float, float, float, float> get_padding() const;
    void set_padding(float left, float right, float top, float bottom);
    std::function<void(int)> get_on_click() const;
    void set_on_click(std::function<void(int)> on_click);
    std::function<void(float, float)> get_on_mouse_move() const;
    void set_on_mouse_move(std::function<void(float, float)> on_mouse_move);
    std::function<void()> get_on_mouse_enter() const;
    void set_on_mouse_enter(std::function<void()> on_mouse_enter);
    void set_background_color(
        std::optional<std::tuple<float, float, float, float>> color);
    std::optional<std::tuple<float, float, float, float>>
    get_background_color() const;
    void set_background_paint(std::shared_ptr<breeze_paint> paint);
    std::shared_ptr<breeze_paint> get_background_paint() const;
    void set_border_radius(float radius);
    float get_border_radius() const;
    void set_border_color(
        std::optional<std::tuple<float, float, float, float>> color);
    std::optional<std::tuple<float, float, float, float>>
    get_border_color() const;
    void set_border_width(float width);
    float get_border_width() const;
    void set_border_paint(std::shared_ptr<breeze_paint> paint);
    std::shared_ptr<breeze_paint> get_border_paint() const;
  };

  struct widgets_factory {
    static std::shared_ptr<js_text_widget> create_text_widget();
    static std::shared_ptr<js_flex_layout_widget> create_flex_layout_widget();
  };

  struct breeze_paint {
    paint_color $paint;
    static std::shared_ptr<breeze_paint> from_color(std::string color);
  };
};
} // namespace mb_shell::js