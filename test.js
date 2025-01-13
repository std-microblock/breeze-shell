import * as shell from "mshell"
import * as std from "std"
import * as os from "os"
try {

    shell.println("Hello, world!")
    shell.println(Object.keys(os))
    shell.menu_controller.add_menu_listener(globalThis.a = (a) => {
        // print call stack
        shell.println(123, new Error().stack)
        // a.menu.add_menu_item_after({
        //     type: 'button',
        //     name: 'test',
        //     action: ()=>{
        //         shell.println(123)
        //     }
        // }, 0)
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