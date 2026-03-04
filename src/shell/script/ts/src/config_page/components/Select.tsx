import { breeze } from "mshell";
import { Text } from "./Text";
import { Button } from "./Button";

export const Select = ({
    label,
    value,
    options,
    onChange
}: {
    label: string;
    value: string;
    options: { value: string; label: string }[];
    onChange: (v: string) => void;
}) => {
    return (
        <flex horizontal alignItems="center" gap={10} justifyContent="space-between">
            <Text fontSize={14}>{label}</Text>
            <flex horizontal gap={5}>
                {options.map(opt => (
                    <Button
                        key={opt.value}
                        selected={value === opt.value}
                        onClick={() => onChange(opt.value)}
                    >
                        <Text fontSize={12}>{opt.label}</Text>
                    </Button>
                ))}
            </flex>
        </flex>
    );
};
