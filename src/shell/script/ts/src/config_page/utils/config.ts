import * as shell from "mshell";

export const loadConfig = () => {
    const current_config_path = shell.breeze.data_directory() + '/config.json';
    const current_config = shell.fs.read(current_config_path);
    return JSON.parse(current_config);
};

export const saveConfig = (config: any) => {
    shell.fs.write(shell.breeze.data_directory() + '/config.json', JSON.stringify(config, null, 4));
};
