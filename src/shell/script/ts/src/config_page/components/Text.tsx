import { breeze } from "mshell";
import { memo } from "react";

export const Text = memo((({
    children,
    fontSize = 14,
    maxWidth = -1,
    color,
    opacity,
    fontWeight = 400
}: {
    children: string | number | Array<string | number>;
    fontSize?: number;
    maxWidth?: number;
    color?: string | number[];
    opacity?: number;
    fontWeight?: number;
}) => {
    if (!color) color = breeze.is_light_theme() ? '#000000ff' : '#ffffffff'
    if (color instanceof Array)
        color = `#${color.slice(0, 4).map(c => c.toString(16).padStart(2, '0')).join('')}`;
    if (opacity !== undefined) {
        const origAlpha = parseInt(color.slice(7, 9), 16);
        const newAlpha = Math.round(origAlpha * opacity);
        color = color.slice(0, 7) + newAlpha.toString(16).padStart(2, '0');
    }
    const text = Array.isArray(children) ? children.join('') : String(children);
    
    return (
        <text
            text={text}
            fontSize={fontSize}
            maxWidth={maxWidth}
            color={color}
            fontWeight={fontWeight}
        />
    );
}));
