import { useState } from "react";

export const useHoverActive = () => {
    const [isHovered, setIsHovered] = useState(false);
    const [isActive, setIsActive] = useState(false);

    const onMouseEnter = () => setIsHovered(true);
    const onMouseLeave = () => setIsHovered(false);
    const onMouseDown = () => setIsActive(true);
    const onMouseUp = () => setIsActive(false);

    return { isHovered, isActive, onMouseEnter, onMouseLeave, onMouseDown, onMouseUp };
};
