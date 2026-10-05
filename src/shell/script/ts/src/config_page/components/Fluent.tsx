import * as shell from "mshell";
import { memo, useEffect, useRef, useState, type ReactNode } from "react";
import { Text } from "./Text";
import { iconElement } from "./Icon";
import { useHoverActive } from "../hooks";
import { showMenu } from "../utils/menu";
import { ICON_CHECK, ICON_CHEVRON_DOWN, ICON_CHEVRON_RIGHT, ICON_CHEVRON_UP } from "../constants";

const COLOR_VARS = [".r", ".g", ".b", ".a"];
const FAST = { duration: 120, easing: "ease_out" as const };

export const fluent = () => {
    const light = shell.breeze.is_light_theme();
    return {
        light,
        accent: light ? "#005FB8FF" : "#60CDFFFF",
        accentHover: light ? "#196EBFFF" : "#5AB8E6FF",
        accentPressed: light ? "#327EC5FF" : "#53A3CCFF",
        accentText: light ? "#FFFFFFFF" : "#000000FF",
        accentSubtle: light ? "#005FB81A" : "#60CDFF1F",
        text: light ? "#000000E4" : "#FFFFFFFF",
        textSecondary: light ? "#0000009E" : "#FFFFFFC5",
        textTertiary: light ? "#00000072" : "#FFFFFF87",
        textDisabled: light ? "#0000005C" : "#FFFFFF5D",
        card: light ? "#FFFFFFB3" : "#FFFFFF0D",
        cardHover: light ? "#F9F9F9E6" : "#FFFFFF15",
        cardPressed: light ? "#F5F5F5CC" : "#FFFFFF08",
        cardStroke: light ? "#0000000F" : "#FFFFFF12",
        control: light ? "#FFFFFFB3" : "#FFFFFF0F",
        controlHover: light ? "#F9F9F9E6" : "#FFFFFF15",
        controlPressed: light ? "#F5F5F5CC" : "#FFFFFF08",
        controlStroke: light ? "#0000001A" : "#FFFFFF17",
        controlStrong: light ? "#00000072" : "#FFFFFF8B",
        controlSolid: light ? "#FFFFFFFF" : "#454545FF",
        subtle: light ? "#0000000A" : "#FFFFFF0F",
        subtleHover: light ? "#00000009" : "#FFFFFF0F",
        subtlePressed: light ? "#00000006" : "#FFFFFF0A",
        divider: light ? "#00000014" : "#FFFFFF15",
        codeBg: light ? "#0000000A" : "#00000040",
        transparent: light ? "#FFFFFF00" : "#FFFFFF00",
    };
};

export type FluentTokens = ReturnType<typeof fluent>;

export const severityStyle = (level: string) => {
    const light = shell.breeze.is_light_theme();
    switch (level) {
        case "critical":
            return { fg: light ? "#C42B1CFF" : "#FF99A4FF", bg: light ? "#FDE7E9E6" : "#442726E6", label: "CRIT" };
        case "error":
            return { fg: light ? "#C42B1CFF" : "#FF99A4FF", bg: light ? "#FDE7E9E6" : "#442726E6", label: "ERROR" };
        case "warn":
        case "warning":
            return { fg: light ? "#9D5D00FF" : "#FCE100FF", bg: light ? "#FFF4CEE6" : "#433519E6", label: "WARN" };
        case "info":
            return { fg: light ? "#005FB8FF" : "#60CDFFFF", bg: light ? "#E5F1FBE6" : "#1F3346E6", label: "INFO" };
        case "success":
            return { fg: light ? "#0F7B0FFF" : "#6CCB5FFF", bg: light ? "#DFF6DDE6" : "#393D1BE6", label: "OK" };
        case "debug":
            return { fg: light ? "#5C5C5CFF" : "#C5C5C5FF", bg: light ? "#0000000D" : "#FFFFFF12", label: "DEBUG" };
        default:
            return { fg: light ? "#8A8A8AFF" : "#9A9A9AFF", bg: light ? "#00000008" : "#FFFFFF0A", label: level.toUpperCase() };
    }
};

