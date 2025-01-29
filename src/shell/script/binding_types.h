#pragma once
#include <functional>
#include <memory>
#include <optional>
#include <stdlib.h>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>

namespace mb_shell {
struct mouse_menu_widget_main;
struct menu_item_widget;
struct menu_widget;
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

struct folder_view_controller {
  void *$hwnd;
  /* IShellBrowser* */
  void *$controller;
  std::string current_path;
  std::string focused_file_path;
  std::vector<std::string> selected_files;

  void change_folder(std::string new_folder_path);
  void focus_file(std::string file_path);
  void open_file(std::string file_path);
  void open_folder(std::string folder_path);
  void scroll_to_file(std::string file_path);
  void refresh();
  void select_all();
  void select_none();
  void invert_selection();
  void copy();
  void cut();
  void paste();
};

struct window_titlebar_controller {
  void *$hwnd;
  void *$controller;
  bool is_click_in_titlebar;
  std::string title;
  std::string executable_path;
  int hwnd;
  int x;
  int y;
  int width;
  int height;
  bool maximized;
  bool minimized;
  bool focused;
  bool visible;

  void set_title(std::string new_title);
  void set_icon(std::string icon_path);
  void set_position(int new_x, int new_y);
  void set_size(int new_width, int new_height);
  void maximize();
  void minimize();
  void restore();
  void close();
  void focus();
  void show();
  void hide();
};

struct input_box_controller {
  void *$hwnd;
  void *$controller;
  std::string text;
  std::string placeholder;
  bool multiline;
  bool password;
  bool readonly;
  bool disabled;
  int x;
  int y;
  int width;
  int height;

  void set_text(std::string new_text);
  void set_placeholder(std::string new_placeholder);
  void set_position(int new_x, int new_y);
  void set_size(int new_width, int new_height);
  void set_multiline(bool new_multiline);
  void set_password(bool new_password);
  void set_readonly(bool new_readonly);
  void set_disabled(bool new_disabled);
  void focus();
  void blur();
  void select_all();
  void select_range(int start, int end);
  void set_selection(int start, int end);
  void insert_text(std::string new_text);
  void delete_text(int start, int end);
  void clear();
};
struct menu_controller;
struct js_menu_action_event_data {};

struct js_menu_data {
  std::optional<std::string> type;
  std::optional<std::string> name;
  std::optional<std::function<void(std::shared_ptr<mb_shell::js::menu_controller>)>> submenu;
  std::optional<std::function<void(mb_shell::js::js_menu_action_event_data)>>
      action;
  std::optional<std::string> icon_svg;
  std::optional<size_t> icon_bitmap;
};

struct menu_item_controller {
  std::weak_ptr<mb_shell::menu_item_widget> $item;
  std::weak_ptr<mb_shell::menu_widget> $menu;
  void set_position(int new_index);
  void set_data(mb_shell::js::js_menu_data data);
  js_menu_data data();
  void remove();
  bool valid();
};

struct menu_item_data {
  std::string type;
  std::optional<std::string> name;
};

struct js_menu_context {
  std::optional<std::shared_ptr<mb_shell::js::folder_view_controller>>
      folder_view;
  std::optional<std::shared_ptr<mb_shell::js::window_titlebar_controller>>
      window_titlebar;
  std::optional<std::shared_ptr<mb_shell::js::input_box_controller>> input_box;
  // 获取当前活动的窗口或指针下的窗口的数据
  static js_menu_context $from_window(void *hwnd);
};

struct menu_controller;
struct menu_info_basic_js {
  std::shared_ptr<mb_shell::js::menu_controller> menu;
  std::shared_ptr<mb_shell::js::js_menu_context> context;
};

struct menu_controller {
  std::weak_ptr<mb_shell::menu_widget> $menu;

  bool valid();
  std::shared_ptr<mb_shell::js::menu_item_controller>
  append_menu_after(mb_shell::js::js_menu_data data, int after_index);

  std::shared_ptr<mb_shell::js::menu_item_controller>
  append_menu(mb_shell::js::js_menu_data data);

  std::shared_ptr<mb_shell::js::menu_item_controller>
  prepend_menu(mb_shell::js::js_menu_data data);

  /*
    0 - a
    1 - b
    2 - c
    set_pos(0, 2)
    0 - b
    1 - c
    2 - a
   */

  void close();
  void clear();
  std::vector<std::shared_ptr<mb_shell::js::menu_item_controller>>
  get_items();
  std::shared_ptr<mb_shell::js::menu_item_controller> get_item(int index);

  static std::function<void()> add_menu_listener(
      std::function<void(mb_shell::js::menu_info_basic_js)> listener);
  ~menu_controller();
};

// system api bindings
struct clipboard {
  static std::string get_text();
  static void set_text(std::string text);
};

struct network {
  static std::string get(std::string url);
  static std::string post(std::string url, std::string data);
  static void get_async(std::string url,
                        std::function<void(std::string)> callback);
  static void post_async(std::string url, std::string data,
                         std::function<void(std::string)> callback);
};

struct subproc_result_data {
  std::string out;
  std::string err;
  int code;
};

struct subproc {
  static subproc_result_data run(std::string cmd);
  static void run_async(std::string cmd,
                        std::function<void(subproc_result_data)> callback);
};

} // namespace mb_shell::js

namespace mb_shell {
extern std::unordered_set<
    std::shared_ptr<std::function<void(js::menu_info_basic_js)>>>
    menu_callbacks_js;
} // namespace mb_shell