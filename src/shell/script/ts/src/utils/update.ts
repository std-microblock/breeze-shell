import * as shell from "mshell";
import { PLUGIN_SOURCES } from "../plugin/constants";
import { get_async } from "./network";
import { getUpdateSource, isAutoUpdateEnabled, loadAppConfig } from "./appConfig";
import { compareVersions } from "./semver";

export type UpdateProgress = {
    phase: "downloading" | "verifying" | "applying";
    downloadedBytes: number;
    totalBytes: number | null;
    percent: number | null;
};

export type PendingUpdate = {
    backupName: string;
    version: string;
    crashCount: number;
};

const DATA_DIR = shell.breeze.data_directory();
const UPDATE_DIR = DATA_DIR + "/update";
const BIN_DIR = DATA_DIR + "/bin";
const SHELL_DLL = DATA_DIR + "/shell.dll";
const INJECTOR_EXE = BIN_DIR + "/breeze.exe";
const PENDING_FILE = DATA_DIR + "/update-pending";
const ROLLED_BACK_FILE = DATA_DIR + "/update-rolledback";
const STATE_FILE = UPDATE_DIR + "/state.json";

const envInt = (name: string, fallback: number) => {
    const value = parseInt(shell.win32.env(name) || "", 10);
    return Number.isFinite(value) && value >= 0 ? value : fallback;
};

const STARTUP_DELAY_MS = envInt("BREEZE_UPDATE_STARTUP_DELAY_MS", 15000);
const CHECK_INTERVAL_MS = envInt("BREEZE_UPDATE_CHECK_INTERVAL_MS", 6 * 60 * 60 * 1000);
const CONFIRM_DELAY_MS = envInt("BREEZE_UPDATE_CONFIRM_MS", 2 * 60 * 1000);

type UpdateState = {
    bad_versions?: string[];
    injector_bootstrapped?: boolean;
    confirmed?: string;
};

const zh = shell.breeze.user_language() === "zh-CN";
const t = (zhText: string, enText: string) => (zh ? zhText : enText);

const normalizeUrl = (url: string) => url.replaceAll("//", "/").replaceAll(":/", "://");

const lockName = (purpose: string) => {
    let hash = 0x811c9dc5;
    for (let i = 0; i < DATA_DIR.length; i++) {
        hash ^= DATA_DIR.charCodeAt(i);
        hash = Math.imul(hash, 0x01000193) >>> 0;
    }
    return `breeze-shell-${purpose}-${hash.toString(16)}`;
};

const tryLock = (purpose: string) => shell.breeze.try_named_mutex(lockName(purpose));