const useEntrance = (delay = 0) => {
    const [shown, setShown] = useState(false);
    useEffect(() => {
        const id = setTimeout(() => setShown(true), 16 + delay);
        return () => clearTimeout(id);
    }, []);
    return shown;
};

const ENTRANCE_VARS = ["opacity", "padding_top", "padding_bottom"];

export const Entrance = ({ delay = 0, offset = 10, children, ...rest }: {
    delay?: number;
    offset?: number;
    children?: ReactNode;
    [key: string]: any;
}) => {
    const shown = useEntrance(delay);
    return (
        <flex
            opacity={shown ? 255 : 0}
            paddingTop={shown ? 0 : offset}
            paddingBottom={shown ? offset : 0}
            alignItems="stretch"
            {...rest}
            animatedVars={ENTRANCE_VARS}
            animationCurve={{ duration: 220, easing: "ease_out", vars: ENTRANCE_VARS }}
        >
            {children}
        </flex>
    );
};

export const Divider = ({ vertical }: { vertical?: boolean }) => {
    const c = fluent();
    return vertical
        ? <flex width={1} backgroundColor={c.divider} />
        : <flex height={1} backgroundColor={c.divider} />;
};

export const Card = memo(({ children, onClick, padding = 14, gap = 8, horizontal, alignItems = "stretch", accentBar, selected }: {
    children?: ReactNode;
    onClick?: () => void;
    padding?: number;
    gap?: number;
    horizontal?: boolean;
    alignItems?: "start" | "center" | "end" | "stretch";
    accentBar?: string;
    selected?: boolean;
}) => {
    const c = fluent();
    const hover = useHoverActive();
    const interactive = !!onClick;
    return (
        <flex
            horizontal
            alignItems="stretch"
            backgroundColor={interactive && hover.isActive ? c.cardPressed : interactive && hover.isHovered ? c.cardHover : c.card}
            borderColor={selected ? c.accent : c.cardStroke}
            borderWidth={selected ? 2 : 1}
            borderRadius={7}
            onClick={onClick ?? (() => { })}
            onMouseEnter={hover.onMouseEnter}
            onMouseLeave={hover.onMouseLeave}
            onMouseDown={hover.onMouseDown}
            onMouseUp={hover.onMouseUp}
            animatedVars={COLOR_VARS}
        >
            {accentBar && (
                <flex paddingTop={10} paddingBottom={10} paddingLeft={1}>
                    <flex width={3} flexGrow={1} borderRadius={1.5} backgroundColor={accentBar} />
                </flex>
            )}
            <flex horizontal={horizontal} padding={padding} gap={gap} alignItems={alignItems} flexGrow={1}>
                {children}
            </flex>
        </flex>
    );
});

export const Pill = memo(({ text, fg, bg, fontSize = 11 }: { text: string; fg: string; bg: string; fontSize?: number }) => {
    const height = Math.round(fontSize * 1.6 + 2);
    return (
        <flex
            horizontal
            height={height}
            backgroundColor={bg}
            borderRadius={4}
            paddingLeft={6}
            paddingRight={6}
            alignItems="center"
            justifyContent="center"
        >
            <Text fontSize={fontSize} fontWeight={600} color={fg}>{text}</Text>
        </flex>
    );
});

export const Badge = memo(({ value, color }: { value: number | string; color?: string }) => {
    const c = fluent();
    const label = String(value);
    return (
        <flex
            horizontal
            height={16}
            borderRadius={8}
            paddingLeft={5}
            paddingRight={5}
            {...(label.length > 1 ? {} : { width: 16 })}
            backgroundColor={color ?? (c.light ? "#C42B1CFF" : "#FF99A4FF")}
            alignItems="center"
            justifyContent="center"
        >
            <Text fontSize={10} fontWeight={600} color={c.light ? "#FFFFFFFF" : "#000000FF"}>{label}</Text>
        </flex>
    );
});

