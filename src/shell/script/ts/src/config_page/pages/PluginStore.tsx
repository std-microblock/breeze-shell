import * as shell from "mshell";
import { Text } from "../components";
import { Button, InfoBar, PageHeader, Pill, fluent } from "../components/Fluent";
import { iconElement } from "../components/Icon";
import { UpdateDataContext, NotificationContext, PluginSourceContext } from "../contexts";
import { useTranslation } from "../hooks";
import { CONTENT_WIDTH, ICON_DOWNLOAD, ICON_EXTENSION, PAGE_BODY_HEIGHT, PLUGIN_SOURCES, SCROLL_GUTTER } from "../constants";
import { memo, useContext, useState } from "react";

const DESC_WIDTH = CONTENT_WIDTH - SCROLL_GUTTER - 16 * 2 - 36 - 16 * 2 - 150;

const PluginStore = memo(() => {
    const { updateData } = useContext(UpdateDataContext)!;
    const { setErrorMessage } = useContext(NotificationContext)!;
    const { currentPluginSource } = useContext(PluginSourceContext)!;
    const { t } = useTranslation();
    const c = fluent();
    const [installing, setInstalling] = useState<Set<string>>(new Set());
    const [, rerender] = useState(0);
    const plugins: any[] = updateData?.plugins ?? [];

    const finish = (name: string) => setInstalling(prev => {
        const next = new Set(prev);
        next.delete(name);
        return next;
    });

    const installPlugin = (plugin: any) => {
        if (installing.has(plugin.name)) return;
        setInstalling(prev => new Set(prev).add(plugin.name));
        const path = shell.breeze.data_directory() + '/scripts/' + plugin.local_path;
        shell.network.get_async(PLUGIN_SOURCES[currentPluginSource] + plugin.path, (data: string) => {
            shell.fs.write(path, data);
            shell.println(t('plugin.installSuccess') + plugin.name);
            finish(plugin.name);
            rerender(n => n + 1);
        }, (e: any) => {
            shell.println(e);
            setErrorMessage(t('plugin.installFailed') + plugin.name);
            finish(plugin.name);
        });
    };

    const localState = (plugin: any) => {
        const base = shell.breeze.data_directory() + '/scripts/' + plugin.local_path;
        const path = shell.fs.exists(base) ? base : shell.fs.exists(base + '.disabled') ? base + '.disabled' : null;
        if (!path) return { installed: false, version: null as string | null };
        const match = shell.fs.read(path).match(/\/\/ @version:\s*(.*)/);
        return { installed: true, version: match ? match[1].trim() : null };
    };

    return (
        <flex gap={16} alignItems="stretch" width={CONTENT_WIDTH}>
            <PageHeader title={t("plugin.store")} subtitle={t("plugin.storeSubtitle", { source: currentPluginSource })} gutter={SCROLL_GUTTER} />
            <flex enableScrolling maxHeight={PAGE_BODY_HEIGHT} alignItems="stretch" paddingRight={SCROLL_GUTTER} paddingBottom={16} gap={4}>
                {!updateData && <InfoBar severity="info" title={t("common.loading")} />}
                {plugins.map((plugin: any) => {
                    const local = localState(plugin);
                    const hasUpdate = local.installed && local.version !== plugin.version;
                    const busy = installing.has(plugin.name);
                    return (
                        <flex key={plugin.name} horizontal gap={16} alignItems="center" padding={16} borderRadius={7}
                            borderWidth={1} borderColor={c.cardStroke} backgroundColor={c.card}>
                            <flex width={36} height={36} borderRadius={8} backgroundColor={c.accentSubtle} justifyContent="center" alignItems="center">
                                {iconElement(ICON_EXTENSION, 18, c.accent)}
                            </flex>
                            <flex gap={4} flexGrow={1} flexShrink={1}>
                                <flex horizontal gap={8} alignItems="center">
                                    <Text fontSize={14} fontWeight={600} color={c.text}>{plugin.name}</Text>
                                    <Text fontSize={12} color={c.textTertiary}>{`v${plugin.version}`}</Text>
                                    {local.installed && !hasUpdate && <Pill text={t("plugin.alreadyInstalled")} fg={c.accent} bg={c.accentSubtle} />}
                                    {hasUpdate && <Pill text={`${local.version ?? "?"} → ${plugin.version}`} fg={c.light ? "#9D5D00FF" : "#FCE100FF"} bg={c.light ? "#FFF4CEE6" : "#433519E6"} />}
                                </flex>
                                <Text fontSize={12} color={c.textSecondary} maxWidth={DESC_WIDTH}>{plugin.description ?? ""}</Text>
                            </flex>
                            <Button
                                icon={ICON_DOWNLOAD}
                                label={busy ? t("plugin.installing") : hasUpdate ? t("plugin.update") : local.installed ? t("plugin.reinstall") : t("plugin.install")}
                                variant={!local.installed || hasUpdate ? "accent" : "standard"}
                                disabled={busy}
                                onClick={() => installPlugin(plugin)}
                            />
                        </flex>
                    );
                })}
            </flex>
        </flex>
    );
});

export default PluginStore;
