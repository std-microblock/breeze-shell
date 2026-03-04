import { createContext } from "react";

export const UpdateDataContext = createContext<{ updateData: any; setUpdateData: (d: any) => void } | null>(null);
