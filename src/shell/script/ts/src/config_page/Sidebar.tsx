import * as shell from "mshell";
import { showMenu } from "./utils";
import { memo, useEffect, useContext } from "react";
import { Button, SidebarItem, Text, iconElement } from "./components";
import {
    ICON_BREEZE,
    ICON_CONTEXT_MENU,
    ICON_UPDATE,
    ICON_PLUGIN_STORE,
    ICON_PLUGIN_CONFIG,
    PLUGIN_SOURCES
} from "./constants";
import { UpdateDataContext, NotificationContext, PluginSourceContext } from "./contexts";
import { useTranslation } from "./utils";

const Sidebar = memo(({
    activePage,
    setActivePage,
    sidebarWidth,
    windowHeight
}: {
    activePage: string;
    setActivePage: (page: string) => void;
    sidebarWidth: number;
    windowHeight: number;
}) => {
    const { t } = useTranslation();
    const { updateData, setUpdateData } = useContext(UpdateDataContext)!;
    const { errorMessage, setErrorMessage, loadingMessage, setLoadingMessage } = useContext(NotificationContext)!;
    const { currentPluginSource, setCurrentPluginSource, cachedPluginIndex, setCachedPluginIndex } = useContext(PluginSourceContext)!;

    useEffect(() => {
        if (errorMessage) {
            const timer = setTimeout(() => {
                setErrorMessage(null);
            }, 3000);
            return () => clearTimeout(timer);
        }
    }, [errorMessage, setErrorMessage]);

    const handleSourceChange = (sourceName: string) => {
        setCurrentPluginSource(sourceName);
        setCachedPluginIndex(null);
        setLoadingMessage(t("切换源中..."));

        shell.network.get_async(PLUGIN_SOURCES[sourceName] + 'plugins-index.json', (data: string) => {
            setCachedPluginIndex(data);
            setUpdateData(JSON.parse(data));
            setLoadingMessage(null);
        }, (e: any) => {
            shell.println('Failed to fetch update data:', e);
            setErrorMessage(t("加载失败"));
            setLoadingMessage(null);
        });
    };

    useEffect(() => {
        handleSourceChange(currentPluginSource);
    }, [currentPluginSource]);

    return (
        <flex
            width={sidebarWidth}
            height={windowHeight}
            backgroundColor={shell.breeze.is_light_theme() ? '#f0f0f077' : '#40404077'}
            padding={10}
            gap={10}
            alignItems="stretch"
            autoSize={false}
        >
            <flex horizontal alignItems="center" gap={3} padding={10}>
                {iconElement(ICON_BREEZE, 24)}
                <Text fontSize={18}>Breeze</Text>
            </flex>
            <SidebarItem onClick={() => setActivePage('context-menu')} icon={ICON_CONTEXT_MENU} isActive={activePage === 'context-menu'}>主配置</SidebarItem>
            <SidebarItem onClick={() => setActivePage('update')} icon={ICON_UPDATE} isActive={activePage === 'update'}>更新</SidebarItem>
            <SidebarItem onClick={() => setActivePage('plugin-store')} icon={ICON_PLUGIN_STORE} isActive={activePage === 'plugin-store'}>插件商店</SidebarItem>
            <SidebarItem onClick={() => setActivePage('plugin-config')} icon={ICON_PLUGIN_CONFIG} isActive={activePage === 'plugin-config'}>插件配置</SidebarItem>
            <spacer />

            {/* 错误提示 */}
            {errorMessage && (
                <flex
                    backgroundColor="#FF4444AA"
                    padding={8}
                    borderRadius={6}
                    paddingBottom={5}
                >
                    <text
                        text={errorMessage}
                        fontSize={12}
                        color="#FFFFFFFF"
                    />
                </flex>
            )}

            {/* 加载提示 */}
            {loadingMessage && (
                <flex
                    backgroundColor="#0078D4AA"
                    padding={8}
                    borderRadius={6}
                    paddingBottom={5}
                >
                    <text
                        text={loadingMessage}
                        fontSize={12}
                        color="#FFFFFFFF"
                    />
                </flex>
            )}

            <Button onClick={() => {
                showMenu(menu => {
                    Object.keys(PLUGIN_SOURCES).forEach(sourceName => {
                        menu.append_menu({
                            name: sourceName,
                            action() {
                                handleSourceChange(sourceName);
                                menu.close();
                            },
                            icon_svg: sourceName === currentPluginSource ? `<svg viewBox="0 0 24 24"><path fill="${shell.breeze.is_light_theme() ? '#000000ff' : '#ffffffff'}" d="M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z"/></svg>` : undefined
                        });
                    });
                });
            }}>
                <Text fontSize={12}>{`${t("更新源")} - ${currentPluginSource}`}</Text>
            </Button>
        </flex>
    );
});

export default Sidebar;