const cleanupFile = (path: string) => {
    if (!shell.fs.exists(path)) {
        return;
    }

    try {
        shell.fs.remove(path);
    } catch {
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

export const resolveSourceBase = (sourceName: string) =>
    shell.win32.env("BREEZE_UPDATE_SOURCE_URL") ?? PLUGIN_SOURCES[sourceName];

export const fetchUpdateIndex = async (sourceName: string) => {
    const data = await get_async(resolveSourceBase(sourceName) + "plugins-index.json");
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

const loadState = (): UpdateState => {
    try {
        if (shell.fs.exists(STATE_FILE)) {
            return JSON.parse(shell.fs.read(STATE_FILE));
        }
    } catch (e) {
        shell.println("[Update] Failed to load state:", e);
    }
    return {};
};

const saveState = (state: UpdateState) => {
    try {
        shell.fs.write(STATE_FILE, JSON.stringify(state, null, 4));
    } catch (e) {
        shell.println("[Update] Failed to save state:", e);
    }
};

export const readPendingUpdate = (): PendingUpdate | null => {
    try {
        if (!shell.fs.exists(PENDING_FILE)) {
            return null;
        }
        const lines = shell.fs.read(PENDING_FILE).split(/\r?\n/);
        const backupName = (lines[0] || "").trim();
        const version = (lines[1] || "").trim();
        if (!backupName || !version) {
            return null;
        }
        return { backupName, version, crashCount: parseInt(lines[2], 10) || 0 };
    } catch {
        return null;
    }
};

const writePendingUpdate = (backupName: string, version: string) => {
    shell.fs.write(PENDING_FILE, `${backupName}\n${version}\n0`);
};

const verifyDownload = (path: string, expected: { version?: string; sha256?: string; size?: number }) => {
    if (!shell.fs.exists(path) || shell.breeze.file_size(path) <= 0) {
        throw new Error(t("下载的文件为空", "Downloaded file is empty"));
    }
    if (!shell.breeze.file_is_x64_pe(path)) {
        throw new Error(t("下载的文件不是有效的 x64 程序", "Downloaded file is not a valid x64 binary"));
    }
    if (expected.size && shell.breeze.file_size(path) !== expected.size) {
        throw new Error(t(`文件大小不符 (期望 ${expected.size})`, `File size mismatch (expected ${expected.size})`));
    }
    if (expected.sha256) {
        const actual = shell.breeze.file_sha256(path).toLowerCase();
        if (actual !== expected.sha256.toLowerCase()) {
            throw new Error(t("sha256 校验失败", "sha256 mismatch"));
        }
    }
    const fileVersion = shell.breeze.file_version(path);
    if (expected.version && fileVersion && fileVersion !== expected.version) {
        throw new Error(t(`文件版本不符 (${fileVersion} != ${expected.version})`, `File version mismatch (${fileVersion} != ${expected.version})`));
    }
};

const backupNameFor = (prefix: string, version: string) =>
    `${prefix === "shell" ? "shell" : "breeze"}-${version || "unknown"}-${Date.now()}.old.${prefix === "shell" ? "dll" : "exe"}`;

export const installShellUpdate = async ({
    updateData,
    sourceName,
    onProgress
}: {
    updateData: any;
    sourceName: string;
    onProgress?: (progress: UpdateProgress) => void;
}) => {
    const remote = updateData?.shell;
    if (!remote?.path || !remote?.version) {
        throw new Error(t("更新源缺少 shell 信息", "Missing shell update info"));
    }

    const state = loadState();
    if ((state.bad_versions || []).includes(remote.version)) {
        throw new Error(t(`版本 ${remote.version} 曾导致崩溃，已跳过`, `Version ${remote.version} was rolled back before, skipped`));
    }

    const alreadyStaged = readPendingUpdate();
    if (alreadyStaged && alreadyStaged.version === remote.version) {
        return;
    }

    const token = tryLock("install");
    if (token < 0) {
        throw new Error(t("另一个进程正在安装更新", "Another process is installing an update"));
    }

    try {
        shell.fs.mkdir(UPDATE_DIR);
        const tempPath = `${UPDATE_DIR}/shell-${remote.version}.dll.download`;
        try {
            cleanupFile(tempPath);
            await downloadFileWithProgress(resolveSourceBase(sourceName) + remote.path, tempPath, onProgress);

            onProgress?.({ phase: "verifying", downloadedBytes: 0, totalBytes: null, percent: null });
            verifyDownload(tempPath, remote);

            onProgress?.({ phase: "applying", downloadedBytes: 0, totalBytes: null, percent: null });

            const backupName = backupNameFor("shell", shell.breeze.file_version(SHELL_DLL) || shell.breeze.version());
            const backupPath = `${UPDATE_DIR}/${backupName}`;
            let movedCurrent = false;

            writePendingUpdate(backupName, remote.version);
            try {
                if (shell.fs.exists(SHELL_DLL)) {
                    shell.fs.rename(SHELL_DLL, backupPath);
                    movedCurrent = true;
                }
                shell.fs.rename(tempPath, SHELL_DLL);
            } catch (e) {
                cleanupFile(PENDING_FILE);
                if (movedCurrent && !shell.fs.exists(SHELL_DLL) && shell.fs.exists(backupPath)) {
                    try {
                        shell.fs.rename(backupPath, SHELL_DLL);
                    } catch {
                    }
                }
                cleanupFile(tempPath);
                throw e;
            }

            shell.notification.send_with_buttons(
                t("Breeze Shell 已更新", "Breeze Shell updated"),
                t(`已更新到 ${remote.version}，重启资源管理器后生效`, `Updated to ${remote.version}, restart Explorer to apply`),
                [
                    [t("重启资源管理器", "Restart Explorer"), () => restartExplorer()],
                    [t("稍后", "Later"), () => { }]
                ]
            );
        } catch (e) {
            cleanupFile(`${UPDATE_DIR}/shell-${remote.version}.dll.download`);
            throw e;
        }
    } finally {
        shell.breeze.release_named_mutex(token);
    }
};

export const installInjectorUpdate = async ({
    updateData,
    sourceName,
    onProgress
}: {
    updateData: any;
    sourceName: string;
    onProgress?: (progress: UpdateProgress) => void;
}) => {
    const remote = updateData?.injector;
    if (!remote?.path || !remote?.version) {
        return;
    }

    const localVersion = shell.fs.exists(INJECTOR_EXE) ? shell.breeze.file_version(INJECTOR_EXE) : "";
    if (localVersion && compareVersions(remote.version, localVersion) <= 0) {
        return;
    }

    const token = tryLock("install");
    if (token < 0) {
        throw new Error(t("另一个进程正在安装更新", "Another process is installing an update"));
    }

    try {
        shell.fs.mkdir(UPDATE_DIR);
        shell.fs.mkdir(BIN_DIR);
        const tempPath = `${UPDATE_DIR}/breeze-${remote.version}.exe.download`;
        cleanupFile(tempPath);
        await downloadFileWithProgress(resolveSourceBase(sourceName) + remote.path, tempPath, onProgress);

        onProgress?.({ phase: "verifying", downloadedBytes: 0, totalBytes: null, percent: null });
        verifyDownload(tempPath, remote);

        onProgress?.({ phase: "applying", downloadedBytes: 0, totalBytes: null, percent: null });

        if (shell.fs.exists(INJECTOR_EXE)) {
            try {
                shell.fs.rename(INJECTOR_EXE, `${UPDATE_DIR}/${backupNameFor("breeze", localVersion)}`);
            } catch (e) {
                cleanupFile(tempPath);
                throw e;
            }
        }
        shell.fs.rename(tempPath, INJECTOR_EXE);
    } finally {
        shell.breeze.release_named_mutex(token);
    }
};

export const bootstrapInjector = async (index: any, sourceName: string) => {
    const state = loadState();
    if (state.injector_bootstrapped && shell.fs.exists(INJECTOR_EXE)) {
        return;
    }
    try {
        await installInjectorUpdate({ updateData: index, sourceName });
        if (shell.fs.exists(INJECTOR_EXE)) {
            shell.subproc.open(INJECTOR_EXE, "migrate");
            state.injector_bootstrapped = true;
            saveState(state);
        }
    } catch (e) {
        shell.println("[Update] Injector bootstrap failed:", e);
    }
};

export const restartExplorer = () => {
    if (shell.fs.exists(INJECTOR_EXE)) {
        shell.subproc.open(INJECTOR_EXE, "restart-explorer");
    } else {
        shell.println("[Update] injector not installed, restart Explorer manually");
    }
};

const confirmPendingUpdate = () => {
    const pending = readPendingUpdate();
    if (!pending || pending.version !== shell.breeze.version()) {
        return;
    }
    cleanupFile(PENDING_FILE);
    cleanupFile(`${UPDATE_DIR}/${pending.backupName}`);

    const state = loadState();
    state.confirmed = pending.version;
    saveState(state);
    shell.println(`[Update] Confirmed shell ${pending.version}`);
};

const absorbRolledBackMarker = (state: UpdateState) => {
    if (!shell.fs.exists(ROLLED_BACK_FILE)) {
        return;
    }
    try {
        const version = shell.fs.read(ROLLED_BACK_FILE).trim();
        if (version) {
            state.bad_versions = [...new Set([...(state.bad_versions || []), version])];
            saveState(state);
            shell.println(`[Update] Shell ${version} was rolled back after crashes, marked bad`);
        }
    } catch (e) {
        shell.println("[Update] Failed to read rollback marker:", e);
    }
    cleanupFile(ROLLED_BACK_FILE);
};

const cleanupLegacyFiles = () => {
    cleanupFile(`${DATA_DIR}/shell_old.dll`);
    cleanupFile(`${DATA_DIR}/shell_new.dll`);
    try {
        for (const path of shell.fs.readdir(DATA_DIR)) {
            const name = path.split("/").pop();
            if (name.startsWith("shell.dll.bak-") || name.startsWith("shell.dll.pre-upgrade-")) {
                cleanupFile(path);
            }
        }
    } catch {
    }
};

const cleanupUpdateFiles = () => {
    const pending = readPendingUpdate();
    try {
        for (const path of shell.fs.readdir(UPDATE_DIR)) {
            const name = path.split("/").pop();
            if (pending && name === pending.backupName) {
                continue;
            }
            if (name.endsWith(".old.dll") || name.endsWith(".old.exe")) {
                cleanupFile(path);
            }
        }
    } catch {
    }
};

const startupMaintenance = () => {
    const state = loadState();
    absorbRolledBackMarker(state);
    cleanupLegacyFiles();
    cleanupUpdateFiles();

    const pending = readPendingUpdate();
    if (pending && pending.version === shell.breeze.version()) {
        setTimeout(() => confirmPendingUpdate(), CONFIRM_DELAY_MS);
    }
    return state;
};

let leaderToken = -1;
const acquireLeadership = () => {
    if (leaderToken >= 0) {
        return true;
    }
    leaderToken = tryLock("update");
    return leaderToken >= 0;
};

const autoUpdateTick = async () => {
    const state = startupMaintenance();

    const config = loadAppConfig();
    if (!isAutoUpdateEnabled(config)) {
        return;
    }
    if (!acquireLeadership()) {
        return;
    }

    const sourceName = getUpdateSource(config);
    const index = await fetchUpdateIndex(sourceName);

    const shellRemote = index?.shell;
    if (shellRemote?.version && compareVersions(shellRemote.version, shell.breeze.version()) > 0 &&
        !(state.bad_versions || []).includes(shellRemote.version)) {
        shell.println(`[Update] Auto update ${shell.breeze.version()} -> ${shellRemote.version}`);
        await installShellUpdate({ updateData: index, sourceName });
        shell.println("[Update] New version downloaded and will be applied after Explorer restarts");
    }

    await bootstrapInjector(index, sourceName);
};

export const runManualUpdate = async ({
    updateData,
    sourceName,
    onProgress
}: {
    updateData: any;
    sourceName: string;
    onProgress?: (progress: UpdateProgress) => void;
}) => {
    const remote = updateData?.shell;
    if (remote?.version && compareVersions(remote.version, shell.breeze.version()) > 0) {
        await installShellUpdate({ updateData, sourceName, onProgress });
    }
    await installInjectorUpdate({ updateData, sourceName, onProgress });
    if (shell.fs.exists(INJECTOR_EXE)) {
        shell.subproc.open(INJECTOR_EXE, "migrate");
    }
};

export const getInjectorInfo = () => {
    if (!shell.fs.exists(INJECTOR_EXE)) {
        return { installed: false, version: "" };
    }
    return { installed: true, version: shell.breeze.file_version(INJECTOR_EXE) };
};

export const runAutoUpdateIfEnabled = () => {
    setTimeout(() => {
        autoUpdateTick().catch(e => shell.println("[Update] Auto update failed:", e));
    }, STARTUP_DELAY_MS);
    setInterval(() => {
        autoUpdateTick().catch(e => shell.println("[Update] Auto update failed:", e));
    }, CHECK_INTERVAL_MS);
};
