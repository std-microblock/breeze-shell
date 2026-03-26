import * as shell from "mshell";
import { Button, Text, Toggle, ThemeCustomEditor, AnimationCustomEditor } from "../components";
import { ContextMenuContext, DebugConsoleContext, LanguageContext } from "../contexts";
import { getNestedValue, setNestedValue } from "../utils";
import { useTranslation } from "../hooks";
import { theme_presets, animation_presets } from "../constants";
import { memo, useContext, useState } from "react";

const ContextMenuConfig = memo(() => {
    const { config, update } = useContext(ContextMenuContext)!;
    const { value: debugConsole, update: updateDebugConsole } = useContext(DebugConsoleContext)!;
    const { language, setLanguage } = useContext(LanguageContext)!;
    const { t } = useTranslation();
    const [, forceUpdate] = useState(0);
    const [showThemeEditor, setShowThemeEditor] = useState(false);
    const [showAnimationEditor, setShowAnimationEditor] = useState(false);

    const languages = [
        { code: 'zh-CN', name: '简体中文' },
        { code: 'en-US', name: 'English' }
    ];

    const currentTheme = config?.theme;
    const currentAnimation =
        config?.theme?.animation && typeof config.theme.animation === "object"
            ? config.theme.animation
            : {};

    const getAllSubkeys = (presets: any) => {
        if (!presets) return [];
        const keys = new Set();
        for (const v of Object.values(presets)) {
            if (v)
                for (const key of Object.keys(v)) {
                    keys.add(key);
                }
        }
        return [...keys];
    };

    const applyPreset = (preset: any, origin: any, presets: any) => {
        const allSubkeys = getAllSubkeys(presets);
        const newPreset = preset ? { ...preset } : {};
        for (let key in origin) {
            if (allSubkeys.includes(key)) continue;
            newPreset[key] = origin[key];
        }
        return newPreset;
    };

    const checkPresetMatch = (current: any, preset: any, excludeKeys: string[] = []) => {
        if (!current) return false;
        if (!preset) return false;
        return Object.keys(preset).every(key => {
            if (excludeKeys.includes(key)) return true;
            return JSON.stringify(current[key]) === JSON.stringify(preset[key]);
        });
    };

    const getCurrentPreset = (current: any, presets: any, excludeKeys: string[] = []) => {
        if (!current || Object.keys(current).length === 0) return "default";
        for (const [name, preset] of Object.entries(presets)) {
            if (preset && checkPresetMatch(current, preset, excludeKeys)) {
                return name;
            }
        }
        return "custom";
    };

    const currentThemePreset = getCurrentPreset(currentTheme, theme_presets, ["animation"]);
    const currentAnimationPreset = getCurrentPreset(currentAnimation, animation_presets);

    if (showThemeEditor) {
        return (
            <ThemeCustomEditor
                theme={currentTheme}
                onUpdate={(newTheme) => {
                    update({ ...config, theme: newTheme });
                }}
                onClose={() => setShowThemeEditor(false)}
            />
        );
    }

    if (showAnimationEditor) {
        return (
            <AnimationCustomEditor
                animation={currentAnimation}
                onUpdate={(newAnimation) => {
                    update({ ...config, theme: { ...(config?.theme || {}), animation: newAnimation } });
                }}
                onClose={() => setShowAnimationEditor(false)}
            />
        );
    }

    return (
        <flex gap={20} alignItems="stretch" width={500} autoSize={false}>
            <Text fontSize={24}>{t("settings.title")}</Text>
            <flex />
            <flex gap={10}>
                <Text fontSize={18}>{t("settings.theme")}</Text>
                <flex horizontal gap={10}>
                    {Object.keys(theme_presets).map(name => (
                        <Button
                            key={name}
                            selected={name === currentThemePreset}
                            onClick={() => {
                                try {
                                    let newTheme;
                                    if (!theme_presets[name]) {
                                        const currentAnim = config?.theme?.animation;
                                        newTheme = currentAnim ? { animation: currentAnim } : undefined;
                                    } else {
                                        newTheme = applyPreset(theme_presets[name], config?.theme, theme_presets);
                                    }
                                    update({ ...config, theme: newTheme });
                                } catch (e) {
                                    shell.println(e);
                                }
                            }}
                        >
                            <Text fontSize={14}>{t(`preset.${name}`)}</Text>
                        </Button>
                    ))}
                    <Button
                        selected={currentThemePreset === "custom"}
                        onClick={() => setShowThemeEditor(true)}
                    >
                        <Text fontSize={14}>{t("preset.custom")}</Text>
                    </Button>
                </flex>
            </flex>

            <flex gap={10}>
                <Text fontSize={18}>{t("settings.animation")}</Text>
                <flex horizontal gap={10}>
                    {Object.keys(animation_presets).map(name => (
                        <Button
                            key={name}
                            selected={name === currentAnimationPreset}
                            onClick={() => {
                                try {
                                    let newAnimation;
                                    if (!animation_presets[name]) {
                                        newAnimation = undefined;
                                    } else {
                                        newAnimation = animation_presets[name];
                                    }
                                    update({ ...config, theme: { ...(config?.theme || {}), animation: newAnimation } });
                                } catch (e) {
                                    shell.println(e);
                                }
                            }}
                        >
                            <Text fontSize={14}>{t(`preset.${name}`)}</Text>
                        </Button>
                    ))}
                    <Button
                        selected={currentAnimationPreset === "custom"}
                        onClick={() => setShowAnimationEditor(true)}
                    >
                        <Text fontSize={14}>{t("preset.custom")}</Text>
                    </Button>
                </flex>
            </flex>

            <flex gap={10}>
                <Text fontSize={18}>{t("settings.language")}</Text>
                <flex horizontal gap={10}>
                    {languages.map(lang => (
                        <Button
                            key={lang.code}
                            selected={language === lang.code}
                            onClick={() => {
                                setLanguage(lang.code);
                                forceUpdate(n => n + 1);
                            }}
                        >
                            <Text fontSize={14}>{lang.name}</Text>
                        </Button>
                    ))}
                </flex>
            </flex>

            <flex gap={10} alignItems="stretch" justifyContent="center">
                <Text fontSize={18}>{t("settings.misc")}</Text>
                <Toggle label={t("settings.debugConsole")} value={debugConsole} onChange={updateDebugConsole} />
                <Toggle label={t("settings.vsync")} value={getNestedValue(config, "vsync") ?? true} onChange={(v) => {
                    const newConfig = { ...config };
                    setNestedValue(newConfig, "vsync", v);
                    update(newConfig);
                }} />
                <Toggle label={t("settings.ignoreOwnerDraw")} value={getNestedValue(config, "ignore_owner_draw") ?? true} onChange={(v) => {
                    const newConfig = { ...config };
                    setNestedValue(newConfig, "ignore_owner_draw", v);
                    update(newConfig);
                }} />
                <Toggle label={t("settings.reverseIfOpenToUp")} value={getNestedValue(config, "reverse_if_open_to_up") ?? true} onChange={(v) => {
                    const newConfig = { ...config };
                    setNestedValue(newConfig, "reverse_if_open_to_up", v);
                    update(newConfig);
                }} />
                <Toggle label={t("settings.acrylicBackground")} value={getNestedValue(config, "theme.acrylic") ?? true} onChange={(v) => {
                    const newConfig = { ...config };
                    setNestedValue(newConfig, "theme.acrylic", v);
                    update(newConfig);
                }} />
                <Toggle label={t("settings.hotkeys")} value={getNestedValue(config, "hotkeys") ?? true} onChange={(v) => {
                    const newConfig = { ...config };
                    setNestedValue(newConfig, "hotkeys", v);
                    update(newConfig);
                }} />
                <Toggle label={t("settings.showSettingsButton")} value={getNestedValue(config, "show_settings_button") ?? true} onChange={(v) => {
                    const newConfig = { ...config };
                    setNestedValue(newConfig, "show_settings_button", v);
                    update(newConfig);
                }} />
            </flex>
        </flex>
    );
});

export default ContextMenuConfig;
