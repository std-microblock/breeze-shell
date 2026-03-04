import i18next from "../../i18n";

export const useTranslation = () => {
    const t = (key: string) => {
        return i18next.t(key);
    };
    return { t, currentLang: i18next.language };
};
