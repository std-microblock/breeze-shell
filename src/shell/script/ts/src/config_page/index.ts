export { ConfigApp as default } from './ConfigApp';

import * as shell from "mshell";
import ConfigApp from './ConfigApp';
import { WINDOW_WIDTH, WINDOW_HEIGHT } from './constants';

let existingConfigWindow: shell.breeze_ui.window | null = null;
let existingConfigRenderer: { unmount: () => void } | null = null;
let configWindowGeneration = 0;

const disposeExistingConfigWindow = () => {
    existingConfigRenderer?.unmount();
    existingConfigRenderer = null;

    if (existingConfigWindow) {
        const win = existingConfigWindow;
        existingConfigWindow = null;
        win.close();
    }
};

export const showConfigPage = (page = 'context-menu') => {
    disposeExistingConfigWindow();
    shell.breeze.allow_js_reload(false);
    const generation = ++configWindowGeneration;
    let renderer: ReturnType<typeof createRenderer> | null = null;
    const win = shell.breeze_ui.window.create_ex("Breeze Config", WINDOW_WIDTH, WINDOW_HEIGHT, () => {
        renderer?.unmount();
        if (existingConfigWindow === win)
            existingConfigWindow = null;
        if (existingConfigRenderer === renderer)
            existingConfigRenderer = null;
        if (configWindowGeneration === generation)
            shell.breeze.allow_js_reload(true);
    });
    existingConfigWindow = win;

    const widget = shell.breeze_ui.widgets_factory.create_flex_layout_widget();
    renderer = createRenderer(widget);
    existingConfigRenderer = renderer;
    renderer.render(React.createElement(ConfigApp, { initialPage: page }));
    win.root_widget = widget
}
