// Generated from project in ./ts
// Don't edit this file directly!

import * as __mshell from "mshell";
const setTimeout = __mshell.infra.setTimeout;
const clearTimeout = __mshell.infra.clearTimeout;


// src/entry.ts
import * as shell5 from "mshell";

// src/menu/constants.ts
import * as shell from "mshell";
var languages = {
  "zh-CN": {},
  "en-US": {
    "\u7BA1\u7406 Breeze Shell": "Manage Breeze Shell",
    "\u63D2\u4EF6\u5E02\u573A / \u66F4\u65B0\u672C\u4F53": "Plugin Market / Update Shell",
    "\u52A0\u8F7D\u4E2D...": "Loading...",
    "\u66F4\u65B0\u4E2D...": "Updating...",
    "\u65B0\u7248\u672C\u5DF2\u4E0B\u8F7D\uFF0C\u5C06\u4E8E\u4E0B\u6B21\u91CD\u542F\u8D44\u6E90\u7BA1\u7406\u5668\u751F\u6548": "New version downloaded, will take effect next time the file manager is restarted",
    "\u66F4\u65B0\u5931\u8D25: ": "Update failed: ",
    "\u63D2\u4EF6\u5B89\u88C5\u6210\u529F: ": "Plugin installed: ",
    "\u5F53\u524D\u6E90: ": "Current source: ",
    "\u5220\u9664": "Delete",
    "\u7248\u672C: ": "Version: ",
    "\u4F5C\u8005: ": "Author: "
  }
};
var ICON_EMPTY = new shell.value_reset();
var ICON_CHECKED = `<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" viewBox="0 0 12 12"><path fill="currentColor" d="M9.765 3.205a.75.75 0 0 1 .03 1.06l-4.25 4.5a.75.75 0 0 1-1.075.015L2.22 6.53a.75.75 0 0 1 1.06-1.06l1.705 1.704l3.72-3.939a.75.75 0 0 1 1.06-.03"/></svg>`;
var ICON_CHANGE = `<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" viewBox="0 0 24 24"><path fill="currentColor" d="m17.66 9.53l-7.07 7.07l-4.24-4.24l1.41-1.41l2.83 2.83l5.66-5.66zM4 12c0-2.33 1.02-4.42 2.62-5.88L9 8.5v-6H3l2.2 2.2C3.24 6.52 2 9.11 2 12c0 5.19 3.95 9.45 9 9.95v-2.02c-3.94-.49-7-3.86-7-7.93m18 0c0-5.19-3.95-9.45-9-9.95v2.02c3.94.49 7 3.86 7 7.93c0 2.33-1.02 4.42-2.62 5.88L15 15.5v6h6l-2.2-2.2c1.96-1.82 3.2-4.41 3.2-7.3"/></svg>`;
var ICON_REPAIR = `<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" viewBox="0 0 24 24"><path fill="currentColor" d="M15.73 3H8.27L3 8.27v7.46L8.27 21h7.46L21 15.73V8.27zM19 14.9L14.9 19H9.1L5 14.9V9.1L9.1 5h5.8L19 9.1z"/><path fill="currentColor" d="M11 7h2v6h-2zm0 8h2v2h-2z"/></svg>`;

// src/menu/configMenu.ts
import * as shell3 from "mshell";

// src/plugin/constants.ts
var PLUGIN_SOURCES = {
  "Github Raw": "https://raw.githubusercontent.com/breeze-shell/plugins-packed/refs/heads/main/",
  "Enlysure": "https://breeze.enlysure.com/",
  "Enlysure Shanghai": "https://breeze-c.enlysure.com/"
};

// src/utils/network.ts
import * as shell2 from "mshell";
var get_async = (url) => {
  url = url.replaceAll("//", "/").replaceAll(":/", "://");
  shell2.println(url);
  return new Promise((resolve, reject) => {
    shell2.network.get_async(encodeURI(url), (data) => {
      resolve(data);
    }, (err) => {
      reject(err);
    });
  });
};

// src/utils/string.ts
var splitIntoLines = (str, maxLen) => {
  const lines = [];
  const maxLenBytes = maxLen * 2;
  for (let i = 0; i < str.length; i) {
    let x = 0;
    let line = str.substr(i, maxLenBytes);
    while (x < maxLen && line.length > x) {
      if (line.charCodeAt(x) > 255) {
        x++;
      }
      if (line.charAt(x) === "\n") {
        x++;
        break;
      }
      x++;
    }
    lines.push(line.substr(0, x).trim());
    i += x;
  }
  return lines;
};

