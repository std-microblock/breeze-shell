import * as shell from "mshell";
import { PLUGIN_SOURCES } from "../plugin/constants";

export const DEFAULT_UPDATE_SOURCE = "Enlysure";

const CONFIG_PATH = shell.breeze.data_directory() + "/config.json";

export const loadAppConfig = () => {
    if (!shell.fs.exists(CONFIG_PATH)) {
        return {};
    }

    try {
        const raw = shell.fs.read(CONFIG_PATH);
        return raw ? JSON.parse(raw) : {};
    } catch (e) {
        shell.println("[Config] Failed to load config:", e);
        return {};
    }
};

export const saveAppConfig = (config: any) => {
    shell.fs.write(CONFIG_PATH, JSON.stringify(config, null, 4));
};

export const isAutoUpdateEnabled = (config: any) => config?.auto_update !== false;

export const getUpdateSource = (config: any) => {
    const source = config?.update_source;
    return source && Object.prototype.hasOwnProperty.call(PLUGIN_SOURCES, source)
        ? source
        : DEFAULT_UPDATE_SOURCE;
};
