import * as shell from "mshell";
import { Button, Text } from "../components";
import { useTranslation } from "../hooks";
import { memo } from "react";

const TestPage = memo(() => {
    const { t } = useTranslation();

    const handleCrash = () => {
        shell.breeze.crash();
    };

    return (
        <flex gap={20}>
            <Text fontSize={24}>{t("test.title")}</Text>
            <flex gap={10}>
                <Button onClick={handleCrash}>
                    <Text fontSize={16}>{t("test.crash")}</Text>
                </Button>
            </flex>
        </flex>
    );
});

export default TestPage;