// src/utils/object.ts
var getNestedValue = (obj, path) => {
  const parts = path.split(".");
  let current = obj;
  for (const part of parts) {
    if (current === void 0 || current === null) return void 0;
    current = current[part];
  }
  return current;
};
var setNestedValue = (obj, path, value) => {
  const parts = path.split(".");
  let current = obj;
  for (let i = 0; i < parts.length - 1; i++) {
    const part = parts[i];
    if (current[part] === void 0 || current[part] === null) {
      current[part] = {};
    }
    current = current[part];
  }
  current[parts[parts.length - 1]] = value;
  return obj;
};

// src/menu/configMenu.ts
var cached_plugin_index = null;
if (shell3.fs.exists(shell3.breeze.data_directory() + "/shell_old.dll")) {
  try {
    shell3.fs.remove(shell3.breeze.data_directory() + "/shell_old.dll");
  } catch (e) {
    shell3.println("Failed to remove old shell.dll: ", e);
  }
}
var current_source = "Enlysure";
var makeBreezeConfigMenu = (mainMenu) => {
  const currentLang = shell3.breeze.user_language() === "zh-CN" ? "zh-CN" : "en-US";
  const t = (key) => {
    return languages[currentLang][key] || key;
  };
  const fg_color = shell3.breeze.is_light_theme() ? "black" : "white";
  const ICON_CHECKED_COLORED = ICON_CHECKED.replaceAll("currentColor", fg_color);
  const ICON_CHANGE_COLORED = ICON_CHANGE.replaceAll("currentColor", fg_color);
  const ICON_REPAIR_COLORED = ICON_REPAIR.replaceAll("currentColor", fg_color);
  return {
    name: t("\u7BA1\u7406 Breeze Shell"),
    submenu(sub) {
      sub.append_menu({
        name: t("\u63D2\u4EF6\u5E02\u573A / \u66F4\u65B0\u672C\u4F53"),
        submenu(sub2) {
          const updatePlugins = async (page) => {
            for (const m of sub2.get_items().slice(1))
              m.remove();
            sub2.append_menu({
              name: t("\u52A0\u8F7D\u4E2D...")
            });
            if (!cached_plugin_index) {
              cached_plugin_index = await get_async(PLUGIN_SOURCES[current_source] + "plugins-index.json");
            }
            const data = JSON.parse(cached_plugin_index);
            for (const m of sub2.get_items().slice(1))
              m.remove();
            const current_version = shell3.breeze.version();
            const remote_version = data.shell.version;
            const exist_old_file = shell3.fs.exists(shell3.breeze.data_directory() + "/shell_old.dll");
            const upd = sub2.append_menu({
              name: exist_old_file ? `\u65B0\u7248\u672C\u5DF2\u4E0B\u8F7D\uFF0C\u5C06\u4E8E\u4E0B\u6B21\u91CD\u542F\u8D44\u6E90\u7BA1\u7406\u5668\u751F\u6548` : current_version === remote_version ? current_version + " (latest)" : `${current_version} -> ${remote_version}`,
              icon_svg: current_version === remote_version ? ICON_CHECKED_COLORED : ICON_CHANGE_COLORED,
              action() {
                if (current_version === remote_version) return;
                const shellPath = shell3.breeze.data_directory() + "/shell.dll";
                const shellOldPath = shell3.breeze.data_directory() + "/shell_old.dll";
                const url = PLUGIN_SOURCES[current_source] + data.shell.path;
                upd.set_data({
                  name: t("\u66F4\u65B0\u4E2D..."),
                  icon_svg: ICON_REPAIR_COLORED,
                  disabled: true
                });
                const downloadNewShell = () => {
                  shell3.network.download_async(url, shellPath, () => {
                    upd.set_data({
                      name: t("\u65B0\u7248\u672C\u5DF2\u4E0B\u8F7D\uFF0C\u5C06\u4E8E\u4E0B\u6B21\u91CD\u542F\u8D44\u6E90\u7BA1\u7406\u5668\u751F\u6548"),
                      icon_svg: ICON_CHECKED_COLORED,
                      disabled: true
                    });
                  }, (e) => {
                    upd.set_data({
                      name: t("\u66F4\u65B0\u5931\u8D25: ") + e,
                      icon_svg: ICON_REPAIR_COLORED,
                      disabled: false
                    });
                  });
                };
                try {
                  if (shell3.fs.exists(shellPath)) {
                    if (shell3.fs.exists(shellOldPath)) {
                      try {
                        shell3.fs.remove(shellOldPath);
                        shell3.fs.rename(shellPath, shellOldPath);
                        downloadNewShell();
                      } catch (e) {
                        upd.set_data({
                          name: t("\u66F4\u65B0\u5931\u8D25: ") + "\u65E0\u6CD5\u79FB\u52A8\u5F53\u524D\u6587\u4EF6",
                          icon_svg: ICON_REPAIR_COLORED,
                          disabled: false
                        });
                      }
                    } else {
                      shell3.fs.rename(shellPath, shellOldPath);
                      downloadNewShell();
                    }
                  } else {
                    downloadNewShell();
                  }
                } catch (e) {
                  upd.set_data({
                    name: t("\u66F4\u65B0\u5931\u8D25: ") + e,
                    icon_svg: ICON_REPAIR_COLORED,
                    disabled: false
                  });
                }
              },
              submenu(sub3) {
                for (const line of splitIntoLines(data.shell.changelog, 40)) {
                  sub3.append_menu({
                    name: line
                  });
                }
              }
            });
            sub2.append_menu({
              type: "spacer"
            });
            const plugins_page = data.plugins.slice((page - 1) * 10, page * 10);
            for (const plugin2 of plugins_page) {
              let install_path = null;
              if (shell3.fs.exists(shell3.breeze.data_directory() + "/scripts/" + plugin2.local_path)) {
                install_path = shell3.breeze.data_directory() + "/scripts/" + plugin2.local_path;
              }
              if (shell3.fs.exists(shell3.breeze.data_directory() + "/scripts/" + plugin2.local_path + ".disabled")) {
                install_path = shell3.breeze.data_directory() + "/scripts/" + plugin2.local_path + ".disabled";
              }
              const installed = install_path !== null;
              const local_version_match = installed ? shell3.fs.read(install_path).match(/\/\/ @version:\s*(.*)/) : null;
              const local_version = local_version_match ? local_version_match[1] : "\u672A\u5B89\u88C5";
              const have_update = installed && local_version !== plugin2.version;
              const disabled = installed && !have_update;
              let preview_sub = null;
              const m = sub2.append_menu({
                name: plugin2.name + (have_update ? ` (${local_version} -> ${plugin2.version})` : ""),
                action() {
                  if (disabled) return;
                  if (preview_sub) {
                    preview_sub.close();
                  }
                  m.set_data({
                    name: plugin2.name,
                    icon_svg: ICON_CHANGE_COLORED,
                    disabled: true
                  });
                  const path = shell3.breeze.data_directory() + "/scripts/" + plugin2.local_path;
                  const url = PLUGIN_SOURCES[current_source] + plugin2.path;
                  get_async(url).then((data2) => {
                    shell3.fs.write(path, data2);
                    m.set_data({
                      name: plugin2.name,
                      icon_svg: ICON_CHECKED_COLORED,
                      action() {
                      },
                      disabled: true
                    });
                    shell3.println(t("\u63D2\u4EF6\u5B89\u88C5\u6210\u529F: ") + plugin2.name);
                    reload_local();
                  }).catch((e) => {
                    m.set_data({
                      name: plugin2.name,
                      icon_svg: ICON_REPAIR_COLORED,
                      submenu(sub3) {
                        sub3.append_menu({
                          name: e
                        });
                        sub3.append_menu({
                          name: url,
                          action() {
                            shell3.clipboard.set_text(url);
                            mainMenu.close();
                          }
                        });
                      },
                      disabled: false
                    });
                    shell3.println(e);
                    shell3.println(e.stack);
                  });
                },
                submenu(sub3) {
                  preview_sub = sub3;
                  sub3.append_menu({
                    name: t("\u7248\u672C: ") + plugin2.version
                  });
                  sub3.append_menu({
                    name: t("\u4F5C\u8005: ") + plugin2.author
                  });
                  for (const line of splitIntoLines(plugin2.description, 40)) {
                    sub3.append_menu({
                      name: line
                    });
                  }
                },
                disabled,
                icon_svg: disabled ? ICON_CHECKED_COLORED : ICON_EMPTY
              });
            }
          };
          const source = sub2.append_menu({
            name: t("\u5F53\u524D\u6E90: ") + current_source,
            submenu(sub3) {
              for (const key in PLUGIN_SOURCES) {
                sub3.append_menu({
                  name: key,
                  action() {
                    current_source = key;
                    cached_plugin_index = null;
                    source.set_data({
                      name: t("\u5F53\u524D\u6E90: ") + key
                    });
                    updatePlugins(1);
                  },
                  disabled: false
                });
              }
            }
          });
          updatePlugins(1);
        }
      });
      sub.append_menu({
        name: t("Breeze \u8BBE\u7F6E"),
        submenu(sub2) {
          const current_config_path = shell3.breeze.data_directory() + "/config.json";
          const current_config = shell3.fs.read(current_config_path);
          let config = JSON.parse(current_config);
          if (!config.plugin_load_order) {
            config.plugin_load_order = [];
          }
          const write_config = () => {
            shell3.fs.write(current_config_path, JSON.stringify(config, null, 4));
          };
          sub2.append_menu({
            name: "\u4F18\u5148\u52A0\u8F7D\u63D2\u4EF6",
            submenu(sub3) {
              const plugins = shell3.fs.readdir(shell3.breeze.data_directory() + "/scripts").map((v) => v.split("/").pop()).filter((v) => v.endsWith(".js")).map((v) => v.replace(".js", ""));
              const isInLoadOrder = {};
              config.plugin_load_order.forEach((name) => {
                isInLoadOrder[name] = true;
              });
              for (const plugin2 of plugins) {
                let isPrioritized = isInLoadOrder[plugin2] === true;
                const btn = sub3.append_menu({
                  name: plugin2,
                  icon_svg: isPrioritized ? ICON_CHECKED_COLORED : ICON_EMPTY,
                  action() {
                    if (isPrioritized) {
                      config.plugin_load_order = config.plugin_load_order.filter((name) => name !== plugin2);
                      isInLoadOrder[plugin2] = false;
                      btn.set_data({
                        icon_svg: ICON_EMPTY
                      });
                    } else {
                      config.plugin_load_order.unshift(plugin2);
                      isInLoadOrder[plugin2] = true;
                      btn.set_data({
                        icon_svg: ICON_CHECKED_COLORED
                      });
                    }
                    isPrioritized = !isPrioritized;
                    write_config();
                  }
                });
              }
            }
          });
          const createBoolToggle = (sub3, label, configPath, defaultValue = false) => {
            let currentValue = getNestedValue(config, configPath) ?? defaultValue;
            const toggle = sub3.append_menu({
              name: label,
              icon_svg: currentValue ? ICON_CHECKED_COLORED : ICON_EMPTY,
              action() {
                currentValue = !currentValue;
                setNestedValue(config, configPath, currentValue);
                write_config();
                toggle.set_data({
                  icon_svg: currentValue ? ICON_CHECKED_COLORED : ICON_EMPTY,
                  disabled: false
                });
              }
            });
            return toggle;
          };
          sub2.append_spacer();
          const theme_presets = {
            "\u9ED8\u8BA4": null,
            "\u7D27\u51D1": {
              radius: 4,
              item_height: 20,
              item_gap: 2,
              item_radius: 3,
              margin: 4,
              padding: 4,
              text_padding: 6,
              icon_padding: 3,
              right_icon_padding: 16,
              multibutton_line_gap: -4
            },
            "\u5BBD\u677E": {
              radius: 6,
              item_height: 24,
              item_gap: 4,
              item_radius: 8,
              margin: 6,
              padding: 6,
              text_padding: 8,
              icon_padding: 4,
              right_icon_padding: 20,
              multibutton_line_gap: -6
            },
            "\u5706\u89D2": {
              radius: 12,
              item_radius: 12
            },
            "\u65B9\u89D2": {
              radius: 0,
              item_radius: 0
            }
          };
          const anim_none = {
            easing: "mutation"
          };
          const animation_presets = {
            "\u9ED8\u8BA4": null,
            "\u5FEB\u901F": {
              "item": {
                "opacity": {
                  "delay_scale": 0
                },
                "width": anim_none,
                "x": anim_none
              },
              "submenu_bg": {
                "opacity": {
                  "delay_scale": 0,
                  "duration": 100
                }
              },
              "main_bg": {
                "opacity": anim_none
              }
            },
            "\u65E0": {
              "item": {
                "opacity": anim_none,
                "width": anim_none,
                "x": anim_none,
                "y": anim_none
              },
              "submenu_bg": {
                "opacity": anim_none,
                "x": anim_none,
                "y": anim_none,
                "w": anim_none,
                "h": anim_none
              },
              "main_bg": {
                "opacity": anim_none,
                "x": anim_none,
                "y": anim_none,
                "w": anim_none,
                "h": anim_none
              }
            }
          };
          const getAllSubkeys = (presets) => {
            if (!presets) return [];
            const keys = /* @__PURE__ */ new Set();
            for (const v of Object.values(presets)) {
              if (v)
                for (const key of Object.keys(v)) {
                  keys.add(key);
                }
            }
            return [...keys];
          };
          const applyPreset = (preset, origin, presets) => {
            const allSubkeys = getAllSubkeys(presets);
            const newPreset = preset;
            for (let key in origin) {
              if (allSubkeys.includes(key)) continue;
              newPreset[key] = origin[key];
            }
            return newPreset;
          };
          const checkPresetMatch = (current, preset) => {
            if (!current) return false;
            if (!preset) return false;
            return Object.keys(preset).every((key) => JSON.stringify(current[key]) === JSON.stringify(preset[key]));
          };
          const getCurrentPreset = (current, presets) => {
            if (!current) return "\u9ED8\u8BA4";
            for (const [name, preset] of Object.entries(presets)) {
              if (preset && checkPresetMatch(current, preset)) {
                return name;
              }
            }
            return "\u81EA\u5B9A\u4E49";
          };
          const updateIconStatus = (sub3, current, presets) => {
            try {
              const currentPreset = getCurrentPreset(current, presets);
              for (const _item of sub3.get_items()) {
                const item = _item.data();
                if (item.name === currentPreset) {
                  _item.set_data({
                    icon_svg: ICON_CHECKED_COLORED,
                    disabled: true
                  });
                } else {
                  _item.set_data({
                    icon_svg: ICON_EMPTY,
                    disabled: false
                  });
                }
              }
              const lastItem = sub3.get_items().pop();
              if (lastItem.data().name === "\u81EA\u5B9A\u4E49" && currentPreset !== "\u81EA\u5B9A\u4E49") {
                lastItem.remove();
              } else if (currentPreset === "\u81EA\u5B9A\u4E49") {
                sub3.append_menu({
                  name: "\u81EA\u5B9A\u4E49",
                  disabled: true,
                  icon_svg: ICON_CHECKED_COLORED
                });
              }
            } catch (e) {
              shell3.println(e, e.stack);
            }
          };
          sub2.append_menu({
            name: "\u4E3B\u9898",
            submenu(sub3) {
              const currentTheme = config.context_menu?.theme;
              for (const [name, preset] of Object.entries(theme_presets)) {
                sub3.append_menu({
                  name,
                  action() {
                    try {
                      if (!preset) {
                        delete config.context_menu.theme;
                      } else {
                        config.context_menu.theme = applyPreset(preset, config.context_menu.theme, theme_presets);
                      }
                      write_config();
                      updateIconStatus(sub3, config.context_menu.theme, theme_presets);
                    } catch (e) {
                      shell3.println(e, e.stack);
                    }
                  }
                });
              }
              updateIconStatus(sub3, currentTheme, theme_presets);
            }
          });
          sub2.append_menu({
            name: "\u52A8\u753B",
            submenu(sub3) {
              const currentAnimation = config.context_menu?.theme?.animation;
              for (const [name, preset] of Object.entries(animation_presets)) {
                sub3.append_menu({
                  name,
                  action() {
                    if (!preset) {
                      if (config.context_menu?.theme) {
                        delete config.context_menu.theme.animation;
                      }
                    } else {
                      if (!config.context_menu) config.context_menu = {};
                      if (!config.context_menu.theme) config.context_menu.theme = {};
                      config.context_menu.theme.animation = preset;
                    }
                    updateIconStatus(sub3, config.context_menu.theme?.animation, animation_presets);
                    write_config();
                  }
                });
              }
              updateIconStatus(sub3, currentAnimation, animation_presets);
            }
          });
          sub2.append_spacer();
          createBoolToggle(sub2, "\u8C03\u8BD5\u63A7\u5236\u53F0", "debug_console", false);
          createBoolToggle(sub2, "\u5782\u76F4\u540C\u6B65", "context_menu.vsync", true);
          createBoolToggle(sub2, "\u5FFD\u7565\u81EA\u7ED8\u83DC\u5355", "context_menu.ignore_owner_draw", true);
          createBoolToggle(sub2, "\u5411\u4E0A\u5C55\u5F00\u65F6\u53CD\u5411\u6392\u5217", "context_menu.reverse_if_open_to_up", true);
          createBoolToggle(sub2, "\u5C1D\u8BD5\u4F7F\u7528 Windows 11 \u5706\u89D2", "context_menu.theme.use_dwm_if_available", true);
          createBoolToggle(sub2, "\u4E9A\u514B\u529B\u80CC\u666F\u6548\u679C", "context_menu.theme.acrylic", true);
        }
      });
      sub.append_spacer();
      const reload_local = () => {
        const installed = shell3.fs.readdir(shell3.breeze.data_directory() + "/scripts").map((v) => v.split("/").pop()).filter((v) => v.endsWith(".js") || v.endsWith(".disabled"));
        for (const m of sub.get_items().slice(3))
          m.remove();
        for (const plugin2 of installed) {
          let disabled = plugin2.endsWith(".disabled");
          let name = plugin2.replace(".js", "").replace(".disabled", "");
          const m = sub.append_menu({
            name,
            icon_svg: disabled ? ICON_EMPTY : ICON_CHECKED_COLORED,
            action() {
              if (disabled) {
                shell3.fs.rename(shell3.breeze.data_directory() + "/scripts/" + name + ".js.disabled", shell3.breeze.data_directory() + "/scripts/" + name + ".js");
                m.set_data({
                  name,
                  icon_svg: ICON_CHECKED_COLORED
                });
              } else {
                shell3.fs.rename(shell3.breeze.data_directory() + "/scripts/" + name + ".js", shell3.breeze.data_directory() + "/scripts/" + name + ".js.disabled");
                m.set_data({
                  name,
                  icon_svg: ICON_EMPTY
                });
              }
              disabled = !disabled;
            },
            submenu(sub2) {
              sub2.append_menu({
                name: t("\u5220\u9664"),
                action() {
                  shell3.fs.remove(shell3.breeze.data_directory() + "/scripts/" + plugin2);
                  m.remove();
                  sub2.close();
                }
              });
              if (on_plugin_menu[name]) {
                on_plugin_menu[name](sub2);
              }
            }
          });
        }
      };
      reload_local();
    }
  };
};

