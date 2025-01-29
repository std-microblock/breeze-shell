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
                shell.println(e.menu.get_items().sort(v=>Math.random() - 0.5).find(v=>v.data().icon_bitmap)?.data().icon_bitmap)
                const m = e.menu.append_menu({
                    name: 'world',
                    action() {
                        shell.println('Hello, world!')
                        m.remove()
                    },
                    // icon_svg: '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"/><line x1="12" y1="8" x2="12" y2="16"/><line x1="8" y1="12" x2="16" y2="12"/></svg>'
                    icon_bitmap: e.menu.get_items().sort(v=>Math.random() - 0.5).find(v=>v.data().icon_bitmap)?.data().icon_bitmap
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