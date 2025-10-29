import * as React from "react";
globalThis.h = React.createElement
globalThis.Fragment = React.Fragment

import * as shell from "mshell"
import { plugin } from "./plugin";

import { createRenderer } from "./react/renderer";
import { showConfigPage } from "./config_page";

const SVG_CONFIG = `<svg xmlns="http://www.w3.org/2000/svg" height="24px" viewBox="0 -960 960 960" width="24px" fill="#e3e3e3"><path d="m370-80-16-128q-13-5-24.5-12T307-235l-119 50L78-375l103-78q-1-7-1-13.5v-27q0-6.5 1-13.5L78-585l110-190 119 50q11-8 23-15t24-12l16-128h220l16 128q13 5 24.5 12t22.5 15l119-50 110 190-103 78q1 7 1 13.5v27q0 6.5-2 13.5l103 78-110 190-118-50q-11 8-23 15t-24 12L590-80H370Zm70-80h79l14-106q31-8 57.5-23.5T639-327l99 41 39-68-86-65q5-14 7-29.5t2-31.5q0-16-2-31.5t-7-29.5l86-65-39-68-99 42q-22-23-48.5-38.5T533-694l-13-106h-79l-14 106q-31 8-57.5 23.5T321-633l-99-41-39 68 86 64q-5 15-7 30t-2 32q0 16 2 31t7 30l-86 65 39 68 99-42q22 23 48.5 38.5T427-266l13 106Zm42-180q58 0 99-41t41-99q0-58-41-99t-99-41q-59 0-99.5 41T342-480q0 58 40.5 99t99.5 41Zm-2-140Z"/></svg>`

// remove possibly existing shell_old.dll if able to
if (shell.fs.exists(shell.breeze.data_directory() + '/shell_old.dll')) {
    try {
        shell.fs.remove(shell.breeze.data_directory() + '/shell_old.dll')
    } catch (e) {
        shell.println('Failed to remove old shell.dll: ', e)
    }
}
shell.menu_controller.add_menu_listener(ctx => {
    if (ctx.context.folder_view?.current_path.startsWith(shell.breeze.data_directory().replaceAll('/', '\\'))) {
        ctx.menu.prepend_menu({
            action() {
                showConfigPage()
            },
            name: "Breeze Config",
            icon_svg: SVG_CONFIG
        })
    }

    ctx.screenside_button.add_button(SVG_CONFIG, () => {
        ctx.menu.close()
        showConfigPage()
    })

    // fixtures
    for (const items of ctx.menu.items) {
        const data = items.data()
        if (data.name_resid === '10580@SHELL32.dll' /* 清空回收站 */ || data.name === '清空回收站') {
            items.set_data({
                disabled: false
            })
        }

        if (data.name?.startsWith('NVIDIA ')) {
            items.set_data({
                icon_svg: `<svg viewBox="0 0 271.7 179.7" xmlns="http://www.w3.org/2000/svg" width="2500" height="1653" fill="#000000"><path d="M101.3 53.6V37.4c1.6-.1 3.2-.2 4.8-.2 44.4-1.4 73.5 38.2 73.5 38.2S148.2 119 114.5 119c-4.5 0-8.9-.7-13.1-2.1V67.7c17.3 2.1 20.8 9.7 31.1 27l23.1-19.4s-16.9-22.1-45.3-22.1c-3-.1-6 .1-9 .4m0-53.6v24.2l4.8-.3c61.7-2.1 102 50.6 102 50.6s-46.2 56.2-94.3 56.2c-4.2 0-8.3-.4-12.4-1.1v15c3.4.4 6.9.7 10.3.7 44.8 0 77.2-22.9 108.6-49.9 5.2 4.2 26.5 14.3 30.9 18.7-29.8 25-99.3 45.1-138.7 45.1-3.8 0-7.4-.2-11-.6v21.1h170.2V0H101.3zm0 116.9v12.8c-41.4-7.4-52.9-50.5-52.9-50.5s19.9-22 52.9-25.6v14h-.1c-17.3-2.1-30.9 14.1-30.9 14.1s7.7 27.3 31 35.2M27.8 77.4s24.5-36.2 73.6-40V24.2C47 28.6 0 74.6 0 74.6s26.6 77 101.3 84v-14c-54.8-6.8-73.5-67.2-73.5-67.2z" fill="#76b900"/></svg>`,
                icon_bitmap: new shell.value_reset()
            })
        }
    }
})

globalThis.plugin = plugin as any
globalThis.React = React
globalThis.createRenderer = createRenderer