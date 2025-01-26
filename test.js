import * as shell from "mshell"
import * as std from "qjs:std"
import * as os from "qjs:os"

shell.println('Reloaded')
shell.menu_controller.add_menu_listener(e => {
    if (e.context.folder_view) {
        const fv = e.context.folder_view

        if (fv.selected_files.length) {
            let fullPath = false
            const menuSelected = e.menu.add_menu_item_after({
                name: '选中的文件',
                type: 'button'
            }, 0)

            const updateMenuSelected = () => {
                menuSelected.set_data({
                    submenu: [{
                        name: fullPath ? '完整路径' : '文件名',
                        action: () => {
                            menuSelected.set_data({
                                submenu: [],
                                name: '123'
                            })
                            shell.println('toggle')
                            fullPath = !fullPath
                            updateMenuSelected()
                        }
                    }, ...fv.selected_files.map(v => {
                        return {
                            name: v,
                            action: () => {
                                shell.clipboard.set_text(v)
                                e.menu.close()
                            }
                        }
                    })]
                })
            }

            updateMenuSelected()

        }
    }
})