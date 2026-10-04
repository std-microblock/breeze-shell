import * as shell from "mshell";
import { memo, useEffect, useState, type ReactNode } from "react";
import { Text } from "./Text";
import { iconElement } from "./Icon";
import { useHoverActive } from "../hooks";

export const fluent = () => {
    const light = shell.breeze.is_light_theme();
    return {
        light,
        accent: light ? "#005FB8" : "#60CDFF",
        accentText: light ? "#FFFFFF" : "#000000",
        text: light ? "#1A1A1AFF" : "#FFFFFFFF",
        textSecondary: light ? "#5C5C5CFF" : "#C5C5C5FF",
        textTertiary: light ? "#8A8A8AFF" : "#9A9A9AFF",
        card: light ? "#FFFFFFB3" : "#FFFFFF0D",
        cardHover: light ? "#F9F9F9D9" : "#FFFFFF15",
        cardPressed: light ? "#F0F0F0D9" : "#FFFFFF08",
        cardStroke: light ? "#0000000F" : "#FFFFFF12",
        subtle: light ? "#0000000A" : "#FFFFFF0F",
        subtleHover: light ? "#00000012" : "#FFFFFF17",
        divider: light ? "#00000014" : "#FFFFFF15",
        codeBg: light ? "#0000000A" : "#00000040",
        transparent: light ? "#FFFFFF00" : "#00000000",
    };
};

export const severityStyle = (level: string) => {
    const light = shell.breeze.is_light_theme();
    switch (level) {
        case "critical":
            return { fg: light ? "#C42B1C" : "#FF99A4", bg: light ? "#FDE7E9E6" : "#442726E6", label: "CRIT" };
        case "error":
            return { fg: light ? "#C42B1C" : "#FF99A4", bg: light ? "#FDE7E9E6" : "#442726E6", label: "ERROR" };
        case "warn":
        case "warning":
            return { fg: light ? "#9D5D00" : "#FCE100", bg: light ? "#FFF4CEE6" : "#433519E6", label: "WARN" };
        case "info":
            return { fg: light ? "#005FB8" : "#60CDFF", bg: light ? "#E5F1FBE6" : "#1F3346E6", label: "INFO" };
        case "debug":
            return { fg: light ? "#5C5C5C" : "#C5C5C5", bg: light ? "#0000000D" : "#FFFFFF12", label: "DEBUG" };
        default:
            return { fg: light ? "#8A8A8A" : "#9A9A9A", bg: light ? "#00000008" : "#FFFFFF0A", label: level.toUpperCase() };
    }
};

export const useEntrance = (delay = 0) => {
    const [shown, setShown] = useState(false);
    useEffect(() => {
        const id = setTimeout(() => setShown(true), 16 + delay);
        return () => clearTimeout(id);
    }, []);
    return shown;
};

const ENTRANCE_VARS = ["opacity", "padding_top", "padding_bottom"];

export const Entrance = ({ delay = 0, offset = 12, padding = 0, children, ...rest }: {
    delay?: number;
    offset?: number;
    padding?: number;
    children?: ReactNode;
    [key: string]: any;
}) => {
    const shown = useEntrance(delay);
    return (
        <flex
            opacity={shown ? 255 : 0}
            paddingLeft={padding}
            paddingRight={padding}
            paddingTop={padding + (shown ? 0 : offset)}
            paddingBottom={padding + (shown ? offset : 0)}
            animatedVars={ENTRANCE_VARS}
            animationCurve={{ duration: 260, easing: "ease_out", vars: ENTRANCE_VARS }}
            alignItems="stretch"
            {...rest}
        >
            {children}
        </flex>
    );
};

export const Card = memo(({ children, onClick, padding = 14, gap = 8, horizontal, alignItems = "stretch", accentBar }: {
    children?: ReactNode;
    onClick?: () => void;
    padding?: number;
    gap?: number;
    horizontal?: boolean;
    alignItems?: "start" | "center" | "end" | "stretch";
    accentBar?: string;
}) => {
    const c = fluent();
    const hover = useHoverActive();
    const interactive = !!onClick;
    return (
        <flex
            horizontal
            alignItems="stretch"
            backgroundColor={interactive && hover.isActive ? c.cardPressed : interactive && hover.isHovered ? c.cardHover : c.card}
            borderColor={c.cardStroke}
            borderWidth={1}
            borderRadius={8}
            onClick={onClick}
            onMouseEnter={hover.onMouseEnter}
            onMouseLeave={hover.onMouseLeave}
            onMouseDown={hover.onMouseDown}
            onMouseUp={hover.onMouseUp}
            animatedVars={[".r", ".g", ".b", ".a"]}
        >
            {accentBar && (
                <flex width={3} autoSize={false} borderRadius={2} backgroundColor={accentBar} />
            )}
            <flex
                horizontal={horizontal}
                padding={padding}
                gap={gap}
                alignItems={alignItems}
                flexGrow={1}
            >
                {children}
            </flex>
        </flex>
    );
});

export const Pill = memo(({ text, fg, bg, fontSize = 11 }: { text: string; fg: string; bg: string; fontSize?: number }) => (
    <flex
        backgroundColor={bg}
        borderRadius={4}
        paddingLeft={6}
        paddingRight={6}
        paddingTop={2}
        paddingBottom={2}
        alignItems="center"
        justifyContent="center"
    >
        <Text fontSize={fontSize} fontWeight={600} color={fg}>{text}</Text>
    </flex>
));