export const Button = ({ label, icon, onClick, variant = "standard", disabled, iconSize = 14, minWidth }: {
    label?: string;
    icon?: string;
    onClick: () => void;
    variant?: "standard" | "accent" | "subtle";
    disabled?: boolean;
    iconSize?: number;
    minWidth?: number;
}) => {
    const c = fluent();
    const hover = useHoverActive();
    const accent = variant === "accent";
    const pressed = hover.isActive && !disabled;
    const hovered = hover.isHovered && !disabled;
    const bg = accent
        ? (disabled ? c.subtle : pressed ? c.accentPressed : hovered ? c.accentHover : c.accent)
        : variant === "subtle"
            ? (pressed ? c.subtlePressed : hovered ? c.subtleHover : c.transparent)
            : (pressed ? c.controlPressed : hovered ? c.controlHover : c.control);
    const fg = disabled ? c.textDisabled : accent ? c.accentText : pressed ? c.textSecondary : c.text;
    return (
        <flex
            horizontal
            alignItems="center"
            justifyContent="center"
            gap={8}
            height={32}
            {...(minWidth ? { width: minWidth } : {})}
            paddingLeft={label ? 12 : 9}
            paddingRight={label ? 12 : 9}
            borderRadius={4}
            borderWidth={variant === "standard" ? 1 : 0}
            borderColor={c.controlStroke}
            backgroundColor={bg}
            onClick={() => { if (!disabled) onClick(); }}
            onMouseEnter={hover.onMouseEnter}
            onMouseLeave={hover.onMouseLeave}
            onMouseDown={hover.onMouseDown}
            onMouseUp={hover.onMouseUp}
            animatedVars={COLOR_VARS}
            animationCurve={{ ...FAST, vars: COLOR_VARS }}
        >
            {icon && iconElement(icon, iconSize, fg)}
            {label && <Text fontSize={13} color={fg}>{label}</Text>}
        </flex>
    );
};

export const IconButton = ({ icon, label, onClick, accent }: {
    icon?: string;
    label?: string;
    onClick: () => void;
    accent?: boolean;
}) => <Button icon={icon} label={label} onClick={onClick} variant={accent ? "accent" : "standard"} />;

export const ToggleSwitch = ({ value, onChange, onLabel, offLabel }: {
    value: boolean;
    onChange: (v: boolean) => void;
    onLabel?: string;
    offLabel?: string;
}) => {
    const c = fluent();
    const hover = useHoverActive();
    const knob = hover.isActive ? 14 : hover.isHovered ? 14 : 12;
    const knobWidth = hover.isActive ? 17 : knob;
    const track = value
        ? (hover.isActive ? c.accentPressed : hover.isHovered ? c.accentHover : c.accent)
        : (hover.isActive ? c.subtlePressed : hover.isHovered ? c.subtleHover : c.transparent);
    return (
        <flex
            horizontal
            alignItems="center"
            gap={12}
            onClick={() => onChange(!value)}
            onMouseEnter={hover.onMouseEnter}
            onMouseLeave={hover.onMouseLeave}
            onMouseDown={hover.onMouseDown}
            onMouseUp={hover.onMouseUp}
        >
            {(onLabel || offLabel) && (
                <Text fontSize={13} color={c.text}>{(value ? onLabel : offLabel) ?? ""}</Text>
            )}
            <flex
                horizontal
                width={40}
                height={20}
                borderRadius={10}
                borderWidth={value ? 0 : 1}
                borderColor={c.controlStrong}
                backgroundColor={track}
                justifyContent={value ? "end" : "start"}
                alignItems="center"
                paddingLeft={knob === 12 ? 4 : 3}
                paddingRight={knob === 12 ? 4 : 3}
                animatedVars={COLOR_VARS}
                animationCurve={{ ...FAST, vars: COLOR_VARS }}
            >
                <flex
                    width={knobWidth}
                    height={knob}
                    borderRadius={7}
                    backgroundColor={value ? c.accentText : c.textSecondary}
                    animatedVars={["x", "width", "height", ...COLOR_VARS]}
                    animationCurve={{ duration: 160, easing: "ease_out", vars: ["x", "width", "height"] }}
                />
            </flex>
        </flex>
    );
};

