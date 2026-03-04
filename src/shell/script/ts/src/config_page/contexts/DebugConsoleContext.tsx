import { createContext } from "react";

export const DebugConsoleContext = createContext<{ value: boolean; update: (v: boolean) => void } | null>(null);
