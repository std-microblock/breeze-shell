#pragma once
#include <functional>
#include <memory>
#include <optional>
#include <stdlib.h>
#include <string>
#include <tuple>
#include <unordered_set>
#include <variant>
#include <vector>

#include "binding_types_breeze_ui.h"

namespace mb_shell {
struct mouse_menu_widget_main;
struct menu_item_widget;
struct menu_item_normal_widget;
struct menu_item_parent_widget;
struct menu_widget;
} // namespace mb_shell

namespace mb_shell::js {

struct folder_view_folder_item {
  // FolderItem
  void *$handler;
  /* IShellView* */
  void *$controller;
  void *$render_target;
  int index;
  std::string parent_path;

  std::string name();
  std::string modify_date();
  std::string path();
  size_t size();
  std::string type();
  /*
    (0) 取消选择该项。
    (1) 选择该项。
    (3) 将项目置于编辑模式。
    (4) 取消选择除指定项的所有项。
    (8) 确保该项显示在视图中。
    (16) 为项目提供焦点。
  */
  void select(int state);
};

// 文件夹视图控制器
// Folder view controller
struct folder_view_controller {
  void *$hwnd;
  /* IShellBrowser* */
  void *$controller;
  void *$render_target;
  // 当前文件夹路径
  // Current folder path
  std::string current_path;
  // 当前焦点文件路径
  // Currently focused file path
  std::string focused_file_path;
  // 选中的文件列表
  // List of selected files
  std::vector<std::string> selected_files;

  // 切换到新文件夹
  // Change to a new folder
  void change_folder(std::string new_folder_path);
  // 打开文件
  // Open a file
  void open_file(std::string file_path);
  // 打开文件夹
  // Open a folder
  void open_folder(std::string folder_path);
  // 刷新视图
  // Refresh view
  void refresh();
  // 复制
  // Copy selected items
  void copy();
  // 剪切
  // Cut selected items
  void cut();
  // 粘贴
  // Paste items
  void paste();
  // 获取项列表
  std::vector<std::shared_ptr<folder_view_folder_item>> items();

  void select(int index, int state);

  void select_none();
};

// special flag struct to indicate that the corresponding
// value should be reset to none instead of unchanged
struct value_reset {};
#define WITH_RESET_OPTION(x)                                                   \
  std::optional<std::variant<x, std::shared_ptr<value_reset>>>

// 窗口标题栏控制器
// Window titlebar controller
struct window_titlebar_controller {
  void *$hwnd;
  void *$controller;
  // 是否在标题栏中点击
  // Whether click is in titlebar
  bool is_click_in_titlebar;
  // 窗口标题
  // Window title
  std::string title;
  // 可执行文件路径
  // Executable path
  std::string executable_path;
  int hwnd;
  // 窗口位置和大小
  // Window position and size
  int x;
  int y;
  int width;
  int height;
  // 窗口状态
  // Window state
  bool maximized;
  bool minimized;
  bool focused;
  bool visible;

  // 设置窗口标题
  // Set window title
  void set_title(std::string new_title);
  // 设置窗口图标
  // Set window icon
  void set_icon(std::string icon_path);
  // 设置窗口位置
  // Set window position
  void set_position(int new_x, int new_y);
  // 设置窗口大小
  // Set window size
  void set_size(int new_width, int new_height);
  // 最大化窗口
  // Maximize window
  void maximize();
  // 最小化窗口
  // Minimize window
  void minimize();
  // 还原窗口
  // Restore window
  void restore();
  // 关闭窗口
  // Close window
  void close();
  // 聚焦窗口
  // Focus window
  void focus();
  // 显示窗口
  // Show window
  void show();
  // 隐藏窗口
  // Hide window
  void hide();
};

// 输入框控制器
// Input box controller
struct input_box_controller {
  void *$hwnd;
  void *$controller;
  // 输入框文本
  // Input box text
  std::string text;
  // 占位符文本
  // Placeholder text
  std::string placeholder;
  // 是否多行
  // Whether multiline
  bool multiline;
  // 是否密码框
  // Whether password field
  bool password;
  // 是否只读
  // Whether readonly
  bool readonly;
  // 是否禁用
  // Whether disabled
  bool disabled;
  // 输入框位置和大小
  // Input box position and size
  int x;
  int y;
  int width;
  int height;