const decimalsOf = (step: number) => step >= 1 ? 0 : Math.min(4, Math.ceil(-Math.log10(step) - 1e-9));
export const formatNumber = (v: number, step: number) => {
    const s = v.toFixed(decimalsOf(step));
    return s.includes(".") ? s.replace(/\.?0+$/, "") : s;
};
const snapTo = (v: number, min: number, max: number, step: number) => {
    const snapped = Math.round((v - min) / step) * step + min;
    return Number(Math.min(max, Math.max(min, snapped)).toFixed(decimalsOf(step)));
};

const THUMB = 20;

export const Slider = ({ value, min, max, step = 1, onChange, width = 200 }: {
    value: number;
    min: number;
    max: number;
    step?: number;
    onChange: (v: number) => void;
    width?: number;
}) => {
    const c = fluent();
    const hover = useHoverActive();
    const lastX = useRef(0);
    const dragging = useRef(false);
    const [, setDragState] = useState(false);
    const span = width - THUMB;
    const frac = max > min ? Math.min(1, Math.max(0, (value - min) / (max - min))) : 0;

    const applyX = (x: number) => {
        const f = Math.min(1, Math.max(0, (x - THUMB / 2) / span));
        const next = snapTo(min + f * (max - min), min, max, step);
        if (next !== value) onChange(next);
    };

    const dot = dragging.current ? 10 : hover.isHovered ? 14 : 12;
    return (
        <flex
            horizontal
            width={width}
            height={32}
            justifyContent="free"
            alignItems="center"
            onMouseEnter={hover.onMouseEnter}
            onMouseLeave={hover.onMouseLeave}
            onMouseMove={(x: number) => {
                lastX.current = x;
                if (dragging.current) applyX(x);
            }}
            onMouseDown={() => {
                dragging.current = true;
                setDragState(true);
                applyX(lastX.current);
            }}
            onMouseUp={() => {
                dragging.current = false;
                setDragState(false);
            }}
        >
            <flex x={THUMB / 2} width={span} height={4} borderRadius={2} backgroundColor={c.controlStrong} />
            <flex x={THUMB / 2} width={Math.max(0.01, frac * span)} height={4} borderRadius={2} backgroundColor={c.accent} />
            <flex
                x={frac * span}
                width={THUMB}
                height={THUMB}
                borderRadius={THUMB / 2}
                backgroundColor={c.controlSolid}
                borderColor={c.controlStroke}
                borderWidth={1}
                justifyContent="center"
                alignItems="center"
            >
                <flex
                    width={dot}
                    height={dot}
                    borderRadius={dot / 2}
                    backgroundColor={dragging.current ? c.accentPressed : hover.isHovered ? c.accentHover : c.accent}
                    animatedVars={["width", "height", "border_radius"]}
                    animationCurve={{ ...FAST, vars: ["width", "height", "border_radius"] }}
                />
            </flex>
        </flex>
    );
};

const KEY_ENTER = 257, KEY_UP = 265, KEY_DOWN = 264, KEY_ESCAPE = 256;

