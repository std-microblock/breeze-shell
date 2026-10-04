import * as shell from "mshell";
import { memo, useEffect, useMemo, useRef, useState } from "react";
import { Text } from "../components";
import { Card, Entrance, IconButton, InfoBar, Pill, SectionHeader, SegmentedControl, fluent, severityStyle } from "../components/Fluent";
import { useTranslation } from "../hooks";
import { CONTENT_WIDTH, ICON_COPY, ICON_OPEN, ICON_REFRESH, SCROLL_GUTTER } from "../constants";

const CARD_INNER = CONTENT_WIDTH - SCROLL_GUTTER - 3 - 28 - 2;
const CODE_WIDTH = CARD_INNER - 16;
const TITLE_WIDTH = CARD_INNER - 60 - 50 - 40 - 60 - 32;

type Tab = "all" | "config" | "script" | "runtime";

const relativeTime = (ms: number, t: (k: string, o?: any) => string) => {
    const diff = Math.max(0, Date.now() - ms) / 1000;
    if (diff < 60) return t("problems.justNow");
    if (diff < 3600) return t("problems.minutesAgo", { n: Math.floor(diff / 60) });
    return t("problems.hoursAgo", { n: Math.floor(diff / 3600) });
};

const severityRank = (s: string) => ({ critical: 0, error: 1, warning: 2, info: 3 } as Record<string, number>)[s] ?? 4;

const ProblemCard = memo(({ p, index, expanded, onToggle }: {
    p: shell.diagnostic_problem;
    index: number;
    expanded: boolean;
    onToggle: () => void;
}) => {
    const { t } = useTranslation();
    const c = fluent();
    const s = severityStyle(p.severity);
    const hasDetail = p.detail && p.detail !== p.title;
    return (
        <Entrance delay={Math.min(index, 8) * 35}>
            <Card onClick={onToggle} accentBar={s.fg} gap={6}>
                <flex horizontal gap={8} alignItems="center">
                    <Pill text={s.label} fg={s.fg} bg={s.bg} fontSize={10} />
                    <Pill text={t(`problems.category.${p.category}`)} fg={c.textSecondary} bg={c.subtle} fontSize={10} />
                    <Text fontSize={13} fontWeight={600} color={c.text} maxWidth={TITLE_WIDTH}>{p.title}</Text>
                    {p.count > 1 && <Pill text={`×${p.count}`} fg={c.accent} bg={c.subtle} fontSize={10} />}
                    <spacer />
                    <Text fontSize={11} color={c.textTertiary}>{relativeTime(p.time, t)}</Text>
                </flex>
                {hasDetail && (
                    <flex padding={8} borderRadius={4} backgroundColor={c.codeBg}>
                        <Text fontSize={12} fontFamily="monospace" maxWidth={CODE_WIDTH} color={c.textSecondary}>
                            {expanded ? p.detail : p.detail.split("\n").slice(0, 3).join("\n").slice(0, 280)}
                        </Text>
                    </flex>
                )}
                {expanded && p.source && (
                    <flex horizontal gap={8} alignItems="center">
                        <Text fontSize={11} color={c.textTertiary} maxWidth={CARD_INNER - 200}>{p.source}</Text>
                        <spacer />
                        <IconButton icon={ICON_COPY} label={t("problems.copy")} onClick={() => shell.clipboard.write_text(`${p.title}\n${p.detail}\n${p.source}`)} />
                        {shell.fs.exists(p.source) && (
                            <IconButton icon={ICON_OPEN} label={t("problems.open")} onClick={() => shell.subproc.open(p.source, "")} />
                        )}
                    </flex>
                )}
            </Card>
        </Entrance>
    );
});

const diffConfig = (actual: any, defaults: any, prefix = ""): { path: string; value: any; def: any }[] => {
    const out: { path: string; value: any; def: any }[] = [];
    if (actual === null || typeof actual !== "object" || Array.isArray(actual)) return out;
    for (const key of Object.keys(actual)) {
        if (key === "$schema") continue;
        const path = prefix ? `${prefix}.${key}` : key;
        const a = actual[key];
        const d = defaults?.[key];
        if (a !== null && typeof a === "object" && !Array.isArray(a)) {
            out.push(...diffConfig(a, d, path));
        } else if (JSON.stringify(a) !== JSON.stringify(d)) {
            out.push({ path, value: a, def: d });
        }
    }
    return out;
};

