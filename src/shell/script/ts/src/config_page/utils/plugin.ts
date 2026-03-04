import * as shell from "mshell";

export const loadPlugins = () => {
    return shell.fs.readdir(shell.breeze.data_directory() + '/scripts')
        .map(v => v.split('/').pop())
        .filter(v => v.endsWith('.js') || v.endsWith('.disabled'))
        .map(v => v.replace('.js', '').replace('.disabled', ''));
};

export const togglePlugin = (name: string) => {
    const path = shell.breeze.data_directory() + '/scripts/' + name;
    if (shell.fs.exists(path + '.js')) {
        shell.fs.rename(path + '.js', path + '.js.disabled');
    } else if (shell.fs.exists(path + '.js.disabled')) {
        shell.fs.rename(path + '.js.disabled', path + '.js');
    }
};

export const deletePlugin = (name: string) => {
    const path = shell.breeze.data_directory() + '/scripts/' + name;
    if (shell.fs.exists(path + '.js')) {
        shell.fs.remove(path + '.js');
    }
    if (shell.fs.exists(path + '.js.disabled')) {
        shell.fs.remove(path + '.js.disabled');
    }
};

export const isPluginInstalled = (plugin: any) => {
    if (shell.fs.exists(shell.breeze.data_directory() + '/scripts/' + plugin.local_path)) {
        return shell.breeze.data_directory() + '/scripts/' + plugin.local_path;
    }
    if (shell.fs.exists(shell.breeze.data_directory() + '/scripts/' + plugin.local_path + '.disabled')) {
        return shell.breeze.data_directory() + '/scripts/' + plugin.local_path + '.disabled';
    }
    return null;
};

export const getPluginVersion = (installPath: string) => {
    const local_version_match = shell.fs.read(installPath).match(/\/\/ @version:\s*(.*)/);
    return local_version_match ? local_version_match[1] : '未安装';
};
