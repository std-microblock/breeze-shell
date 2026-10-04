import * as shell from "mshell";
import { memo, useEffect, useMemo, useRef, useState } from "react";
import { Text } from "../components";
import { Card, ChipToggle, Entrance, IconButton, Pill, SectionHeader, fluent, severityStyle } from "../components/Fluent";
import { useTranslation } from "../hooks";
import { CONTENT_WIDTH, ICON_CLEAR, ICON_COPY, ICON_OPEN, ICON_PAUSE, ICON_PLAY, SCROLL_GUTTER } from "../constants";

const CARD_INNER = CONTENT_WIDTH - SCROLL_GUTTER - 3 - 16 - 2;
const CODE_WIDTH = CARD_INNER - 16;
const VALUE_WIDTH = CODE_WIDTH - 90;
const HEADLINE_WIDTH = CARD_INNER - 80 - 50 - 40 - 24;

type Level = "trace" | "debug" | "info" | "warn" | "error" | "critical";
const LEVELS: Level[] = ["debug", "info", "warn", "error", "critical"];
const MAX_KEEP = 1500;
const MAX_VISIBLE = 200;

const formatTime = (ms: number) => {
    const d = new Date(ms);
    const p = (n: number, w = 2) => String(n).padStart(w, "0");
    return `${p(d.getHours())}:${p(d.getMinutes())}:${p(d.getSeconds())}.${p(d.getMilliseconds(), 3)}`;
};

const tryParse = (s: string): any => {
    if (!s) return undefined;
    try {
        return JSON.parse(s);
    } catch {
        return undefined;
    }
};

