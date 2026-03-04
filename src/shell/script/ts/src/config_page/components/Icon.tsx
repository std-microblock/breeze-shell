import { breeze } from "mshell";

export const iconElement = (svg: string, width = 14) => (
    <img
        svg={svg.replace(
            '<svg ',
            `<svg fill="${breeze.is_light_theme() ? '#000000ff' : '#ffffffff'}\" `
        )}
        width={width}
        height={width}
    />
);
