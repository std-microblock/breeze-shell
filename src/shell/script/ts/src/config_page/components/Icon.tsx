import { breeze } from "mshell";

const fillAttrs = (color: string) => {
    const hex = color.replace("#", "");
    if (hex.length === 8) {
        const alpha = parseInt(hex.slice(6, 8), 16) / 255;
        return `fill="#${hex.slice(0, 6)}" fill-opacity="${alpha.toFixed(3)}"`;
    }
    return `fill="#${hex}"`;
};

export const iconElement = (svg: string, width = 14, color?: string) => (
    <img
        svg={svg.replace('<svg ', `<svg ${fillAttrs(color ?? (breeze.is_light_theme() ? '#000000E4' : '#FFFFFFFF'))} `)}
        width={width}
        height={width}
    />
);
