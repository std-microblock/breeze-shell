import * as shell from "mshell";
import { Button, Text, SimpleMarkdownRender, Toggle } from "../components";
import { AppConfigContext, UpdateDataContext, NotificationContext, PluginSourceContext } from "../contexts";
import { useTranslation } from "../hooks";
import { memo, useContext, useEffect, useState, useMemo } from "react";
import { formatBytes, installShellUpdate, UpdateProgress } from "../../utils/update";

const UpdatePage = memo(() => {
    const { config, updateConfig } = useContext(AppConfigContext)!;
    const { updateData } = useContext(UpdateDataContext)!;
    const { setErrorMessage } = useContext(NotificationContext)!;
    const { currentPluginSource } = useContext(PluginSourceContext)!;
    const { t } = useTranslation();
    const current_version = useMemo(() => shell.breeze.version(), []);

    const [exist_old_file, set_exist_old_file] = useState(false);
    const [isUpdating, setIsUpdating] = useState(false);
    const [progress, setProgress] = useState<UpdateProgress | null>(null);

    useEffect(() => {
        set_exist_old_file(shell.fs.exists(shell.breeze.data_directory() + '/shell_old.dll'));
    }, []);

    if (!updateData) {
        return <Text>{t("common.loading")}</Text>;
    }

    const remote_version = updateData.shell.version;
    const autoUpdateEnabled = config?.auto_update !== false;
    const progressPercent = progress?.percent != null ? Math.round(progress.percent * 100) : null;
    const progressLabel = progress?.phase === "applying"
        ? t("update.applying")
        : progress
            ? progress.totalBytes != null
                ? `${t("update.downloading")} ${formatBytes(progress.downloadedBytes)} / ${formatBytes(progress.totalBytes)} (${progressPercent}%)`
                : `${t("update.downloading")} ${formatBytes(progress.downloadedBytes)}`
            : null;

    const updateShell = async () => {
        if (isUpdating) return;

        setIsUpdating(true);
        setProgress({
            phase: "downloading",
            downloadedBytes: 0,
            totalBytes: null,
            percent: null
        });

        try {
            await installShellUpdate({
                updateData,
                sourceName: currentPluginSource,
                onProgress: setProgress
            });
            shell.println(t('update.updateSuccess'));
            set_exist_old_file(true);
        } catch (e) {
            const message = String(e);
            shell.println(t('update.updateFailed') + message);
            setErrorMessage(t('update.updateFailed') + message);
        } finally {
            setProgress(null);
            setIsUpdating(false);
        }
    };

    return (
        <flex gap={20}>
            <Text fontSize={24}>{t("update.title")}</Text>
            <Toggle label={t("update.autoUpdate")} value={autoUpdateEnabled} onChange={(value) => {
                updateConfig(current => ({ ...current, auto_update: value }));
            }} />
            <flex gap={10}>
                <Text>{`${t("update.currentVersion")}: ${current_version}`}</Text>
                <Text>{`${t("update.latestVersion")}: ${remote_version}`}</Text>
                <Button onClick={current_version === remote_version || isUpdating ? () => { } : () => { void updateShell(); }}>
                    <Text>{
                        isUpdating ? (progressPercent != null ? `${t("update.updating")} ${progressPercent}%` : t("update.updating")) :
                            exist_old_file ? t("update.updateSuccess") : (current_version === remote_version ? (current_version + ' (latest)') : `${current_version} -> ${remote_version}`)}</Text>
                </Button>
                {progressLabel && (
                    <flex gap={6}>
                        <Text>{progressLabel}</Text>
                        <flex
                            width={320}
                            height={10}
                            autoSize={false}
                            borderRadius={5}
                            backgroundColor={shell.breeze.is_light_theme() ? "#d8d8d8" : "#3d3d3d"}
                        >
                            <flex
                                width={progressPercent != null ? Math.max(10, Math.round(320 * progress.percent!)) : 24}
                                height={10}
                                autoSize={false}
                                borderRadius={5}
                                backgroundColor="#2979FF"
                            />
                        </flex>
                    </flex>
                )}
            </flex>
            <flex gap={10}>
                <Text>{t("update.changelog")}</Text>
                <flex enableScrolling maxHeight={500} width={550} gap={10}>
                    <SimpleMarkdownRender text={updateData.shell.changelog} maxWidth={550} />
                </flex>
            </flex>
        </flex>
    );
});

export default UpdatePage;
