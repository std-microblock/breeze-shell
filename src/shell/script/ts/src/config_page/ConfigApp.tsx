import * as shell from "mshell";
import { WINDOW_WIDTH, WINDOW_HEIGHT, SIDEBAR_WIDTH } from "./constants";
import { loadConfig, saveConfig } from "./utils";
import {
    ContextMenuContext,
    DebugConsoleContext,
    PluginLoadOrderContext,
    UpdateDataContext,
    NotificationContext,
    PluginSourceContext
} from "./contexts";
import Sidebar from "./Sidebar";
import ContextMenuConfig from "./ContextMenuConfig";
import UpdatePage from "./UpdatePage";
import PluginStore from "./PluginStore";
import PluginConfig from "./PluginConfig";
import { useState, useEffect } from "react";

export const ConfigApp = () => {
    const [activePage, setActivePage] = useState('context-menu');
    const [contextMenuConfig, setContextMenuConfig] = useState<any>({});
    const [debugConsole, setDebugConsole] = useState<boolean>(false);
    const [pluginLoadOrder, setPluginLoadOrder] = useState<any[]>([]);
    const [updateData, setUpdateData] = useState<any>(null);
    const [config, setConfig] = useState<any>({});
    const [errorMessage, setErrorMessage] = useState<string | null>(null);
    const [loadingMessage, setLoadingMessage] = useState<string | null>(null);
    const [currentPluginSource, setCurrentPluginSource] = useState<string>('Enlysure');
    const [cachedPluginIndex, setCachedPluginIndex] = useState<any>(null);

    useEffect(() => {
        const current_config_path = shell.breeze.data_directory() + '/config.json';
        const current_config = shell.fs.read(current_config_path);
        const parsed = JSON.parse(current_config);
        setConfig(parsed);
        setContextMenuConfig(parsed.context_menu || {});
        setDebugConsole(parsed.debug_console || false);
        setPluginLoadOrder(parsed.plugin_load_order || []);
    }, []);

    const updateContextMenu = (newConfig: any) => {
        setContextMenuConfig(newConfig);
        const newGlobal = { ...config, context_menu: newConfig };
        setConfig(newGlobal);
        saveConfig(newGlobal);
    };

    const updateDebugConsole = (value: boolean) => {
        setDebugConsole(value);
        const newGlobal = { ...config, debug_console: value };
        setConfig(newGlobal);
        saveConfig(newGlobal);
    };

    const updatePluginLoadOrder = (order: any[]) => {
        setPluginLoadOrder(order);
        const newGlobal = { ...config, plugin_load_order: order };
        setConfig(newGlobal);
        saveConfig(newGlobal);
    };

    return (
        <ContextMenuContext.Provider value={{ config: contextMenuConfig, update: updateContextMenu }}>
            <DebugConsoleContext.Provider value={{ value: debugConsole, update: updateDebugConsole }}>
                <PluginLoadOrderContext.Provider value={{ order: pluginLoadOrder, update: updatePluginLoadOrder }}>
                    <UpdateDataContext.Provider value={{ updateData, setUpdateData }}>
                        <NotificationContext.Provider value={{
                            errorMessage,
                            setErrorMessage,
                            loadingMessage,
                            setLoadingMessage
                        }}>
                            <PluginSourceContext.Provider value={{
                                currentPluginSource,
                                setCurrentPluginSource,
                                cachedPluginIndex,
                                setCachedPluginIndex
                            }}>
                                <flex horizontal width={WINDOW_WIDTH} height={WINDOW_HEIGHT} autoSize={false} gap={10}>
                                    <Sidebar
                                        activePage={activePage}
                                        setActivePage={setActivePage}
                                        sidebarWidth={SIDEBAR_WIDTH}
                                        windowHeight={WINDOW_HEIGHT}
                                    />
                                    <flex padding={20}>
                                        {activePage === 'context-menu' && <ContextMenuConfig />}
                                        {activePage === 'update' && <UpdatePage />}
                                        {activePage === 'plugin-store' && <PluginStore />}
                                        {activePage === 'plugin-config' && <PluginConfig />}
                                    </flex>
                                </flex>
                            </PluginSourceContext.Provider>
                        </NotificationContext.Provider>
                    </UpdateDataContext.Provider>
                </PluginLoadOrderContext.Provider>
            </DebugConsoleContext.Provider>
        </ContextMenuContext.Provider>
    );
};

export default ConfigApp;