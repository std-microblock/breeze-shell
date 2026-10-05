import * as shell from "mshell";
import { SimpleMarkdownRender, Text } from "../components";
import {
    Button, InfoBar, PageHeader, ProgressBar, SettingsCard, SettingsGroup, ToggleSwitch, fluent
} from "../components/Fluent";
import { AppConfigContext, UpdateDataContext, NotificationContext, PluginSourceContext } from "../contexts";
import { useTranslation } from "../hooks";
import { memo, useContext, useEffect, useState, useMemo } from "react";
import { formatBytes, UpdateProgress, runManualUpdate, readPendingUpdate, restartExplorer, getInjectorInfo } from "../../utils/update";
import { compareVersions } from "../../utils/semver";
import { CONTENT_WIDTH, ICON_BREEZE, ICON_DOWNLOAD, ICON_HISTORY, ICON_SYNC, PAGE_BODY_HEIGHT, SCROLL_GUTTER } from "../constants";

const BODY_WIDTH = CONTENT_WIDTH - SCROLL_GUTTER;

const UpdatePage = memo(() => {
    const { config, updateConfig } = useContext(AppConfigContext)!;
    const { updateData } = useContext(UpdateDataContext)!;
    const { setErrorMessage } = useContext(NotificationContext)!;
    const { currentPluginSource } = useContext(PluginSourceContext)!;
    const { t } = useTranslation();
    const c = fluent();
    const current_version = useMemo(() => shell.breeze.version(), []);

    const [pending, setPending] = useState<{ version: string } | null>(null);
    const [injector, setInjector] = useState(getInjectorInfo());
    const [isUpdating, setIsUpdating] = useState(false);
    const [progress, setProgress] = useState<UpdateProgress | null>(null);

    useEffect(() => {
        setPending(readPendingUpdate());
        setInjector(getInjectorInfo());
    }, []);

    const autoUpdateEnabled = config?.auto_update !== false;
    const header = <PageHeader title={t("update.title")} gutter={SCROLL_GUTTER} />;
    const autoUpdateCard = (
        <SettingsCard icon={ICON_SYNC} title={t("update.autoUpdate")} description={t("update.autoUpdateDesc")}>
            <ToggleSwitch value={autoUpdateEnabled} onChange={value => updateConfig(current => ({ ...current, auto_update: value }))}
                onLabel={t("settings.on")} offLabel={t("settings.off")} />
        </SettingsCard>
    );

    if (!updateData) {
        return (
            <flex gap={16} alignItems="stretch" width={CONTENT_WIDTH}>
                {header}
                <flex gap={4} alignItems="stretch" paddingRight={SCROLL_GUTTER}>
                    <InfoBar severity="info" title={t("common.loading")} message={`${t("update.currentVersion")}: ${current_version}`} />
                    {autoUpdateCard}
                </flex>
            </flex>
        );
    }

    const remote_version = updateData.shell.version;
    const pendingRestart = pending != null;
    const canUpdate = compareVersions(remote_version, current_version) > 0 || !injector.installed;
    const progressPercent = progress?.percent != null ? Math.round(progress.percent * 100) : null;
    const progressLabel = progress?.phase === "verifying"
        ? t("update.verifying")
        : progress?.phase === "applying"
            ? t("update.applying")
            : progress
                ? progress.totalBytes != null
                    ? `${t("update.downloading")} ${formatBytes(progress.downloadedBytes)} / ${formatBytes(progress.totalBytes)} (${progressPercent}%)`
                    : `${t("update.downloading")} ${formatBytes(progress.downloadedBytes)}`
                : null;

    const updateShell = async () => {
        if (isUpdating) return;
        setIsUpdating(true);
        setProgress({ phase: "downloading", downloadedBytes: 0, totalBytes: null, percent: null });
        try {
            await runManualUpdate({ updateData, sourceName: currentPluginSource, onProgress: setProgress });
            shell.println(t('update.updateSuccess'));
            setPending(readPendingUpdate());
            setInjector(getInjectorInfo());
        } catch (e) {
            const message = String(e);
            shell.println(t('update.updateFailed') + message);
            setErrorMessage(t('update.updateFailed') + message);
        } finally {
            setProgress(null);
            setIsUpdating(false);
        }
    };

    const status = pendingRestart
        ? t("update.pendingRestart")
        : canUpdate ? t("update.available", { version: remote_version }) : t("update.upToDate");

    return (
        <flex gap={16} alignItems="stretch" width={CONTENT_WIDTH}>
            {header}
            <flex enableScrolling maxHeight={PAGE_BODY_HEIGHT} alignItems="stretch" paddingRight={SCROLL_GUTTER} paddingBottom={16} gap={4}>
                <flex gap={14} padding={18} borderRadius={7} borderWidth={1} borderColor={c.cardStroke} backgroundColor={c.card} alignItems="stretch">
                    <flex horizontal gap={16} alignItems="center">
                        <flex width={48} height={48} borderRadius={10} backgroundColor={c.accentSubtle} justifyContent="center" alignItems="center">
                            <img svg={ICON_BREEZE.replace("<svg ", `<svg fill="${c.accent.slice(0, 7)}" `)} width={28} height={28} />
                        </flex>
                        <flex gap={3} flexGrow={1}>
                            <Text fontSize={18} fontWeight={600} color={c.text}>{status}</Text>
                            <Text fontSize={12} color={c.textSecondary}>
                                {`${t("update.currentVersion")} ${current_version}  ·  ${t("update.latestVersion")} ${remote_version}`}
                            </Text>
                        </flex>
                        {pendingRestart && !isUpdating
                            ? <Button label={t("update.restartNow")} variant="accent" onClick={() => restartExplorer()} />
                            : <Button
                                icon={ICON_DOWNLOAD}
                                label={isUpdating ? t("update.updating") : canUpdate ? t("update.updateNow") : t("update.checked")}
                                variant={canUpdate ? "accent" : "standard"}
                                disabled={!canUpdate || isUpdating}
                                onClick={() => { void updateShell(); }}
                            />}
                    </flex>
                    {progressLabel && (
                        <flex gap={8} alignItems="stretch">
                            <Text fontSize={12} color={c.textSecondary}>{progressLabel}</Text>
                            <ProgressBar value={progress?.percent ?? null} width={BODY_WIDTH - 38} />
                        </flex>
                    )}
                </flex>
                {autoUpdateCard}
                <SettingsCard icon={ICON_HISTORY} title={t("update.injectorVersion")}
                    description={updateData.injector?.version ? `${t("update.latestVersion")}: ${updateData.injector.version}` : undefined}>
                    <Text fontSize={13} color={c.textSecondary}>{injector.installed ? (injector.version || 'unknown') : t("update.injectorNotInstalled")}</Text>
                </SettingsCard>
                <SettingsGroup title={t("update.changelog")}>
                    <flex gap={8} padding={18} borderRadius={7} borderWidth={1} borderColor={c.cardStroke} backgroundColor={c.card} alignItems="stretch">
                        <SimpleMarkdownRender text={updateData.shell.changelog} maxWidth={BODY_WIDTH - 40} />
                    </flex>
                </SettingsGroup>
            </flex>
        </flex>
    );
});

export default UpdatePage;
