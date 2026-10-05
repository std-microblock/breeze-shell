import { memo } from "react";
import { Text } from "./Text";
import { iconElement } from "./Icon";
import { Badge, fluent } from "./Fluent";
import { useHoverActive } from "../hooks";

const COLOR_VARS = [".r", ".g", ".b", ".a"];

export const SidebarItem = memo(({ onClick, icon, isActive, badge, children, trailing }: {
    onClick: () => void;
    icon: string;
    isActive: boolean;
    badge?: number;
    children: string;
    trailing?: string;
}) => {
    const c = fluent();
    const { isHovered, isActive: isPressed, onMouseEnter, onMouseLeave, onMouseDown, onMouseUp } = useHoverActive();
    return (
        <flex
            onClick={onClick}
            backgroundColor={isPressed ? c.subtlePressed : isActive || isHovered ? c.subtle : c.transparent}
            height={36}
            paddingRight={10}
            alignItems="center"
            horizontal
            gap={12}
            borderRadius={4}
            onMouseEnter={onMouseEnter}
            onMouseLeave={onMouseLeave}
            onMouseDown={onMouseDown}
            onMouseUp={onMouseUp}
            animatedVars={COLOR_VARS}
            animationCurve={{ duration: 120, easing: "ease_out", vars: COLOR_VARS }}
        >
            <flex width={3} height={isActive ? 16 : 0} borderRadius={1.5} backgroundColor={isActive ? c.accent : c.transparent}
                animatedVars={["height"]} animationCurve={{ duration: 180, easing: "ease_out", vars: ["height"] }} />
            {iconElement(icon, 16, isPressed ? c.textSecondary : c.text)}
            <Text fontSize={13} fontWeight={isActive ? 600 : 400} color={isPressed ? c.textSecondary : c.text}>{children}</Text>
            <spacer />
            {trailing && <Text fontSize={12} color={c.textSecondary}>{trailing}</Text>}
            {badge ? <Badge value={badge} /> : null}
        </flex>
    );
});