const Segment = ({ label, selected, onClick, badge }: {
    label: string;
    selected: boolean;
    onClick: () => void;
    badge?: number;
}) => {
    const c = fluent();
    const hover = useHoverActive();
    return (
        <flex
            horizontal
            alignItems="center"
            gap={6}
            paddingLeft={12}
            paddingRight={12}
            paddingTop={5}
            paddingBottom={5}
            borderRadius={5}
            onClick={onClick}
            backgroundColor={selected ? c.accent : hover.isHovered ? c.subtleHover : c.transparent}
            onMouseEnter={hover.onMouseEnter}
            onMouseLeave={hover.onMouseLeave}
            onMouseDown={hover.onMouseDown}
            onMouseUp={hover.onMouseUp}
            animatedVars={[".r", ".g", ".b", ".a"]}
        >
            <Text fontSize={12} color={selected ? c.accentText : c.text}>{label}</Text>
            {badge !== undefined && (
                <Text fontSize={11} color={selected ? c.accentText : c.textTertiary}>{String(badge)}</Text>
            )}
        </flex>
    );
};

export const SegmentedControl = <T extends string>({ options, value, onChange }: {
    options: { value: T; label: string; badge?: number }[];
    value: T;
    onChange: (v: T) => void;
}) => {
    const c = fluent();
    return (
        <flex horizontal gap={2} padding={3} borderRadius={7} backgroundColor={c.subtle} borderColor={c.cardStroke} borderWidth={1}>
            {options.map(o => (
                <Segment key={o.value} label={o.label} badge={o.badge} selected={o.value === value} onClick={() => onChange(o.value)} />
            ))}
        </flex>
    );
};

export const ChipToggle = ({ label, selected, onClick, color }: {
    label: string;
    selected: boolean;
    onClick: () => void;
    color: { fg: string; bg: string };
}) => {
    const c = fluent();
    const hover = useHoverActive();
    return (
        <flex
            horizontal
            alignItems="center"
            gap={6}
            paddingLeft={10}
            paddingRight={10}
            paddingTop={4}
            paddingBottom={4}
            borderRadius={12}
            onClick={onClick}
            borderWidth={1}
            borderColor={selected ? color.fg : c.cardStroke}
            backgroundColor={selected ? color.bg : hover.isHovered ? c.subtleHover : c.subtle}
            onMouseEnter={hover.onMouseEnter}
            onMouseLeave={hover.onMouseLeave}
            animatedVars={[".r", ".g", ".b", ".a"]}
        >
            <flex width={6} height={6} autoSize={false} borderRadius={3} backgroundColor={selected ? color.fg : c.textTertiary} animatedVars={[".r", ".g", ".b", ".a"]} />
            <Text fontSize={12} color={selected ? color.fg : c.textSecondary}>{label}</Text>
        </flex>
    );
};

export const IconButton = ({ icon, label, onClick, accent }: {
    icon?: string;
    label?: string;
    onClick: () => void;
    accent?: boolean;
}) => {
    const c = fluent();
    const hover = useHoverActive();
    const bg = accent
        ? (hover.isActive ? (c.light ? "#005FB8CC" : "#60CDFFCC") : hover.isHovered ? (c.light ? "#005FB8E6" : "#60CDFFE6") : c.accent)
        : (hover.isActive ? c.cardPressed : hover.isHovered ? c.subtleHover : c.subtle);
    return (
        <flex
            horizontal
            alignItems="center"
            gap={6}
            paddingLeft={label ? 12 : 7}
            paddingRight={label ? 12 : 7}
            paddingTop={6}
            paddingBottom={6}
            borderRadius={5}
            borderWidth={1}
            borderColor={accent ? c.transparent : c.cardStroke}
            backgroundColor={bg}
            onClick={onClick}
            onMouseEnter={hover.onMouseEnter}
            onMouseLeave={hover.onMouseLeave}
            onMouseDown={hover.onMouseDown}
            onMouseUp={hover.onMouseUp}
            animatedVars={[".r", ".g", ".b", ".a"]}
        >
            {icon && iconElement(icon, 14, accent ? c.accentText : undefined)}
            {label && <Text fontSize={12} color={accent ? c.accentText : c.text}>{label}</Text>}
        </flex>
    );
};

export const InfoBar = ({ severity, title, message, children, maxTextWidth = 480 }: {
    severity: string;
    title: string;
    message?: string;
    children?: ReactNode;
    maxTextWidth?: number;
}) => {
    const s = severityStyle(severity);
    const c = fluent();
    return (
        <flex
            horizontal
            gap={10}
            padding={12}
            borderRadius={6}
            backgroundColor={s.bg}
            borderColor={c.cardStroke}
            borderWidth={1}
            alignItems="start"
        >
            <flex width={16} height={16} autoSize={false} borderRadius={8} backgroundColor={s.fg} justifyContent="center" alignItems="center">
                <Text fontSize={11} fontWeight={700} color={c.light ? "#FFFFFFFF" : "#000000FF"}>{severity === "info" ? "i" : "!"}</Text>
            </flex>
            <flex gap={4} flexGrow={1} alignItems="stretch">
                <Text fontSize={13} fontWeight={600} color={c.text} maxWidth={maxTextWidth}>{title}</Text>
                {message && <Text fontSize={12} color={c.textSecondary} maxWidth={maxTextWidth}>{message}</Text>}
                {children}
            </flex>
        </flex>
    );
};

export const SectionHeader = ({ title, subtitle, children, gutter = 0 }: { title: string; subtitle?: string; children?: ReactNode; gutter?: number }) => {
    const c = fluent();
    return (
        <flex horizontal alignItems="end" justifyContent="space-between" paddingRight={gutter}>
            <flex gap={2}>
                <Text fontSize={26} fontWeight={600} color={c.text}>{title}</Text>
                {subtitle && <Text fontSize={12} color={c.textSecondary}>{subtitle}</Text>}
            </flex>
            <flex horizontal gap={6} alignItems="center">{children}</flex>
        </flex>
    );
};
