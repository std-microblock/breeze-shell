import { breeze } from "mshell";
import { memo } from "react";
import { Text } from "./Text";
import { iconElement } from "./Icon";
import { useHoverActive } from "../hooks";

export const SidebarItem = memo((({
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
}));
