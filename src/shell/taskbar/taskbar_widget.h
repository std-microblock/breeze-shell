#pragma once
#include "../config.h"
#include "animator.h"
#include "extra_widgets.h"
#include "hbitmap_utils.h"
#include "nanovg_wrapper.h"
#include "taskbar.h"
#include "ui.h"
#include "widget.h"
#include <algorithm>
#include <functional>
#include <memory>
#include <optional>

namespace mb_shell::taskbar {
struct window_info {
  HWND hwnd;
  std::string title;
  std::string class_name;
  HICON icon_handle;
  bool active;

  bool operator==(const window_info &other) const {
    return hwnd == other.hwnd && title == other.title &&
           class_name == other.class_name && icon_handle == other.icon_handle &&
           active == other.active;
  }
};

struct window_stack_info {
  std::vector<window_info> windows;
  bool active;

  // if there are at least one window same in both stacks, they are same stack
  bool is_same(const window_stack_info &other) const {
    if (windows.empty() || other.windows.empty()) {
      return false;
    }

    for (const auto &win : windows) {
      if (std::find(other.windows.begin(), other.windows.end(), win) !=
          other.windows.end()) {
        return true;
      }
    }
    return false;
  }
};

std::vector<window_info> get_window_list();
std::vector<window_stack_info> get_window_stacks();

struct app_list_stack_widget : public ui::widget {
  window_stack_info stack;

  std::optional<ui::NVGImage> icon;
  app_list_stack_widget(const window_stack_info &stack) : stack(stack) {}

  void render(ui::nanovg_context ctx) override;

  void update_stack(const window_stack_info &new_stack) {
    stack = new_stack;

    // Reset icon to reload it next time
    icon.reset();
  }

  void update(ui::update_context &ctx) override {
    ui::widget::update(ctx);
    width->reset_to(40);
    height->reset_to(40);
  }
};

struct app_list_widget : public ui::widget_flex {
  std::vector<
      std::pair<std::shared_ptr<app_list_stack_widget>, window_stack_info>>
      stacks;

  app_list_widget() {
    horizontal = true;
    gap = 10;
  }

  void update_stacks() {
    auto new_stacks = get_window_stacks();
    std::println("Updating app list stacks, found {} stacks",
                 new_stacks.size());
    // firstly, try to match existing stacks with new ones
    for (auto &new_stack : new_stacks) {
      auto it = std::find_if(stacks.begin(), stacks.end(),
                             [&new_stack](const auto &pair) {
                               return pair.second.is_same(new_stack);
                             });
      if (it != stacks.end()) {
        // Found existing stack, update it
        it->second = new_stack;
        it->first->update_stack(new_stack);
      } else {
        // New stack, create a widget for it
        std::println("Adding new stack with {} windows",
                     new_stack.windows.size());
        auto widget = std::make_shared<app_list_stack_widget>(new_stack);
        stacks.emplace_back(widget, new_stack);
        add_child(widget);
      }
    }

    // Remove stacks that are no longer present
    stacks.erase(std::remove_if(stacks.begin(), stacks.end(),
                                [&new_stacks](const auto &pair) {
                                  return std::none_of(
                                      new_stacks.begin(), new_stacks.end(),
                                      [&pair](const auto &new_stack) {
                                        return pair.second.is_same(new_stack);
                                      });
                                }),
                 stacks.end());
  }
};

struct taskbar_widget : public ui::widget_flex {
  taskbar_widget() {
    auto app_list = emplace_child<app_list_widget>();
    app_list->update_stacks();
  }
};
} // namespace mb_shell::taskbar