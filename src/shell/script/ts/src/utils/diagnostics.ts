import * as shell from "mshell";
import i18next from "../i18n";

const notificationsEnabled = () => {
    try {
        return !!JSON.parse(shell.diagnostics.effective_config()).debug_console;
    } catch {
        return false;
    }
};

const stringify = (v: any) => {
    if (typeof v === "string") return v;
    if (v instanceof Error) return `${v.message}${v.stack ? "\n" + v.stack : ""}`;
    try {
        return JSON.stringify(v);
    } catch {
        return String(v);
    }
};

const callerSource = () => {
    const stack = new Error().stack?.split("\n") ?? [];
    const frame = stack.find((l, i) => i > 2 && !l.includes("diagnostics"));
    return frame ? frame.trim().replace(/^at\s+/, "") : "";
};

const write = (level: string, args: any[]) => {
    const plain = args.filter(a => typeof a !== "object" || a === null || a instanceof Error);
    const objects = args.filter(a => typeof a === "object" && a !== null && !(a instanceof Error));
    let fields = "";
    if (objects.length === 1 && !Array.isArray(objects[0])) fields = stringify(objects[0]);
    else if (objects.length > 0) fields = stringify(objects);
    const message = plain.map(stringify).join(" ") || (fields ? "(object)" : "");
    shell.diagnostics.log(level, message, fields, callerSource());
};

export const installConsole = () => {
    globalThis.console = {
        ...globalThis.console,
        log: (...a: any[]) => write("info", a),
        info: (...a: any[]) => write("info", a),
        debug: (...a: any[]) => write("debug", a),
        warn: (...a: any[]) => write("warn", a),
        error: (...a: any[]) => write("error", a),
    } as Console;
};

export const startProblemNotifier = (openProblems: () => void) => {
    const tick = () => {
        const fresh = shell.diagnostics.take_new_problems()
            .filter(p => p.severity === "error" || p.severity === "critical");
        if (fresh.length === 0 || !notificationsEnabled()) return;
        const t = i18next.t.bind(i18next);
        const first = fresh[0];
        const more = fresh.length > 1 ? ` (${t("problems.toastMore", { n: fresh.length - 1 })})` : "";
        shell.notification.send_with_buttons(
            t("problems.toastTitle"),
            `${first.title}${more}`,
            [[t("problems.toastView"), openProblems]]
        );
    };
    setTimeout(tick, 1500);
    setInterval(tick, 3000);
};
