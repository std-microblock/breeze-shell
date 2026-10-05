import { Text } from "./Text";
import { fluent } from "./Fluent";

const HEADINGS: [string, number][] = [["#### ", 14], ["### ", 15], ["## ", 17], ["# ", 20]];

const inline = (s: string) => s.replace(/\*\*(.+?)\*\*/g, "$1").replace(/`([^`]+)`/g, "$1");

export const SimpleMarkdownRender = ({ text, maxWidth }: { text: string, maxWidth: number }) => {
    const c = fluent();
    return (
        <>
            {(text ?? "").split('\n').filter(raw => !raw.trim().startsWith("<")).map((raw, index) => {
                const line = inline(raw.trim());
                const heading = HEADINGS.find(([prefix]) => line.startsWith(prefix));
                if (heading)
                    return <Text key={index} fontSize={heading[1]} fontWeight={600} color={c.text} maxWidth={maxWidth}>{line.slice(heading[0].length).trim()}</Text>;
                if (line.startsWith("- ") || line.startsWith("* "))
                    return <Text key={index} fontSize={13} color={c.textSecondary} maxWidth={maxWidth}>{`•  ${line.slice(2)}`}</Text>;
                return <Text key={index} fontSize={13} color={c.textSecondary} maxWidth={maxWidth}>{inline(raw)}</Text>;
            })}
        </>
    );
};