export const NumberBox = ({ value, min, max, step = 1, onChange, width = 76, suffix }: {
    value: number;
    min: number;
    max: number;
    step?: number;
    onChange: (v: number) => void;
    width?: number;
    suffix?: string;
}) => {
    const c = fluent();
    const [draft, setDraft] = useState<string | null>(null);
    const shown = draft ?? formatNumber(value, step);
    const commit = (text = draft) => {
        setDraft(null);
        if (text === null) return;
        const parsed = parseFloat(text.trim());
        if (!Number.isFinite(parsed)) return;
        const next = snapTo(parsed, min, max, step);
        if (next !== value) onChange(next);
    };
    return (
        <flex horizontal alignItems="center" gap={6}>
            <textbox
                value={shown}
                width={width}
                height={32}
                fontSize={13}
                borderRadius={4}
                paddingX={10}
                backgroundColor={c.control}
                borderColor={c.controlStroke}
                focusBorderColor={c.accent}
                textColor={c.text}
                placeholderColor={c.textTertiary}
                caretColor={c.text}
                selectionColor={c.accentSubtle}
                onChange={setDraft}
                onBlur={() => commit()}
                onKeyDown={(key: number) => {
                    if (key === KEY_ENTER) { commit(); return true; }
                    if (key === KEY_ESCAPE) { setDraft(null); return true; }
                    if (key === KEY_UP || key === KEY_DOWN) {
                        const base = draft !== null && Number.isFinite(parseFloat(draft)) ? parseFloat(draft) : value;
                        setDraft(null);
                        const next = snapTo(base + (key === KEY_UP ? step : -step), min, max, step);
                        if (next !== value) onChange(next);
                        return true;
                    }
                    return false;
                }}
            />
            {suffix && <Text fontSize={12} color={c.textSecondary}>{suffix}</Text>}
        </flex>
    );
};

export const SliderField = ({ value, min, max, step = 1, onChange, sliderWidth = 180, suffix, fieldMax }: {
    value: number;
    min: number;
    max: number;
    step?: number;
    onChange: (v: number) => void;
    sliderWidth?: number;
    suffix?: string;
    fieldMax?: number;
}) => (
    <flex horizontal alignItems="center" gap={12}>
        <Slider value={Math.min(value, max)} min={min} max={max} step={step} onChange={onChange} width={sliderWidth} />
        <NumberBox value={value} min={min} max={fieldMax ?? max} step={step} onChange={onChange} suffix={suffix} />
    </flex>
);