// src/plugin/core.ts
import * as shell4 from "mshell";
var config_directory_main = shell4.breeze.data_directory() + "/config/";
var config_dir_watch_callbacks = /* @__PURE__ */ new Set();
shell4.fs.mkdir(config_directory_main);
shell4.fs.watch(config_directory_main, (path, type) => {
  for (const callback of config_dir_watch_callbacks) {
    callback(path, type);
  }
});
globalThis.on_plugin_menu = {};
var plugin = (import_meta, default_config = {}) => {
  const CONFIG_FILE = "config.json";
  const { name, url } = import_meta;
  const languages2 = {};
  const nameNoExt = name.endsWith(".js") ? name.slice(0, -3) : name;
  let config = default_config;
  const on_reload_callbacks = /* @__PURE__ */ new Set();
  const plugin2 = {
    i18n: {
      define: (lang, data) => {
        languages2[lang] = data;
      },
      t: (key) => {
        return languages2[shell4.breeze.user_language()][key] || key;
      }
    },
    set_on_menu: (callback) => {
      globalThis.on_plugin_menu[nameNoExt] = callback;
    },
    config_directory: config_directory_main + nameNoExt + "/",
    config: {
      read_config() {
        if (shell4.fs.exists(plugin2.config_directory + CONFIG_FILE)) {
          try {
            config = JSON.parse(shell4.fs.read(plugin2.config_directory + CONFIG_FILE));
          } catch (e) {
            shell4.println(`[${name}] \u914D\u7F6E\u6587\u4EF6\u89E3\u6790\u5931\u8D25: ${e}`);
          }
        }
      },
      write_config() {
        shell4.fs.write(plugin2.config_directory + CONFIG_FILE, JSON.stringify(config, null, 4));
      },
      get(key) {
        return getNestedValue(config, key) || getNestedValue(default_config, key) || null;
      },
      set(key, value) {
        setNestedValue(config, key, value);
        plugin2.config.write_config();
      },
      all() {
        return config;
      },
      on_reload(callback) {
        const dispose = () => {
          on_reload_callbacks.delete(callback);
        };
        on_reload_callbacks.add(callback);
        return dispose;
      }
    },
    log(...args) {
      shell4.println(`[${name}]`, ...args);
    }
  };
  shell4.fs.mkdir(plugin2.config_directory);
  plugin2.config.read_config();
  config_dir_watch_callbacks.add((path, type) => {
    const relativePath = path.replace(config_directory_main, "");
    if (relativePath === `${nameNoExt}\\${CONFIG_FILE}`) {
      shell4.println(`[${name}] \u914D\u7F6E\u6587\u4EF6\u53D8\u66F4: ${path} ${type}`);
      plugin2.config.read_config();
      for (const callback of on_reload_callbacks) {
        callback(config);
      }
    }
  });
  return plugin2;
};

