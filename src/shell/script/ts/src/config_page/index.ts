// Export all components and utilities from the config module
export { ConfigApp as default } from './ConfigApp';

// Export individual components
export { default as Sidebar } from './Sidebar';
export { default as ContextMenuConfig } from './pages/ContextMenuConfig';
export { default as UpdatePage } from './pages/UpdatePage';
export { default as PluginStore } from './pages/PluginStore';
export { default as PluginConfig } from './pages/PluginConfig';

// Export UI components
export {
    Button,
    Text,
    TextButton,
    Toggle,
    SidebarItem,
    PluginCheckbox,
    PluginMoreButton,
    PluginItem,
    SimpleMarkdownRender,
    iconElement
} from './components';

// Export contexts
export {
    ContextMenuContext,
    DebugConsoleContext,
    PluginLoadOrderContext,
    UpdateDataContext,
    NotificationContext
} from './contexts';

// Export utilities
export {
    getNestedValue,
    setNestedValue,
    getAllSubkeys,
    applyPreset,
    checkPresetMatch,
    getCurrentPreset,
    loadConfig,
    saveConfig,
    loadPlugins as reloadPlugins,
    togglePlugin,
    deletePlugin,
    isPluginInstalled,
    getPluginVersion
} from './utils';

// Export hooks
export {
    useTranslation,
    useHoverActive
} from './hooks';

// Export constants
export {
    languages,
    PLUGIN_SOURCES,
    ICON_CONTEXT_MENU,
    ICON_UPDATE,
    ICON_PLUGIN_STORE,
    ICON_PLUGIN_CONFIG,
    ICON_MORE_VERT,
    ICON_BREEZE,
    theme_presets,
    animation_presets,
    WINDOW_WIDTH,
    WINDOW_HEIGHT,
    SIDEBAR_WIDTH
} from './constants';

import * as shell from "mshell";
import ConfigApp from './ConfigApp';

let existingConfigWindow: shell.breeze_ui.window | null = null;
export const showConfigPage = () => {
    shell.breeze.allow_js_reload(false);
    const win = shell.breeze_ui.window.create_ex("Breeze Config", 800, 600, () => {
        shell.breeze.allow_js_reload(true)
        if (existingConfigWindow === win)
            existingConfigWindow = null;
    });
    if (existingConfigWindow)
        existingConfigWindow.close();
    existingConfigWindow = win;

    const widget = shell.breeze_ui.widgets_factory.create_flex_layout_widget();
    const renderer = createRenderer(widget);
    renderer.render(React.createElement(ConfigApp, null));
    win.root_widget = widget
}