export const ComboBox = <T extends string>({ value, options, onChange, width = 160 }: {
    value: T;
    options: { value: T; label: string }[];
    onChange: (v: T) => void;
    width?: number;
}) => {
    const c = fluent();
    const hover = useHoverActive();
    const current = options.find(o => o.value === value);
    const open = () => showMenu(menu => {
        for (const o of options) {
            menu.append_menu({
                name: o.label,
                icon_svg: o.value === value ? ICON_CHECK.replace("<svg ", `<svg fill="${c.light ? "#000000" : "#FFFFFF"}" `) : undefined,
                action() {
                    onChange(o.value);
                    menu.close();
                }
            });
        }
    });
    return (
        <flex
            horizontal
            width={width}
            height={32}
            alignItems="center"
            paddingLeft={11}
            paddingRight={10}
            borderRadius={4}
            borderWidth={1}
            borderColor={c.controlStroke}
            backgroundColor={hover.isActive ? c.controlPressed : hover.isHovered ? c.controlHover : c.control}
            onClick={open}
            onMouseEnter={hover.onMouseEnter}
            onMouseLeave={hover.onMouseLeave}
            onMouseDown={hover.onMouseDown}
            onMouseUp={hover.onMouseUp}
            animatedVars={COLOR_VARS}
            animationCurve={{ ...FAST, vars: COLOR_VARS }}
        >
            <Text fontSize={13} color={c.text}>{current?.label ?? String(value)}</Text>
            <spacer />
            {iconElement(ICON_CHEVRON_DOWN, 14, c.textSecondary)}
        </flex>
    );
};

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
            height={28}
            paddingLeft={12}
            paddingRight={12}
            borderRadius={4}
            onClick={onClick}
            backgroundColor={selected ? c.controlSolid : hover.isHovered ? c.subtleHover : c.transparent}
            borderColor={selected ? c.controlStroke : c.transparent}
            borderWidth={1}
            onMouseEnter={hover.onMouseEnter}
            onMouseLeave={hover.onMouseLeave}
            onMouseDown={hover.onMouseDown}
            onMouseUp={hover.onMouseUp}
            animatedVars={COLOR_VARS}
            animationCurve={{ ...FAST, vars: COLOR_VARS }}
        >
            <Text fontSize={13} fontWeight={selected ? 600 : 400} color={selected ? c.text : c.textSecondary}>{label}</Text>
            {badge !== undefined && (
                <Text fontSize={12} color={selected ? c.accent : c.textTertiary}>{String(badge)}</Text>
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
        <flex horizontal>
            <flex horizontal gap={2} padding={2} borderRadius={6} backgroundColor={c.subtle} borderColor={c.cardStroke} borderWidth={1}>
                {options.map(o => (
                    <Segment key={o.value} label={o.label} badge={o.badge} selected={o.value === value} onClick={() => onChange(o.value)} />
                ))}
            </flex>
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
            height={26}
            paddingLeft={10}
            paddingRight={11}
            borderRadius={13}
            onClick={onClick}
            borderWidth={1}
            borderColor={selected ? color.fg : c.controlStroke}
            backgroundColor={selected ? color.bg : hover.isHovered ? c.controlHover : c.control}
            onMouseEnter={hover.onMouseEnter}
            onMouseLeave={hover.onMouseLeave}
            animatedVars={COLOR_VARS}
            animationCurve={{ ...FAST, vars: COLOR_VARS }}
        >
            <flex width={6} height={6} borderRadius={3} backgroundColor={selected ? color.fg : c.textTertiary} />
            <Text fontSize={12} color={selected ? color.fg : c.textSecondary}>{label}</Text>
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
        <flex horizontal gap={12} padding={14} borderRadius={6} backgroundColor={s.bg} borderColor={c.cardStroke} borderWidth={1} alignItems="start">
            <flex width={16} height={16} borderRadius={8} backgroundColor={s.fg} justifyContent="center" alignItems="center">
                <Text fontSize={11} fontWeight={700} color={c.light ? "#FFFFFFFF" : "#000000FF"}>{severity === "info" ? "i" : severity === "success" ? "✓" : "!"}</Text>
            </flex>
            <flex gap={4} flexGrow={1} alignItems="stretch">
                <Text fontSize={13} fontWeight={600} color={c.text} maxWidth={maxTextWidth}>{title}</Text>
                {message && <Text fontSize={12} color={c.textSecondary} maxWidth={maxTextWidth}>{message}</Text>}
                {children}
            </flex>
        </flex>
    );
};

export const ProgressBar = ({ value, width }: { value: number | null; width: number }) => {
    const c = fluent();
    const f = value === null ? 0.25 : Math.min(1, Math.max(0, value));
    return (
        <flex horizontal width={width} height={4} alignItems="center">
            <flex width={Math.max(4, f * width)} height={3} borderRadius={1.5} backgroundColor={c.accent}
                animatedVars={["width"]} animationCurve={{ duration: 200, easing: "ease_out", vars: ["width"] }} />
            <flex flexGrow={1} height={1} backgroundColor={c.controlStrong} />
        </flex>
    );
};

const Crumb = ({ label, onClick }: { label: string; onClick: () => void }) => {
    const c = fluent();
    const hover = useHoverActive();
    return (
        <flex onClick={onClick} onMouseEnter={hover.onMouseEnter} onMouseLeave={hover.onMouseLeave}>
            <Text fontSize={28} fontWeight={600} color={hover.isHovered ? c.text : c.textSecondary}>{label}</Text>
        </flex>
    );
};

