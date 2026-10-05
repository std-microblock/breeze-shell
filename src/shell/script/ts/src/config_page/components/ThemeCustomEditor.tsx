import { memo } from "react";
import {
    Button, ExpanderRow, PageHeader, SettingsGroup, SliderField, ToggleSwitch, fluent
} from "./Fluent";
import { useTranslation } from "../hooks";
import { presetKeys, resolveFlat, themePreviewSvg, withPath } from "../utils";
import { CONTENT_WIDTH, PAGE_BODY_HEIGHT, SCROLL_GUTTER, theme_presets } from "../constants";

type NumberRow = { key: string; label: string; min: number; max: number; step: number; suffix?: string; scale?: number };

const List = ({ children }: { children?: any }) => {
    const c = fluent();
    return (
        <flex alignItems="stretch" borderRadius={7} borderWidth={1} borderColor={c.cardStroke} backgroundColor={c.card} paddingTop={4} paddingBottom={4}>
            {children}
        </flex>
    );
};

export const ThemeCustomEditor = memo(({ theme, defaultTheme, onUpdate, onClose }: {
    theme: any;
    defaultTheme: any;
    onUpdate: (theme: any) => void;
    onClose: () => void;
}) => {
    const { t } = useTranslation();
    const c = fluent();
    const value = (key: string) => theme?.[key] ?? defaultTheme?.[key] ?? 0;
    const set = (key: string, v: any) => onUpdate(withPath(theme, key, v, defaultTheme));

    const size: NumberRow[] = [
        { key: "radius", label: "radius", min: 0, max: 20, step: 0.5 },
        { key: "item_height", label: "itemHeight", min: 16, max: 40, step: 1 },
        { key: "item_gap", label: "itemGap", min: 0, max: 12, step: 0.5 },
        { key: "item_radius", label: "itemRadius", min: 0, max: 20, step: 0.5 },
        { key: "margin", label: "margin", min: 0, max: 20, step: 0.5 },
        { key: "padding", label: "padding", min: 0, max: 20, step: 0.5 },
    ];
    const text: NumberRow[] = [
        { key: "font_size", label: "fontSize", min: 10, max: 24, step: 1 },
        { key: "text_padding", label: "textPadding", min: 0, max: 24, step: 0.5 },
        { key: "icon_padding", label: "iconPadding", min: 0, max: 16, step: 0.5 },
        { key: "right_icon_padding", label: "rightIconPadding", min: 0, max: 40, step: 1 },
        { key: "multibutton_line_gap", label: "multibuttonLineGap", min: -12, max: 12, step: 0.5 },
    ];
    const effects: NumberRow[] = [
        { key: "background_opacity", label: "backgroundOpacity", min: 0, max: 100, step: 1, suffix: "%", scale: 100 },
        { key: "border_width", label: "borderWidth", min: 0, max: 4, step: 0.5 },
        { key: "shadow_size", label: "shadowSize", min: 0, max: 30, step: 1 },
    ];

    const rows = (list: NumberRow[], tail?: any) => (
        <List>
            {list.map((r, i) => {
                const scale = r.scale ?? 1;
                return (
                    <ExpanderRow key={r.key} title={t(`customEditor.theme.${r.label}`)} indent={16} last={!tail && i === list.length - 1}>
                        <SliderField
                            value={Math.round(value(r.key) * scale * 1000) / 1000}
                            min={r.min}
                            max={r.max}
                            step={r.step}
                            suffix={r.suffix}
                            onChange={v => set(r.key, v / scale)}
                        />
                    </ExpanderRow>
                );
            })}
            {tail}
        </List>
    );

    const managedKeys = [...size, ...text, ...effects].map(r => r.key).concat(["acrylic", "use_dwm_if_available"]);
    const reset = () => {
        const next = { ...(theme ?? {}) };
        for (const k of managedKeys) delete next[k];
        onUpdate(next);
    };

    const keys = presetKeys(theme_presets);
    const previewW = 300, previewH = 168;
    const resolved = { ...resolveFlat(theme, defaultTheme, keys) };

    return (
        <flex gap={16} alignItems="stretch" width={CONTENT_WIDTH}>
            <PageHeader title={t("customEditor.theme.title")} parent={t("settings.title")} onBack={onClose} gutter={SCROLL_GUTTER}>
                <Button label={t("customEditor.reset")} onClick={reset} />
                <Button label={t("customEditor.theme.done")} variant="accent" onClick={onClose} />
            </PageHeader>
            <flex enableScrolling maxHeight={PAGE_BODY_HEIGHT} alignItems="stretch" paddingRight={SCROLL_GUTTER} paddingBottom={16} gap={4}>
                <flex horizontal gap={16} alignItems="center" padding={16} borderRadius={7} borderWidth={1} borderColor={c.cardStroke} backgroundColor={c.card}>
                    <img svg={themePreviewSvg(resolved, c.light, previewW, previewH)} width={previewW} height={previewH} />
                    <flex gap={6} flexShrink={1}>
                        <text text={t("customEditor.theme.preview")} fontSize={14} fontWeight={600} color={c.text} />
                        <text text={t("customEditor.theme.previewDesc")} fontSize={12} color={c.textSecondary} maxWidth={CONTENT_WIDTH - SCROLL_GUTTER - previewW - 60} />
                    </flex>
                </flex>
                <SettingsGroup title={t("customEditor.theme.sizeSettings")}>{rows(size)}</SettingsGroup>
                <SettingsGroup title={t("customEditor.theme.textAndIcon")}>{rows(text)}</SettingsGroup>
                <SettingsGroup title={t("customEditor.theme.effects")}>
                    {rows(effects, (
                        <>
                            <ExpanderRow title={t("customEditor.theme.acrylic")} indent={16}>
                                <ToggleSwitch value={!!value("acrylic")} onChange={v => set("acrylic", v)} onLabel={t("settings.on")} offLabel={t("settings.off")} />
                            </ExpanderRow>
                            <ExpanderRow title={t("customEditor.theme.useDwm")} description={t("customEditor.theme.useDwmDesc")} indent={16} last>
                                <ToggleSwitch value={!!value("use_dwm_if_available")} onChange={v => set("use_dwm_if_available", v)} onLabel={t("settings.on")} offLabel={t("settings.off")} />
                            </ExpanderRow>
                        </>
                    ))}
                </SettingsGroup>
            </flex>
        </flex>
    );
});
