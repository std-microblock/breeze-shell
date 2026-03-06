import i18next from 'i18next';
import { initReactI18next } from 'react-i18next';
import * as shell from 'mshell';
import zhCN from './locales/zh-CN.json';
import enUS from './locales/en-US.json';

i18next
    .use(initReactI18next)
    .init({
        lng: shell.breeze.user_language() || 'zh-CN',
        fallbackLng: 'zh-CN',
        resources: {
            'zh-CN': {
                translation: zhCN
            },
            'en-US': {
                translation: enUS
            }
        },
        interpolation: {
            escapeValue: false
        }
    });

export const changeLanguage = (lng: string) => {
    i18next.changeLanguage(lng);
};

export const getCurrentLanguage = () => {
    return i18next.language;
};

export default i18next;

