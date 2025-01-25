import * as shell from "mshell"
import * as std from "qjs:std"
import * as os from "qjs:os"
const setTimeout = os.setTimeout.bind(os)
const console = {
    log: shell.println,
    error: shell.println,
}

shell.menu_controller.add_menu_listener((e) => {
    const menus = e.menu.get_menu_items()
    const index = menus.length
    if (e.context.input_box) {
        let count = 0;
        const handle = e.menu.add_menu_item_after({
            type: 'button',
            name: '随机字符串',
            action: () => {
                e.context.input_box.set_text(Math.random().toString(36).substr(2))
                e.menu.close()
            }
        }, index - 4)
    }

    if (e.context.folder_view) {
        if (e.context.folder_view.selected_files.length)
            e.menu.add_menu_item_after({
                type: 'button',
                name: `选中了 ${e.context.folder_view.selected_files.length} 个文件`,
                action: () => {
                    shell.clipboard.set_text(e.context.folder_view.selected_files.map(file => file.name).join('\n'))
                    e.menu.close()
                },
                submenu: e.context.folder_view.selected_files.length === 1 ? [
                    {
                        type: 'button',
                        name: '复制文件名',
                        action: () => {
                            shell.clipboard.set_text(e.context.folder_view.selected_files[0].name)
                            e.menu.close()
                        }
                    },
                    {
                        type: 'button',
                        name: '复制文件路径',
                        action: () => {
                            shell.clipboard.set_text(e.context.folder_view.selected_files[0].path)
                            e.menu.close()
                        }
                    }
                ] : e.context.folder_view.selected_files.map(file => ({
                    type: 'button',
                    name: file.split('\\').pop(),
                    action: () => {
                        shell.clipboard.set_text(file.name)
                        e.menu.close()
                    }
                }))
            }, 1)

        // const menu = e.menu.add_menu_item_after({
        //     type: 'button',
        //     name: '加载中...'
        // }, 0)

        // shell.network.get_async('https://open.iciba.com/dsapi/', data => {
        //     const d = JSON.parse(data)
        //     menu.set_data({
        //         name: d.note,
        //         type: 'button',
        //         action: () => {
        //             shell.clipboard.set_text(d.note)
        //             e.menu.close()
        //         }
        //     })
        // })

        // const menu = e.menu.add_menu_item_after({
        //     type: 'button',
        //     name: 'VirusTotal 查询',
        //     action: () => {
        //         const file = e.context.folder_view.selected_files[0]
        //         shell.network.get_async(`https://www.virustotal.com/vtapi/v2/file/report?apikey=APIKEY&resource=`, data => {
        //             const d = JSON.parse(data)
        //             if (d.response_code === 0) {
        //                 shell.alert('未查询到结果')
        //             } else {
        //                 shell.alert(`检测结果：${d.positives}/${d.total}，详情请查看：${d.permalink}`)
        //             }
        //         })
        //     }
        // }, 0)

        if (e.context.folder_view.selected_files.length === 0) {
            const paths = e.context.folder_view.current_path.split('\\')

            e.menu.add_menu_item_after({
                type: 'button',
                name: '切换到...',
                action: () => {
                    e.context.folder_view.change_folder(paths[0])
                    e.menu.close()
                },
                submenu: paths.map((path, index) => ({
                    type: 'button',
                    name: path,
                    action: () => {
                        e.context.folder_view.change_folder(paths.slice(0, index + 1).join('\\'))
                        e.menu.close()
                    }
                }))
            }, 0)
        }
    }
})