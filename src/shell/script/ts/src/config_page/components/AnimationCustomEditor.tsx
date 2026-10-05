import { memo, useMemo } from "react";
import {
    Button, ComboBox, Divider, Expander, NumberBox, PageHeader, SettingsGroup, Slider, fluent
} from "./Fluent";
import { iconElement } from "./Icon";
import { Text } from "./Text";
import { useTranslation } from "../hooks";
import { easingCurveSvg, flatten, withPath } from "../utils";
import { CONTENT_WIDTH, ICON_MOTION, ICON_RESET, PAGE_BODY_HEIGHT, SCROLL_GUTTER } from "../constants";

type Conf = { duration?: number; easing?: string; delay_scale?: number };
type Group = { key: string; title: string; desc: string; props: string[]; numbers?: { key: string; label: string; min: number; max: number; step: number }[] };

const FALLBACK: Conf = { duration: 150, easing: "ease_in_out", delay_scale: 1 };

const EasingPicker = ({ value, onChange, options }: {
    value: string;
    onChange: (v: string) => void;
    options: { value: string; label: string }[];
}) => {
    const c = fluent();
    return (
        <flex horizontal alignItems="center" gap={10}>
            <img svg={easingCurveSvg(value, c.accent.slice(0, 7), c.light ? "#000000" : "#FFFFFF")} width={40} height={28} />
            <ComboBox value={value} options={options} onChange={onChange} width={150} />
        </flex>
    );
};

const PropertyEditor = ({ label, conf, defaults, onChange, options, last }: {
    label: string;
    conf: Conf;
    defaults: Conf;
    onChange: (field: keyof Conf, v: any) => void;
    options: { value: string; label: string }[];
    last?: boolean;
}) => {
    const { t } = useTranslation();
    const c = fluent();
    const v = { ...FALLBACK, ...defaults, ...conf };
    const modified = Object.keys(conf ?? {}).length > 0;
    const off = v.easing === "mutation";
    return (
        <flex alignItems="stretch">
            <flex gap={8} paddingLeft={50} paddingRight={16} paddingTop={12} paddingBottom={12} alignItems="stretch">
                <flex horizontal alignItems="center" gap={8}>
                    <Text fontSize={13} fontWeight={600} color={c.text}>{label}</Text>
                    {modified && <flex width={6} height={6} borderRadius={3} backgroundColor={c.accent} />}
                    <spacer />
                    <EasingPicker value={v.easing!} options={options} onChange={e => onChange("easing", e)} />
                    <flex width={32} height={32} justifyContent="center" alignItems="center"
                        opacity={modified ? 255 : 0}
                        onClick={() => { if (modified) onChange("duration", undefined); }}>
                        {iconElement(ICON_RESET, 14, c.textSecondary)}
                    </flex>
                </flex>
                {!off && (
                    <flex horizontal alignItems="center" gap={10}>
                        <Text fontSize={12} color={c.textSecondary}>{t("customEditor.animation.duration")}</Text>
                        <Slider value={Math.min(v.duration!, 1000)} min={0} max={1000} step={10} width={170} onChange={d => onChange("duration", d)} />
                        <NumberBox value={v.duration!} min={0} max={5000} step={10} width={64} suffix="ms" onChange={d => onChange("duration", d)} />
                        <spacer />
                        <Text fontSize={12} color={c.textSecondary}>{t("customEditor.animation.delayScale")}</Text>
                        <NumberBox value={v.delay_scale!} min={0} max={10} step={0.1} width={56} suffix="×" onChange={d => onChange("delay_scale", d)} />
                    </flex>
                )}
            </flex>
            {!last && <flex paddingLeft={50} alignItems="stretch"><Divider /></flex>}
        </flex>
    );
};

const NumberRow = ({ label, value, min, max, step, onChange, last }: {
    label: string; value: number; min: number; max: number; step: number; onChange: (v: number) => void; last?: boolean;
}) => {
    const c = fluent();
    return (
        <flex alignItems="stretch">
            <flex horizontal alignItems="center" gap={10} paddingLeft={50} paddingRight={16} paddingTop={10} paddingBottom={10}>
                <Text fontSize={13} fontWeight={600} color={c.text}>{label}</Text>
                <spacer />
                <Slider value={value} min={min} max={max} step={step} width={170} onChange={onChange} />
                <NumberBox value={value} min={min} max={max} step={step} width={64} onChange={onChange} />
            </flex>
            {!last && <flex paddingLeft={50} alignItems="stretch"><Divider /></flex>}
        </flex>
    );
};

