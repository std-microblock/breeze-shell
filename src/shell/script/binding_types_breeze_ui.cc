#include "../contextmenu/menu_widget.h"
#include "binding_types.h"
#include "ui.h"
#include "widget.h"

namespace mb_shell::js {
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
  if (!$widget)
    return "";
  auto text_widget = std::dynamic_pointer_cast<ui::text_widget>($widget);
  if (!text_widget)
    return "";
  return text_widget->text;
}

void breeze_ui::js_text_widget::set_text(std::string text) {
  if (!$widget)
    return;
  auto text_widget = std::dynamic_pointer_cast<ui::text_widget>($widget);
  if (!text_widget)
    return;
  text_widget->text = text;
}

int breeze_ui::js_text_widget::get_font_size() const {
  if (!$widget)
    return 0;
  auto text_widget = std::dynamic_pointer_cast<ui::text_widget>($widget);
  if (!text_widget)
    return 0;
  return text_widget->font_size;
}

void breeze_ui::js_text_widget::set_font_size(int size) {
  if (!$widget)
    return;
  auto text_widget = std::dynamic_pointer_cast<ui::text_widget>($widget);
  if (!text_widget)
    return;
  text_widget->font_size = size;
}

std::tuple<float, float, float, float>
breeze_ui::js_text_widget::get_color() const {
  if (!$widget)
    return {0.0f, 0.0f, 0.0f, 0.0f};
  auto text_widget = std::dynamic_pointer_cast<ui::text_widget>($widget);
  if (!text_widget)
    return {0.0f, 0.0f, 0.0f, 0.0f};
  auto color_array = *text_widget->color;
  return {color_array[0], color_array[1], color_array[2], color_array[3]};
}

