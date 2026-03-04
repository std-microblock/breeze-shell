import { breeze } from "mshell";
import { Text } from "./Text";
import { useHoverActive } from "../hooks";

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
