import { ThemeCustomEditor, AnimationCustomEditor } from "../components";
import {
    ChoiceCard, ComboBox, Expander, PageHeader, SettingsCard, SettingsGroup, ToggleSwitch, fluent
} from "../components/Fluent";
import { AppConfigContext, ContextMenuContext, LanguageContext } from "../contexts";
import {
    applyThemePreset, animationPreviewSvg, matchAnimationPreset, matchThemePreset, presetKeys,
    resolveFlat, themePreviewSvg, withPath
} from "../utils";
import { useTranslation } from "../hooks";
import {
    theme_presets, animation_presets, CONTENT_WIDTH, SCROLL_GUTTER, PAGE_BODY_HEIGHT,
    ICON_PALETTE, ICON_MOTION, ICON_OPACITY, ICON_SYNC, ICON_BRUSH, ICON_SWAP_VERT, ICON_KEYBOARD,
    ICON_PLUGIN_CONFIG, ICON_FOLDER, ICON_LANGUAGE, ICON_BELL
} from "../constants";
import { memo, useContext, useState } from "react";

const LANGUAGES = [
    { value: "zh-CN", label: "简体中文" },
    { value: "en-US", label: "English" }
];

const GALLERY_WIDTH = CONTENT_WIDTH - SCROLL_GUTTER - 2 - 32;
const GAP = 10;

const Gallery = ({ items, columns }: { items: any[]; columns: number }) => {
    const rows: any[][] = [];
    for (let i = 0; i < items.length; i += columns) rows.push(items.slice(i, i + columns));
    return (
        <flex gap={GAP} padding={16} alignItems="start">
            {rows.map((row, i) => <flex key={i} horizontal gap={GAP}>{row}</flex>)}
        </flex>
    );
};

