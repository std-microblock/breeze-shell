#pragma once
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>

namespace mb_shell {
struct menu_widget;
struct menu_info_basic {
  std::string from;
  std::shared_ptr<mb_shell::menu_widget> menu;
};
extern std::unordered_set<std::shared_ptr<std::function<void(menu_info_basic)>>>
    menu_callbacks;
} // namespace mb_shell

namespace mb_shell::js {
struct example_struct_jni {
  int a;
  int b;

  int add1(int a, int b) { return a + b; }
  std::variant<int, std::string> add2(std::string a, std::string b) {
    return a + b;
  }
  std::string c;
};

struct js_menu_action_event_data {};

struct js_menu_data {
  std::optional<std::string> type;
  std::optional<std::string> name;
  std::optional<std::vector<std::shared_ptr<mb_shell::js::js_menu_data>>>
      submenu;
  std::optional<std::function<void(mb_shell::js::js_menu_action_event_data)>>
      action;
  std::optional<std::string> icon_path;
};

struct menu_item_data {
  std::string type;
  std::optional<std::string> name;
};

struct menu_controller;
struct menu_info_basic_js {
  std::string from;
  std::shared_ptr<mb_shell::js::menu_controller> menu;
};

struct menu_controller {
  std::weak_ptr<mb_shell::menu_widget> $menu;

  bool valid();
  bool add_menu_item_after(mb_shell::js::js_menu_data data, int after_index);

  bool set_menu_item(int index, mb_shell::js::js_menu_data data);
  /*
    0 - a
    1 - b
    2 - c
    set_pos(0, 2)
    0 - b
    1 - c
    2 - a
   */
  bool set_menu_item_position(int index, int new_index);
  bool remove_menu_item(int index);

  std::vector<std::shared_ptr<mb_shell::js::menu_item_data>> get_menu_items();
  std::shared_ptr<mb_shell::js::menu_item_data> get_menu_item(int index);

  std::function<void()>
  add_menu_listener(std::function<void(mb_shell::js::menu_info_basic_js)> listener);
  std::unordered_set<void *> $listeners_to_dispose;
  ~menu_controller();
};
} // namespace mb_shell::js