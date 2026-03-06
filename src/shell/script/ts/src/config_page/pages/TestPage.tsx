import * as shell from "mshell";
import { Button, Text } from "../components";
import { useTranslation } from "../hooks";
import { memo } from "react";

const TestPage = memo(() => {
    const { t } = useTranslation();

    return (
        <flex gap={20}>
            <Text fontSize={24}>{t("test.title")}</Text>
            <flex gap={10}>
                <Button onClick={() => shell.breeze.crash_cpu_exception()}>
                    <Text fontSize={16}>{t("test.cpu_exception_crash")}</Text>
                </Button>
                <Button onClick={() => shell.breeze.crash_cpp_exception()}>
                    <Text fontSize={16}>{t("test.cpp_exception_crash")}</Text>
                </Button>
                <Button onClick={() => { throw new Error("This is a test JavaScript exception crash"); }}>
                    <Text fontSize={16}>{t("test.js_exception_crash")}</Text>
                </Button>
            </flex>


            <flex autoSize={false} width={100} height={200} alignItems="center" justifyContent="center" backgroundColor="#0000ff" >
                <flex backgroundColor="#ff0000">
                    <Text fontSize={16} color="#ffffff">Hello，你好</Text>
                </flex>
            </flex>
        </flex>
    );
});

export default TestPage;
