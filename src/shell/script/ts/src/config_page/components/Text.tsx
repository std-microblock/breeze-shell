import { breeze } from "mshell";
import { memo } from "react";

export const Text = memo((({
    children,
    fontSize = 14,
    maxWidth = -1
}: {
    children: string;
    fontSize?: number;
    maxWidth?: number;
}) => {
    const isLightTheme = breeze.is_light_theme();
    return (
        <text
            text={children}
            fontSize={fontSize}
            maxWidth={maxWidth}
            color={isLightTheme ? '#000000ff' : '#ffffffff'}
        />
    );
}));
