# 配置文件格式说明

本项目的配置文件采用 JSON
格式，用于定义应用程序的运行时配置。以下是配置文件的详细说明。

## 配置文件结构

```json
{
  "default_main_font": "<path_to_main_font>",
  "default_fallback_font": "<path_to_fallback_font>",
  "context_menu": {
    "theme": {
      "use_dwm_if_available": true,
      "background_opacity": 1.0,
      "acrylic": true,
      "radius": 6.0,
      "font_size": 14.0,
      "item_height": 25.0,
      "item_gap": 5.0,
      "item_radius": 100.0,
      "margin": 5.0,
      "acrylic_opacity": 0.1,
      "animation": {
        "item": {
          "opacity": {
            "duration": 200.0,
            "easing": "ease_in_out",
            "delay_scale": 1.0
          },
          "x": {
            "duration": 200.0,
            "easing": "ease_in_out",
            "delay_scale": 1.0
          }
        },
        "main_bg": {
          "opacity": {
            "duration": 200.0,
            "easing": "ease_in_out",
            "delay_scale": 1.0
          }
        },
        "submenu_bg": {
          "opacity": {
            "duration": 200.0,
            "easing": "ease_in_out",
            "delay_scale": 1.0
          }
        }
      }
    },
    "vsync": true,
    "position": {
      "padding_vertical": 20,
      "padding_horizontal": 0
    }
  },
  "debug_console": false
}
```

## 字段说明

### `default_main_font`

- **类型**: 字符串 (文件路径)
- **说明**: 默认主字体文件的路径。

### `default_fallback_font`

- **类型**: 字符串 (文件路径)
- **说明**: 默认备用字体文件的路径。

### `context_menu`

- **类型**: 对象
- **说明**: 上下文菜单的配置。

#### `theme`

- **类型**: 对象
- **说明**: 上下文菜单的主题配置。

##### `use_dwm_if_available`

- **类型**: 布尔值
- **说明**: 是否在可用时使用 DWM（Desktop Window Manager）效果。

##### `background_opacity`

- **类型**: 浮点数
- **说明**: 背景透明度，取值范围为 `0.0` 到 `1.0`。

##### `acrylic`

- **类型**: 布尔值
- **说明**: 是否启用亚克力效果。

##### `radius`

- **类型**: 浮点数
- **说明**: 圆角半径。

##### `font_size`

- **类型**: 浮点数
- **说明**: 字体大小。

##### `item_height`

- **类型**: 浮点数
- **说明**: 菜单项的高度。

##### `item_gap`

- **类型**: 浮点数
- **说明**: 菜单项之间的间距。

##### `item_radius`

- **类型**: 浮点数
- **说明**: 菜单项的圆角半径。

##### `margin`

- **类型**: 浮点数
- **说明**: 菜单的外边距。

##### `acrylic_opacity`

- **类型**: 浮点数
- **说明**: 亚克力效果的透明度，取值范围为 `0.0` 到 `1.0`。

##### `animation`

- **类型**: 对象
- **说明**: 动画效果的配置。

###### `item`

- **类型**: 对象
- **说明**: 菜单项的动画配置。

####### `opacity`

- **类型**: 对象
- **说明**: 透明度动画配置。

######## `duration`

- **类型**: 浮点数
- **说明**: 动画持续时间（毫秒）。

######## `easing`

- **类型**: 字符串
- **说明**: 动画缓动类型，可选值为 `ease_in_out` 等。

######## `delay_scale`

- **类型**: 浮点数
- **说明**: 动画延迟比例。

####### `x`

- **类型**: 对象
- **说明**: X 轴位移动画配置。

######## `duration`

- **类型**: 浮点数
- **说明**: 动画持续时间（毫秒）。

######## `easing`

- **类型**: 字符串
- **说明**: 动画缓动类型，可选值为 `ease_in_out` 等。

######## `delay_scale`

- **类型**: 浮点数
- **说明**: 动画延迟比例。

###### `bg`

- **类型**: 对象
- **说明**: 背景动画配置。

####### `main_bg`

- **类型**: 对象
- **说明**: 主背景的动画配置。

######## `opacity`

- **类型**: 对象
- **说明**: 透明度动画配置。

######### `duration`

- **类型**: 浮点数
- **说明**: 动画持续时间（毫秒）。

######### `easing`

- **类型**: 字符串
- **说明**: 动画缓动类型，可选值为 `ease_in_out` 等。

######### `delay_scale`

- **类型**: 浮点数
- **说明**: 动画延迟比例。

####### `submenu_bg`

- **类型**: 对象
- **说明**: 子菜单背景的动画配置。

######## `opacity`

- **类型**: 对象
- **说明**: 透明度动画配置。

######### `duration`

- **类型**: 浮点数
- **说明**: 动画持续时间（毫秒）。

######### `easing`

- **类型**: 字符串
- **说明**: 动画缓动类型，可选值为 `ease_in_out` 等。

######### `delay_scale`

- **类型**: 浮点数
- **说明**: 动画延迟比例。

#### `vsync`

- **类型**: 布尔值
- **说明**: 是否启用垂直同步。

#### `position`

- **类型**: 对象
- **说明**: 菜单位置的配置。

##### `padding_vertical`

- **类型**: 整数
- **说明**: 垂直方向的内边距。

##### `padding_horizontal`

- **类型**: 整数
- **说明**: 水平方向的内边距。

### `debug_console`

- **类型**: 布尔值
- **说明**: 是否启用调试控制台。

```
```
