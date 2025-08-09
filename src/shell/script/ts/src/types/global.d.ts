import * as shell from "mshell";

declare global {
    var on_plugin_menu: { [key: string]: (sub: shell.menu_controller) => void };
    var plugin: <T = object>(import_meta: { name: string, url: string }, default_config?: T) => {
        i18n: {
            define(lang: string, data: { [key: string]: string }): void;
            t(key: string): string;
        };
        set_on_menu(callback: (m: shell.menu_controller) => void): void;
        config_directory: string;
        config: {
            read_config(): void;
            write_config(): void;
            get(key: string): any;
            set(key: string, value: any): void;
            all(): T;
            on_reload(callback: (config: T) => void): () => void;
        };
        log(...args: any[]): void;
    };
}