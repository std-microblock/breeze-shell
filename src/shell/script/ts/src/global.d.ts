import { ShellPluginHelper } from "./plugin";

declare global {
    var plugin: ShellPluginHelper;
    var on_plugin_menu: {
        [key: string]: any;
    }

    var React: typeof import('react');
    var createRenderer: typeof import('./react/renderer').createRenderer;
}