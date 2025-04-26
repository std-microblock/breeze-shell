import * as shell from "mshell"

const PLUGIN_SOURCES = {
    'Github Raw': 'https://raw.githubusercontent.com/breeze-shell/plugins-packed/refs/heads/main/',
    'Enlysure': 'https://breeze.enlysure.com/',
    'Enlysure Shanghai': 'https://breeze-c.enlysure.com/'
}

let current_source = 'Enlysure'
const get_async = url => {
    url = url.replaceAll('//', '/').replaceAll(':/', '://')
    shell.println(url)
    return new Promise((resolve, reject) => {
        shell.network.get_async(encodeURI(url), data => {
            resolve(data)
        }, err => {
            reject(err)
        })
    })
}

const splitIntoLines = (str, maxLen) => {
    const lines = []
    // one chinese char = 2 english char
    const maxLenBytes = maxLen * 2
    for (let i = 0; i < str.length; i) {
        let x = 0;
        let line = str.substr(i, maxLenBytes)
        while (x < maxLen && line.length > x) {
            if (line.charCodeAt(x) > 255) {
                x++
            }

            if (line.charAt(x) === '\n') {
                x++;
                break
            }

            x++
        }
        lines.push(line.substr(0, x).trim())
        i += x
    }

    return lines
}

// Breeze infrastructure

globalThis.on_plugin_menu = {};

const config_directory_main = shell.breeze.data_directory() + '/config/';

const config_dir_watch_callbacks = new Set();

shell.fs.mkdir(config_directory_main)
shell.fs.watch(config_directory_main, (event, filename) => {
    for (const callback of config_dir_watch_callbacks) {
        callback(event, filename)
    }
})


const getNestedValue = (obj, path) => {
    const parts = path.split('.');
    let current = obj;

    for (const part of parts) {
        if (current === undefined || current === null) return undefined;
        current = current[part];
    }

    return current;
};

const setNestedValue = (obj, path, value) => {
    const parts = path.split('.');
    let current = obj;

    for (let i = 0; i < parts.length - 1; i++) {
        const part = parts[i];
        if (current[part] === undefined || current[part] === null) {
            current[part] = {};
        }
        current = current[part];
    }

    current[parts[parts.length - 1]] = value;
    return obj;
};

const plugin = (import_meta, default_config = {}) => {
    const CONFIG_FILE = 'config.json'

    const { name, url } = import_meta
    const languages = {}

    const nameNoExt = name.endsWith('.js') ? name.slice(0, -3) : name

    let config = default_config

    const on_reload_callbacks = new Set()

    const plugin = {
        i18n: {
            define: (lang, data) => {
                languages[lang] = data
            },
            t: (key) => {
                return languages[shell.breeze.user_language()][key] || key
            }
        },
        set_on_menu: (callback) => {
            on_plugin_menu[nameNoExt] = callback
        },
        config_directory: config_directory_main + nameNoExt + '/',
        config: {
            read_config() {
                if (shell.fs.exists(plugin.config_directory + CONFIG_FILE)) {
                    try {
                        config = JSON.parse(shell.fs.read(plugin.config_directory + CONFIG_FILE))
                    } catch (e) {
                        shell.println(`[${name}] 配置文件解析失败: ${e}`)
                    }
                }
            },
            write_config() {
                shell.fs.write(plugin.config_directory + CONFIG_FILE, JSON.stringify(config, null, 4))
            },
            get(key) {
                return getNestedValue(config, key) || getNestedValue(default_config, key) || null
            },
            set(key, value) {
                setNestedValue(config, key, value)
                plugin.config.write_config()
            },
            all() {
                return config
            },
            on_reload(callback) {
                const dispose = () => {
                    on_reload_callbacks.delete(callback)
                }
                on_reload_callbacks.add(callback)
                return dispose
            }
        },
        log(...args) {
            shell.println(`[${name}]`, ...args)
        }
    }

    shell.fs.mkdir(plugin.config_directory)
    plugin.config.read_config()
    config_dir_watch_callbacks.add((event, filename) => {
        if (filename === `${nameNoExt}\\${CONFIG_FILE}`) {
            shell.println(`[${name}] 配置文件变更: ${event} ${filename}`)
            plugin.config.read_config()
            for (const callback of on_reload_callbacks) {
                callback(config)
            }
        }
    })

    return plugin
}