export const PageHeader = ({ title, subtitle, parent, onBack, children, gutter = 0 }: {
    title: string;
    subtitle?: string;
    parent?: string;
    onBack?: () => void;
    children?: ReactNode;
    gutter?: number;
}) => {
    const c = fluent();
    return (
        <flex horizontal alignItems="end" justifyContent="space-between" paddingRight={gutter}>
            <flex gap={6}>
                <flex horizontal alignItems="center" gap={10}>
                    {parent && onBack && <Crumb label={parent} onClick={onBack} />}
                    {parent && onBack && iconElement(ICON_CHEVRON_RIGHT, 18, c.textSecondary)}
                    <Text fontSize={28} fontWeight={600} color={c.text}>{title}</Text>
                </flex>
                {subtitle && <Text fontSize={12} color={c.textSecondary}>{subtitle}</Text>}
            </flex>
            <flex horizontal gap={8} alignItems="center">{children}</flex>
        </flex>
    );
};

export const SectionHeader = PageHeader;

export const SettingsGroup = ({ title, children, gap = 4 }: { title?: string; children?: ReactNode; gap?: number }) => {
    const c = fluent();
    return (
        <flex gap={gap} alignItems="stretch">
            {title && (
                <flex paddingTop={14} paddingBottom={6}>
                    <Text fontSize={14} fontWeight={600} color={c.text}>{title}</Text>
                </flex>
            )}
            {children}
        </flex>
    );
};

export const SettingsCard = ({ icon, title, description, children, onClick, descriptionWidth = 360, chevron }: {
    icon?: string;
    title: string;
    description?: string;
    children?: ReactNode;
    onClick?: () => void;
    descriptionWidth?: number;
    chevron?: boolean;
}) => {
    const c = fluent();
    const hover = useHoverActive();
    const interactive = !!onClick;
    return (
        <flex
            horizontal
            alignItems="center"
            gap={16}
            paddingLeft={16}
            paddingRight={16}
            paddingTop={description ? 13 : 16}
            paddingBottom={description ? 13 : 16}
            borderRadius={7}
            borderWidth={1}
            borderColor={c.cardStroke}
            backgroundColor={interactive && hover.isActive ? c.cardPressed : interactive && hover.isHovered ? c.cardHover : c.card}
            onClick={onClick ?? (() => { })}
            onMouseEnter={hover.onMouseEnter}
            onMouseLeave={hover.onMouseLeave}
            onMouseDown={hover.onMouseDown}
            onMouseUp={hover.onMouseUp}
            animatedVars={COLOR_VARS}
            animationCurve={{ ...FAST, vars: COLOR_VARS }}
        >
            {icon && iconElement(icon, 18, c.text)}
            <flex gap={2} flexGrow={1} flexShrink={1}>
                <Text fontSize={14} color={c.text}>{title}</Text>
                {description && <Text fontSize={12} color={c.textSecondary} maxWidth={descriptionWidth}>{description}</Text>}
            </flex>
            <flex horizontal alignItems="center" gap={8}>{children}</flex>
            {chevron && iconElement(ICON_CHEVRON_RIGHT, 14, c.textSecondary)}
        </flex>
    );
};