  // 设置文本
  // Set text
  void set_text(std::string new_text);
  // 设置占位符
  // Set placeholder
  void set_placeholder(std::string new_placeholder);
  // 设置位置
  // Set position
  void set_position(int new_x, int new_y);
  // 设置大小
  // Set size
  void set_size(int new_width, int new_height);
  // 设置是否多行
  // Set multiline state
  void set_multiline(bool new_multiline);
  // 设置是否为密码框
  // Set password field state
  void set_password(bool new_password);
  // 设置是否只读
  // Set readonly state
  void set_readonly(bool new_readonly);
  // 设置是否禁用
  // Set disabled state
  void set_disabled(bool new_disabled);
  // 获取焦点
  // Get focus
  void focus();
  // 失去焦点
  // Lose focus
  void blur();
  // 全选文本
  // Select all text
  void select_all();
  // 选择文本范围
  // Select text range
  void select_range(int start, int end);
  // 设置选择范围
  // Set selection range
  void set_selection(int start, int end);
  // 插入文本
  // Insert text
  void insert_text(std::string new_text);
  // 删除文本
  // Delete text
  void delete_text(int start, int end);
  // 清空文本
  // Clear text
  void clear();
};

struct menu_controller;

// 菜单动作事件数据
// Menu action event data
struct js_menu_action_event_data {};

// 菜单数据结构
// Menu data structure
struct js_menu_data {
  // 菜单项类型
  // Menu item type
  std::optional<std::string> type;
  // 菜单项名称
  // Menu item name
  std::optional<std::string> name;
  // 子菜单回调函数
  // Submenu callback function
  WITH_RESET_OPTION(std::function<void(std::shared_ptr<menu_controller>)>)
  submenu;
  // 菜单动作回调函数
  // Menu action callback function
  WITH_RESET_OPTION(std::function<void(js_menu_action_event_data)>)
  action;
  // SVG图标
  // SVG icon
  WITH_RESET_OPTION(std::string) icon_svg;
  // 位图图标
  // Bitmap icon
  WITH_RESET_OPTION(size_t) icon_bitmap;
  // 是否禁用
  // Whether disabled
  std::optional<bool> disabled;
  // 仅作为信息标识，不影响行为
  // Only for information, set this changes nothing
  std::optional<int64_t> wID;
  std::optional<std::string> name_resid;
  std::optional<std::string> origin_name;
};

struct menu_item_controller {
  std::weak_ptr<mb_shell::menu_item_widget> $item;
  std::variant<std::weak_ptr<mb_shell::menu_widget>,
               std::weak_ptr<mb_shell::menu_item_parent_widget>>
      $parent;
  void set_position(int new_index);
  void set_data(js_menu_data data);
  js_menu_data data();
  void remove();
  bool valid();
};

struct menu_item_parent_item_controller {
  std::weak_ptr<mb_shell::menu_item_parent_widget> $item;
  std::weak_ptr<mb_shell::menu_widget> $menu;
  std::vector<std::shared_ptr<menu_item_controller>> children();
  void set_position(int new_index);
  void remove();
  bool valid();

  std::shared_ptr<menu_item_controller> append_child_after(js_menu_data data,
                                                           int after_index);

  inline std::shared_ptr<menu_item_controller> append_child(js_menu_data data) {
    return append_child_after(data, -1);
  }

  inline std::shared_ptr<menu_item_controller>
  prepend_child(js_menu_data data) {
    return append_child_after(data, 0);
  }
};

struct window_prop_data {
  std::string key;
  std::variant<size_t, std::string> value;
};

struct caller_window_data {
  std::vector<window_prop_data> props;
  int x;
  int y;
  int width;
  int height;
  bool maximized;
  bool minimized;
  bool focused;
  bool visible;
  std::string executable_path;
  std::string title;
};

struct js_menu_context {
  std::optional<std::shared_ptr<folder_view_controller>> folder_view;
  std::optional<std::shared_ptr<window_titlebar_controller>> window_titlebar;
  std::optional<std::shared_ptr<input_box_controller>> input_box;
  caller_window_data window_info;
  // 获取当前活动的窗口或指针下的窗口的数据
  static js_menu_context $from_window(void *hwnd);
};

struct menu_controller;
struct menu_info_basic_js {
  std::shared_ptr<menu_controller> menu;
  std::shared_ptr<js_menu_context> context;
};

struct menu_controller {
  std::weak_ptr<mb_shell::menu_widget> $menu;
  std::shared_ptr<mb_shell::menu_widget> $menu_detached;

