import * as shell from "mshell";
import { Button, Text, NumberBox, Select } from "../components";
import React, { memo } from "react";
import { breeze } from "mshell";
import { useTranslation } from "../hooks";


const OptionGroup = ({ label, children }: { label: string; children: React.ReactNode }) => {
    return (
        <flex gap={6} backgroundColor={breeze.is_light_theme() ? '#ffffff40' : '#2a2a2a40'} padding={16} borderRadius={14}>
            <Text fontSize={15}>{label}</Text>
            <flex gap={5} paddingLeft={20}>
                {children}
            </flex>
        </flex>
    );
}

export const AnimationCustomEditor = memo(({
    animation,
    onUpdate,
    onClose
}: {
    animation: any;
    onUpdate: (animation: any) => void;
    onClose: () => void;
}) => {
    const isLightTheme = breeze.is_light_theme();
    const { t } = useTranslation();

    const easingOptions = [
        { value: "mutation", label: t("preset.none") },
        { value: "linear", label: t("customEditor.animation.linear") },
        { value: "ease_in", label: t("customEditor.animation.easeIn") },
        { value: "ease_out", label: t("customEditor.animation.easeOut") },
        { value: "ease_in_out", label: t("customEditor.animation.easeInOut") }
    ];

    const updateItemAnim = (prop: string, key: string, value: any) => {
        const newAnim = { ...animation };
        if (!newAnim.item) newAnim.item = {};
        if (!newAnim.item[prop]) newAnim.item[prop] = {};
        newAnim.item[prop][key] = value;
        onUpdate(newAnim);
    };

    const updateBgAnim = (bgType: string, prop: string, key: string, value: any) => {
        const newAnim = { ...animation };
        if (!newAnim[bgType]) newAnim[bgType] = {};
        if (!newAnim[bgType][prop]) newAnim[bgType][prop] = {};
        newAnim[bgType][prop][key] = value;
        onUpdate(newAnim);
    };

    const getItemAnimValue = (prop: string, key: string, defaultValue: any) => {
        return animation?.item?.[prop]?.[key] ?? defaultValue;
    };

    const getBgAnimValue = (bgType: string, prop: string, key: string, defaultValue: any) => {
        return animation?.[bgType]?.[prop]?.[key] ?? defaultValue;
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
                <Text fontSize={20}>{t("customEditor.animation.title")}</Text>
                <Button onClick={onClose}>
                    <Text fontSize={14}>{t("customEditor.animation.done")}</Text>
                </Button>
            </flex>

            <flex gap={10}>
                <OptionGroup label={t("customEditor.animation.itemOpacity")}>
                    <NumberBox
                        label={t("customEditor.animation.duration")}
                        value={getItemAnimValue('opacity', 'duration', 200)}
                        onChange={(v) => updateItemAnim('opacity', 'duration', v)}
                        min={0}
                        max={1000}
                        step={50}
                    />
                    <NumberBox
                        label={t("customEditor.animation.delayScale")}
                        value={getItemAnimValue('opacity', 'delay_scale', 1.0)}
                        onChange={(v) => updateItemAnim('opacity', 'delay_scale', v)}
                        min={0}
                        max={5}
                        step={0.1}
                    />
                    <Select
                        label={t("customEditor.animation.easing")}
                        value={getItemAnimValue('opacity', 'easing', 'ease_in_out')}
                        options={easingOptions}
                        onChange={(v) => updateItemAnim('opacity', 'easing', v)}
                    />
                </OptionGroup>


                <OptionGroup label={t("customEditor.animation.itemX")}>
                    <NumberBox
                        label={t("customEditor.animation.duration")}
                        value={getItemAnimValue('x', 'duration', 200)}
                        onChange={(v) => updateItemAnim('x', 'duration', v)}
                        min={0}
                        max={1000}
                        step={50}
                    />
                    <Select
                        label={t("customEditor.animation.easing")}
                        value={getItemAnimValue('x', 'easing', 'ease_in_out')}
                        options={easingOptions}
                        onChange={(v) => updateItemAnim('x', 'easing', v)}
                    />
                </OptionGroup>

                <OptionGroup label={t("customEditor.animation.mainBg")}>
                    <NumberBox
                        label={t("customEditor.animation.opacityDuration")}
                        value={getBgAnimValue('main_bg', 'opacity', 'duration', 200)}
                        onChange={(v) => updateBgAnim('main_bg', 'opacity', 'duration', v)}
                        min={0}
                        max={1000}
                        step={50}
                    />
                    <Select
                        label={t("customEditor.animation.easing")}
                        value={getBgAnimValue('main_bg', 'opacity', 'easing', 'ease_in_out')}
                        options={easingOptions}
                        onChange={(v) => updateBgAnim('main_bg', 'opacity', 'easing', v)}
                    />
                </OptionGroup>


                <OptionGroup label={t("customEditor.animation.submenuBg")}>
                    <NumberBox
                        label={t("customEditor.animation.opacityDuration")}
                        value={getBgAnimValue('submenu_bg', 'opacity', 'duration', 200)}
                        onChange={(v) => updateBgAnim('submenu_bg', 'opacity', 'duration', v)}
                        min={0}
                        max={1000}
                        step={50}
                    />
                    <Select
                        label={t("customEditor.animation.easing")}
                        value={getBgAnimValue('submenu_bg', 'opacity', 'easing', 'ease_in_out')}
                        options={easingOptions}
                        onChange={(v) => updateBgAnim('submenu_bg', 'opacity', 'easing', v)}
                    />
                </OptionGroup>
            </flex>
        </flex>
    );
});
