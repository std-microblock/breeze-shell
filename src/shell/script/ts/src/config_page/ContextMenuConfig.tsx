import * as shell from "mshell";
import { Button, Text, Toggle } from "./components";
import { ContextMenuContext, DebugConsoleContext } from "./contexts";
import { useTranslation, getNestedValue, setNestedValue } from "./utils";
import { theme_presets, animation_presets } from "./constants";
import { memo, useContext } from "react";
const ContextMenuConfig = memo(() => {
    const { config, update } = useContext(ContextMenuContext)!;
    const { value: debugConsole, update: updateDebugConsole } = useContext(DebugConsoleContext)!;
    const { t } = useTranslation();

    const currentTheme = config?.theme;
    const currentAnimation = config?.theme?.animation;

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

    const checkPresetMatch = (current: any, preset: any) => {
        if (!current) return false;
        if (!preset) return false;
        return Object.keys(preset).every(key => JSON.stringify(current[key]) === JSON.stringify(preset[key]));
    };

    const getCurrentPreset = (current: any, presets: any) => {
        if (!current) return "默认";
        for (const [name, preset] of Object.entries(presets)) {
            if (preset && checkPresetMatch(current, preset)) {
                return name;
            }
        }
        return "自定义";
    };

    const currentThemePreset = getCurrentPreset(currentTheme, theme_presets);
    const currentAnimationPreset = getCurrentPreset(currentAnimation, animation_presets);

    return (
        <flex gap={20} alignItems="stretch" width={500} autoSize={false}>
            <Text fontSize={24}>{t("Breeze 设置")}</Text>
            <flex />
            <flex gap={10}>
                <Text fontSize={18}>{t("主题")}</Text>
                <flex horizontal gap={10}>
                    {Object.keys(theme_presets).map(name => (
                        <Button
                            key={name}
                            selected={name === currentThemePreset}
                            onClick={() => {
                                try {
                                    let newTheme;
                                    if (!theme_presets[name]) {
                                        newTheme = undefined;
                                    } else {
                                        newTheme = applyPreset(theme_presets[name], config?.theme, theme_presets);
                                    }
                                    update(newTheme ? { ...config, theme: newTheme } : { ...config, theme: undefined });
                                } catch (e) {
                                    shell.println(e);
                                }
                            }}
                        >
                            <Text fontSize={14}>{name}</Text>
                        </Button>
                    ))}
                </flex>
            </flex>

            <flex gap={10}>
                <Text fontSize={18}>{t("动画")}</Text>
                <flex horizontal gap={10}>
                    {Object.keys(animation_presets).map(name => (
                        <Button
                            key={name}
                            onClick={() => {
                                try {
                                    let newAnimation;
                                    if (!animation_presets[name]) {
                                        newAnimation = undefined;
                                    } else {
                                        newAnimation = animation_presets[name];
                                    }
                                    update({ ...config, theme: { ...config.theme, animation: newAnimation } });
                                } catch (e) {
                                    shell.println(e);
                                }
                            }}
                        >
                            <Text fontSize={14}>{name}</Text>
                        </Button>
                    ))}
                </flex>
            </flex>

            <flex gap={10} alignItems="stretch" justifyContent="center">
                <Text fontSize={18}>{t("杂项")}</Text>
                <Toggle label={t("调试控制台")} value={debugConsole} onChange={updateDebugConsole} />
                <Toggle label={t("垂直同步")} value={getNestedValue(config, "vsync") ?? true} onChange={(v) => {
                    const newConfig = { ...config };
                    setNestedValue(newConfig, "vsync", v);
                    update(newConfig);
                }} />
                <Toggle label={t("忽略自绘菜单")} value={getNestedValue(config, "ignore_owner_draw") ?? true} onChange={(v) => {
                    const newConfig = { ...config };
                    setNestedValue(newConfig, "ignore_owner_draw", v);
                    update(newConfig);
                }} />
                <Toggle label={t("向上展开时反向排列")} value={getNestedValue(config, "reverse_if_open_to_up") ?? true} onChange={(v) => {
                    const newConfig = { ...config };
                    setNestedValue(newConfig, "reverse_if_open_to_up", v);
                    update(newConfig);
                }} />
                <Toggle label={t("尝试使用 Windows 11 圆角")} value={getNestedValue(config, "theme.use_dwm_if_available") ?? true} onChange={(v) => {
                    const newConfig = { ...config };
                    setNestedValue(newConfig, "theme.use_dwm_if_available", v);
                    update(newConfig);
                }} />
                <Toggle label={t("亚克力背景效果")} value={getNestedValue(config, "theme.acrylic") ?? true} onChange={(v) => {
                    const newConfig = { ...config };
                    setNestedValue(newConfig, "theme.acrylic", v);
                    update(newConfig);
                }} />
                <Toggle label={t("键盘热键")} value={getNestedValue(config, "hotkeys") ?? true} onChange={(v) => {
                    const newConfig = { ...config };
                    setNestedValue(newConfig, "hotkeys", v);
                    update(newConfig);
                }} />
            </flex>
        </flex>
    );
});

export default ContextMenuConfig;