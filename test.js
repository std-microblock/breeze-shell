import * as shell from "mshell"
import * as std from "std"
import * as os from "os"
try {

    shell.println("Hello, world!")
    shell.println(Object.keys(os))
    shell.menu_controller.add_menu_listener((e) => {
        // print call stack
        shell.println(e.context.folder_view.current_folder_path, e.context.folder_view.focused_file_path)
        // shell.println(
        //     a.menu?.get_menu_items()[0].name,
        //     a.menu?.get_menu_items()[0].type,
        // )
        const menus = e.menu?.get_menu_items()
        // append after `复制`
        const index = menus?.findIndex((item) => item.name === "复制")

        e.menu.add_menu_item_after({
            type: 'button',
            name: 'Shuffle Buttons51211212',
            action: () => {
                for (const item of menus) {
                    item.set_position(Math.floor(menus.length * Math.random()))
                }
            },
        }, 0)


        // a.menu?.add_menu_item_after({
        //     type: 'button',
        //     name: '测试 JavaScript 按钮',
        //     action: () => {
        //         try {
        //             //    a.menu.set_menu_item_position(
        //             //         a.menu.get_menu_items().findIndex((item) => item.name === "测试 JavaScript 按钮"),
        //             //         Math.floor(a.menu.get_menu_items().length * Math.random())
        //             //     )


        //             // shuffle all items
        // for (let i = 0; i < a.menu.get_menu_items().length; i++) {
        //     a.menu.set_menu_item_position(
        //         i,
        //         Math.floor(a.menu.get_menu_items().length * Math.random())
        //     )
        // }
        //         } catch (e) {
        //             shell.println("Error: " + e);
        //             if (e.stack) {
        //                 shell.println("Stack: " + e.stack);
        //             }
        //         }
        //     }
        // }, index)
    })

    os.setTimeout(() => {
        shell.println("timeout")
    }, 100)
} catch (e) {
    shell.println("Error: " + e);
    if (e.stack) {
        shell.println("Stack: " + e.stack);
    }
}