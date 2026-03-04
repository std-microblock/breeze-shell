import { breeze } from "mshell";
import { memo } from "react";
import { iconElement } from "./Icon";
import { ICON_MORE_VERT } from "../constants";
import { useHoverActive } from "../hooks";

export const PluginMoreButton = memo((({ onClick }: { onClick: () => void }) => {
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
}));
