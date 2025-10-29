import * as shell from "mshell";
import { Button, Text, SimpleMarkdownRender } from "./components";
import { UpdateDataContext, NotificationContext, PluginSourceContext } from "./contexts";
import { useTranslation } from "./utils";
import { PLUGIN_SOURCES } from "./constants";
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
        return <Text>{t("加载中...")}</Text>;
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
                shell.println(t('新版本已下载，将于下次重启资源管理器生效'));
                setIsUpdating(false);
                set_exist_old_file(true);
            }, e => {
                shell.println(t('更新失败: ') + e);
                setIsUpdating(false);
                setErrorMessage(t('更新失败: ') + e);
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
                        shell.println(t('更新失败: ') + '无法移动当前文件');
                        setIsUpdating(false);
                        setErrorMessage(t('更新失败: ') + '无法移动当前文件');
                    }
                } else {
                    shell.fs.rename(shellPath, shellOldPath);
                    downloadNewShell();
                }
            } else {
                downloadNewShell();
            }
        } catch (e) {
            shell.println(t('更新失败: ') + e);
            setIsUpdating(false);
            setErrorMessage(t('更新失败: ') + e);
        }
    };

    return (
        <flex gap={20}>
            <Text fontSize={24}>{t("插件市场")}</Text>
            <flex gap={10}>
                <Text>{`当前版本: ${current_version}`}</Text>
                <Text>{`最新版本: ${remote_version}`}</Text>
                <Button onClick={current_version === remote_version || isUpdating ? () => { } : updateShell}>
                    <Text>{
                        isUpdating ? t("更新中...") :
                            exist_old_file ? t("新版本已下载，将于下次重启资源管理器生效") : (current_version === remote_version ? (current_version + ' (latest)') : `${current_version} -> ${remote_version}`)}</Text>
                </Button>
            </flex>
            <flex gap={10}>
                <Text>更新日志:</Text>
                <flex enableScrolling maxHeight={500} width={550} gap={10}>
                    <SimpleMarkdownRender text={updateData.shell.changelog} maxWidth={550} />
                </flex>
            </flex>
        </flex>
    );
});

export default UpdatePage;