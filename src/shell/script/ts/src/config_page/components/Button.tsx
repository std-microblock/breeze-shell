import { breeze } from "mshell";
import { memo, type ReactNode } from "react";
import { useHoverActive } from "../hooks";

export const Button = memo((({
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
}));