// src/entry.ts
if (shell5.fs.exists(shell5.breeze.data_directory() + "/shell_old.dll")) {
  try {
    shell5.fs.remove(shell5.breeze.data_directory() + "/shell_old.dll");
  } catch (e) {
    shell5.println("Failed to remove old shell.dll: ", e);
  }
}
shell5.menu_controller.add_menu_listener((ctx) => {
  if (ctx.context.folder_view?.current_path.startsWith(shell5.breeze.data_directory().replaceAll("/", "\\"))) {
    ctx.menu.prepend_menu(makeBreezeConfigMenu(ctx.menu));
  }
  for (const items of ctx.menu.get_items()) {
    const data = items.data();
    if (data.name_resid === "10580@SHELL32.dll" || data.name === "\u6E05\u7A7A\u56DE\u6536\u7AD9") {
      items.set_data({
        disabled: false
      });
    }
    if (data.name?.startsWith("NVIDIA ")) {
      items.set_data({
        icon_svg: `<svg viewBox="0 0 271.7 179.7" xmlns="http://www.w3.org/2000/svg" width="2500" height="1653" fill="#000000"><path d="M101.3 53.6V37.4c1.6-.1 3.2-.2 4.8-.2 44.4-1.4 73.5 38.2 73.5 38.2S148.2 119 114.5 119c-4.5 0-8.9-.7-13.1-2.1V67.7c17.3 2.1 20.8 9.7 31.1 27l23.1-19.4s-16.9-22.1-45.3-22.1c-3-.1-6 .1-9 .4m0-53.6v24.2l4.8-.3c61.7-2.1 102 50.6 102 50.6s-46.2 56.2-94.3 56.2c-4.2 0-8.3-.4-12.4-1.1v15c3.4.4 6.9.7 10.3.7 44.8 0 77.2-22.9 108.6-49.9 5.2 4.2 26.5 14.3 30.9 18.7-29.8 25-99.3 45.1-138.7 45.1-3.8 0-7.4-.2-11-.6v21.1h170.2V0H101.3zm0 116.9v12.8c-41.4-7.4-52.9-50.5-52.9-50.5s19.9-22 52.9-25.6v14h-.1c-17.3-2.1-30.9 14.1-30.9 14.1s7.7 27.3 31 35.2M27.8 77.4s24.5-36.2 73.6-40V24.2C47 28.6 0 74.6 0 74.6s26.6 77 101.3 84v-14c-54.8-6.8-73.5-67.2-73.5-67.2z" fill="#76b900"/></svg>`,
        icon_bitmap: new shell5.value_reset()
      });
    }
  }
});
globalThis.plugin = plugin;
