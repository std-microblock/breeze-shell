import * as shell from "mshell"

const PLUGIN_SOURCES = {
    'Github Raw': 'https://raw.githubusercontent.com/breeze-shell/plugins-packed/refs/heads/main/',
    'Enlysure': 'https://breeze.enlysure.com/'
}

let current_source = 'Enlysure'
const get_async = url => {
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
    for (let i = 0; i < str.length; i += maxLenBytes) {
        let x = 0;
        let line = str.substr(i, maxLenBytes)
        while (x < maxLen && line.length > x) {
            if (line.charCodeAt(x) > 255) {
                x++
            }

            if (line.charAt(x) === '\n') {
                break
            }

            x++
        }
        lines.push(line.substr(0, x))
    }

    return lines
}

globalThis.on_plugin_menu = {};

shell.menu_controller.add_menu_listener(ctx => {
    const color = shell.breeze.is_light_theme() ? 'black' : 'white'
    const ICON_EMPTY = new shell.value_reset()
    const ICON_CHECKED = `<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" viewBox="0 0 12 12"><path fill="currentColor" d="M9.765 3.205a.75.75 0 0 1 .03 1.06l-4.25 4.5a.75.75 0 0 1-1.075.015L2.22 6.53a.75.75 0 0 1 1.06-1.06l1.705 1.704l3.72-3.939a.75.75 0 0 1 1.06-.03"/></svg>`.replaceAll('currentColor', color)
    const ICON_CHANGE = `<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" viewBox="0 0 24 24"><path fill="currentColor" d="m17.66 9.53l-7.07 7.07l-4.24-4.24l1.41-1.41l2.83 2.83l5.66-5.66zM4 12c0-2.33 1.02-4.42 2.62-5.88L9 8.5v-6H3l2.2 2.2C3.24 6.52 2 9.11 2 12c0 5.19 3.95 9.45 9 9.95v-2.02c-3.94-.49-7-3.86-7-7.93m18 0c0-5.19-3.95-9.45-9-9.95v2.02c3.94.49 7 3.86 7 7.93c0 2.33-1.02 4.42-2.62 5.88L15 15.5v6h6l-2.2-2.2c1.96-1.82 3.2-4.41 3.2-7.3"/></svg>`.replaceAll('currentColor', color)
    const ICON_REPAIR = `<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" viewBox="0 0 24 24"><path fill="currentColor" d="M15.73 3H8.27L3 8.27v7.46L8.27 21h7.46L21 15.73V8.27zM19 14.9L14.9 19H9.1L5 14.9V9.1L9.1 5h5.8L19 9.1z"/><path fill="currentColor" d="M11 7h2v6h-2zm0 8h2v2h-2z"/></svg>`.replaceAll('currentColor', color)

    if (ctx.context.folder_view?.current_path.startsWith(shell.breeze.data_directory().replaceAll('/', '\\'))) {
        ctx.menu.prepend_menu({
            name: "管理 Breeze Shell",
            submenu(sub) {
                sub.append_menu({
                    name: "插件市场 / 更新本体",
                    submenu(sub) {
                        const updatePlugins = async (page) => {
                            for (const m of sub.get_items().slice(1))
                                m.remove()

                            sub.append_menu({
                                name: '加载中...'
                            })
                            const res = await get_async(PLUGIN_SOURCES[current_source] + 'plugins-index.json');
                            const data = JSON.parse(res)

                            for (const m of sub.get_items().slice(1))
                                m.remove()

                            const current_version = shell.breeze.version();
                            const remote_version = data.shell.version;
                            const upd = sub.append_menu({
                                name: current_version === remote_version ? (current_version + ' (latest)') : `${current_version} -> ${remote_version}`,
                                icon_svg: current_version === remote_version ? ICON_CHECKED : ICON_CHANGE,
                                action() {
                                    if (current_version === remote_version) return
                                    const path = shell.breeze.data_directory() + '/shell_new.dll'
                                    const url = PLUGIN_SOURCES[current_source] + data.shell.path
                                    upd.set_data({
                                        name: '更新中...',
                                        icon_svg: ICON_REPAIR,
                                        disabled: true
                                    })
                                    shell.network.download_async(url, path, () => {
                                        upd.set_data({
                                            name: '新版本已下载，将于下次重启资源管理器生效',
                                            icon_svg: ICON_CHECKED,
                                            disabled: true
                                        })
                                    }, e => {
                                        upd.set_data({
                                            name: '更新失败: ' + e,
                                            icon_svg: ICON_REPAIR,
                                            disabled: false
                                        })
                                    })
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

                                            shell.println('插件安装成功: ' + plugin.name)

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
                                                            ctx.menu.close()
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
                                            name: '版本: ' + plugin.version
                                        })
                                        sub.append_menu({
                                            name: '作者: ' + plugin.author
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
                            name: '当前源: ' + current_source,
                            submenu(sub) {
                                for (const key in PLUGIN_SOURCES) {
                                    sub.append_menu({
                                        name: key,
                                        action() {
                                            current_source = key

                                            source.set_data({
                                                name: '当前源: ' + key
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

                const reload_local = () => {
                    const installed = shell.fs.readdir(shell.breeze.data_directory() + '/scripts')
                        .map(v => v.split('/').pop())
                        .filter(v => v.endsWith('.js') || v.endsWith('.disabled'))

                    for (const m of sub.get_items().slice(1))
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
                                    name: '删除',
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
        })
    }

    // fixtures
    for (const items of ctx.menu.get_items()) {
        const data = items.data()
        if (data.name_resid === '10580@SHELL32.dll' /* 清空回收站 */) {
            items.set_data({
                disabled: false
            })
        }
    }
})

