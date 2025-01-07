import * as shell from "mshell"

try {
    shell.println("Hello, world!")
    shell.println(Object.keys(shell))
    shell.menu_controller.test()
} catch (e) {
    shell.println("Error: " + e);
    if (e.stack) {
        shell.println("Stack: " + e.stack);
    }
}