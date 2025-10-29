import { breeze } from "mshell";
import { showMenu, useHoverActive } from "./utils";
import { type ReactNode } from "react";
import { ICON_MORE_VERT } from "./constants";

import { useState, useEffect, memo, createContext, useContext, useMemo } from "react";

// Icon element creator
export const iconElement = (svg: string, width = 14) => (
    <img 
        svg={svg.replace(
            '<svg ',
            `<svg fill="${breeze.is_light_theme() ? '#000000ff' : '#ffffffff'}" `
        )} 
        width={width} 
        height={width} 
    />
);

// Simple Markdown Renderer
export const SimpleMarkdownRender = ({ text, maxWidth }: { text: string, maxWidth: number }) => {
    return (
        <>
            {text.split('\n').map((line, index) => (
                line.trim().startsWith('# ') ? <Text key={index} fontSize={22} maxWidth={maxWidth}>{line.trim().substring(2).trim()}</Text> :
                    line.trim().startsWith('## ') ? <Text key={index} fontSize={20} maxWidth={maxWidth}>{line.trim().substring(3).trim()}</Text> :
                        line.trim().startsWith('### ') ? <Text key={index} fontSize={18} maxWidth={maxWidth}>{line.trim().substring(4).trim()}</Text> :
                            line.trim().startsWith('#### ') ? <Text key={index} fontSize={16} maxWidth={maxWidth}>{line.trim().substring(5).trim()}</Text> :
                                <Text key={index} fontSize={14} maxWidth={maxWidth}>{line}</Text>
            ))}
        </>
    );
};

export const Button = memo(({
    onClick,
    children,
    selected
}: {
    onClick: () => void;
    children: ReactNode;
    selected?: boolean;
}) => {
    const isLightTheme = breeze.is_light_theme()
    const { isHovered, isActive, onMouseEnter, onMouseLeave, onMouseDown, onMouseUp } = useHoverActive();
    return (
        <flex
            onClick={onClick}
            backgroundColor={
                isActive ? (isLightTheme ? '#c0c0c0cc' : '#505050cc') :
                    isHovered ? (isLightTheme ? '#e0e0e0cc' : '#606060cc') :
                        (isLightTheme ? '#f0f0f0cc' : '#404040cc')
            }
            borderRadius={8}
            paddingLeft={12}
            paddingRight={12}
            paddingTop={8}
            paddingBottom={8}
            autoSize={true}
            justifyContent="center"
            alignItems="center"
            horizontal
            gap={6}
            borderWidth={selected ? 2 : 0}
            borderColor="#2979FF"
            onMouseEnter={onMouseEnter}
            onMouseLeave={onMouseLeave}
            onMouseDown={onMouseDown}
            onMouseUp={onMouseUp}
            animatedVars={['.r', '.g', '.b', '.a']}
        >
            {children}
        </flex>
    );
});

export const Text = memo(({
    children,
    fontSize = 14,
    maxWidth = -1
}: {
    children: string;
    fontSize?: number;
    maxWidth?: number;
}) => {
    const isLightTheme = breeze.is_light_theme();
    return (
        <text
            text={children}
            fontSize={fontSize}
            maxWidth={maxWidth}
            color={isLightTheme ? '#000000ff' : '#ffffffff'}
        />
    );
});

export const TextButton = memo(({
    onClick,
    children,
    icon
}: {
    onClick: () => void;
    children: string;
    icon?: string;
}) => {
    return (
        <Button onClick={onClick}>
            {
                icon ? iconElement(icon, 14) : null
            }
            <Text fontSize={14}>{children}</Text>
        </Button>
    );
});

export const Toggle = ({
    label,
    value,
    onChange
}: {
    label: string;
    value: boolean;
    onChange: (v: boolean) => void;
}) => {
    const isLightTheme = breeze.is_light_theme();
    const {
        isHovered, isActive, onMouseEnter, onMouseLeave, onMouseDown, onMouseUp
    } = useHoverActive();

    return (
        <flex horizontal alignItems="center" gap={10} justifyContent="space-between">
            <Text>{label}</Text>
            <flex
                width={40}
                height={20}
                borderRadius={10}
                backgroundColor={value ? '#0078D4' :
                    isHovered ?
                        (isLightTheme ? '#CCCCCCAA' : '#555555AA') :
                        (isLightTheme ? '#CCCCCC77' : '#55555577')}
                justifyContent={value ? 'end' : 'start'}
                horizontal
                alignItems="center"
                onClick={() => onChange(!value)}
                autoSize={false}
                onMouseEnter={onMouseEnter}
                padding={
                    (isHovered || isActive) ? 2 : 3
                }
                onMouseLeave={onMouseLeave}
                onMouseDown={onMouseDown}
                onMouseUp={onMouseUp}
                animatedVars={['.r', '.g', '.b', '.a']}
                borderWidth={0.5}
                borderColor={value ? '#00000000' : (isLightTheme ? '#5A5A5A5' : '#CECDD0')}
            >
                <flex
                    width={isActive ? 19 : isHovered ? 16 : 14}
                    height={(isHovered || isActive) ? 16 : 14}
                    borderRadius={8}
                    backgroundColor={value ? (isLightTheme ? '#FFFFFF' : '#000000') : (isLightTheme ? '#5A5A5A' : '#CECDD0')}
                    animatedVars={['x', 'width', 'height']}
                    autoSize={false}
                />
            </flex>
        </flex>
    );
}

