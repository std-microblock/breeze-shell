#pragma once
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace ui {
struct widget;
}

namespace mb_shell::js_impl {
struct widget_js_base;
}

namespace mb_shell::js {
struct breeze_ui {
  struct js_widget {
    std::shared_ptr<ui::widget> $widget;

    js_widget() = default;
    js_widget(std::shared_ptr<ui::widget> widget) : $widget(widget) {}

    std::vector<std::shared_ptr<js_widget>> children() const;
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
    std::vector<std::shared_ptr<js_widget>> children() const;
    void append_child(std::shared_ptr<js_widget> child);
    void prepend_child(std::shared_ptr<js_widget> child);
    void remove_child(std::shared_ptr<js_widget> child);
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
  };

  struct widgets_factory {
    static std::shared_ptr<js_text_widget> create_text_widget();
    static std::shared_ptr<js_flex_layout_widget> create_flex_layout_widget();
  };
};
} // namespace mb_shell::js