import * as shell from "mshell";

export const doOneCommanderCompat = () => {
    shell.menu_controller.add_menu_listener(m => {
        const do_action = (keys: string[]) => () => {
            m.menu.close();
            shell.infra.setTimeout(() => {
                shell.win32.simulate_hotkeys(keys);
            }, 50);
            shell.infra.setTimeout(() => {
                shell.win32.simulate_hotkeys(keys);
            }, 70);
            shell.infra.setTimeout(() => {
                shell.win32.simulate_hotkeys(keys);
            }, 100);
        }

        for (const i of m.menu.items) {
            if (i.data().name === "重命名" || i.data().name === "Rename") {
                i.update_data({
                    action: do_action(['f2'])
                })
            }
        }

        const fill = shell.breeze.is_light_theme() ? "fill=\"#000000\"" : "fill=\"#FFFFFF\"";
        const zh = shell.breeze.user_language().startsWith('zh');
        const NEW_NAME = zh ? "新建" : "New";
        const CREATE_FOLDER_NAME = zh ? "文件夹" : "Folder";
        const CREATE_FILE_NAME = zh ? "文件" : "File";
        m.menu.append_item_after({
            name: NEW_NAME,
            submenu(m) {
                m.append_item({
                    name: CREATE_FOLDER_NAME,
                    action: do_action(['ctrl', 'shift', 'n']),
                    icon_svg: `<svg xmlns="http://www.w3.org/2000/svg" ${fill} viewBox="0 0 24 24"><path d="M10 4H2v16h20V6H12l-2-2z"/></svg>`
                })
                m.append_item({
                    name: CREATE_FILE_NAME,
                    action: do_action(['ctrl', 'n']),
                    icon_svg: `<svg xmlns="http://www.w3.org/2000/svg" ${fill} viewBox="0 0 24 24"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8l-6-6zM13 3.5L18.5 9H13V3.5z"/></svg>`
                })
            }
        }, -2)
    })
}
