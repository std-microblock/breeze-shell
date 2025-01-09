import * as shell from "mshell"

try {
    
    shell.println("Hello, world!")
    shell.println(Object.keys(shell.menu_controller))
    // shell.menu_controller.add_menu_listener((a)=>{
    //     a.menu.add_menu_item_after({
    //         type: 'button',
    //         name: 'test',
    //         action: ()=>{
    //             shell.println(123)
    //         }
    //     }, 0)
    // })
} catch (e) {
    shell.println("Error: " + e);
    if (e.stack) {
        shell.println("Stack: " + e.stack);
    }
}