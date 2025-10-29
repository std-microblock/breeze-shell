import { createContext } from "react";

// Context for context menu configuration
export const ContextMenuContext = createContext<{ config: any; update: (c: any) => void } | null>(null);

// Context for debug console state
export const DebugConsoleContext = createContext<{ value: boolean; update: (v: boolean) => void } | null>(null);

// Context for plugin load order
export const PluginLoadOrderContext = createContext<{ order: any[]; update: (o: any[]) => void } | null>(null);

// Context for update data
export const UpdateDataContext = createContext<{ updateData: any; setUpdateData: (d: any) => void } | null>(null);

// Context for notifications (error and loading messages)
export const NotificationContext = createContext<{
    errorMessage: string | null;
    setErrorMessage: (msg: string | null) => void;
    loadingMessage: string | null;
    setLoadingMessage: (msg: string | null) => void;
} | null>(null);

// Context for plugin source management
export const PluginSourceContext = createContext<{
    currentPluginSource: string;
    setCurrentPluginSource: (source: string) => void;
    cachedPluginIndex: any;
    setCachedPluginIndex: (index: any) => void;
} | null>(null);