export const SidebarItem = memo(({
    onClick,
    icon,
    isActive,
    children
}: {
    onClick: () => void;
    icon: string;
    isActive: boolean;
    children: string;
}) => {
    const isLightTheme = breeze.is_light_theme();
    const { isHovered, isActive: isPressed, onMouseEnter, onMouseLeave, onMouseDown, onMouseUp } = useHoverActive();
    return (
        <flex
            onClick={onClick}
            backgroundColor={
                isActive ? (isLightTheme ? '#c0c0c077' : '#50505077') :
                    isPressed ? (isLightTheme ? '#c0c0c0cc' : '#505050cc') :
                        isHovered ? (isLightTheme ? '#e0e0e0cc' : '#606060cc') :
                            (isLightTheme ? '#e0e0e000' : '#60606000')
            }
            paddingLeft={0}
            paddingRight={12}
            paddingTop={8}
            paddingBottom={8}
            autoSize={false}
            height={32}
            justifyContent="start"
            alignItems="center"
            horizontal
            gap={6}
            borderRadius={6}
            onMouseEnter={onMouseEnter}
            onMouseLeave={onMouseLeave}
            onMouseDown={onMouseDown}
            onMouseUp={onMouseUp}
            animatedVars={['.r', '.g', '.b', '.a']}
        >
            <flex width={3} height={isActive ? 15 : 0} backgroundColor={isActive ? '#2979FF' : '#00000000'}
                borderRadius={3} autoSize={false} animatedVars={['.a', 'height']} />
            {iconElement(icon, 14)}
            <Text fontSize={14}>{children}</Text>
        </flex>
    );
});

export const PluginCheckbox = memo(({ isEnabled, onToggle }: { isEnabled: boolean; onToggle: () => void }) => {
    const isLightTheme = breeze.is_light_theme();
    const { isHovered, isActive, onMouseEnter, onMouseLeave, onMouseDown, onMouseUp } = useHoverActive();
    return (
        <flex
            width={20}
            height={20}
            borderRadius={4}
            borderWidth={1}
            borderColor={isLightTheme ? '#CCCCCC' : '#555555'}
            backgroundColor={isEnabled ? (isActive ? '#1E5F99' : isHovered ? '#3F7FBF' : '#2979FF') :
                (isActive ? (isLightTheme ? '#c0c0c0cc' : '#505050cc') :
                    isHovered ? (isLightTheme ? '#e0e0e0cc' : '#606060cc') :
                        (isLightTheme ? '#e0e0e066' : '#60606066'))}
            justifyContent="center"
            alignItems="center"
            onClick={onToggle}
            onMouseEnter={onMouseEnter}
            onMouseLeave={onMouseLeave}
            onMouseDown={onMouseDown}
            onMouseUp={onMouseUp}
            animatedVars={['.r', '.g', '.b', '.a']}
        >
            {isEnabled ? (
                <img
                    svg={`<svg viewBox="0 0 24 24"><path fill="${isLightTheme ? '#000000' : '#FFFFFF'}" d="M19 6.41L17.59 5 12 10.59 6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 12 13.41 17.59 19 19 17.59 13.41 12z"/></svg>`}
                    width={14}
                    height={14}
                />
            ) : (
                <flex width={14} height={14} autoSize={false} />
            )}
        </flex>
    );
});

export const PluginMoreButton = memo(({ onClick }: { onClick: () => void }) => {
    const isLightTheme = breeze.is_light_theme();
    const { isHovered, isActive, onMouseEnter, onMouseLeave, onMouseDown, onMouseUp } = useHoverActive();
    return (
        <flex
            width={32}
            height={32}
            borderRadius={16}
            justifyContent="center"
            alignItems="center"
            backgroundColor={isActive ? (isLightTheme ? '#c0c0c0cc' : '#505050cc') :
                isHovered ? (isLightTheme ? '#e0e0e0cc' : '#606060cc') :
                    '#00000000'}
            onClick={onClick}
            onMouseEnter={onMouseEnter}
            onMouseLeave={onMouseLeave}
            onMouseDown={onMouseDown}
            onMouseUp={onMouseUp}
            animatedVars={['.r', '.g', '.b', '.a']}
        >
            {iconElement(ICON_MORE_VERT, 16)}
        </flex>
    );
});

export const PluginItem = memo(({
    name,
    isEnabled,
    isPrioritized,
    onToggle,
    onMoreClick
}: {
    name: string;
    isEnabled: boolean;
    isPrioritized: boolean;
    onToggle: () => void;
    onMoreClick: (name: string) => void;
}) => {
    const isLightTheme = breeze.is_light_theme();
    return (
        <flex
            horizontal
            alignItems="center"
            gap={12}
            padding={12}
            borderRadius={8}
        >
            <flex
                width={8}
                height={8}
                borderRadius={4}
                backgroundColor={isEnabled ? '#4CAF50' : '#9E9E9E'}
                autoSize={false}
            />
            <flex flexGrow={1}>
                <Text fontSize={14}>
                    {name}
                </Text>
                {isPrioritized && (
                    <flex padding={4}>
                        <Text fontSize={10}>优先加载</Text>
                    </flex>
                )}
            </flex>

            <spacer />
            <PluginCheckbox isEnabled={isEnabled} onToggle={onToggle} />
            <PluginMoreButton onClick={() => onMoreClick(name)} />
        </flex>
    );
});