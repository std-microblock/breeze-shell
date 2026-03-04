import { breeze } from "mshell";
import { Text } from "./Text";
import { useHoverActive } from "../hooks";

export const NumberBox = ({
    label,
    value,
    onChange,
    min = 0,
    max = 100,
    step = 1
}: {
    label: string;
    value: number;
    onChange: (v: number) => void;
    min?: number;
    max?: number;
    step?: number;
}) => {
    const isLightTheme = breeze.is_light_theme();

    const handleDecrease = () => {
        const newValue = Math.max(min, value - step);
        onChange(newValue);
    };

    const handleIncrease = () => {
        const newValue = Math.min(max, value + step);
        onChange(newValue);
    };

    return (
        <flex horizontal alignItems="center" gap={10} justifyContent="space-between">
            <Text fontSize={14}>{label}</Text>
            <NumberBoxControl
                value={value}
                step={step}
                onDecrease={handleDecrease}
                onIncrease={handleIncrease}
                isLightTheme={isLightTheme}
            />
        </flex>
    );
};

const NumberBoxControl = ({
    value,
    step,
    onDecrease,
    onIncrease,
    isLightTheme
}: {
    value: number;
    step: number;
    onDecrease: () => void;
    onIncrease: () => void;
    isLightTheme: boolean;
}) => {
    const decreaseHover = useHoverActive();
    const increaseHover = useHoverActive();

    return (
        <flex
            horizontal
            borderRadius={6}
            backgroundColor={isLightTheme ? '#ffffff44' : '#2a2a2a44'}
            borderWidth={1}
            borderColor={isLightTheme ? '#e0e0e044' : '#3a3a3a44'}
            alignItems="center"
            paddingLeft={4}
            paddingRight={4}
        >
            <flex
                width={28}
                height={28}
                justifyContent="center"
                alignItems="center"
                onClick={onDecrease}
                backgroundColor={
                    decreaseHover.isActive ? (isLightTheme ? '#d0d0d0' : '#404040') :
                    decreaseHover.isHovered ? (isLightTheme ? '#e8e8e8' : '#353535') :
                    (isLightTheme ? '#ffffff00' : '#2a2a2a00')
                }
                borderRadius={4}
                onMouseEnter={decreaseHover.onMouseEnter}
                onMouseLeave={decreaseHover.onMouseLeave}
                onMouseDown={decreaseHover.onMouseDown}
                onMouseUp={decreaseHover.onMouseUp}
                autoSize={false}
                animatedVars={['.a']}
            >
                <Text fontSize={16} color={isLightTheme ? [80, 80, 80, 255] : [200, 200, 200, 255]}>−</Text>
            </flex>
            <flex
                width={60}
                height={32}
                justifyContent="center"
                alignItems="center"
                autoSize={false}
            >
                <Text fontSize={13} color={isLightTheme ? [40, 40, 40, 255] : [220, 220, 220, 255]}>
                    {value.toFixed(step < 1 ? 1 : 0)}
                </Text>
            </flex>
            <flex
                width={28}
                height={28}
                justifyContent="center"
                alignItems="center"
                onClick={onIncrease}
                backgroundColor={
                    increaseHover.isActive ? (isLightTheme ? '#d0d0d0' : '#404040') :
                    increaseHover.isHovered ? (isLightTheme ? '#e8e8e8' : '#353535') :
                    (isLightTheme ? '#ffffff00' : '#2a2a2a00')
                }
                borderRadius={4}
                onMouseEnter={increaseHover.onMouseEnter}
                onMouseLeave={increaseHover.onMouseLeave}
                onMouseDown={increaseHover.onMouseDown}
                onMouseUp={increaseHover.onMouseUp}
                autoSize={false}
                animatedVars={['.a']}
            >
                <Text fontSize={16} color={isLightTheme ? [80, 80, 80, 255] : [200, 200, 200, 255]}>+</Text>
            </flex>
        </flex>
    );
};
