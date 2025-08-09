import * as shell from "mshell"
import { PLUGIN_SOURCES } from "../plugin/constants"
import { get_async } from "../utils/network"
import { splitIntoLines } from "../utils/string"
import { getNestedValue, setNestedValue } from "../utils/object"
import { config_dir_watch_callbacks } from "../plugin/core"
import { languages, ICON_EMPTY, ICON_CHECKED, ICON_CHANGE, ICON_REPAIR } from "./constants"

let cached_plugin_index: any = null

// remove possibly existing shell_old.dll if able to
if (shell.fs.exists(shell.breeze.data_directory() + '/shell_old.dll')) {
    try {
        shell.fs.remove(shell.breeze.data_directory() + '/shell_old.dll')
    } catch (e) {
        shell.println('Failed to remove old shell.dll: ', e)
    }
}

let current_source = 'Enlysure'

export const makeBreezeConfigMenu = (mainMenu) => {
    const currentLang = shell.breeze.user_language() === 'zh-CN' ? 'zh-CN' : 'en-US'
    const t = (key: string) => {
        return languages[currentLang][key] || key
    }

    const fg_color = shell.breeze.is_light_theme() ? 'black' : 'white'
    const ICON_CHECKED_COLORED = ICON_CHECKED.replaceAll('currentColor', fg_color)
    const ICON_CHANGE_COLORED = ICON_CHANGE.replaceAll('currentColor', fg_color)
    const ICON_REPAIR_COLORED = ICON_REPAIR.replaceAll('currentColor', fg_color)

    return {
        name: t("管理 Breeze Shell"),
        submenu(sub) {
            sub.append_menu({
                name: t("插件市场 / 更新本体"),
                submenu(sub) {
                    const updatePlugins = async (page) => {
                        for (const m of sub.get_items().slice(1))
                            m.remove()

                        sub.append_menu({
                            name: t('加载中...')
                        })

                        if (!cached_plugin_index) {
                            cached_plugin_index = await get_async(PLUGIN_SOURCES[current_source] + 'plugins-index.json')
                        }
                        const data = JSON.parse(cached_plugin_index)

                        for (const m of sub.get_items().slice(1))
                            m.remove()

                        const current_version = shell.breeze.version();
                        const remote_version = data.shell.version;

                        const exist_old_file = shell.fs.exists(shell.breeze.data_directory() + '/shell_old.dll')

                        const upd = sub.append_menu({
                            name: exist_old_file ?
                                `新版本已下载，将于下次重启资源管理器生效` :
                                (current_version === remote_version ?
                                    (current_version + ' (latest)') :
                                    `${current_version} -> ${remote_version}`),
                            icon_svg: current_version === remote_version ? ICON_CHECKED_COLORED : ICON_CHANGE_COLORED,
                            action() {
                                if (current_version === remote_version) return
                                const shellPath = shell.breeze.data_directory() + '/shell.dll'
                                const shellOldPath = shell.breeze.data_directory() + '/shell_old.dll'
                                const url = PLUGIN_SOURCES[current_source] + data.shell.path

                                upd.set_data({
                                    name: t('更新中...'),
                                    icon_svg: ICON_REPAIR_COLORED,
                                    disabled: true
                                })

                                const downloadNewShell = () => {
                                    shell.network.download_async(url, shellPath, () => {
                                        upd.set_data({
                                            name: t('新版本已下载，将于下次重启资源管理器生效'),
                                            icon_svg: ICON_CHECKED_COLORED,
                                            disabled: true
                                        })
                                    }, e => {
                                        upd.set_data({
                                            name: t('更新失败: ') + e,
                                            icon_svg: ICON_REPAIR_COLORED,
                                            disabled: false
                                        })
                                    })
                                }

                                try {
                                    if (shell.fs.exists(shellPath)) {
                                        if (shell.fs.exists(shellOldPath)) {
                                            try {
                                                shell.fs.remove(shellOldPath)
                                                shell.fs.rename(shellPath, shellOldPath)
                                                downloadNewShell()
                                            } catch (e) {
                                                upd.set_data({
                                                    name: t('更新失败: ') + '无法移动当前文件',
                                                    icon_svg: ICON_REPAIR_COLORED,
                                                    disabled: false
                                                })
                                            }
                                        } else {
                                            shell.fs.rename(shellPath, shellOldPath)
                                            downloadNewShell()
                                        }
                                    } else {
                                        downloadNewShell()
                                    }
                                } catch (e) {
                                    upd.set_data({
                                        name: t('更新失败: ') + e,
                                        icon_svg: ICON_REPAIR_COLORED,
                                        disabled: false
                                    })
                                }
                            },
                            submenu(sub) {
                                for (const line of splitIntoLines(data.shell.changelog, 40)) {
                                    sub.append_menu({
                                        name: line
                                    })
                                }
                            }
                        })

                        sub.append_menu({
                            type: 'spacer'
                        })

                        const plugins_page = data.plugins.slice((page - 1) * 10, page * 10)
                        for (const plugin of plugins_page) {
                            let install_path = null;
                            if (shell.fs.exists(shell.breeze.data_directory() + '/scripts/' + plugin.local_path)) {
                                install_path = shell.breeze.data_directory() + '/scripts/' + plugin.local_path
                            }

                            if (shell.fs.exists(shell.breeze.data_directory() + '/scripts/' + plugin.local_path + '.disabled')) {
                                install_path = shell.breeze.data_directory() + '/scripts/' + plugin.local_path + '.disabled'
                            }
                            const installed = install_path !== null

                            const local_version_match = installed ? shell.fs.read(install_path).match(/\/\/ @version:\s*(.*)/) : null
                            const local_version = local_version_match ? local_version_match[1] : '未安装'
                            const have_update = installed && local_version !== plugin.version

                            const disabled = installed && !have_update

                            let preview_sub = null
                            const m = sub.append_menu({
                                name: plugin.name + (have_update ? ` (${local_version} -> ${plugin.version})` : ''),
                                action() {
                                    if (disabled) return
                                    if (preview_sub) {
                                        preview_sub.close()
                                    }
                                    m.set_data({
                                        name: plugin.name,
                                        icon_svg: ICON_CHANGE_COLORED,
                                        disabled: true
                                    })
                                    const path = shell.breeze.data_directory() + '/scripts/' + plugin.local_path
                                    const url = PLUGIN_SOURCES[current_source] + plugin.path
                                    get_async(url).then(data => {
                                        shell.fs.write(path, data as string)
                                        m.set_data({
                                            name: plugin.name,
                                            icon_svg: ICON_CHECKED_COLORED,
                                            action() { },
                                            disabled: true
                                        })

                                        shell.println(t('插件安装成功: ') + plugin.name)

                                        reload_local()
                                    }).catch(e => {
                                        m.set_data({
                                            name: plugin.name,
                                            icon_svg: ICON_REPAIR_COLORED,
                                            submenu(sub) {
                                                sub.append_menu({
                                                    name: e
                                                })
                                                sub.append_menu({
                                                    name: url,
                                                    action() {
                                                        shell.clipboard.set_text(url)
                                                        mainMenu.close()
                                                    }
                                                })
                                            },
                                            disabled: false
                                        })

                                        shell.println(e)
                                        shell.println(e.stack)
                                    })
                                },
                                submenu(sub) {
                                    preview_sub = sub
                                    sub.append_menu({
                                        name: t('版本: ') + plugin.version
                                    })
                                    sub.append_menu({
                                        name: t('作者: ') + plugin.author
                                    })

                                    for (const line of splitIntoLines(plugin.description, 40)) {
                                        sub.append_menu({
                                            name: line
                                        })
                                    }
                                },
                                disabled: disabled,
                                icon_svg: disabled ? ICON_CHECKED_COLORED : ICON_EMPTY,
                            })
                        }

                    }
                    const source = sub.append_menu({
                        name: t('当前源: ') + current_source,
                        submenu(sub) {
                            for (const key in PLUGIN_SOURCES) {
                                sub.append_menu({
                                    name: key,
                                    action() {
                                        current_source = key
                                        cached_plugin_index = null
                                        source.set_data({
                                            name: t('当前源: ') + key
                                        })
                                        updatePlugins(1)
                                    },
                                    disabled: false
                                })
                            }
                        }
                    })

                    updatePlugins(1)
                }
            })
            sub.append_menu({
                name: t("Breeze 设置"),
                submenu(sub) {
                    const current_config_path = shell.breeze.data_directory() + '/config.json'
                    const current_config = shell.fs.read(current_config_path)
                    let config = JSON.parse(current_config);
                    if (!config.plugin_load_order) {
                        config.plugin_load_order = [];
                    }

                    const write_config = () => {
                        shell.fs.write(current_config_path, JSON.stringify(config, null, 4))
                    }

                    sub.append_menu({
                        name: "优先加载插件",
                        submenu(sub) {
                            const plugins = shell.fs.readdir(shell.breeze.data_directory() + '/scripts')
                                .map(v => v.split('/').pop())
                                .filter(v => v.endsWith('.js'))
                                .map(v => v.replace('.js', ''));

                            const isInLoadOrder = {};
                            config.plugin_load_order.forEach(name => {
                                isInLoadOrder[name] = true;
                            });

                            for (const plugin of plugins) {
                                let isPrioritized = isInLoadOrder[plugin] === true;

                                const btn = sub.append_menu({
                                    name: plugin,
                                    icon_svg: isPrioritized ? ICON_CHECKED_COLORED : ICON_EMPTY,
                                    action() {
                                        if (isPrioritized) {
                                            config.plugin_load_order = config.plugin_load_order.filter(name => name !== plugin);
                                            isInLoadOrder[plugin] = false;
                                            btn.set_data({
                                                icon_svg: ICON_EMPTY
                                            });
                                        } else {
                                            config.plugin_load_order.unshift(plugin);
                                            isInLoadOrder[plugin] = true;
                                            btn.set_data({
                                                icon_svg: ICON_CHECKED_COLORED
                                            });
                                        }

                                        isPrioritized = !isPrioritized
                                        write_config();
                                    }
                                });
                            }
                        }
                    });

                    const createBoolToggle = (sub, label, configPath, defaultValue = false) => {
                        let currentValue = getNestedValue(config, configPath) ?? defaultValue;

                        const toggle = sub.append_menu({
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

                    sub.append_spacer()

                    const theme_presets = {
                        "默认": null,
                        "紧凑": {
                            radius: 4.0,
                            item_height: 20.0,
                            item_gap: 2.0,
                            item_radius: 3.0,
                            margin: 4.0,
                            padding: 4.0,
                            text_padding: 6.0,
                            icon_padding: 3.0,
                            right_icon_padding: 16.0,
                            multibutton_line_gap: -4.0
                        },
                        "宽松": {
                            radius: 6.0,
                            item_height: 24.0,
                            item_gap: 4.0,
                            item_radius: 8.0,
                            margin: 6.0,
                            padding: 6.0,
                            text_padding: 8.0,
                            icon_padding: 4.0,
                            right_icon_padding: 20.0,
                            multibutton_line_gap: -6.0
                        },
                        "圆角": {
                            radius: 12.0,
                            item_radius: 12.0
                        },
                        "方角": {
                            radius: 0.0,
                            item_radius: 0.0
                        }
                    };

                    const anim_none = {
                        easing: "mutation",
                    }
                    const animation_presets = {
                        "默认": null,
                        "快速": {
                            "item": {
                                "opacity": {
                                    "delay_scale": 0
                                },
                                "width": anim_none,
                                "x": anim_none,
                            },
                            "submenu_bg": {
                                "opacity": {
                                    "delay_scale": 0,
                                    "duration": 100
                                }
                            },
                            "main_bg": {
                                "opacity": anim_none,
                            }
                        },
                        "无": {
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
                        if (!presets) return []
                        const keys = new Set();

                        for (const v of Object.values(presets)) {
                            if (v)
                                for (const key of Object.keys(v)) {
                                    keys.add(key);
                                }
                        }

                        return [...keys]
                    }

                    const applyPreset = (preset, origin, presets) => {
                        const allSubkeys = getAllSubkeys(presets);
                        const newPreset = preset;
                        for (let key in origin) {
                            if (allSubkeys.includes(key)) continue;
                            newPreset[key] = origin[key];
                        }
                        return newPreset;
                    }

                    const checkPresetMatch = (current, preset) => {
                        if (!current) return false;
                        if (!preset) return false;
                        return Object.keys(preset).every(key => JSON.stringify(current[key]) === JSON.stringify(preset[key]))

                    };

                    const getCurrentPreset = (current, presets) => {
                        if (!current) return "默认";
                        for (const [name, preset] of Object.entries(presets)) {
                            if (preset && checkPresetMatch(current, preset)) {
                                return name;
                            }
                        }
                        return "自定义";
                    };

                    const updateIconStatus = (sub, current, presets) => {
                        try {
                            const currentPreset = getCurrentPreset(current, presets);
                            for (const _item of sub.get_items()) {
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

                            const lastItem = sub.get_items().pop()
                            if (lastItem.data().name === "自定义" && currentPreset !== "自定义") {
                                lastItem.remove()
                            } else if (currentPreset === "自定义") {
                                sub.append_menu({
                                    name: "自定义",
                                    disabled: true,
                                    icon_svg: ICON_CHECKED_COLORED,
                                });
                            }
                        } catch (e) {
                            shell.println(e, e.stack)
                        }
                    }

                    sub.append_menu({
                        name: "主题",
                        submenu(sub) {
                            const currentTheme = config.context_menu?.theme;

                            for (const [name, preset] of Object.entries(theme_presets)) {
                                sub.append_menu({
                                    name,
                                    action() {
                                        try {
                                            if (!preset) {
                                                delete config.context_menu.theme;
                                            } else {
                                                config.context_menu.theme = applyPreset(preset, config.context_menu.theme, theme_presets);
                                            }
                                            write_config();
                                            updateIconStatus(sub, config.context_menu.theme, theme_presets);
                                        } catch (e) {
                                            shell.println(e, e.stack)
                                        }
                                    }
                                });
                            }

                            updateIconStatus(sub, currentTheme, theme_presets);
                        }
                    });

                    sub.append_menu({
                        name: "动画",
                        submenu(sub) {
                            const currentAnimation = config.context_menu?.theme?.animation;

                            for (const [name, preset] of Object.entries(animation_presets)) {
                                sub.append_menu({
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

                                        updateIconStatus(sub, config.context_menu.theme?.animation, animation_presets);
                                        write_config();
                                    }
                                });
                            }

                            updateIconStatus(sub, currentAnimation, animation_presets);
                        }
                    });

                    sub.append_spacer()

                    createBoolToggle(sub, "调试控制台", "debug_console", false);
                    createBoolToggle(sub, "垂直同步", "context_menu.vsync", true);
                    createBoolToggle(sub, "忽略自绘菜单", "context_menu.ignore_owner_draw", true);
                    createBoolToggle(sub, "向上展开时反向排列", "context_menu.reverse_if_open_to_up", true);
                    createBoolToggle(sub, "尝试使用 Windows 11 圆角", "context_menu.theme.use_dwm_if_available", true);
                    createBoolToggle(sub, "亚克力背景效果", "context_menu.theme.acrylic", true);
                }
            })

            sub.append_spacer()


            const reload_local = () => {
                const installed = shell.fs.readdir(shell.breeze.data_directory() + '/scripts')
                    .map(v => v.split('/').pop())
                    .filter(v => v.endsWith('.js') || v.endsWith('.disabled'))

                for (const m of sub.get_items().slice(3))
                    m.remove()

                for (const plugin of installed) {
                    let disabled = plugin.endsWith('.disabled')
                    let name = plugin.replace('.js', '').replace('.disabled', '')
                    const m = sub.append_menu({
                        name,
                        icon_svg: disabled ? ICON_EMPTY : ICON_CHECKED_COLORED,
                        action() {
                            if (disabled) {
                                shell.fs.rename(shell.breeze.data_directory() + '/scripts/' + name + '.js.disabled', shell.breeze.data_directory() + '/scripts/' + name + '.js')
                                m.set_data({
                                    name,
                                    icon_svg: ICON_CHECKED_COLORED
                                })
                            } else {
                                shell.fs.rename(shell.breeze.data_directory() + '/scripts/' + name + '.js', shell.breeze.data_directory() + '/scripts/' + name + '.js.disabled')
                                m.set_data({
                                    name,
                                    icon_svg: ICON_EMPTY
                                })
                            }

                            disabled = !disabled
                        },
                        submenu(sub) {
                            sub.append_menu({
                                name: t('删除'),
                                action() {
                                    shell.fs.remove(shell.breeze.data_directory() + '/scripts/' + plugin)
                                    m.remove()
                                    sub.close()
                                }
                            })

                            if (on_plugin_menu[name]) {
                                on_plugin_menu[name](sub)
                            }
                        }
                    })
                }
            }

            reload_local()
        }
    }
}