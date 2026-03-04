import * as shell from "mshell";
import { Button, Text, SimpleMarkdownRender } from "../components";
import { UpdateDataContext, NotificationContext, PluginSourceContext } from "../contexts";
import { useTranslation } from "../hooks";
import { PLUGIN_SOURCES } from "../constants";
import { memo, useContext, useEffect, useState, useMemo } from "react";

const UpdatePage = memo(() => {
    const { updateData } = useContext(UpdateDataContext)!;
    const { setErrorMessage } = useContext(NotificationContext)!;
    const { currentPluginSource } = useContext(PluginSourceContext)!;
    const { t } = useTranslation();
    const current_version = useMemo(() => shell.breeze.version(), []);

    const [exist_old_file, set_exist_old_file] = useState(false);
    const [isUpdating, setIsUpdating] = useState(false);

    useEffect(() => {
        set_exist_old_file(shell.fs.exists(shell.breeze.data_directory() + '/shell_old.dll'));
    }, []);

    if (!updateData) {
        return <Text>{t("common.loading")}</Text>;
    }

    const remote_version = updateData.shell.version;

    const updateShell = () => {
        if (isUpdating) return;

        setIsUpdating(true);
        const shellPath = shell.breeze.data_directory() + '/shell.dll';
        const shellOldPath = shell.breeze.data_directory() + '/shell_old.dll';
        const url = PLUGIN_SOURCES[currentPluginSource] + updateData.shell.path;

        const downloadNewShell = () => {
            shell.network.download_async(url, shellPath, () => {
                shell.println(t('update.updateSuccess'));
                setIsUpdating(false);
                set_exist_old_file(true);
            }, e => {
                shell.println(t('update.updateFailed') + e);
                setIsUpdating(false);
                setErrorMessage(t('update.updateFailed') + e);
            });
        };

        try {
            if (shell.fs.exists(shellPath)) {
                if (shell.fs.exists(shellOldPath)) {
                    try {
                        shell.fs.remove(shellOldPath);
                        shell.fs.rename(shellPath, shellOldPath);
                        downloadNewShell();
                    } catch (e) {
                        shell.println(t('update.updateFailed') + t('update.cannotMoveFile'));
                        setIsUpdating(false);
                        setErrorMessage(t('update.updateFailed') + t('update.cannotMoveFile'));
                    }
                } else {
                    shell.fs.rename(shellPath, shellOldPath);
                    downloadNewShell();
                }
            } else {
                downloadNewShell();
            }
        } catch (e) {
            shell.println(t('update.updateFailed') + e);
            setIsUpdating(false);
            setErrorMessage(t('update.updateFailed') + e);
        }
    };

    return (
        <flex gap={20}>
            <Text fontSize={24}>{t("plugin.store")}</Text>
            <flex gap={10}>
                <Text>{`${t("update.currentVersion")}: ${current_version}`}</Text>
                <Text>{`${t("update.latestVersion")}: ${remote_version}`}</Text>
                <Button onClick={current_version === remote_version || isUpdating ? () => { } : updateShell}>
                    <Text>{
                        isUpdating ? t("update.updating") :
                            exist_old_file ? t("update.updateSuccess") : (current_version === remote_version ? (current_version + ' (latest)') : `${current_version} -> ${remote_version}`)}</Text>
                </Button>
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
