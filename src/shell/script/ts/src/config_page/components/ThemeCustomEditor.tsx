import * as shell from "mshell";
import { Button, Text, NumberBox, Toggle } from "../components";
import React, { memo } from "react";
import { breeze } from "mshell";
import { useTranslation } from "../hooks";

const OptionGroup = ({ label, children }: { label: string; children: React.ReactNode }) => {
    const isLightTheme = breeze.is_light_theme();
    return (
        <flex
            gap={10}
            backgroundColor={isLightTheme ? '#ffffff50' : '#2a2a2a50'}
            padding={16}
            borderRadius={14}
            alignItems="stretch"
        >
            <Text fontSize={16} opacity={0.8}>{label}</Text>
            <flex gap={8} paddingLeft={10} paddingTop={4}>
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
    const { t } = useTranslation();

    const updateValue = (key: string, value: any) => {
        const newTheme = { ...theme, [key]: value };
        onUpdate(newTheme);
    };

    const roundValue = (value: number, resolution: number): number => {
        const factor = 1 / resolution;
        return Math.round((value + Number.EPSILON * factor) * factor) / factor;
    };

    return (
        <flex
            gap={20}
            alignItems="stretch"
            width={700}
            enableScrolling
            maxHeight={600}
            paddingLeft={25}
            paddingRight={25}
            paddingTop={5}
            paddingBottom={20}
        >
            <flex horizontal justifyContent="space-between" alignItems="center">
                <Text fontSize={22}>{t("customEditor.theme.title")}</Text>
                <Button onClick={onClose}>
                    <Text fontSize={14}>{t("customEditor.theme.done")}</Text>
                </Button>
            </flex>

            <flex gap={15} alignItems="stretch">
                <OptionGroup label={t("customEditor.theme.sizeSettings")}>
                    <flex horizontal gap={10} alignItems="center">
                        <flex gap={10} alignItems="stretch">
                            <NumberBox
                                label={t("customEditor.theme.radius")}
                                value={theme?.radius ?? 6.0}
                                onChange={(v) => updateValue('radius', roundValue(v, 0.5))}
                                min={0}
                                max={20}
                                step={0.5}
                            />
                            <NumberBox
                                label={t("customEditor.theme.itemHeight")}
                                value={theme?.item_height ?? 23.0}
                                onChange={(v) => updateValue('item_height', Math.round(v))}
                                min={15}
                                max={40}
                                step={1}
                            />
                            <NumberBox
                                label={t("customEditor.theme.itemGap")}
                                value={theme?.item_gap ?? 3.0}
                                onChange={(v) => updateValue('item_gap', roundValue(v, 0.5))}
                                min={0}
                                max={10}
                                step={0.5}
                            />
                        </flex>
                        <flex gap={10} alignItems="stretch">
                            <NumberBox
                                label={t("customEditor.theme.itemRadius")}
                                value={theme?.item_radius ?? 5.0}
                                onChange={(v) => updateValue('item_radius', roundValue(v, 0.5))}
                                min={0}
                                max={20}
                                step={0.5}
                            />
                            <NumberBox
                                label={t("customEditor.theme.margin")}
                                value={theme?.margin ?? 5.0}
                                onChange={(v) => updateValue('margin', roundValue(v, 0.5))}
                                min={0}
                                max={20}
                                step={0.5}
                            />
                            <NumberBox
                                label={t("customEditor.theme.padding")}
                                value={theme?.padding ?? 6.0}
                                onChange={(v) => updateValue('padding', roundValue(v, 0.5))}
                                min={0}
                                max={20}
                                step={0.5}
                            />
                        </flex>
                    </flex>
                </OptionGroup>

                <OptionGroup label={t("customEditor.theme.textAndIcon")}>
                    <flex horizontal gap={10}>
                        <flex gap={10} alignItems="stretch">
                            <NumberBox
                                label={t("customEditor.theme.fontSize")}
                                value={theme?.font_size ?? 14.0}
                                onChange={(v) => updateValue('font_size', Math.round(v))}
                                min={10}
                                max={24}
                                step={1}
                            />
                            <NumberBox
                                label={t("customEditor.theme.textPadding")}
                                value={theme?.text_padding ?? 8.0}
                                onChange={(v) => updateValue('text_padding', roundValue(v, 0.5))}
                                min={0}
                                max={20}
                                step={0.5}
                            />
                        </flex>
                        <flex gap={10} alignItems="stretch">
                            <NumberBox
                                label={t("customEditor.theme.iconPadding")}
                                value={theme?.icon_padding ?? 4.0}
                                onChange={(v) => updateValue('icon_padding', roundValue(v, 0.5))}
                                min={0}
                                max={20}
                                step={0.5}
                            />
                            <NumberBox
                                label={t("customEditor.theme.rightIconPadding")}
                                value={theme?.right_icon_padding ?? 20.0}
                                onChange={(v) => updateValue('right_icon_padding', Math.round(v))}
                                min={0}
                                max={40}
                                step={1}
                            />
                        </flex>
                    </flex>
                </OptionGroup>

                <OptionGroup label={t("customEditor.theme.effects")}>
                    <Toggle
                        label={t("customEditor.theme.acrylic")}
                        value={theme?.acrylic ?? true}
                        onChange={(v) => updateValue('acrylic', v)}
                    />
                    <NumberBox
                        label={t("customEditor.theme.backgroundOpacity")}
                        value={theme?.background_opacity ?? 1.0}
                        onChange={(v) => updateValue('background_opacity', roundValue(v, 0.05))}
                        min={0}
                        max={1}
                        step={0.05}
                    />
                </OptionGroup>
            </flex>
        </flex>
    );
});
