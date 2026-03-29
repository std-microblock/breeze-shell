import * as shell from "mshell";
import { Button, Text } from "../components";
import { useTranslation } from "../hooks";
import { memo, useState } from "react";

const TestPage = memo(() => {
    const { t } = useTranslation();
    const [singleLineValue, setSingleLineValue] = useState("你尴尬Say个Hi 没位坐下来");
    const [multiLineValue, setMultiLineValue] = useState(
        "你的影子被日落拉长\n思念在很远的地方\n喔喔喔喔 轻轻的唱年少的轻狂\nAabcd1234567890!@#$%^&*()_+"
    );
    const [focusState, setFocusState] = useState("none");

    return (
        <flex maxHeight={500} enableScrolling>
            <flex gap={20}>
                <Text fontSize={24}>{t("test.title")}</Text>
                {/* <flex gap={10}>
                <Button onClick={() => shell.breeze.crash_cpu_exception()}>
                    <Text fontSize={16}>{t("test.cpu_exception_crash")}</Text>
                </Button>
                <Button onClick={() => shell.breeze.crash_cpp_exception()}>
                    <Text fontSize={16}>{t("test.cpp_exception_crash")}</Text>
                </Button>
                <Button
                    onClick={() => {
                        throw new Error("This is a test JavaScript exception crash");
                    }}
                >
                    <Text fontSize={16}>{t("test.js_exception_crash")}</Text>
                </Button>
            </flex> */}

                <flex gap={10}>
                    <Text fontSize={16}>Textbox Test</Text>
                    <textbox
                        value={singleLineValue}
                        placeholder="单行输入框"
                        width={420}
                        height={34}
                        fontSize={14}
                        onChange={setSingleLineValue}
                        onFocus={() => setFocusState("single")}
                        onBlur={() => setFocusState("none")}
                    />
                    <textbox
                        value={multiLineValue}
                        placeholder="多行输入框"
                        width={420}
                        height={120}
                        fontSize={14}
                        backgroundColor="#000000ff"
                        textColor="#ffffff"
                        caretColor="#ff00ff"
                        multiline
                        onChange={setMultiLineValue}
                        onFocus={() => setFocusState("multi")}
                        onBlur={() => setFocusState("none")}
                    />
                    <Text fontSize={13} fontWeight={900}>Focused: {focusState}</Text>
                    <Text fontSize={13}>Single line value: {singleLineValue}</Text>
                </flex>

                <flex>
                    {
                        [100, 200, 300, 400, 500, 600, 700, 800, 900].map(w => (
                            <Text key={w} fontSize={16} fontWeight={w}>
                                fw-{w} 你好世界
                            </Text>
                        ))
                    }
                </flex>

                <flex
                    autoSize={false}
                    width={100}
                    height={200}
                    alignItems="center"
                    justifyContent="center"
                    backgroundColor="#0000ff"
                >
                    <flex backgroundColor="#ff0000">
                        <Text fontSize={16} color="#ffffff">
                            Hello，你好
                        </Text>
                    </flex>
                </flex>
            </flex>
        </flex>
    );
});

export default TestPage;
