import { breeze } from "mshell";
import { memo } from "react";
import { useHoverActive } from "../hooks";

export const PluginCheckbox = memo((({ isEnabled, onToggle }: { isEnabled: boolean; onToggle: () => void }) => {
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
}));
