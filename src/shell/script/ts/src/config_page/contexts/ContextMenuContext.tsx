import { createContext } from "react";

export const ContextMenuContext = createContext<{
    config: any;
    defaultConfig: any;
    update: (c: any) => void;
} | null>(null);
