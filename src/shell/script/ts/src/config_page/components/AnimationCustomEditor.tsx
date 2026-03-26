import * as shell from "mshell";
import { Button, Text, NumberBox, Select, iconElement } from "../components";
import React, { memo, useCallback, useMemo, useState } from "react";
import { breeze } from "mshell";
import { useTranslation } from "../hooks";
import { ICON_EXPAND_LESS, ICON_EXPAND_MORE } from "../constants";

interface AnimatedFloatConf {
    duration?: number;
    easing?: string;
    delay_scale?: number;
}

interface AnimationConfig {
    item?: {
        opacity?: AnimatedFloatConf;
        x?: AnimatedFloatConf;
        y?: AnimatedFloatConf;
        width?: AnimatedFloatConf;
        blur?: AnimatedFloatConf;
    };
    main_bg?: {
        opacity?: AnimatedFloatConf;
        x?: AnimatedFloatConf;
        y?: AnimatedFloatConf;
        w?: AnimatedFloatConf;
        h?: AnimatedFloatConf;
    };
    submenu_bg?: {
        opacity?: AnimatedFloatConf;
        x?: AnimatedFloatConf;
        y?: AnimatedFloatConf;
        w?: AnimatedFloatConf;
        h?: AnimatedFloatConf;
    };
}

const AnimatedPropertyEditor = memo(({
    label,
    value,
    onChange,
    easingOptions
}: {
    label: string;
    value: AnimatedFloatConf;
    onChange: (value: AnimatedFloatConf) => void;
    easingOptions: Array<{ value: string; label: string }>;
}) => {
    const { t } = useTranslation();
    const isLightTheme = breeze.is_light_theme();

    return (
        <flex
            gap={8}
            backgroundColor={isLightTheme ? '#ffffff30' : '#1a1a1a30'}
            padding={12}
            borderRadius={10}
        >
            <Text fontSize={13} opacity={0.8}>{label}</Text>
            <flex horizontal gap={10}>
                <NumberBox
                    label={t("customEditor.animation.duration")}
                    value={value.duration ?? 200}
                    onChange={(v) => onChange({ ...value, duration: Math.round(v) })}
                    min={0}
                    max={2000}
                    step={50}
                />
                <NumberBox
                    label={t("customEditor.animation.delayScale")}
                    value={value.delay_scale ?? 1.0}
                    onChange={(v) => onChange({ ...value, delay_scale: Math.round(v * 10) / 10 })}
                    min={0}
                    max={5}
                    step={0.1}
                />
            </flex>

            <Select
                label={t("customEditor.animation.easing")}
                value={value.easing ?? 'ease_in_out'}
                options={easingOptions}
                onChange={(v) => onChange({ ...value, easing: v })}
            />
        </flex>
    );
});

const PropertyGroupEditor = memo(({
    title,
    groupKey,
    properties,
    groupData,
    onUpdate,
    easingOptions
}: {
    title: string;
    groupKey: string;
    properties: Array<{ key: string; label: string }>;
    groupData: any;
    onUpdate: (groupKey: string, propKey: string, value: AnimatedFloatConf) => void;
    easingOptions: Array<{ value: string; label: string }>;
}) => {
    const isLightTheme = breeze.is_light_theme();
    const [folded, setFolded] = useState(true);
    const getPropertyValue = (propKey: string): AnimatedFloatConf => {
        return groupData?.[propKey] ?? {};
    };

    return (
        <flex
            backgroundColor={isLightTheme ? '#ffffff50' : '#2a2a2a50'}
            padding={16}
            borderRadius={14}
            animatedVars={['height', 'width']}
            alignItems="stretch"
        >
            <flex autoSize={false} width={480} height={0} />
            <flex onClick={() => setFolded(!folded)} horizontal justifyContent="space-between">
                <Text fontSize={16}>{title}</Text>
                {folded && iconElement(ICON_EXPAND_MORE, 16)}
            </flex>
            {
                !folded && <flex gap={8} paddingLeft={10} paddingTop={10}>
                    {properties.map(({ key, label }) => (
                        <AnimatedPropertyEditor
                            key={key}
                            label={label}
                            value={getPropertyValue(key)}
                            onChange={(value) => onUpdate(groupKey, key, value)}
                            easingOptions={easingOptions}
                        />
                    ))}
                </flex>
            }
        </flex>
    );
});

export const AnimationCustomEditor = memo(({
    animation,
    onUpdate,
    onClose
}: {
    animation: AnimationConfig;
    onUpdate: (animation: AnimationConfig) => void;
    onClose: () => void;
}) => {
    const { t } = useTranslation();
    const safeAnimation = useMemo<AnimationConfig>(() => {
        return animation && typeof animation === "object" ? animation : {};
    }, [animation]);

    const handleUpdate = useCallback((groupKey: string, propKey: string, value: AnimatedFloatConf) => {
        try {
            const newAnim = { ...safeAnimation } as AnimationConfig & Record<string, any>;
            newAnim[groupKey] = { ...((safeAnimation as Record<string, any>)?.[groupKey] || {}), [propKey]: value };
            onUpdate(newAnim);
        } catch (e) {
            shell.println("[Config] Failed to update animation config:", e);
        }
    }, [safeAnimation, onUpdate]);

    const easingOptions = useMemo(() => [
        { value: "mutation", label: t("preset.none") },
        { value: "linear", label: t("customEditor.animation.linear") },
        { value: "ease_in", label: t("customEditor.animation.easeIn") },
        { value: "ease_out", label: t("customEditor.animation.easeOut") },
        { value: "ease_in_out", label: t("customEditor.animation.easeInOut") }
    ], [t]);

    const animationGroups = useMemo(() => [
        {
            title: t("customEditor.animation.menuItem"),
            groupKey: "item",
            properties: [
                { key: "opacity", label: t("customEditor.animation.opacity") },
                { key: "x", label: t("customEditor.animation.x") },
                { key: "y", label: t("customEditor.animation.y") },
                { key: "width", label: t("customEditor.animation.width") },
                { key: "blur", label: t("customEditor.animation.blur") }
            ]
        },
        {
            title: t("customEditor.animation.mainBg"),
            groupKey: "main_bg",
            properties: [
                { key: "opacity", label: t("customEditor.animation.opacity") },
                { key: "x", label: t("customEditor.animation.x") },
                { key: "y", label: t("customEditor.animation.y") },
                { key: "w", label: t("customEditor.animation.width") },
                { key: "h", label: t("customEditor.animation.height") }
            ]
        },
        {
            title: t("customEditor.animation.submenuBg"),
            groupKey: "submenu_bg",
            properties: [
                { key: "opacity", label: t("customEditor.animation.opacity") },
                { key: "x", label: t("customEditor.animation.x") },
                { key: "y", label: t("customEditor.animation.y") },
                { key: "w", label: t("customEditor.animation.width") },
                { key: "h", label: t("customEditor.animation.height") }
            ]
        }
    ], [t]);

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
                <Text fontSize={22}>{t("customEditor.animation.title")}</Text>
                <Button onClick={onClose}>
                    <Text fontSize={14}>{t("customEditor.animation.done")}</Text>
                </Button>
            </flex>

            <flex gap={15}>
                {animationGroups.map((group) => (
                    <PropertyGroupEditor
                        key={group.groupKey}
                        title={group.title}
                        groupKey={group.groupKey}
                        properties={group.properties}
                        groupData={(safeAnimation as Record<string, any>)?.[group.groupKey]}
                        onUpdate={handleUpdate}
                        easingOptions={easingOptions}
                    />
                ))}
            </flex>
        </flex>
    );
});
