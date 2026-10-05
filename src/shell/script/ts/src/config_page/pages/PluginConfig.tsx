import * as shell from "mshell";
import { showMenu, loadPlugins, togglePlugin, deletePlugin } from "../utils";
import {
    Button, InfoBar, PageHeader, Pill, SettingsCard, SettingsGroup, ToggleSwitch, fluent
} from "../components/Fluent";
import { PluginLoadOrderContext } from "../contexts";
import { useTranslation } from "../hooks";
import { CONTENT_WIDTH, ICON_EXTENSION, ICON_FOLDER, ICON_MORE_VERT, PAGE_BODY_HEIGHT, SCROLL_GUTTER } from "../constants";
import { memo, useContext, useEffect, useState } from "react";

const PluginConfig = memo(() => {
    const { order, update } = useContext(PluginLoadOrderContext)!;
    const { t } = useTranslation();
    const c = fluent();
    const [installedPlugins, setInstalledPlugins] = useState<string[]>([]);

    const reloadPluginsList = () => setInstalledPlugins(loadPlugins());
    useEffect(() => { reloadPluginsList(); }, []);

    const isPrioritized = (name: string) => order?.includes(name) || false;
    const isEnabled = (name: string) => shell.fs.exists(shell.breeze.data_directory() + '/scripts/' + name + '.js');

    const togglePrioritize = (name: string) => {
        const newOrder = [...(order || [])];
        const index = newOrder.indexOf(name);
        if (index >= 0) newOrder.splice(index, 1);
        else newOrder.unshift(name);
        update(newOrder);
    };

    const showContextMenu = (pluginName: string) => {
        showMenu(menu => {
            menu.append_menu({
                name: isPrioritized(pluginName) ? t('plugin.cancelPriority') : t('plugin.setPriority'),
                action() {
                    togglePrioritize(pluginName);
                    menu.close();
                }
            });
            menu.append_menu({
                name: t("common.delete"),
                action() {
                    deletePlugin(pluginName);
                    reloadPluginsList();
                    menu.close();
                }
            });
            if (on_plugin_menu[pluginName]) {
                on_plugin_menu[pluginName](menu)
            }
        });
    };

    const card = (name: string) => (
        <SettingsCard key={name} icon={ICON_EXTENSION} title={name}>
            {isPrioritized(name) && <Pill text={t("plugin.priorityShort")} fg={c.accent} bg={c.accentSubtle} />}
            <ToggleSwitch
                value={isEnabled(name)}
                onChange={() => { togglePlugin(name); reloadPluginsList(); }}
                onLabel={t("settings.on")}
                offLabel={t("settings.off")}
            />
            <Button icon={ICON_MORE_VERT} variant="subtle" onClick={() => showContextMenu(name)} />
        </SettingsCard>
    );

    const prioritized = installedPlugins.filter(isPrioritized);
    const regular = installedPlugins.filter(name => !isPrioritized(name));

    return (
        <flex gap={16} alignItems="stretch" width={CONTENT_WIDTH}>
            <PageHeader title={t("plugin.config")} subtitle={t("plugin.configSubtitle", { n: installedPlugins.length })} gutter={SCROLL_GUTTER}>
                <Button icon={ICON_FOLDER} label={t("plugin.openFolder")} onClick={() => shell.subproc.open(shell.breeze.data_directory() + '/scripts', "")} />
            </PageHeader>
            <flex enableScrolling maxHeight={PAGE_BODY_HEIGHT} alignItems="stretch" paddingRight={SCROLL_GUTTER} paddingBottom={16} gap={4}>
                {installedPlugins.length === 0 && <InfoBar severity="info" title={t("plugin.empty")} message={t("plugin.emptyDesc")} />}
                {prioritized.length > 0 && <SettingsGroup title={t("plugin.priorityLoad")}>{prioritized.map(card)}</SettingsGroup>}
                {regular.length > 0 && <SettingsGroup title={t("plugin.installed")}>{regular.map(card)}</SettingsGroup>}
            </flex>
        </flex>
    );
});

export default PluginConfig;
