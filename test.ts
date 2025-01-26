import * as shell from "mshell"
import * as std from "qjs:std"
import * as os from "qjs:os"

// This file is a JavaScript file, only named .ts for strict type checking.
shell.println('Reloaded')
shell.menu_controller.add_menu_listener(e => {
    if (e.context.folder_view) {
        const fv = e.context.folder_view
        // const menu = e.menu.append_menu({
        //     name: '测试',
        //     type: 'button',
        //     action: () => {
        //         shell.network.get_async('https://open.iciba.com/dsapi/', data => {
        //             const d = JSON.parse(data)
        //             menu.set_data({
        //                 name: d.note,
        //                 type: 'button',
        //                 action: () => {
        //                     shell.clipboard.set_text(d.note)
        //                     e.menu.close()
        //                 }
        //             })
        //         })
        //     }
        // })

        if (fv.selected_files.length) {
            let fullPath = false
            const menuSelected = e.menu.prepend_menu({
                name: '选中的文件',
                type: 'button',
                submenu: m => {

                    let fullPath = true

                    const btn = m.append_menu({
                        name: fullPath ? '显示文件名' : '显示完整路径',
                        type: 'button',
                        action: () => {
                            fullPath = !fullPath
                            const items = m.get_items()

                            for (let i = 2; i < items.length; i++) {
                                const item = items[i]
                                const txt = fullPath ? fv.selected_files[i - 2] : fv.selected_files[i - 2].split('\\').pop() ?? ''
                                item.set_data({
                                    name: txt,
                                    action: () => {
                                        shell.clipboard.set_text(txt)
                                        m.close()
                                    }
                                })
                            }

                            btn.set_data({
                                name: fullPath ? '显示文件名' : '显示完整路径'
                            })
                        }
                    })

                    m.append_menu({
                        type: 'spacer'
                    })

                    for (const path of fv.selected_files) {
                        m.append_menu({
                            name: fullPath ? path : path.split('/').pop(),
                            type: 'button',
                            action: () => {
                                shell.clipboard.set_text(path)
                                m.close()
                            }
                        })
                    }
                }
            })


        }

        e.menu.append_menu({
            name: '测试',
            type: 'button',
            action: () => {
                e.menu.append_menu({
                    name: '测试',
                    type: 'button',
                    action: () => {
                        shell.println('测试')
                    }
                })
            }
        })
    }
})