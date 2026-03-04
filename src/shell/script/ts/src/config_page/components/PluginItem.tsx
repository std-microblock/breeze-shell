import { breeze } from "mshell";
import { memo } from "react";
import { Text } from "./Text";
import { PluginCheckbox } from "./PluginCheckbox";
import { PluginMoreButton } from "./PluginMoreButton";

export const PluginItem = memo((({
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
}));
