# 配置文件格式说明

本项目的配置文件采用 JSON 格式，推荐使用 VSCode 进行编辑。

本项目配置文件默认位于 `%USERPROFILE%/.breeze-shell/config.json`。

编辑配置文件并保存后，插件将会自动重载配置，无需重新启动。

**如果保存后弹出了黑窗口，这大概是因为你的配置文件有错误，请阅读黑窗口内的报错信息并修复错误**

## Schema

Breeze Shell 配置文件的 JSON Schema 位于
[resources/schema.json](./resources/schema.json)，在配置文件内写入

```json
{
  "$schema": "https://raw.githubusercontent.com/std-microblock/breeze-shell/refs/heads/master/resources/schema.json"
}
```

即可在 VSCode 中看到配置文件类型检查及补全。

## 配置文件结构

以下为一份带有注释的完整默认 JSON 配置，注意其**不能**直接填入 config.json
当中，因为配置文件解析当前不支持注释

```json5
{
  "context_menu": {
    "theme": {
      // 在 Windows 11 下使用 DWM 圆角而不是 SetWindowRgn 圆角
      "use_dwm_if_available": true,
      // 启用亚克力背景效果
      "acrylic": true,
      // 圆角大小，仅在不使用 DWM 圆角时生效
      "radius": 6.0,
      // 字体大小，可调整此项以对齐缩放后的整数倍率字体大小以避免模糊
      "font_size": 14.0,
      // 项高度
      "item_height": 23.0,
      // 项间距
      "item_gap": 3.0,
      // 项圆角大小
      "item_radius": 5.0,
      // 外边距
      "margin": 5.0,
      // 内边距
      "padding": 6.0,
      // 文笔内边距
      "text_padding": 8.0,
      // 图标内边距
      "icon_padding": 4.0,
      // 右侧图标（展开图标）边距
      "right_icon_padding": 20.0,
      // 横排按钮间距（此处为负值以抵消项边距的效果）
      "multibutton_line_gap": -6.0,
      // 在亮色主题下的亚克力背景颜色
      "acrylic_color_light": "#fefefe00",
      // 在暗色主题下的亚克力背景颜色
      "acrylic_color_dark": "#28282800",
      // 背景透明度
      "background_opacity":1.0,
      // 动画相关
      "animation": {
        // 菜单项动画
        "item": {
          // animated_float_conf: 通用动画配置
          "opacity": {
            // 持续时长
            "duration": 200.0,
            // 动画曲线
            // 可为：
            // mutation (关闭动画)
            // linear (线性)
            // ease_in, ease_out, ease_in_out (三种缓动曲线)
            "easing": "ease_in_out",
            // 对延迟时间的缩放
            // 即：如果本来是在开始总动画 50ms 后显示该动画，
            //     若 delay_scale 为 2 则在 100ms 后才显示
            "delay_scale": 1.0
          },
          // 同 opacity，以下均省略
          "x": animated_float_conf,
          "y": animated_float_conf,
          "width": animated_float_conf
        },
        // 主菜单的背景
        "main_bg": {
          "opacity": animated_float_conf,
          "x": animated_float_conf,
          "y": animated_float_conf,
          "w": animated_float_conf,
          "h": animated_float_conf
        },
        // 子菜单的背景，同主菜单
        "submenu_bg": {
          ...
        }
      },
      // 使用自绘边框、阴影（需关闭 dwm 边框）
      "use_self_drawn_border": true,
      // 自定义边框示例配置
      "border_width": 2.5,
      // 支持渐变 [linear-gradient(angle, color1, color2) / radial-gradient(radius, color1, color2)]
      "border_color_dark": "linear-gradient(30, #DE73DF, #E5C07B)",
      "shadow_size": 20,
      "shadow_color_dark_from": "#ff000033",
      "shadow_color_dark_to": "#00ff0000"
    },
    // 启用垂直同步
    "vsync": true,
    // 不替换 owner draw 的菜单
    "ignore_owner_draw": true,
    // 在向上展开时将所有项反向
    "reverse_if_open_to_up": true,
    // 调试选项，搜索更大范围的图标，不建议打开
    "search_large_dwItemData_range": false,
    // 定位相关
    "position": {
      // 竖直边距
      "padding_vertical": 20,
      // 水平边距
      "padding_horizontal": 0
    }
  },

  // 开启调试窗口
  "debug_console": false,

  // 主字体
  "font_path_main": "C:\\WINDOWS\\Fonts\\segoeui.ttf",
  // 副字体
  "font_path_fallback": "C:\\WINDOWS\\Fonts\\msyh.ttc",
  // 使用 hook 方式加载更多 resid
  "res_string_loader_use_hook": false,
  // 调试选项，避免更改 UI 窗口大小
  "avoid_resize_ui": false,
  // 插件加载顺序，在越前面的越先加载
  // 格式为插件的无拓展名文件名
  // 如：Windows 11 Icon Pack
  "plugin_load_order": [],
  // 全局默认动画效果
  "default_animation": animated_float_conf
}
```

## 示例配置文件

#### 禁用所有动画

```json
{
  "default_animation": {
    "easing": "mutation"
  }
}
```
