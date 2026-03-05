import * as shell from "mshell";
import { Button, Text, NumberBox, Toggle } from "../components";
import React, { memo, useState, useMemo } from "react";
import { breeze } from "mshell";
import { useTranslation } from "../hooks";

const debounce = (func: Function, delay: number) => {
    let timer = null;
    return function (...args: any[]) {
        if (timer)
            clearTimeout(timer)
        timer = setTimeout(() => {
            timer = null;
            func(...args)
        }, delay)
    }
};

const OptionGroup = ({ label, children }: { label: string; children: React.ReactNode }) => {
    return (
        <flex gap={6} backgroundColor={breeze.is_light_theme() ? '#ffffff40' : '#2a2a2a40'} padding={16} borderRadius={14}>
            <Text fontSize={15}>{label}</Text>
            <flex gap={5} paddingLeft={20} alignItems="stretch">
                {children}
            </flex>
        </flex>
    );
};

export const ThemeCustomEditor = memo(({
    theme,
    onUpdate,
    onClose
}: {
    theme: any;
    onUpdate: (theme: any) => void;
    onClose: () => void;
}) => {
    const isLightTheme = breeze.is_light_theme();
    const { t } = useTranslation();
    const [localTheme, setLocalTheme] = useState(theme);

    const debouncedSave = useMemo(
        () => debounce((newTheme: any) => {
            onUpdate(newTheme);
        }, 500),
        [onUpdate]
    );

    const updateValue = (key: string, value: any) => {
        const newTheme = { ...localTheme, [key]: value };
        setLocalTheme(newTheme);
        debouncedSave(newTheme);
    };

    return (
        <flex
            gap={20}
            alignItems="stretch"
            width={600}
            enableScrolling
            maxHeight={550}
            paddingLeft={25}
            paddingRight={25}
            paddingTop={5}
        >
            <flex horizontal justifyContent="space-between" alignItems="center">
                <Text fontSize={20}>{t("customEditor.theme.title")}</Text>
                <Button onClick={onClose}>
                    <Text fontSize={14}>{t("customEditor.theme.done")}</Text>
                </Button>
            </flex>

            <flex horizontal gap={20} alignItems="start">
                <flex gap={10} flexGrow={1}>
                    <OptionGroup label={t("customEditor.theme.sizeSettings")}>
                        <NumberBox
                            label={t("customEditor.theme.radius")}
                            value={localTheme?.radius ?? 6.0}
                            onChange={(v) => updateValue('radius', v)}
                            min={0}
                            max={20}
                            step={0.5}
                        />
                        <NumberBox
                            label={t("customEditor.theme.itemHeight")}
                            value={localTheme?.item_height ?? 23.0}
                            onChange={(v) => updateValue('item_height', v)}
                            min={15}
                            max={40}
                            step={1}
                        />
                        <NumberBox
                            label={t("customEditor.theme.itemGap")}
                            value={localTheme?.item_gap ?? 3.0}
                            onChange={(v) => updateValue('item_gap', v)}
                            min={0}
                            max={10}
                            step={0.5}
                        />
                        <NumberBox
                            label={t("customEditor.theme.itemRadius")}
                            value={localTheme?.item_radius ?? 5.0}
                            onChange={(v) => updateValue('item_radius', v)}
                            min={0}
                            max={20}
                            step={0.5}
                        />
                        <NumberBox
                            label={t("customEditor.theme.margin")}
                            value={localTheme?.margin ?? 5.0}
                            onChange={(v) => updateValue('margin', v)}
                            min={0}
                            max={20}
                            step={0.5}
                        />
                        <NumberBox
                            label={t("customEditor.theme.padding")}
                            value={localTheme?.padding ?? 6.0}
                            onChange={(v) => updateValue('padding', v)}
                            min={0}
                            max={20}
                            step={0.5}
                        />
                    </OptionGroup>
                </flex>

                <flex gap={10} flexGrow={1}>
                    <OptionGroup label={t("customEditor.theme.textAndIcon")}>
                        <NumberBox
                            label={t("customEditor.theme.fontSize")}
                            value={localTheme?.font_size ?? 14.0}
                            onChange={(v) => updateValue('font_size', v)}
                            min={10}
                            max={24}
                            step={1}
                        />
                        <NumberBox
                            label={t("customEditor.theme.textPadding")}
                            value={localTheme?.text_padding ?? 8.0}
                            onChange={(v) => updateValue('text_padding', v)}
                            min={0}
                            max={20}
                            step={0.5}
                        />
                        <NumberBox
                            label={t("customEditor.theme.iconPadding")}
                            value={localTheme?.icon_padding ?? 4.0}
                            onChange={(v) => updateValue('icon_padding', v)}
                            min={0}
                            max={20}
                            step={0.5}
                        />
                        <NumberBox
                            label={t("customEditor.theme.rightIconPadding")}
                            value={localTheme?.right_icon_padding ?? 20.0}
                            onChange={(v) => updateValue('right_icon_padding', v)}
                            min={0}
                            max={40}
                            step={1}
                        />
                    </OptionGroup>

                    <OptionGroup label={t("customEditor.theme.effects")}>
                        <Toggle
                            label={t("customEditor.theme.useDwm")}
                            value={localTheme?.use_dwm_if_available ?? true}
                            onChange={(v) => updateValue('use_dwm_if_available', v)}
                        />
                        <Toggle
                            label={t("customEditor.theme.acrylic")}
                            value={localTheme?.acrylic ?? true}
                            onChange={(v) => updateValue('acrylic', v)}
                        />
                        <NumberBox
                            label={t("customEditor.theme.backgroundOpacity")}
                            value={localTheme?.background_opacity ?? 1.0}
                            onChange={(v) => updateValue('background_opacity', v)}
                            min={0}
                            max={1}
                            step={0.05}
                        />
                    </OptionGroup>
                </flex>
            </flex>
        </flex>
    );
});