export const Expander = ({ icon, title, description, header, children, defaultOpen = false, descriptionWidth = 340 }: {
    icon?: string;
    title: string;
    description?: string;
    header?: ReactNode;
    children?: ReactNode;
    defaultOpen?: boolean;
    descriptionWidth?: number;
}) => {
    const c = fluent();
    const hover = useHoverActive();
    const [open, setOpen] = useState(defaultOpen);
    return (
        <flex alignItems="stretch" borderRadius={7} borderWidth={1} borderColor={c.cardStroke} backgroundColor={c.card}>
            <flex
                horizontal
                alignItems="center"
                gap={16}
                paddingLeft={16}
                paddingRight={14}
                paddingTop={description ? 13 : 16}
                paddingBottom={description ? 13 : 16}
                borderRadius={7}
                backgroundColor={hover.isActive ? c.subtlePressed : hover.isHovered ? c.subtleHover : c.transparent}
                onClick={() => setOpen(!open)}
                onMouseEnter={hover.onMouseEnter}
                onMouseLeave={hover.onMouseLeave}
                onMouseDown={hover.onMouseDown}
                onMouseUp={hover.onMouseUp}
                animatedVars={COLOR_VARS}
                animationCurve={{ ...FAST, vars: COLOR_VARS }}
            >
                {icon && iconElement(icon, 18, c.text)}
                <flex gap={2} flexGrow={1} flexShrink={1}>
                    <Text fontSize={14} color={c.text}>{title}</Text>
                    {description && <Text fontSize={12} color={c.textSecondary} maxWidth={descriptionWidth}>{description}</Text>}
                </flex>
                <flex horizontal alignItems="center" gap={8}>{header}</flex>
                <flex width={28} height={28} justifyContent="center" alignItems="center">
                    {iconElement(open ? ICON_CHEVRON_UP : ICON_CHEVRON_DOWN, 14, c.textSecondary)}
                </flex>
            </flex>
            {open && <Divider />}
            {open && (
                <Entrance offset={6}>
                    <flex alignItems="stretch">{children}</flex>
                </Entrance>
            )}
        </flex>
    );
};

export const ExpanderRow = ({ title, description, children, last, descriptionWidth = 260, indent = 50 }: {
    title: string;
    description?: string;
    children?: ReactNode;
    last?: boolean;
    descriptionWidth?: number;
    indent?: number;
}) => {
    const c = fluent();
    return (
        <flex alignItems="stretch">
            <flex horizontal alignItems="center" gap={16} paddingLeft={indent} paddingRight={16} paddingTop={10} paddingBottom={10}>
                <flex gap={2} flexGrow={1} flexShrink={1}>
                    <Text fontSize={13} color={c.text}>{title}</Text>
                    {description && <Text fontSize={12} color={c.textSecondary} maxWidth={descriptionWidth}>{description}</Text>}
                </flex>
                <flex horizontal alignItems="center" gap={8}>{children}</flex>
            </flex>
            {!last && <flex paddingLeft={indent} alignItems="stretch"><Divider /></flex>}
        </flex>
    );
};

export const ChoiceCard = ({ label, preview, previewWidth, previewHeight, selected, onClick, width }: {
    label: string;
    preview: string;
    previewWidth: number;
    previewHeight: number;
    selected: boolean;
    onClick: () => void;
    width: number;
}) => {
    const c = fluent();
    const hover = useHoverActive();
    return (
        <flex
            width={width}
            alignItems="stretch"
            borderRadius={7}
            borderWidth={selected ? 2 : 1}
            borderColor={selected ? c.accent : hover.isHovered ? c.controlStrong : c.cardStroke}
            backgroundColor={hover.isActive ? c.cardPressed : hover.isHovered ? c.cardHover : c.card}
            onClick={onClick}
            onMouseEnter={hover.onMouseEnter}
            onMouseLeave={hover.onMouseLeave}
            onMouseDown={hover.onMouseDown}
            onMouseUp={hover.onMouseUp}
            animatedVars={COLOR_VARS}
            animationCurve={{ ...FAST, vars: COLOR_VARS }}
        >
            <flex alignItems="center" justifyContent="center" paddingTop={10} paddingBottom={6}>
                <img svg={preview} width={previewWidth} height={previewHeight} />
            </flex>
            <flex horizontal alignItems="center" justifyContent="center" gap={6} paddingBottom={10}>
                {selected && iconElement(ICON_CHECK, 13, c.accent)}
                <Text fontSize={13} fontWeight={selected ? 600 : 400} color={c.text}>{label}</Text>
            </flex>
        </flex>
    );
};