const extractInlineJson = (message: string): { head: string; data: any } | null => {
    const start = message.search(/[\[{]/);
    if (start < 0) return null;
    const data = tryParse(message.slice(start).trim());
    if (data === undefined || typeof data !== "object" || data === null) return null;
    return { head: message.slice(0, start).trim(), data };
};

const valueColor = (v: any, c: ReturnType<typeof fluent>) => {
    if (typeof v === "string") return c.light ? "#0F7B0FFF" : "#6CCB5FFF";
    if (typeof v === "number") return c.light ? "#8764B8FF" : "#C6A5F7FF";
    if (typeof v === "boolean" || v === null) return c.accent + "FF";
    return c.text;
};

const JsonTree = ({ data, depth = 0 }: { data: any; depth?: number }) => {
    const c = fluent();
    const entries = Array.isArray(data) ? data.map((v, i) => [String(i), v] as const) : Object.entries(data);
    return (
        <flex gap={2} paddingLeft={depth ? 14 : 0} alignItems="stretch">
            {entries.map(([k, v]) => {
                const nested = v !== null && typeof v === "object";
                return (
                    <flex key={k} gap={2} alignItems="stretch">
                        <flex horizontal gap={6}>
                            <Text fontSize={12} fontFamily="monospace" color={c.textSecondary}>{`${k}:`}</Text>
                            {!nested && (
                                <Text fontSize={12} fontFamily="monospace" maxWidth={VALUE_WIDTH - depth * 14} color={valueColor(v, c)}>
                                    {typeof v === "string" ? `"${v}"` : String(v)}
                                </Text>
                            )}
                            {nested && (
                                <Text fontSize={12} fontFamily="monospace" color={c.textTertiary}>
                                    {Array.isArray(v) ? `[${v.length}]` : `{${Object.keys(v).length}}`}
                                </Text>
                            )}
                        </flex>
                        {nested && depth < 4 && <JsonTree data={v} depth={depth + 1} />}
                    </flex>
                );
            })}
        </flex>
    );
};

const LogRow = memo(({ entry, expanded, onToggle }: {
    entry: shell.log_entry;
    expanded: boolean;
    onToggle: (id: number) => void;
}) => {
    const c = fluent();
    const s = severityStyle(entry.level);
    const fields = tryParse(entry.fields);
    const inline = !fields ? extractInlineJson(entry.message) : null;
    const structured = fields ?? inline?.data;
    const headline = inline ? inline.head || entry.message : entry.message;
    const firstLine = headline.split("\n")[0];
    const multiline = headline.includes("\n") || headline.length > 80;

    return (
        <Entrance offset={6}>
            <Card padding={8} gap={6} onClick={() => onToggle(entry.id)} accentBar={s.fg}>
                <flex horizontal gap={8} alignItems="center">
                    <Text fontSize={11} fontFamily="monospace" color={c.textTertiary}>{formatTime(entry.time)}</Text>
                    <Pill text={s.label} fg={s.fg} bg={s.bg} fontSize={10} />
                    <Text fontSize={12} color={c.text} maxWidth={HEADLINE_WIDTH}>
                        {expanded ? firstLine : firstLine.slice(0, 80) + (multiline || firstLine.length > 80 ? " …" : "")}
                    </Text>
                    {structured && <Pill text="{ }" fg={c.accent} bg={c.subtle} fontSize={10} />}
                </flex>
                {expanded && (
                    <flex gap={8} alignItems="stretch">
                        {multiline && (
                            <flex padding={8} borderRadius={4} backgroundColor={c.codeBg}>
                                <Text fontSize={12} fontFamily="monospace" maxWidth={CODE_WIDTH} color={c.text}>{headline}</Text>
                            </flex>
                        )}
                        {structured && (
                            <flex padding={8} borderRadius={4} backgroundColor={c.codeBg} alignItems="stretch">
                                <JsonTree data={structured} />
                            </flex>
                        )}
                        <flex horizontal gap={12}>
                            <Text fontSize={11} color={c.textTertiary}>{`#${entry.id}`}</Text>
                            <Text fontSize={11} color={c.textTertiary}>{`thread ${entry.thread}`}</Text>
                            {entry.source && <Text fontSize={11} color={c.textTertiary} maxWidth={CODE_WIDTH - 140}>{entry.source}</Text>}
                        </flex>
                    </flex>
                )}
            </Card>
        </Entrance>
    );
});

const LogViewer = memo(() => {
    const { t } = useTranslation();
    const c = fluent();
    const [entries, setEntries] = useState<shell.log_entry[]>([]);
    const [enabled, setEnabled] = useState<Record<string, boolean>>({ debug: false, info: true, warn: true, error: true, critical: true });
    const [query, setQuery] = useState("");
    const [paused, setPaused] = useState(false);
    const [expanded, setExpanded] = useState<Record<number, boolean>>({});
    const lastId = useRef(0);
    const pausedRef = useRef(false);
    pausedRef.current = paused;

    const poll = () => {
        if (pausedRef.current) return;
        const fresh = shell.diagnostics.logs(lastId.current, MAX_KEEP);
        if (fresh.length === 0) return;
        lastId.current = fresh[fresh.length - 1].id;
        setEntries(prev => {
            const merged = prev.concat(fresh);
            return merged.length > MAX_KEEP ? merged.slice(merged.length - MAX_KEEP) : merged;
        });
    };

    useEffect(() => {
        poll();
        const id = setInterval(poll, 400);
        return () => clearInterval(id);
    }, []);

    const counts = useMemo(() => {
        const r: Record<string, number> = {};
        for (const e of entries) r[e.level] = (r[e.level] || 0) + 1;
        return r;
    }, [entries]);

    const visible = useMemo(() => {
        const q = query.trim().toLowerCase();
        const filtered = entries.filter(e =>
            (e.level === "trace" ? enabled.debug : (enabled[e.level] ?? true)) &&
            (!q || e.message.toLowerCase().includes(q) || e.fields.toLowerCase().includes(q) || e.source.toLowerCase().includes(q)));
        return filtered.slice(Math.max(0, filtered.length - MAX_VISIBLE)).reverse();
    }, [entries, enabled, query]);

    const toggle = (id: number) => setExpanded(prev => ({ ...prev, [id]: !prev[id] }));

    const copyVisible = () => {
        const text = visible.slice().reverse()
            .map(e => `[${formatTime(e.time)}] [${e.level}] ${e.message}${e.fields ? " " + e.fields : ""}`)
            .join("\n");
        shell.clipboard.write_text(text);
    };

    return (
        <flex gap={14} alignItems="stretch" width={CONTENT_WIDTH} autoSize={false}>
            <SectionHeader title={t("logs.title")} subtitle={t("logs.subtitle", { n: entries.length })} gutter={SCROLL_GUTTER}>
                <IconButton icon={paused ? ICON_PLAY : ICON_PAUSE} label={paused ? t("logs.resume") : t("logs.pause")} onClick={() => setPaused(!paused)} accent={paused} />
                <IconButton icon={ICON_COPY} onClick={copyVisible} />
                <IconButton icon={ICON_OPEN} onClick={() => shell.subproc.open(shell.diagnostics.log_file_path(), "")} />
                <IconButton icon={ICON_CLEAR} onClick={() => { shell.diagnostics.clear_logs(); setEntries([]); setExpanded({}); }} />
            </SectionHeader>

            <flex horizontal gap={6} alignItems="center">
                {LEVELS.map(l => (
                    <ChipToggle
                        key={l}
                        label={`${t(`logs.level.${l}`)} ${counts[l] || 0}`}
                        selected={!!enabled[l]}
                        color={severityStyle(l)}
                        onClick={() => setEnabled(prev => ({ ...prev, [l]: !prev[l] }))}
                    />
                ))}
            </flex>

            <flex horizontal>
                <textbox
                    value={query}
                    placeholder={t("logs.search")}
                    width={CONTENT_WIDTH - SCROLL_GUTTER}
                    height={32}
                    fontSize={13}
                    borderRadius={5}
                    backgroundColor={c.light ? "#FFFFFFB3" : "#FFFFFF0F"}
                    borderColor={c.light ? "#00000024" : "#FFFFFF1F"}
                    focusBorderColor={c.accent}
                    textColor={c.text}
                    placeholderColor={c.textTertiary}
                    caretColor={c.text}
                    onChange={setQuery}
                />
            </flex>

            <flex enableScrolling maxHeight={410} gap={4} alignItems="stretch" paddingRight={SCROLL_GUTTER}>
                {visible.length === 0 && (
                    <Entrance>
                        <flex padding={30} alignItems="center" justifyContent="center">
                            <Text fontSize={13} color={c.textTertiary}>{t("logs.empty")}</Text>
                        </flex>
                    </Entrance>
                )}
                {visible.map(e => (
                    <LogRow key={e.id} entry={e} expanded={!!expanded[e.id]} onToggle={toggle} />
                ))}
            </flex>
        </flex>
    );
});

export default LogViewer;
