import { createContext } from "react";

export const PluginLoadOrderContext = createContext<{ order: any[]; update: (o: any[]) => void } | null>(null);
