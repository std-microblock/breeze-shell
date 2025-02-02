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

// 文件夹视图控制器
// Folder view controller
struct folder_view_controller {
  void *$hwnd;
  /* IShellBrowser* */
  void *$controller;
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
  // 聚焦到指定文件
  // Focus on specified file
  void focus_file(std::string file_path);
  // 打开文件
  // Open a file
  void open_file(std::string file_path);
  // 打开文件夹
  // Open a folder
  void open_folder(std::string folder_path);
  // 滚动到指定文件
  // Scroll to specified file
  void scroll_to_file(std::string file_path);
  // 刷新视图
  // Refresh view
  void refresh();
  // 全选
  // Select all items
  void select_all();
  // 取消全选
  // Deselect all items
  void select_none();
  // 反选
  // Invert selection
  void invert_selection();
  // 复制
  // Copy selected items
  void copy();
  // 剪切
  // Cut selected items
  void cut();
  // 粘贴
  // Paste items
  void paste();
};

// special flag struct to indicate that the corresponding
// value should be reset to none instead of unchanged
struct value_reset {};
#define WITH_RESET_OPTION(x) std::optional<std::variant<x, std::shared_ptr<mb_shell::js::value_reset>>>

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
  WITH_RESET_OPTION(
          std::function<void(std::shared_ptr<mb_shell::js::menu_controller>)>)
  submenu;
  // 菜单动作回调函数
  // Menu action callback function
  WITH_RESET_OPTION(
             std::function<void(mb_shell::js::js_menu_action_event_data)>)
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

  // 检查菜单控制器是否有效
  // Check if menu controller is valid
  bool valid();

  // 在指定索引后添加菜单项
  // Append menu item after specified index
  std::shared_ptr<mb_shell::js::menu_item_controller>
  append_menu_after(mb_shell::js::js_menu_data data, int after_index);

  // 在末尾添加菜单项
  // Append menu item at end
  std::shared_ptr<mb_shell::js::menu_item_controller>
  append_menu(mb_shell::js::js_menu_data data);

  // 在开头添加菜单项
  // Prepend menu item at beginning
  std::shared_ptr<mb_shell::js::menu_item_controller>
  prepend_menu(mb_shell::js::js_menu_data data);

  // 关闭菜单
  // Close menu
  void close();

  // 清除所有菜单项
  // Clear all menu items
  void clear();

  // 获取所有菜单项
  // Get all menu items
  std::vector<std::shared_ptr<mb_shell::js::menu_item_controller>> get_items();

  // 获取指定索引的菜单项
  // Get menu item at index
  std::shared_ptr<mb_shell::js::menu_item_controller> get_item(int index);

  // 添加菜单事件监听器
  // Add menu event listener
  static std::function<void()> add_menu_listener(
      std::function<void(mb_shell::js::menu_info_basic_js)> listener);

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
};

struct breeze {
  static std::string version();
  static std::string data_directory();
  static bool is_light_theme();
};

struct win32 {
  static std::string resid_from_string(std::string str);
  static size_t load_library(std::string path);
};

} // namespace mb_shell::js

namespace mb_shell {
extern std::unordered_set<
    std::shared_ptr<std::function<void(js::menu_info_basic_js)>>>
    menu_callbacks_js;
} // namespace mb_shell