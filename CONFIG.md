# Configuration Guide

## Overview
This document describes the configuration options for customizing the context menu theme in the application. The configuration file follows a nested JSON structure and is stored at `%HOMEPATH%/.breeze-shell/config.json` by default.

## Configuration Structure
```json
{
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
      "margin": 5.0
    }
  }
}
```

## Configuration Parameters

### General Settings
| Key                    | Type    | Default | Description                                                                 |
|------------------------|---------|---------|-----------------------------------------------------------------------------|
| `use_dwm_if_available` | Boolean | `true`  | Enables Desktop Window Manager (DWM) border effects when available                 |
| `background_opacity`   | Float   | `1.0`   | Background transparency (0.0 = fully transparent, 1.0 = fully opaque)       |
| `acrylic`              | Boolean | `true`  | Enables acrylic blur effect for modern Windows visual styles                |

### Geometry Settings
| Key           | Type  | Default | Description                              |
|---------------|-------|---------|------------------------------------------|
| `radius`      | Float | `6.0`   | Corner radius for menu background (px)   |
| `item_radius` | Float | `100.0` | Corner radius for menu items (px)        |
| `margin`      | Float | `5.0`   | Outer margin around menu (px)            |
| `item_height` | Float | `25.0`  | Height of individual menu items (px)     |
| `item_gap`    | Float | `5.0`   | Vertical spacing between menu items (px) |

### Typography
| Key         | Type  | Default | Description               |
|-------------|-------|---------|---------------------------|
| `font_size` | Float | `14.0`  | Text size for menu items  |


## Example Configuration
```json
{
  "context_menu": {
    "theme": {
      "use_dwm_if_available": false,
      "background_opacity": 0.9,
      "acrylic": false,
      "radius": 8.0,
      "font_size": 15.0,
      "item_height": 28.0,
      "item_gap": 6.0,
      "item_radius": 8.0,
      "margin": 8.0
    }
  }
}
```

## Notes
1. Most changes can be applied without restarting. If not, restart to see if it works.
2. Percentage-based values use `0.0`-`1.0` range (0-100%)