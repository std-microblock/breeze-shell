[中文 →](./CONFIG_zh.md)

# Configuration File Format Description

This project's configuration file adopts the JSON format, and it is recommended to use **VSCode** for editing.

The default location of the configuration file is:  
`%USERPROFILE%/.breeze-shell/config.json`.  

When the configuration file is saved, the plugin will automatically reload the configuration without requiring a restart.

**If a black window appears after saving, it indicates an error in the configuration file. Please read the error message in the black window and fix the issue accordingly.**

---

## Schema

The JSON Schema for the Breeze Shell configuration file is located at  
[resources/schema.json](./resources/schema.json). To enable type checking and autocompletion in VSCode, add the following line to your configuration file:

```json
{
  "$schema": "https://raw.githubusercontent.com/std-microblock/breeze-shell/refs/heads/master/resources/schema.json"
}
```

This allows VSCode to provide real-time validation and suggestions .

---

## Configuration File Structure

The following is a fully annotated default JSON configuration. Note that this **cannot** be directly copied into `config.json` as JSON does not support comments.

```json5
{
  "context_menu": {
    "theme": {
      // Use DWM-rounded corners instead of SetWindowRgn rounded corners on Windows 11
      "use_dwm_if_available": true,
      // Enable acrylic background effect
      "acrylic": true,
      // Corner radius (only effective when DWM-rounded corners are not used)
      "radius": 6.0,
      // Font size (adjust to align with scaled integer font sizes to avoid blurring)
      "font_size": 14.0,
      // Item height
      "item_height": 23.0,
      // Item spacing
      "item_gap": 3.0,
      // Item corner radius
      "item_radius": 5.0,
      // Margin
      "margin": 5.0,
      // Padding
      "padding": 6.0,
      // Text padding
      "text_padding": 8.0,
      // Icon padding
      "icon_padding": 4.0,
      // Right icon (expand icon) padding
      "right_icon_padding": 20.0,
      // Horizontal button spacing (negative value to offset item spacing)
      "multibutton_line_gap": -6.0,
      // Acrylic background color in light themes
      "acrylic_color_light": "#fefefe00",
      // Acrylic background color in dark themes
      "acrylic_color_dark": "#28282800",
      // Background opacity
      "background_opacity": 1.0,
      // Animation settings
      "animation": {
        // Menu item animations
        "item": {
          // animated_float_conf: General animation configuration
          "opacity": {
            // Duration in milliseconds
            "duration": 200.0,
            // Animation curve
            // Options: 
            // mutation (disable animation), 
            // linear (linear), 
            // ease_in, ease_out, ease_in_out (easing curves)
            "easing": "ease_in_out",
            // Delay scaling factor
            // Example: If the original delay is 50ms, 
            // a delay_scale of 2 results in a 100ms delay
            "delay_scale": 1.0
          },
          // Same structure as opacity for x, y, width
          "x": animated_float_conf,
          "y": animated_float_conf,
          "width": animated_float_conf
        },
        // Main menu background animation
        "main_bg": {
          "opacity": animated_float_conf,
          "x": animated_float_conf,
          "y": animated_float_conf,
          "w": animated_float_conf,
          "h": animated_float_conf
        },
        // Submenu background animation (same as main menu)
        "submenu_bg": {
          ...
        }
      },
      // Use custom-drawn border/shadow (disable DWM border)
      "use_self_drawn_border": true,
      // Custom border width example
      "border_width": 2.5,
      // Gradient support: [linear-gradient(angle, color1, color2) / radial-gradient(radius, color1, color2)]
      "border_color_dark": "linear-gradient(30, #DE73DF, #E5C07B)",
      "shadow_size": 20,
      "shadow_color_dark_from": "#ff000033",
      "shadow_color_dark_to": "#00ff0000"
    },
    // Enable vertical sync
    "vsync": true,
    // Do not replace owner-drawn menus
    "ignore_owner_draw": true,
    // Reverse all items when expanding upward
    "reverse_if_open_to_up": true,
    // Debug option: search a larger range for icons (not recommended)
    "search_large_dwItemData_range": false,
    // Positioning settings
    "position": {
      // Vertical padding
      "padding_vertical": 20,
      // Horizontal padding
      "padding_horizontal": 0
    },
    // Enable hotkeys
    "hotkeys": true
  },

  // Enable debug console
  "debug_console": false,

  // Primary font path
  "font_path_main": "C:\\WINDOWS\\Fonts\\segoeui.ttf",
  // Fallback font path
  "font_path_fallback": "C:\\WINDOWS\\Fonts\\msyh.ttc",
  // Use hook to load additional resource strings
  "res_string_loader_use_hook": false,
  // Debug option: avoid resizing UI windows
  "avoid_resize_ui": false,
  // Plugin load order (plugins listed first load earlier)
  // Format: plugin filename without extension
  // Example: "Windows 11 Icon Pack"
  "plugin_load_order": [],
  // Global default animation
  "default_animation": animated_float_conf
}
```

---

## Example Configuration

### Disable All Animations

```json
{
  "default_animation": {
    "easing": "mutation"
  }
}
```

This configuration disables all animations by setting the easing curve to `mutation` .
