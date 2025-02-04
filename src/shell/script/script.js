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
        lines.push(line.substr(0, x))
        i += x
    }

    return lines
}

globalThis.on_plugin_menu = {};

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

shell.menu_controller.add_menu_listener(ctx => {
    const currentLang = shell.breeze.user_language() === 'zh-CN' ? 'zh-CN' : 'en-US'
    const t = (key) => {
        return languages[currentLang][key] || key
    }

    const fg_color = shell.breeze.is_light_theme() ? 'black' : 'white'
    const ICON_EMPTY = new shell.value_reset()
    const ICON_CHECKED = `<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" viewBox="0 0 12 12"><path fill="currentColor" d="M9.765 3.205a.75.75 0 0 1 .03 1.06l-4.25 4.5a.75.75 0 0 1-1.075.015L2.22 6.53a.75.75 0 0 1 1.06-1.06l1.705 1.704l3.72-3.939a.75.75 0 0 1 1.06-.03"/></svg>`.replaceAll('currentColor', fg_color)
    const ICON_CHANGE = `<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" viewBox="0 0 24 24"><path fill="currentColor" d="m17.66 9.53l-7.07 7.07l-4.24-4.24l1.41-1.41l2.83 2.83l5.66-5.66zM4 12c0-2.33 1.02-4.42 2.62-5.88L9 8.5v-6H3l2.2 2.2C3.24 6.52 2 9.11 2 12c0 5.19 3.95 9.45 9 9.95v-2.02c-3.94-.49-7-3.86-7-7.93m18 0c0-5.19-3.95-9.45-9-9.95v2.02c3.94.49 7 3.86 7 7.93c0 2.33-1.02 4.42-2.62 5.88L15 15.5v6h6l-2.2-2.2c1.96-1.82 3.2-4.41 3.2-7.3"/></svg>`.replaceAll('currentColor', fg_color)
    const ICON_REPAIR = `<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" viewBox="0 0 24 24"><path fill="currentColor" d="M15.73 3H8.27L3 8.27v7.46L8.27 21h7.46L21 15.73V8.27zM19 14.9L14.9 19H9.1L5 14.9V9.1L9.1 5h5.8L19 9.1z"/><path fill="currentColor" d="M11 7h2v6h-2zm0 8h2v2h-2z"/></svg>`.replaceAll('currentColor', fg_color)

    if (ctx.context.folder_view?.current_path.startsWith(shell.breeze.data_directory().replaceAll('/', '\\'))) {
        ctx.menu.prepend_menu({
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
                            const upd = sub.append_menu({
                                name: current_version === remote_version ? (current_version + ' (latest)') : `${current_version} -> ${remote_version}`,
                                icon_svg: current_version === remote_version ? ICON_CHECKED : ICON_CHANGE,
                                action() {
                                    if (current_version === remote_version) return
                                    const path = shell.breeze.data_directory() + '/shell_new.dll'
                                    const url = PLUGIN_SOURCES[current_source] + data.shell.path
                                    upd.set_data({
                                        name: t('更新中...'),
                                        icon_svg: ICON_REPAIR,
                                        disabled: true
                                    })
                                    shell.network.download_async(url, path, () => {
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

        if (data.name?.startsWith('NVIDIA ')) {
            items.set_data({
                icon_svg: `<svg fill="none" viewBox="0 0 24 24">
	<path fill="${fg_color} d="M8.948 8.798v-1.43a6.7 6.7 0 0 1 .424-.018c3.922-.124 6.493 3.374 6.493 3.374s-2.774 3.851-5.75 3.851c-.398 0-.787-.062-1.158-.185v-4.346c1.528.185 1.837.857 2.747 2.385l2.04-1.714s-1.492-1.952-4-1.952a6.016 6.016 0 0 0-.796.035m0-4.735v2.138l.424-.027c5.45-.185 9.01 4.47 9.01 4.47s-4.08 4.964-8.33 4.964c-.37 0-.733-.035-1.095-.097v1.325c.3.035.61.062.91.062 3.957 0 6.82-2.023 9.593-4.408.459.371 2.34 1.263 2.73 1.652-2.633 2.208-8.772 3.984-12.253 3.984-.335 0-.653-.018-.971-.053v1.864H24V4.063z"/>
	<path fill="${fg_color}" d="m0 10.326v1.131c-3.657-.654-4.673-4.46-4.673-4.46s1.758-1.944 4.673-2.262v1.237H8.94c-1.528-.186-2.73 1.245-2.73 1.245s.68 2.412 2.739 3.11M2.456 10.9s2.164-3.197 6.5-3.533V6.201C4.153 6.59 0 10.653 0 10.653s2.35 6.802 8.948 7.42v-1.237c-4.84-.6-6.492-5.936-6.492-5.936z"/>
</svg>`
            })
        }
    }
})

