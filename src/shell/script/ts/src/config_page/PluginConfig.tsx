import * as shell from "mshell";
import { showMenu } from "./utils";
import { Text, PluginItem } from "./components";
import { PluginLoadOrderContext } from "./contexts";
import { useTranslation, loadPlugins, togglePlugin, deletePlugin } from "./utils";
import { memo, useContext, useEffect, useState } from "react";

const PluginConfig = memo(() => {
    const { order, update } = useContext(PluginLoadOrderContext)!;
    const { t } = useTranslation();

    const [installedPlugins, setInstalledPlugins] = useState<string[]>([]);

    useEffect(() => {
        reloadPluginsList();
    }, []);

    const reloadPluginsList = () => {
        const plugins = loadPlugins();
        setInstalledPlugins(plugins);
    };

    const handleTogglePlugin = (name: string) => {
        togglePlugin(name);
        reloadPluginsList();
    };

    const handleDeletePlugin = (name: string) => {
        deletePlugin(name);
        reloadPluginsList();
    };

    const isPrioritized = (name: string) => {
        return order?.includes(name) || false;
    };

    const togglePrioritize = (name: string) => {
        const newOrder = [...(order || [])];
        if (newOrder.includes(name)) {
            const index = newOrder.indexOf(name);
            newOrder.splice(index, 1);
        } else {
            newOrder.unshift(name);
        }
        update(newOrder);
    };

    const showContextMenu = (pluginName: string) => {
        showMenu(menu => {
            menu.append_menu({
                name: isPrioritized(pluginName) ? '取消优先加载' : '设为优先加载',
                action() {
                    togglePrioritize(pluginName);
                    menu.close();
                }
            });
            menu.append_menu({
                name: t("删除"),
                action() {
                    handleDeletePlugin(pluginName);
                    menu.close();
                }
            });
            if (on_plugin_menu[pluginName]) {
                on_plugin_menu[pluginName](menu)
            }
        });
    };

    const prioritizedPlugins = installedPlugins.filter(name => isPrioritized(name));
    const regularPlugins = installedPlugins.filter(name => !isPrioritized(name));

    return (
        <flex gap={20} width={580} height={550} autoSize={false} alignItems="stretch">
            <Text fontSize={24}>{t("插件配置")}</Text>

            <flex enableScrolling maxHeight={500} alignItems="stretch">
                {prioritizedPlugins.length > 0 && (
                    <flex gap={10} alignItems="stretch" paddingBottom={10} paddingTop={10}>
                        <Text fontSize={16}>{t("优先加载插件")}</Text>
                        {prioritizedPlugins.map(name => {
                            const isEnabled = shell.fs.exists(shell.breeze.data_directory() + '/scripts/' + name + '.js');
                            return (
                                <PluginItem
                                    key={name}
                                    name={name}
                                    isEnabled={isEnabled}
                                    isPrioritized={true}
                                    onToggle={() => handleTogglePlugin(name)}
                                    onMoreClick={showContextMenu}
                                />
                            );
                        })}

                        <flex height={1} autoSize={false} backgroundColor={shell.breeze.is_light_theme() ? '#E0E0E0' : '#505050'} />
                    </flex>
                )}
                <flex gap={10} alignItems="stretch">
                    {prioritizedPlugins.length === 0 && <Text fontSize={16}>{t("已安装插件")}</Text>}
                    {regularPlugins.map(name => {
                        const isEnabled = shell.fs.exists(shell.breeze.data_directory() + '/scripts/' + name + '.js');
                        return (
                            <PluginItem
                                key={name}
                                name={name}
                                isEnabled={isEnabled}
                                isPrioritized={false}
                                onToggle={() => handleTogglePlugin(name)}
                                onMoreClick={showContextMenu}
                            />
                        );
                    })}
                </flex>
            </flex>

        </flex>
    );
});

export default PluginConfig;