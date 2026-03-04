import { breeze } from "mshell";
import { memo } from "react";

export const Text = memo((({
    children,
    fontSize = 14,
    maxWidth = -1,
    color
}: {
    children: string;
    fontSize?: number;
    maxWidth?: number;
    color?: string | number[];
}) => {
    if (!color) color = breeze.is_light_theme() ? '#000000ff' : '#ffffffff'
    if (color instanceof Array)
        color = `#${color.slice(0, 4).map(c => c.toString(16).padStart(2, '0')).join('')}`;

    return (
        <text
            text={children}
            fontSize={fontSize}
            maxWidth={maxWidth}
            color={color}
        />
    );
}));
