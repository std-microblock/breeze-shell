import { createContext } from "react";

export const AppConfigContext = createContext<{
    config: any;
    updateConfig: (updater: (config: any) => any) => void;
} | null>(null);
