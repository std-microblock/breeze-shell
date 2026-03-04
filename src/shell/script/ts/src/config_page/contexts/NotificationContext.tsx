import { createContext } from "react";

export const NotificationContext = createContext<{
    errorMessage: string | null;
    setErrorMessage: (msg: string | null) => void;
    loadingMessage: string | null;
    setLoadingMessage: (msg: string | null) => void;
} | null>(null);
