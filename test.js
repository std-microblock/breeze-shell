import * as shell from "mshell"
import * as std from "std"
import * as os from "os"
try {
    shell.menu_controller.add_menu_listener((e) => {
        const menus = e.menu.get_menu_items()
        const index = menus.length
        if (e.context.input_box) {
            e.menu.add_menu_item_after({
                type: 'button',
                name: '测试 JavaScript 按钮',
                action: () => {
                
                    std.setTimeout(() => {
                        e.context.input_box.set_text("Hello, world!" + Math.random())
                        e.menu.close()
                    }, 1000);
                }
            }, index)
        }
    })
} catch (e) {
    shell.println("Error: " + e);
    if (e.stack) {
        shell.println("Stack: " + e.stack);
    }
}