import { createContext } from "react";

export const PluginSourceContext = createContext<{
    currentPluginSource: string;
    setCurrentPluginSource: (source: string) => void;
    cachedPluginIndex: any;
    setCachedPluginIndex: (index: any) => void;
} | null>(null);