  // 检查菜单控制器是否有效
  // Check if menu controller is valid
  bool valid();

  // 在指定索引后添加菜单项
  // Append menu item after specified index
  std::shared_ptr<menu_item_controller> append_item_after(js_menu_data data,
                                                          int after_index);

  void append_widget_after(
      std::shared_ptr<mb_shell::js::breeze_ui::js_widget> widget,
      int after_index);

  // 在指定索引后添加水平菜单母项
  std::shared_ptr<menu_item_parent_item_controller>
  append_parent_item_after(int after_index);

  // 在末尾添加水平菜单母项
  inline std::shared_ptr<menu_item_parent_item_controller>
  append_parent_item() {
    return append_parent_item_after(-1);
  }

  // 在开头添加水平菜单母项
  inline std::shared_ptr<menu_item_parent_item_controller>
  prepend_parent_item() {
    return append_parent_item_after(0);
  }

  // 在末尾添加菜单项
  // Append menu item at end
  inline std::shared_ptr<menu_item_controller> append_item(js_menu_data data) {
    return append_item_after(data, -1);
  }

  // 在开头添加菜单项
  // Prepend menu item at beginning
  inline std::shared_ptr<menu_item_controller> prepend_item(js_menu_data data) {
    return append_item_after(data, 0);
  }

  // 在开头添加 Spacer
  // Prepend Spacer
  inline void prepend_spacer() { prepend_item({.type = "spacer"}); }

  // 在末尾添加 Spacer
  // Append Spacer
  inline void append_spacer() { append_item({.type = "spacer"}); }

  // 关闭菜单
  // Close menu
  void close();

  // 清除所有菜单项
  // Clear all menu items
  void clear();

  // 获取所有菜单项
  // Get all menu items
  std::vector<std::shared_ptr<menu_item_controller>> get_items();

  // 获取指定索引的菜单项
  // Get menu item at index
  std::shared_ptr<menu_item_controller> get_item(int index);

  // 添加菜单事件监听器
  // Add menu event listener
  static std::function<void()>
  add_menu_listener(std::function<void(menu_info_basic_js)> listener);

  // Only for compatibility
  inline std::shared_ptr<menu_item_controller> prepend_menu(js_menu_data data) {
    return append_item_after(data, 0);
  }
  inline std::shared_ptr<menu_item_controller> append_menu(js_menu_data data) {
    return append_item_after(data, -1);
  }
  inline std::shared_ptr<menu_item_controller>
  append_menu_after(js_menu_data data, int after_index) {
    return append_item_after(data, after_index);
  }

  static std::shared_ptr<menu_controller> create_detached();

  ~menu_controller();
};

// 系统剪贴板操作
// System clipboard operations
struct clipboard {
  // 从剪贴板获取文本
  // Get text from clipboard
  static std::string get_text();

  // 设置文本到剪贴板
  // Set text to clipboard
  static void set_text(std::string text);
};

// 网络操作
// Network operations
struct network {
  // 同步HTTP GET请求
  // Synchronous HTTP GET request
  static std::string get(std::string url);

  // 同步HTTP POST请求
  // Synchronous HTTP POST request
  static std::string post(std::string url, std::string data);

  // 异步HTTP GET请求
  // Asynchronous HTTP GET request
  static void get_async(std::string url,
                        std::function<void(std::string)> callback,
                        std::function<void(std::string)> error_callback);

  // 异步HTTP POST请求
  // Asynchronous HTTP POST request
  static void post_async(std::string url, std::string data,
                         std::function<void(std::string)> callback,
                         std::function<void(std::string)> error_callback);

  // 下载文件
  // Download file
  static void download_async(std::string url, std::string path,
                             std::function<void()> callback,
                             std::function<void(std::string)> error_callback);
};

// 子进程执行结果
// Subprocess execution result
struct subproc_result_data {
  // 标准输出
  // Standard output
  std::string out;

  // 标准错误
  // Standard error
  std::string err;

  // 退出码
  // Exit code
  int code;
};

// 子进程操作
// Subprocess operations
struct subproc {
  // 同步运行命令
  // Run command synchronously
  static subproc_result_data run(std::string cmd);

