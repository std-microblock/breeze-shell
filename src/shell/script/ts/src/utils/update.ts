import * as shell from "mshell";
import { PLUGIN_SOURCES } from "../plugin/constants";
import { get_async } from "./network";
import { getUpdateSource, isAutoUpdateEnabled, loadAppConfig } from "./appConfig";

export type UpdateProgress = {
    phase: "downloading" | "applying";
    downloadedBytes: number;
    totalBytes: number | null;
    percent: number | null;
};

const normalizeUrl = (url: string) => url.replaceAll("//", "/").replaceAll(":/", "://");

const cleanupFile = (path: string) => {
    if (!shell.fs.exists(path)) {
        return;
    }

    try {
        shell.fs.remove(path);
    } catch {
        // Best effort cleanup for stale temp files.
    }
};

export const formatBytes = (bytes: number) => {
    if (!Number.isFinite(bytes) || bytes <= 0) {
        return "0 B";
    }

    const units = ["B", "KB", "MB", "GB"];
    let value = bytes;
    let unitIndex = 0;

    while (value >= 1024 && unitIndex < units.length - 1) {
        value /= 1024;
        unitIndex += 1;
    }

    const fixed = unitIndex === 0 ? 0 : 1;
    return `${value.toFixed(fixed)} ${units[unitIndex]}`;
};

export const fetchUpdateIndex = async (sourceName: string) => {
    const data = await get_async(PLUGIN_SOURCES[sourceName] + "plugins-index.json");
    return JSON.parse(data as string);
};

const downloadFileWithProgress = (url: string, path: string, onProgress?: (progress: UpdateProgress) => void) =>
    new Promise<void>((resolve, reject) => {
        shell.network.download_with_progress_async(
            encodeURI(normalizeUrl(url)),
            path,
            () => resolve(),
            (error: string) => reject(new Error(error)),
            (downloadedBytes: number, totalBytes: number) => {
                onProgress?.({
                    phase: "downloading",
                    downloadedBytes,
                    totalBytes: totalBytes > 0 ? totalBytes : null,
                    percent: totalBytes > 0 ? Math.min(downloadedBytes / totalBytes, 1) : null
                });
            }
        );
    });

const promoteDownloadedShell = (tempPath: string, shellPath: string, shellOldPath: string) => {
    let movedCurrentShell = false;

    try {
        if (shell.fs.exists(shellOldPath)) {
            shell.fs.remove(shellOldPath);
        }

        if (shell.fs.exists(shellPath)) {
            shell.fs.rename(shellPath, shellOldPath);
            movedCurrentShell = true;
        }

        shell.fs.rename(tempPath, shellPath);
    } catch (e) {
        cleanupFile(tempPath);

        if (movedCurrentShell && !shell.fs.exists(shellPath) && shell.fs.exists(shellOldPath)) {
            try {
                shell.fs.rename(shellOldPath, shellPath);
            } catch {
                // Rollback is best effort. Surface the original error below.
            }
        }

        throw e;
    }
};

export const installShellUpdate = async ({
    updateData,
    sourceName,
    onProgress
}: {
    updateData: any;
    sourceName: string;
    onProgress?: (progress: UpdateProgress) => void;
}) => {
    if (!updateData?.shell?.path) {
        throw new Error("Missing shell update path");
    }

    const shellPath = shell.breeze.data_directory() + "/shell.dll";
    const shellOldPath = shell.breeze.data_directory() + "/shell_old.dll";
    const tempPath = shell.breeze.data_directory() + "/shell.dll.download";
    const url = PLUGIN_SOURCES[sourceName] + updateData.shell.path;

    cleanupFile(tempPath);
    await downloadFileWithProgress(url, tempPath, onProgress);

    onProgress?.({
        phase: "applying",
        downloadedBytes: 0,
        totalBytes: null,
        percent: null
    });

    promoteDownloadedShell(tempPath, shellPath, shellOldPath);
};

export const runAutoUpdateIfEnabled = async () => {
    const config = loadAppConfig();
    if (!isAutoUpdateEnabled(config)) {
        return;
    }

    const sourceName = getUpdateSource(config);

    try {
        const updateData = await fetchUpdateIndex(sourceName);
        const remoteVersion = updateData?.shell?.version;
        const currentVersion = shell.breeze.version();

        if (!remoteVersion || remoteVersion === currentVersion) {
            return;
        }

        shell.println(`[Update] Auto update ${currentVersion} -> ${remoteVersion}`);
        await installShellUpdate({ updateData, sourceName });
        shell.println("[Update] New version downloaded and will be applied after Explorer restarts");
    } catch (e) {
        shell.println("[Update] Auto update failed:", e);
    }
};
