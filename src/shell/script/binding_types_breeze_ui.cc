#include "../contextmenu/menu_widget.h"
#include "binding_types.h"
#include "ui.h"
#include "widget.h"
#include <memory>

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

void breeze_ui::js_widget::append_child_after(std::shared_ptr<js_widget> child,
                                              int after_index) {
  if (child && child->$widget) {
    $widget->children_dirty = true;
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
    }
  }
}

std::shared_ptr<breeze_ui::js_text_widget>
breeze_ui::widgets_factory::create_text_widget() {
  auto text_widget = std::make_shared<ui::text_widget>();

  auto res = std::make_shared<js_text_widget>();
  res->$widget = std::dynamic_pointer_cast<ui::widget>(text_widget);
  return res;
}

std::shared_ptr<breeze_ui::js_flex_layout_widget>
breeze_ui::widgets_factory::create_flex_layout_widget() {
  auto layout_widget = std::make_shared<ui::widget_flex>();
  auto res = std::make_shared<js_flex_layout_widget>();
  res->$widget = std::dynamic_pointer_cast<ui::widget>(layout_widget);
  return res;
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
// std::shared_ptr<breeze_ui::js_widget>
// breeze_ui::js_widget::clone(bool with_children) const {
//   auto new_widget = std::make_shared<js_widget>();

// #define TRY_CAST(type)                                                         \
//   if (auto casted = std::dynamic_pointer_cast<type>($widget)) {                \
//     new_widget =                                                               \
//         std::make_shared<js_widget>(std::dynamic_pointer_cast<ui::widget>(     \
//             std::make_shared<type>(*casted.get())));                           \
//   }

//   TRY_CAST(ui::widget_flex)
//   TRY_CAST(ui::text_widget)
// #undef TRY_CAST
//   if (!new_widget || !new_widget->$widget) {
//     return nullptr;
//   }

//   if (with_children) {
//     for (const auto &child : children()) {
//       if (auto cloned = child->clone())
//         new_widget->append_child(cloned);
//       else
//         return nullptr; // If any child fails to clone, return nullptr
//     }
//   }
//   return new_widget;
// }
std::variant<std::shared_ptr<breeze_ui::js_widget>,
             std::shared_ptr<breeze_ui::js_text_widget>,
             std::shared_ptr<breeze_ui::js_flex_layout_widget>>
breeze_ui::js_widget::downcast() {
#define TRY_DOWNCAST(type)                                                     \
  if (auto casted =                                                            \
          std::dynamic_pointer_cast<type>(this->shared_from_this())) {         \
    return casted;                                                             \
  }
  TRY_DOWNCAST(js_text_widget);
  TRY_DOWNCAST(js_flex_layout_widget);
#undef TRY_DOWNCAST

  return this->shared_from_this();
}
} // namespace mb_shell::js