export const AnimationCustomEditor = memo(({ animation, defaultAnimation, onUpdate, globalAnimation, onGlobalUpdate, onClose }: {
    animation: any;
    defaultAnimation: any;
    onUpdate: (animation: any) => void;
    globalAnimation: Conf;
    onGlobalUpdate: (conf: Conf) => void;
    onClose: () => void;
}) => {
    const { t } = useTranslation();
    const c = fluent();

    const easingOptions = useMemo(() => [
        { value: "mutation", label: t("customEditor.animation.instant") },
        { value: "linear", label: t("customEditor.animation.linear") },
        { value: "ease_in", label: t("customEditor.animation.easeIn") },
        { value: "ease_out", label: t("customEditor.animation.easeOut") },
        { value: "ease_in_out", label: t("customEditor.animation.easeInOut") }
    ], [t]);

    const groups: Group[] = [
        { key: "main", title: "mainMenu", desc: "mainMenuDesc", props: ["y"] },
        {
            key: "item", title: "menuItem", desc: "menuItemDesc", props: ["opacity", "x", "y", "width", "blur"],
            numbers: [{ key: "appear_blur", label: "appearBlur", min: 0, max: 10, step: 0.5 }]
        },
        {
            key: "main_bg", title: "mainBg", desc: "mainBgDesc", props: ["opacity", "x", "y", "w", "h"],
            numbers: [
                { key: "appear_w_scale", label: "appearWScale", min: 0, max: 1, step: 0.05 },
                { key: "appear_h_scale", label: "appearHScale", min: 0, max: 1, step: 0.05 }
            ]
        },
        {
            key: "submenu_bg", title: "submenuBg", desc: "submenuBgDesc", props: ["opacity", "x", "y", "w", "h"],
            numbers: [
                { key: "appear_w_scale", label: "appearWScale", min: 0, max: 1, step: 0.05 },
                { key: "appear_h_scale", label: "appearHScale", min: 0, max: 1, step: 0.05 }
            ]
        },
    ];

    const propLabel = (p: string) => t(`customEditor.animation.${p === "w" ? "width" : p === "h" ? "height" : p}`);

    const setField = (path: string, field: keyof Conf, value: any) => {
        if (value === undefined) {
            onUpdate(withPath(animation, path, undefined));
            return;
        }
        onUpdate(withPath(animation, `${path}.${field}`, value, defaultAnimation));
    };

    const global = { ...FALLBACK, ...globalAnimation };
    const setGlobal = (field: keyof Conf, value: any) => {
        const next: Conf = { ...globalAnimation };
        if (value === undefined) {
            onGlobalUpdate({});
            return;
        }
        (next as any)[field] = value;
        for (const k of Object.keys(next) as (keyof Conf)[])
            if (next[k] === FALLBACK[k]) delete next[k];
        onGlobalUpdate(next);
    };

    const modifiedIn = (group: string) => Object.keys(flatten(animation?.[group] ?? {})).length;

    return (
        <flex gap={16} alignItems="stretch" width={CONTENT_WIDTH}>
            <PageHeader title={t("customEditor.animation.title")} parent={t("settings.title")} onBack={onClose} gutter={SCROLL_GUTTER}>
                <Button label={t("customEditor.reset")} onClick={() => onUpdate({})} />
                <Button label={t("customEditor.animation.done")} variant="accent" onClick={onClose} />
            </PageHeader>
            <flex enableScrolling maxHeight={PAGE_BODY_HEIGHT} alignItems="stretch" paddingRight={SCROLL_GUTTER} paddingBottom={16} gap={4}>
                <SettingsGroup title={t("customEditor.animation.global")}>
                    <Expander icon={ICON_MOTION} title={t("customEditor.animation.globalTitle")} description={t("customEditor.animation.globalDesc")} defaultOpen>
                        <PropertyEditor
                            label={t("customEditor.animation.fallback")}
                            conf={globalAnimation}
                            defaults={FALLBACK}
                            options={easingOptions}
                            onChange={setGlobal}
                            last
                        />
                    </Expander>
                </SettingsGroup>
                <SettingsGroup title={t("customEditor.animation.contextMenu")}>
                    {groups.map(g => {
                        const count = modifiedIn(g.key);
                        return (
                            <Expander
                                key={g.key}
                                icon={ICON_MOTION}
                                title={t(`customEditor.animation.${g.title}`)}
                                description={t(`customEditor.animation.${g.desc}`)}
                                header={count > 0
                                    ? <Text fontSize={12} color={c.accent}>{t("customEditor.animation.modified", { n: count })}</Text>
                                    : undefined}
                            >
                                {g.props.map((p, i) => (
                                    <PropertyEditor
                                        key={p}
                                        label={propLabel(p)}
                                        conf={animation?.[g.key]?.[p] ?? {}}
                                        defaults={defaultAnimation?.[g.key]?.[p] ?? {}}
                                        options={easingOptions}
                                        onChange={(field, v) => setField(`${g.key}.${p}`, field, v)}
                                        last={i === g.props.length - 1 && !g.numbers?.length}
                                    />
                                ))}
                                {(g.numbers ?? []).map((n, i) => (
                                    <NumberRow
                                        key={n.key}
                                        label={t(`customEditor.animation.${n.label}`)}
                                        value={animation?.[g.key]?.[n.key] ?? defaultAnimation?.[g.key]?.[n.key] ?? 0}
                                        min={n.min}
                                        max={n.max}
                                        step={n.step}
                                        onChange={v => onUpdate(withPath(animation, `${g.key}.${n.key}`, v, defaultAnimation))}
                                        last={i === g.numbers!.length - 1}
                                    />
                                ))}
                            </Expander>
                        );
                    })}
                </SettingsGroup>
            </flex>
        </flex>
    );
});
