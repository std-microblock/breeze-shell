import { Text } from "./Text";

export const SimpleMarkdownRender = ({ text, maxWidth }: { text: string, maxWidth: number }) => {
    return (
        <>
            {text.split('\n').map((line, index) => (
                line.trim().startsWith('# ') ? <Text key={index} fontSize={22} maxWidth={maxWidth}>{line.trim().substring(2).trim()}</Text> :
                    line.trim().startsWith('## ') ? <Text key={index} fontSize={20} maxWidth={maxWidth}>{line.trim().substring(3).trim()}</Text> :
                        line.trim().startsWith('### ') ? <Text key={index} fontSize={18} maxWidth={maxWidth}>{line.trim().substring(4).trim()}</Text> :
                            line.trim().startsWith('#### ') ? <Text key={index} fontSize={16} maxWidth={maxWidth}>{line.trim().substring(5).trim()}</Text> :
                                <Text key={index} fontSize={14} maxWidth={maxWidth}>{line}</Text>
            ))}
        </>
    );
};
