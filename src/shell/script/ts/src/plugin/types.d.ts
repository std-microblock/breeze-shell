
type FieldPath<T, K extends string> = K extends keyof T ? T[K] : K extends `${infer K1}.${infer K2}` ? K1 extends keyof T ? FieldPath<T[K1], K2> : never : never;

declare function plugin<T = object>(import_meta: { url: string }, default_config?: T): {
    i18n: {
        define(lang: string, data: { [key: string]: string }): void;
        t(key: string): string;
    };
    set_on_menu(callback: (m:
        import('mshell').menu_controller
    ) => void): void;
    config_directory: string;
    config: {
        read_config(): void;
        write_config(): void;
        get<K extends string>(key: K): FieldPath<T, K>;
        set<K extends string>(key: K, value: FieldPath<T, K>): void;
        all(): T;
    };
    log(...args: any[]): void;
};

declare type ShellPluginHelper = ReturnType<typeof plugin>;

export {
    ShellPluginHelper
}