void breeze_ui::js_text_widget::set_color(
    std::optional<std::tuple<float, float, float, float>> color) {
  if (!$widget)
    return;
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
bool breeze_ui::js_flex_layout_widget::get_horizontal() const {
  return $widget && std::dynamic_pointer_cast<ui::widget_flex>($widget)
             ? std::dynamic_pointer_cast<ui::widget_flex>($widget)->horizontal
             : false;
}
void breeze_ui::js_flex_layout_widget::set_horizontal(bool horizontal) {
  if ($widget) {
    auto flex_widget = std::dynamic_pointer_cast<ui::widget_flex>($widget);
    if (flex_widget) {
      flex_widget->horizontal = horizontal;
    }
  }
}

void breeze_ui::js_flex_layout_widget::append_child(
    std::shared_ptr<js_widget> child) {
  if (!$widget)
    return;
  auto flex_widget = std::dynamic_pointer_cast<ui::widget_flex>($widget);
  if (!flex_widget)
    return;
  if (child && child->$widget) {
    flex_widget->add_child(
        std::dynamic_pointer_cast<ui::widget>(child->$widget));
  }
}

void breeze_ui::js_flex_layout_widget::prepend_child(
    std::shared_ptr<js_widget> child) {
  if (!$widget)
    return;
  auto flex_widget = std::dynamic_pointer_cast<ui::widget_flex>($widget);
  if (!flex_widget)
    return;
  if (child && child->$widget) {
    flex_widget->children.insert(
        flex_widget->children.begin(),
        std::dynamic_pointer_cast<ui::widget>(child->$widget));
  }
}

void breeze_ui::js_flex_layout_widget::remove_child(
    std::shared_ptr<js_widget> child) {
  if (!$widget)
    return;
  auto flex_widget = std::dynamic_pointer_cast<ui::widget_flex>($widget);
  if (!flex_widget)
    return;
  if (child && child->$widget) {
    auto it = std::find(flex_widget->children.begin(),
                        flex_widget->children.end(), child->$widget);
    if (it != flex_widget->children.end()) {
      flex_widget->children.erase(it);
    }
  }
}

std::vector<std::shared_ptr<breeze_ui::js_widget>>
breeze_ui::js_flex_layout_widget::children() const {
  std::vector<std::shared_ptr<js_widget>> result;
  if (!$widget)
    return result;

  auto flex_widget = std::dynamic_pointer_cast<ui::widget_flex>($widget);
  if (!flex_widget)
    return result;

  for (const auto &child : flex_widget->children) {
    result.push_back(std::make_shared<js_widget>(child));
  }
  return result;
}

std::shared_ptr<breeze_ui::js_text_widget>
breeze_ui::widgets_factory::create_text_widget() {
  auto text_widget = std::make_shared<ui::text_widget>();

  return std::make_shared<js_text_widget>(
      std::dynamic_pointer_cast<ui::widget>(text_widget));
}

std::shared_ptr<breeze_ui::js_flex_layout_widget>
breeze_ui::widgets_factory::create_flex_layout_widget() {
  auto layout_widget = std::make_shared<ui::widget_flex>();
  return std::make_shared<js_flex_layout_widget>(
      std::dynamic_pointer_cast<ui::widget>(layout_widget));
}

float breeze_ui::js_flex_layout_widget::get_padding_left() const {
  if (!$widget)
    return 0.0f;
  auto flex_widget = std::dynamic_pointer_cast<ui::widget_flex>($widget);
  if (!flex_widget)
    return 0.0f;
  return flex_widget->padding_left->dest();
}
void breeze_ui::js_flex_layout_widget::set_padding_left(float padding) {
  if (!$widget)
    return;
  auto flex_widget = std::dynamic_pointer_cast<ui::widget_flex>($widget);
  if (!flex_widget)
    return;
  flex_widget->padding_left->animate_to(padding);
}
float breeze_ui::js_flex_layout_widget::get_padding_right() const {
  if (!$widget)
    return 0.0f;
  auto flex_widget = std::dynamic_pointer_cast<ui::widget_flex>($widget);
  if (!flex_widget)
    return 0.0f;
  return flex_widget->padding_right->dest();
}
void breeze_ui::js_flex_layout_widget::set_padding_right(float padding) {
  if (!$widget)
    return;
  auto flex_widget = std::dynamic_pointer_cast<ui::widget_flex>($widget);
  if (!flex_widget)
    return;
  flex_widget->padding_right->animate_to(padding);
}
float breeze_ui::js_flex_layout_widget::get_padding_top() const {
  if (!$widget)
    return 0.0f;
  auto flex_widget = std::dynamic_pointer_cast<ui::widget_flex>($widget);
  if (!flex_widget)
    return 0.0f;
  return flex_widget->padding_top->dest();
}
void breeze_ui::js_flex_layout_widget::set_padding_top(float padding) {
  if (!$widget)
    return;
  auto flex_widget = std::dynamic_pointer_cast<ui::widget_flex>($widget);
  if (!flex_widget)
    return;
  flex_widget->padding_top->animate_to(padding);
}
float breeze_ui::js_flex_layout_widget::get_padding_bottom() const {
  if (!$widget)
    return 0.0f;
  auto flex_widget = std::dynamic_pointer_cast<ui::widget_flex>($widget);
  if (!flex_widget)
    return 0.0f;
  return flex_widget->padding_bottom->dest();
}
void breeze_ui::js_flex_layout_widget::set_padding_bottom(float padding) {
  if (!$widget)
    return;
  auto flex_widget = std::dynamic_pointer_cast<ui::widget_flex>($widget);
  if (!flex_widget)
    return;
  flex_widget->padding_bottom->animate_to(padding);
}
std::tuple<float, float, float, float>
breeze_ui::js_flex_layout_widget::get_padding() const {
  return {get_padding_left(), get_padding_right(), get_padding_top(),
          get_padding_bottom()};
}
void breeze_ui::js_flex_layout_widget::set_padding(float left, float right,
                                                   float top, float bottom) {
  set_padding_left(left);
  set_padding_right(right);
  set_padding_top(top);
  set_padding_bottom(bottom);
}
} // namespace mb_shell::js
