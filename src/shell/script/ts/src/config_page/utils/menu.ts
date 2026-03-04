import { menu_controller } from "mshell";

export const showMenu = (callback: (ctl: menu_controller) => void) => {
    const menu = menu_controller.create_detached();
    callback(menu);
    menu.show_at_cursor();
}
