import { loadAppConfig, saveAppConfig } from "../../utils/appConfig";

export const loadConfig = () => loadAppConfig();

export const saveConfig = (config: any) => {
    saveAppConfig(config);
};