  // 异步运行命令
  // Run command asynchronously
  static void run_async(std::string cmd,
                        std::function<void(subproc_result_data)> callback);

  // 同步打开东西
  // Open something synchronously
  static void open(std::string path, std::string args = "");

  // 异步打开东西
  // Open something asynchronously
  static void open_async(std::string path, std::string args,
                         std::function<void()> callback);
};

// 文件系统操作
// File system operations
struct fs {
  // 获取当前工作目录
  // Get current working directory
  static std::string cwd();

  // 设置当前工作目录
  // Set current working directory
  static void chdir(std::string path);

  // 判断路径是否存在
  // Check if path exists
  static bool exists(std::string path);

  // 判断路径是否为目录
  // Check if path is directory
  static bool isdir(std::string path);

  // 创建目录
  // Create directory
  static void mkdir(std::string path);

  // 删除目录
  // Remove directory
  static void rmdir(std::string path);

  // 重命名文件或目录
  // Rename file or directory
  static void rename(std::string old_path, std::string new_path);

  // 删除文件
  // Remove file
  static void remove(std::string path);

  // 复制文件
  // Copy file
  static void copy(std::string src_path, std::string dest_path);

  // 移动文件
  // Move file
  static void move(std::string src_path, std::string dest_path);

  // 读取文件
  // Read file
  static std::string read(std::string path);

  // 写入文件
  // Write file
  static void write(std::string path, std::string data);

  // 以二进制模式读取文件
  // Read file in binary mode
  static std::vector<uint8_t> read_binary(std::string path);

  // 以二进制模式写入文件
  // Write file in binary mode
  static void write_binary(std::string path, std::vector<uint8_t> data);

  // 读取目录
  // Read directory
  static std::vector<std::string> readdir(std::string path);

  // 使用 SHFileOperation 拷贝文件/文件夹
  // Copy file with SHFileOperation
  // 这会模拟资源管理器中“复制”的行为，即显示进度窗口，UAC请求窗口等
  static void copy_shfile(std::string src_path, std::string dest_path,
                          std::function<void(bool, std::string)> callback);

  // 使用 SHFileOperation 移动文件/文件夹
  // Move file with SHFileOperation
  // 这会模拟资源管理器中“移动”的行为，即显示进度窗口，UAC请求窗口等
  static void move_shfile(std::string src_path, std::string dest_path,
                          std::function<void(bool)> callback);

  // 监测文件/文件夹变动
  // Watch file/folder changes
  // added 0
  // removed 1
  // modified 2
  // renamed_old 3
  // renamed_new 4
  static std::function<void()>
  watch(std::string path, std::function<void(std::string, int)> callback);
};

struct breeze {
  static std::string version();
  static std::string hash();
  static std::string branch();
  static std::string build_date();
  static std::string data_directory();
  static bool is_light_theme();
  static std::string user_language();
};

struct win32 {
  static std::string resid_from_string(std::string str);
  static std::string string_from_resid(std::string str);
  static std::vector<std::string> all_resids_from_string(std::string str);
  static size_t load_library(std::string path);
  static std::optional<std::string> env(std::string name);
  static size_t load_file_icon(std::string path);

  static int32_t reg_get_dword(std::string key, std::string name);
  static std::string reg_get_string(std::string key, std::string name);
  static int64_t reg_get_qword(std::string key, std::string name);
  static void reg_set_dword(std::string key, std::string name, int32_t value);
  static void reg_set_string(std::string key, std::string name,
                             std::string value);
  static void reg_set_qword(std::string key, std::string name, int64_t value);

  static bool is_key_down(std::string key);
};

struct notification {
  static void send_basic(std::string message);
  static void send_with_image(std::string message, std::string path);
  static void send_title_text(std::string title, std::string message,
                              std::string image_path);
  static void send_with_buttons(
      std::string title, std::string message,
      std::vector<std::pair<std::string, std::function<void()>>> buttons);
};

struct infra {
  static int setTimeout(std::function<void()> callback, int delay);
  static void clearTimeout(int id);
  static int setInterval(std::function<void()> callback, int delay);
  static void clearInterval(int id);

  static std::string atob(std::string base64);
  static std::string btoa(std::string str);
};

} // namespace mb_shell::js

namespace mb_shell {
extern std::unordered_set<
    std::shared_ptr<std::function<void(js::menu_info_basic_js)>>>
    menu_callbacks_js;
} // namespace mb_shell