const ConfigSummary = memo(() => {
    const { t } = useTranslation();
    const c = fluent();
    const [revision, setRevision] = useState(0);
    const overrides = useMemo(() => {
        try {
            return diffConfig(JSON.parse(shell.diagnostics.effective_config()), JSON.parse(shell.diagnostics.default_config()));
        } catch {
            return [];
        }
    }, [revision]);

    return (
        <Entrance delay={60}>
            <Card gap={10}>
                <flex horizontal alignItems="center" gap={8}>
                    <flex gap={2} flexGrow={1}>
                        <Text fontSize={14} fontWeight={600} color={c.text}>{t("problems.config.title")}</Text>
                        <Text fontSize={11} color={c.textTertiary} maxWidth={CARD_INNER - 230}>{shell.diagnostics.config_file_path()}</Text>
                    </flex>
                    <IconButton icon={ICON_REFRESH} label={t("problems.config.reload")} onClick={() => { shell.diagnostics.reload_config(); setRevision(r => r + 1); }} />
                    <IconButton icon={ICON_OPEN} label={t("problems.open")} onClick={() => shell.subproc.open(shell.diagnostics.config_file_path(), "")} />
                </flex>
                <Text fontSize={12} color={c.textSecondary}>{t("problems.config.overrides", { n: overrides.length })}</Text>
                {overrides.length > 0 && (
                    <flex gap={3} padding={8} borderRadius={4} backgroundColor={c.codeBg} alignItems="stretch">
                        {overrides.slice(0, 40).map(o => (
                            <flex key={o.path} horizontal gap={8}>
                                <Text fontSize={12} fontFamily="monospace" color={c.textSecondary} maxWidth={CODE_WIDTH * 0.45}>{o.path}</Text>
                                <Text fontSize={12} fontFamily="monospace" color={c.text} maxWidth={CODE_WIDTH * 0.3}>{JSON.stringify(o.value)}</Text>
                                {o.def !== undefined && (
                                    <Text fontSize={11} fontFamily="monospace" color={c.textTertiary} maxWidth={CODE_WIDTH * 0.25}>{`(${t("problems.config.default")} ${JSON.stringify(o.def)})`}</Text>
                                )}
                            </flex>
                        ))}
                    </flex>
                )}
            </Card>
        </Entrance>
    );
});

const ProblemsPage = memo(() => {
    const { t } = useTranslation();
    const c = fluent();
    const [problems, setProblems] = useState<shell.diagnostic_problem[]>([]);
    const [tab, setTab] = useState<Tab>("all");
    const [expanded, setExpanded] = useState<Record<string, boolean>>({});
    const revision = useRef(-1);

    const refresh = () => {
        const r = shell.diagnostics.problems_revision();
        if (r === revision.current) return;
        revision.current = r;
        setProblems(shell.diagnostics.problems());
    };

    useEffect(() => {
        refresh();
        const id = setInterval(refresh, 700);
        return () => clearInterval(id);
    }, []);

    const sorted = useMemo(() => problems.slice().sort((a, b) =>
        severityRank(a.severity) - severityRank(b.severity) || b.time - a.time), [problems]);
    const shown = tab === "all" ? sorted : sorted.filter(p => p.category === tab);
    const count = (cat: string) => problems.filter(p => p.category === cat).length;
    const errors = problems.filter(p => severityRank(p.severity) <= 1).length;

    const keyOf = (p: shell.diagnostic_problem) => `${p.category}|${p.title}`;

    return (
        <flex gap={14} alignItems="stretch" width={CONTENT_WIDTH} autoSize={false}>
            <SectionHeader title={t("problems.title")} subtitle={t("problems.subtitle")} gutter={SCROLL_GUTTER}>
                {tab !== "config" && problems.some(p => p.category === "runtime") && (
                    <IconButton label={t("problems.clearRuntime")} onClick={() => shell.diagnostics.clear_problems("runtime")} />
                )}
            </SectionHeader>

            <SegmentedControl<Tab>
                value={tab}
                onChange={setTab}
                options={[
                    { value: "all", label: t("problems.tab.all"), badge: problems.length },
                    { value: "config", label: t("problems.tab.config"), badge: count("config") },
                    { value: "script", label: t("problems.tab.script"), badge: count("script") },
                    { value: "runtime", label: t("problems.tab.runtime"), badge: count("runtime") },
                ]}
            />

            <flex enableScrolling maxHeight={440} gap={8} alignItems="stretch" paddingRight={SCROLL_GUTTER}>
                {tab === "all" && (
                    <Entrance>
                        {problems.length === 0
                            ? <InfoBar severity="info" title={t("problems.healthy.title")} message={t("problems.healthy.message")} maxTextWidth={CODE_WIDTH - 30} />
                            : <InfoBar severity={errors ? "error" : "warning"} title={t("problems.summary", { n: problems.length, errors })} />}
                    </Entrance>
                )}
                {tab === "config" && <ConfigSummary />}
                {shown.map((p, i) => (
                    <ProblemCard
                        key={keyOf(p)}
                        p={p}
                        index={i}
                        expanded={!!expanded[keyOf(p)]}
                        onToggle={() => setExpanded(prev => ({ ...prev, [keyOf(p)]: !prev[keyOf(p)] }))}
                    />
                ))}
                {tab !== "all" && shown.length === 0 && (
                    <Entrance delay={80}>
                        <flex padding={24} alignItems="center">
                            <Text fontSize={13} color={c.textTertiary}>{t("problems.none")}</Text>
                        </flex>
                    </Entrance>
                )}
            </flex>
        </flex>
    );
});

export default ProblemsPage;