const ContextMenuConfig = memo(() => {
    const { config, defaultConfig, update } = useContext(ContextMenuContext)!;
    const app = useContext(AppConfigContext)!;
    const { language, setLanguage } = useContext(LanguageContext)!;
    const { t } = useTranslation();
    const c = fluent();
    const [view, setView] = useState<"main" | "theme" | "animation">("main");

    const theme = config?.theme ?? {};
    const defaultTheme = defaultConfig?.theme ?? {};
    const animation = theme.animation && typeof theme.animation === "object" ? theme.animation : {};
    const defaultAnimation = defaultTheme.animation ?? {};

    const themePreset = matchThemePreset(theme, defaultTheme, theme_presets);
    const animationPreset = matchAnimationPreset(animation, defaultAnimation, animation_presets);

    const setTheme = (next: any) => update({ ...config, theme: next });
    const setAnimation = (next: any) => {
        const nextTheme = { ...theme };
        if (next && Object.keys(next).length) nextTheme.animation = next;
        else delete nextTheme.animation;
        setTheme(nextTheme);
    };
    const setValue = (path: string, value: any) => update(withPath(config, path, value, defaultConfig));
    const valueOf = (path: string) =>
        path.split(".").reduce((o: any, k) => o?.[k], config) ?? path.split(".").reduce((o: any, k) => o?.[k], defaultConfig);

    if (view === "theme") {
        return (
            <ThemeCustomEditor
                theme={theme}
                defaultTheme={defaultTheme}
                onUpdate={setTheme}
                onClose={() => setView("main")}
            />
        );
    }

    if (view === "animation") {
        return (
            <AnimationCustomEditor
                animation={animation}
                defaultAnimation={defaultAnimation}
                onUpdate={setAnimation}
                globalAnimation={app.config?.default_animation ?? {}}
                onGlobalUpdate={(next) => app.updateConfig(cur => {
                    const out = { ...cur };
                    if (next && Object.keys(next).length) out.default_animation = next;
                    else delete out.default_animation;
                    return out;
                })}
                onClose={() => setView("main")}
            />
        );
    }

    const themeKeys = presetKeys(theme_presets);
    const themeCardWidth = Math.floor((GALLERY_WIDTH - GAP * 2) / 3);
    const themePreviewW = themeCardWidth - 18;
    const themePreviewH = Math.round(themePreviewW * 0.6);
    const themeCards = [
        ...Object.keys(theme_presets).map(name => (
            <ChoiceCard
                key={name}
                width={themeCardWidth}
                label={t(`preset.${name}`)}
                preview={themePreviewSvg(resolveFlat(theme_presets[name] ?? {}, defaultTheme, themeKeys), c.light, themePreviewW, themePreviewH)}
                previewWidth={themePreviewW}
                previewHeight={themePreviewH}
                selected={themePreset === name}
                onClick={() => setTheme(applyThemePreset(theme, theme_presets[name], theme_presets))}
            />
        )),
        <ChoiceCard
            key="custom"
            width={themeCardWidth}
            label={t("preset.custom")}
            preview={themePreviewSvg(resolveFlat(theme, defaultTheme, themeKeys), c.light, themePreviewW, themePreviewH)}
            previewWidth={themePreviewW}
            previewHeight={themePreviewH}
            selected={themePreset === "custom"}
            onClick={() => setView("theme")}
        />
    ];

    const animCardWidth = Math.floor((GALLERY_WIDTH - GAP * 3) / 4);
    const animPreviewW = animCardWidth - 18;
    const animPreviewH = Math.round(animPreviewW * 0.64);
    const animationCards = [
        ...Object.keys(animation_presets).map(name => (
            <ChoiceCard
                key={name}
                width={animCardWidth}
                label={t(`preset.${name}`)}
                preview={animationPreviewSvg(name as any, c.light, animPreviewW, animPreviewH)}
                previewWidth={animPreviewW}
                previewHeight={animPreviewH}
                selected={animationPreset === name}
                onClick={() => setAnimation(animation_presets[name] ? JSON.parse(JSON.stringify(animation_presets[name])) : undefined)}
            />
        )),
        <ChoiceCard
            key="custom"
            width={animCardWidth}
            label={t("preset.custom")}
            preview={animationPreviewSvg("custom", c.light, animPreviewW, animPreviewH)}
            previewWidth={animPreviewW}
            previewHeight={animPreviewH}
            selected={animationPreset === "custom"}
            onClick={() => setView("animation")}
        />
    ];

    const toggle = (path: string, icon: string, key: string) => (
        <SettingsCard key={path} icon={icon} title={t(`settings.${key}`)} description={t(`settings.${key}Desc`)}>
            <ToggleSwitch value={!!valueOf(path)} onChange={v => setValue(path, v)} onLabel={t("settings.on")} offLabel={t("settings.off")} />
        </SettingsCard>
    );

    return (
        <flex gap={16} alignItems="stretch" width={CONTENT_WIDTH}>
            <PageHeader title={t("settings.title")} gutter={SCROLL_GUTTER} />
            <flex enableScrolling maxHeight={PAGE_BODY_HEIGHT} alignItems="stretch" paddingRight={SCROLL_GUTTER} paddingBottom={16}>
                <SettingsGroup title={t("settings.appearance")}>
                    <Expander
                        icon={ICON_PALETTE}
                        title={t("settings.theme")}
                        description={t("settings.themeDesc")}
                        header={<flex><text text={t(`preset.${themePreset}`)} fontSize={13} color={c.textSecondary} /></flex>}
                        defaultOpen
                    >
                        <Gallery items={themeCards} columns={3} />
                    </Expander>
                    <Expander
                        icon={ICON_MOTION}
                        title={t("settings.animation")}
                        description={t("settings.animationDesc")}
                        header={<flex><text text={t(`preset.${animationPreset}`)} fontSize={13} color={c.textSecondary} /></flex>}
                        defaultOpen
                    >
                        <Gallery items={animationCards} columns={4} />
                    </Expander>
                    {toggle("theme.acrylic", ICON_OPACITY, "acrylicBackground")}
                </SettingsGroup>

                <SettingsGroup title={t("settings.behavior")}>
                    {toggle("vsync", ICON_SYNC, "vsync")}
                    {toggle("ignore_owner_draw", ICON_BRUSH, "ignoreOwnerDraw")}
                    {toggle("reverse_if_open_to_up", ICON_SWAP_VERT, "reverseIfOpenToUp")}
                    {toggle("hotkeys", ICON_KEYBOARD, "hotkeys")}
                    {toggle("show_settings_button", ICON_PLUGIN_CONFIG, "showSettingsButton")}
                    {toggle("patch_explorerframe_dll", ICON_FOLDER, "patchExplorerFrameDll")}
                </SettingsGroup>

                <SettingsGroup title={t("settings.general")}>
                    <SettingsCard icon={ICON_LANGUAGE} title={t("settings.language")} description={t("settings.languageDesc")}>
                        <ComboBox value={LANGUAGES.some(l => l.value === language) ? language : "zh-CN"} options={LANGUAGES} onChange={setLanguage} />
                    </SettingsCard>
                    <SettingsCard icon={ICON_BELL} title={t("settings.debugConsole")} description={t("settings.debugConsoleDesc")}>
                        <ToggleSwitch
                            value={!!app.config?.debug_console}
                            onChange={v => app.updateConfig(cur => ({ ...cur, debug_console: v }))}
                            onLabel={t("settings.on")}
                            offLabel={t("settings.off")}
                        />
                    </SettingsCard>
                </SettingsGroup>
            </flex>
        </flex>
    );
});

export default ContextMenuConfig;
