import { memo } from "react";
import { Button } from "./Button";
import { Text } from "./Text";
import { iconElement } from "./Icon";

export const TextButton = memo((({
    onClick,
    children,
    icon
}: {
    onClick: () => void;
    children: string;
    icon?: string;
}) => {
    return (
        <Button onClick={onClick}>
            {
                icon ? iconElement(icon, 14) : null
            }
            <Text fontSize={14}>{children}</Text>
        </Button>
    );
}));
