import * as shell from "mshell";
import { showMenu } from "./utils";
import { memo, useEffect, useContext, useState } from "react";
import { SidebarItem, Text, iconElement } from "./components";
import { fluent } from "./components/Fluent";
import {
    ICON_BREEZE,
    ICON_CONTEXT_MENU,
    ICON_HISTORY,
    ICON_STORE,
    ICON_EXTENSION,
    ICON_BUG,
    ICON_LOGS,
    ICON_PROBLEMS,
    ICON_CLOUD,
    ICON_CHECK,
    PLUGIN_SOURCES
} from "./constants";
import { UpdateDataContext, NotificationContext, PluginSourceContext } from "./contexts";
import { useTranslation } from "./hooks";

const Toast = ({ text, severity }: { text: string; severity: "error" | "info" }) => {
    const c = fluent();
    const bg = severity === "error" ? (c.light ? "#FDE7E9F2" : "#442726F2") : (c.light ? "#E5F1FBF2" : "#1F3346F2");
    const fg = severity === "error" ? (c.light ? "#C42B1CFF" : "#FF99A4FF") : c.accent;
    return (
        <flex horizontal gap={8} padding={10} borderRadius={6} backgroundColor={bg} borderColor={c.cardStroke} borderWidth={1} alignItems="center">
            <flex width={6} height={6} borderRadius={3} backgroundColor={fg} />
            <Text fontSize={12} color={c.text} maxWidth={170}>{text}</Text>
        </flex>
    );
};

const Sidebar = memo(({ activePage, setActivePage, sidebarWidth, windowHeight }: {
    activePage: string;
    setActivePage: (page: string) => void;
    sidebarWidth: number;
    windowHeight: number;
}) => {
    const { t } = useTranslation();
    const c = fluent();
    const { setUpdateData } = useContext(UpdateDataContext)!;
    const { errorMessage, setErrorMessage, loadingMessage, setLoadingMessage } = useContext(NotificationContext)!;
    const { currentPluginSource, setCurrentPluginSource, setCachedPluginIndex } = useContext(PluginSourceContext)!;

    useEffect(() => {
        if (errorMessage) {
            const timer = setTimeout(() => setErrorMessage(null), 3000);
            return () => clearTimeout(timer);
        }
    }, [errorMessage, setErrorMessage]);

    useEffect(() => {
        setLoadingMessage(t("common.switching"));
        shell.network.get_async(PLUGIN_SOURCES[currentPluginSource] + 'plugins-index.json', (data: string) => {
            setCachedPluginIndex(data);
            setUpdateData(JSON.parse(data));
            setLoadingMessage(null);
        }, (e: any) => {
            shell.println('Failed to fetch update data:', e);
            setErrorMessage(t("common.loadFailed"));
            setLoadingMessage(null);
        });
    }, [currentPluginSource]);

    const [problemCount, setProblemCount] = useState(0);
    useEffect(() => {
        const refresh = () => setProblemCount(shell.diagnostics.problems().length);
        refresh();
        const id = setInterval(refresh, 1000);
        return () => clearInterval(id);
    }, []);

    const item = (page: string, icon: string, label: string, badge?: number) => (
        <SidebarItem key={page} onClick={() => setActivePage(page)} icon={icon} isActive={activePage === page} badge={badge}>{label}</SidebarItem>
    );

    const pickSource = () => showMenu(menu => {
        for (const sourceName of Object.keys(PLUGIN_SOURCES)) {
            menu.append_menu({
                name: sourceName,
                action() {
                    setCurrentPluginSource(sourceName);
                    menu.close();
                },
                icon_svg: sourceName === currentPluginSource
                    ? ICON_CHECK.replace("<svg ", `<svg fill="${c.light ? "#000000" : "#FFFFFF"}" `)
                    : undefined
            });
        }
    });

    return (
        <flex width={sidebarWidth} height={windowHeight} paddingLeft={8} paddingRight={8} paddingTop={8} paddingBottom={10} gap={4} alignItems="stretch">
            <flex horizontal alignItems="center" gap={10} paddingLeft={14} paddingTop={10} paddingBottom={14}>
                {iconElement(ICON_BREEZE, 20, c.accent)}
                <Text fontSize={15} fontWeight={600} color={c.text}>Breeze</Text>
            </flex>
            {item('context-menu', ICON_CONTEXT_MENU, t('sidebar.mainConfig'))}
            {item('update', ICON_HISTORY, t('sidebar.update'))}
            {item('plugin-store', ICON_STORE, t('sidebar.pluginStore'))}
            {item('plugin-config', ICON_EXTENSION, t('sidebar.pluginConfig'))}
            <flex paddingTop={6} paddingBottom={6} paddingLeft={8} paddingRight={8} alignItems="stretch">
                <flex height={1} backgroundColor={c.divider} />
            </flex>
            {item('problems', ICON_PROBLEMS, t('sidebar.problems'), problemCount)}
            {item('logs', ICON_LOGS, t('sidebar.logs'))}
            {item('test', ICON_BUG, t('test.title'))}
            <spacer />
            {errorMessage && <Toast text={errorMessage} severity="error" />}
            {loadingMessage && <Toast text={loadingMessage} severity="info" />}
            <SidebarItem onClick={pickSource} icon={ICON_CLOUD} isActive={false} trailing={currentPluginSource}>{t("sidebar.updateSource")}</SidebarItem>
        </flex>
    );
});

export default Sidebar;