globalThis.plugin = plugin

const languages = {
    'zh-CN': {
    },
    'en-US': {
        '管理 Breeze Shell': 'Manage Breeze Shell',
        '插件市场 / 更新本体': 'Plugin Market / Update Shell',
        '加载中...': 'Loading...',
        '更新中...': 'Updating...',
        '新版本已下载，将于下次重启资源管理器生效': 'New version downloaded, will take effect next time the file manager is restarted',
        '更新失败: ': 'Update failed: ',
        '插件安装成功: ': 'Plugin installed: ',
        '当前源: ': 'Current source: ',
        '删除': 'Delete',
        '版本: ': 'Version: ',
        '作者: ': 'Author: '
    }
}

let cached_plugin_index = null

// remove possibly existing shell_old.dll if able to
if (shell.fs.exists(shell.breeze.data_directory() + '/shell_old.dll')) {
    try {
        shell.fs.remove(shell.breeze.data_directory() + '/shell_old.dll')
    } catch (e) {
        shell.println('Failed to remove old shell.dll: ', e)
    }
}

const makeBreezeConfigMenu = (mainMenu) => {
    const currentLang = shell.breeze.user_language() === 'zh-CN' ? 'zh-CN' : 'en-US'
    const t = (key) => {
        return languages[currentLang][key] || key
    }

    const fg_color = shell.breeze.is_light_theme() ? 'black' : 'white'
    const ICON_EMPTY = new shell.value_reset()
    const ICON_CHECKED = `<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" viewBox="0 0 12 12"><path fill="currentColor" d="M9.765 3.205a.75.75 0 0 1 .03 1.06l-4.25 4.5a.75.75 0 0 1-1.075.015L2.22 6.53a.75.75 0 0 1 1.06-1.06l1.705 1.704l3.72-3.939a.75.75 0 0 1 1.06-.03"/></svg>`.replaceAll('currentColor', fg_color)
    const ICON_CHANGE = `<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" viewBox="0 0 24 24"><path fill="currentColor" d="m17.66 9.53l-7.07 7.07l-4.24-4.24l1.41-1.41l2.83 2.83l5.66-5.66zM4 12c0-2.33 1.02-4.42 2.62-5.88L9 8.5v-6H3l2.2 2.2C3.24 6.52 2 9.11 2 12c0 5.19 3.95 9.45 9 9.95v-2.02c-3.94-.49-7-3.86-7-7.93m18 0c0-5.19-3.95-9.45-9-9.95v2.02c3.94.49 7 3.86 7 7.93c0 2.33-1.02 4.42-2.62 5.88L15 15.5v6h6l-2.2-2.2c1.96-1.82 3.2-4.41 3.2-7.3"/></svg>`.replaceAll('currentColor', fg_color)
    const ICON_REPAIR = `<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" viewBox="0 0 24 24"><path fill="currentColor" d="M15.73 3H8.27L3 8.27v7.46L8.27 21h7.46L21 15.73V8.27zM19 14.9L14.9 19H9.1L5 14.9V9.1L9.1 5h5.8L19 9.1z"/><path fill="currentColor" d="M11 7h2v6h-2zm0 8h2v2h-2z"/></svg>`.replaceAll('currentColor', fg_color)

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
                            icon_svg: current_version === remote_version ? ICON_CHECKED : ICON_CHANGE,
                            action() {
                                if (current_version === remote_version) return
                                const shellPath = shell.breeze.data_directory() + '/shell.dll'
                                const shellOldPath = shell.breeze.data_directory() + '/shell_old.dll'
                                const url = PLUGIN_SOURCES[current_source] + data.shell.path

                                upd.set_data({
                                    name: t('更新中...'),
                                    icon_svg: ICON_REPAIR,
                                    disabled: true
                                })

                                const downloadNewShell = () => {
                                    shell.network.download_async(url, shellPath, () => {
                                        upd.set_data({
                                            name: t('新版本已下载，将于下次重启资源管理器生效'),
                                            icon_svg: ICON_CHECKED,
                                            disabled: true
                                        })
                                    }, e => {
                                        upd.set_data({
                                            name: t('更新失败: ') + e,
                                            icon_svg: ICON_REPAIR,
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
                                                    icon_svg: ICON_REPAIR,
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
                                        icon_svg: ICON_REPAIR,
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

                            const local_version = installed ? shell.fs.read(install_path).match(/\/\/ @version:\s*(.*)/)[1] : '未安装'
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
                                        icon_svg: ICON_CHANGE,
                                        disabled: true
                                    })
                                    const path = shell.breeze.data_directory() + '/scripts/' + plugin.local_path
                                    const url = PLUGIN_SOURCES[current_source] + plugin.path
                                    get_async(url).then(data => {
                                        shell.fs.write(path, data)
                                        m.set_data({
                                            name: plugin.name,
                                            icon_svg: ICON_CHECKED,
                                            action() { },
                                            disabled: true
                                        })

                                        shell.println(t('插件安装成功: ') + plugin.name)

                                        reload_local()
                                    }).catch(e => {
                                        m.set_data({
                                            name: plugin.name,
                                            icon_svg: ICON_REPAIR,
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
                                icon_svg: disabled ? ICON_CHECKED : ICON_EMPTY,
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
                                    icon_svg: isPrioritized ? ICON_CHECKED : ICON_EMPTY,
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
                                                icon_svg: ICON_CHECKED
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
                            icon_svg: currentValue ? ICON_CHECKED : ICON_EMPTY,
                            action() {
                                currentValue = !currentValue;
                                setNestedValue(config, configPath, currentValue);
                                write_config();
                                toggle.set_data({
                                    icon_svg: currentValue ? ICON_CHECKED : ICON_EMPTY,
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
                                        icon_svg: ICON_CHECKED,
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
                                    icon_svg: ICON_CHECKED,
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
                        icon_svg: disabled ? ICON_EMPTY : ICON_CHECKED,
                        action() {
                            if (disabled) {
                                shell.fs.rename(shell.breeze.data_directory() + '/scripts/' + name + '.js.disabled', shell.breeze.data_directory() + '/scripts/' + name + '.js')
                                m.set_data({
                                    name,
                                    icon_svg: ICON_CHECKED
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

globalThis.makeBreezeConfigMenu = makeBreezeConfigMenu

shell.menu_controller.add_menu_listener(ctx => {
    if (ctx.context.folder_view?.current_path.startsWith(shell.breeze.data_directory().replaceAll('/', '\\'))) {
        ctx.menu.prepend_menu(makeBreezeConfigMenu(ctx.menu))
    }

    // fixtures
    for (const items of ctx.menu.get_items()) {
        const data = items.data()
        if (data.name_resid === '10580@SHELL32.dll' /* 清空回收站 */ || data.name === '清空回收站') {
            items.set_data({
                disabled: false
            })
        }

        if (data.name?.startsWith('NVIDIA ')) {
            items.set_data({
                icon_svg: `<svg viewBox="0 0 271.7 179.7" xmlns="http://www.w3.org/2000/svg" width="2500" height="1653" fill="#000000"><path d="M101.3 53.6V37.4c1.6-.1 3.2-.2 4.8-.2 44.4-1.4 73.5 38.2 73.5 38.2S148.2 119 114.5 119c-4.5 0-8.9-.7-13.1-2.1V67.7c17.3 2.1 20.8 9.7 31.1 27l23.1-19.4s-16.9-22.1-45.3-22.1c-3-.1-6 .1-9 .4m0-53.6v24.2l4.8-.3c61.7-2.1 102 50.6 102 50.6s-46.2 56.2-94.3 56.2c-4.2 0-8.3-.4-12.4-1.1v15c3.4.4 6.9.7 10.3.7 44.8 0 77.2-22.9 108.6-49.9 5.2 4.2 26.5 14.3 30.9 18.7-29.8 25-99.3 45.1-138.7 45.1-3.8 0-7.4-.2-11-.6v21.1h170.2V0H101.3zm0 116.9v12.8c-41.4-7.4-52.9-50.5-52.9-50.5s19.9-22 52.9-25.6v14h-.1c-17.3-2.1-30.9 14.1-30.9 14.1s7.7 27.3 31 35.2M27.8 77.4s24.5-36.2 73.6-40V24.2C47 28.6 0 74.6 0 74.6s26.6 77 101.3 84v-14c-54.8-6.8-73.5-67.2-73.5-67.2z" fill="#76b900"/></svg>`,
                icon_bitmap: new shell.value_reset()
            })
        }
    }
})

