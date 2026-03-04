import * as shell from "mshell";
import { Button, Text } from "../components";
import { UpdateDataContext, NotificationContext, PluginSourceContext } from "../contexts";
import { useTranslation } from "../hooks";
import { PLUGIN_SOURCES } from "../constants";
import { memo, useContext, useEffect, useState } from "react";

const PluginStore = memo(() => {
    const { updateData } = useContext(UpdateDataContext)!;
    const { setErrorMessage } = useContext(NotificationContext)!;
    const { currentPluginSource } = useContext(PluginSourceContext)!;
    const { t } = useTranslation();

    const [plugins, setPlugins] = useState<any[]>([]);
    const [installingPlugins, setInstallingPlugins] = useState<Set<string>>(new Set());

    useEffect(() => {
        if (updateData) {
            setPlugins(updateData.plugins);
        }
    }, [updateData]);

    const installPlugin = (plugin: any) => {
        if (installingPlugins.has(plugin.name)) return;

        setInstallingPlugins(prev => new Set(prev).add(plugin.name));
        const path = shell.breeze.data_directory() + '/scripts/' + plugin.local_path;
        const url = PLUGIN_SOURCES[currentPluginSource] + plugin.path;
        shell.network.get_async(url, (data: string) => {
            shell.fs.write(path, data);
            shell.println(t('plugin.installSuccess') + plugin.name);
            setInstallingPlugins(prev => {
                const newSet = new Set(prev);
                newSet.delete(plugin.name);
                return newSet;
            });
        }, (e: any) => {
            shell.println(e);
            setErrorMessage(t('plugin.installFailed') + plugin.name);
            setInstallingPlugins(prev => {
                const newSet = new Set(prev);
                newSet.delete(plugin.name);
                return newSet;
            });
        });
    };

    const [_, rerender] = useState(0);

    return (
        <flex gap={20}>
            <Text fontSize={24}>{t("plugin.store")}</Text>
            <flex gap={10} alignItems="stretch" width={570} height={500} autoSize={false}>
                <flex enableScrolling maxHeight={500} alignItems="stretch">
                    {plugins.map((plugin: any) => {
                        let install_path = null;
                        if (shell.fs.exists(shell.breeze.data_directory() + '/scripts/' + plugin.local_path)) {
                            install_path = shell.breeze.data_directory() + '/scripts/' + plugin.local_path;
                        }
                        if (shell.fs.exists(shell.breeze.data_directory() + '/scripts/' + plugin.local_path + '.disabled')) {
                            install_path = shell.breeze.data_directory() + '/scripts/' + plugin.local_path + '.disabled';
                        }
                        const installed = install_path !== null;
                        const local_version_match = installed ? shell.fs.read(install_path).match(/\/\/ @version:\s*(.*)/) : null;
                        const local_version = local_version_match ? local_version_match[1] : t('plugin.notInstalled');
                        const have_update = installed && local_version !== plugin.version;

                        return (
                            <flex key={plugin.name} horizontal alignItems="center">
                                <flex autoSize={false} width={4} height={20} borderRadius={2} backgroundColor={
                                    installed ? (have_update ? '#FFA500' : '#2979FF') : (shell.breeze.is_light_theme() ? '#C0C0C0aa' : '#505050aa')
                                } />
                                <flex gap={10} padding={10} borderRadius={8}
                                    flexGrow={1} horizontal>
                                    <flex gap={10} alignItems="stretch" flexGrow={1}>
                                        <Text fontSize={18}>{plugin.name}</Text>
                                        <Text>{plugin.description}</Text>
                                    </flex>
                                    <flex gap={10} alignItems="center" flexShrink={0}>
                                        <Button onClick={() => installPlugin(plugin)}>
                                            <Text>{installingPlugins.has(plugin.name) ? t("plugin.installing") : (installed ? (have_update ? `${t("plugin.update")} (${local_version} -> ${plugin.version})` : t('plugin.alreadyInstalled')) : t('plugin.install'))}</Text>
                                        </Button>
                                    </flex>
                                </flex>
                            </flex>
                        );
                    })}
                </flex>
            </flex>
        </flex>
    );
});

export default PluginStore;
