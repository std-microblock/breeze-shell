import * as shell from "mshell"
import * as std from "qjs:std"
import * as os from "qjs:os"

// This file is a JavaScript file, only named .ts for strict type checking.

shell.menu_controller.add_menu_listener(e => {
    if (e.context.folder_view) {
        // We are in a folder view

        e.menu.prepend_menu({
            name: 'hello',
            action() {
                const m = e.menu.append_menu({
                    name: 'world',
                    action() {
                        shell.println('Hello, world!')
                        m.remove()
                    }
                })
            }
        })

        // Test network request
        const yiyan = e.menu.prepend_menu({
            name: '每日一言',
            action() {
                yiyan.set_data({
                    name: 'loading...',
                    action() {}
                })
                shell.network.get_async('https://open.iciba.com/dsapi/', data=>{
                    const j = JSON.parse(data)
                    yiyan.set_data({
                        name: j.content,
                        action() {
                            shell.clipboard.set_text(j.content)
                            e.menu.close()
                        }
                    })
                })
            }
        })
    }
})