import * as shell from "mshell";
import { theme_presets, animation_presets } from "./constants";
import { menu_controller } from "mshell";
import { useState, useEffect, memo } from "react";

export const useHoverActive = () => {
    const [isHovered, setIsHovered] = useState(false);
    const [isActive, setIsActive] = useState(false);

    const onMouseEnter = () => setIsHovered(true);
    const onMouseLeave = () => setIsHovered(false);
    const onMouseDown = () => setIsActive(true);
    const onMouseUp = () => setIsActive(false);

    return { isHovered, isActive, onMouseEnter, onMouseLeave, onMouseDown, onMouseUp };
};


export const showMenu = (callback: (ctl: menu_controller) => void) => {
    const menu = menu_controller.create_detached();
    callback(menu);
    menu.show_at_cursor();
}

// Utility functions for nested object manipulation
export const getNestedValue = (obj: any, path: string) => {
    return path.split('.').reduce((o, k) => o?.[k], obj);
};

export const setNestedValue = (obj: any, path: string, value: any) => {
    const keys = path.split('.');
    const last = keys.pop()!;
    const target = keys.reduce((o, k) => o[k] = o[k] || {}, obj);
    target[last] = value;
};

// Translation helper
export const useTranslation = () => {
    const currentLang = shell.breeze.user_language() === 'zh-CN' ? 'zh-CN' : 'en-US';
    const t = (key: string) => {
        const languages = {
            'zh-CN': {
                "管理 Breeze Shell": "管理 Breeze Shell",
                "插件市场": "插件市场",
                "加载中...": "加载中...",
                "更新中...": "更新中...",
                "安装中...": "安装中...",
                "新版本已下载，将于下次重启资源管理器生效": "新版本已下载，将于下次重启资源管理器生效",
                "更新失败: ": "更新失败: ",
                "插件安装成功: ": "插件安装成功: ",
                "版本: ": "版本: ",
                "作者: ": "作者: ",
                "删除": "删除",
                "Breeze 设置": "Breeze 设置",
                "优先加载插件": "优先加载插件",
                "调试控制台": "调试控制台",
                "垂直同步": "垂直同步",
                "忽略自绘菜单": "忽略自绘菜单",
                "向上展开时反向排列": "向上展开时反向排列",
                "尝试使用 Windows 11 圆角": "尝试使用 Windows 11 圆角",
                "亚克力背景效果": "亚克力背景效果",
                "主题": "主题",
                "动画": "动画",
                "当前源: ": "当前源: ",
                "插件": "插件",
                "插件源": "插件源",
                "换源": "换源",
                "请稍候": "请稍候",
                "切换源中...": "切换源中...",
                "加载失败": "加载失败",
                "网络错误": "网络错误",
                "切换源成功": "切换源成功"
            },
            'en-US': {}
        };
        return languages[currentLang][key] || key;
    };
    return { t, currentLang };
};

// Theme preset utilities
export const getAllSubkeys = (presets: any) => {
    if (!presets) return [];
    const keys = new Set();
    for (const v of Object.values(presets)) {
        if (v)
            for (const key of Object.keys(v)) {
                keys.add(key);
            }
    }
    return [...keys];
};

export const applyPreset = (preset: any, origin: any, presets: any) => {
    const allSubkeys = getAllSubkeys(presets);
    const newPreset = preset ? { ...preset } : {};
    for (let key in origin) {
        if (allSubkeys.includes(key)) continue;
        newPreset[key] = origin[key];
    }
    return newPreset;
};

export const checkPresetMatch = (current: any, preset: any) => {
    if (!current) return false;
    if (!preset) return false;
    return Object.keys(preset).every(key => JSON.stringify(current[key]) === JSON.stringify(preset[key]));
};

export const getCurrentPreset = (current: any, presets: any) => {
    if (!current) return "默认";
    for (const [name, preset] of Object.entries(presets)) {
        if (preset && checkPresetMatch(current, preset)) {
            return name;
        }
    }
    return "自定义";
};

// Config file operations
export const loadConfig = () => {
    const current_config_path = shell.breeze.data_directory() + '/config.json';
    const current_config = shell.fs.read(current_config_path);
    return JSON.parse(current_config);
};

export const saveConfig = (config: any) => {
    shell.fs.write(shell.breeze.data_directory() + '/config.json', JSON.stringify(config, null, 4));
};

// Plugin utilities
export const loadPlugins = () => {
    return shell.fs.readdir(shell.breeze.data_directory() + '/scripts')
        .map(v => v.split('/').pop())
        .filter(v => v.endsWith('.js') || v.endsWith('.disabled'))
        .map(v => v.replace('.js', '').replace('.disabled', ''));
};

export const togglePlugin = (name: string) => {
    const path = shell.breeze.data_directory() + '/scripts/' + name;
    if (shell.fs.exists(path + '.js')) {
        shell.fs.rename(path + '.js', path + '.js.disabled');
    } else if (shell.fs.exists(path + '.js.disabled')) {
        shell.fs.rename(path + '.js.disabled', path + '.js');
    }
};

export const deletePlugin = (name: string) => {
    const path = shell.breeze.data_directory() + '/scripts/' + name;
    if (shell.fs.exists(path + '.js')) {
        shell.fs.remove(path + '.js');
    }
    if (shell.fs.exists(path + '.js.disabled')) {
        shell.fs.remove(path + '.js.disabled');
    }
};

export const isPluginInstalled = (plugin: any) => {
    if (shell.fs.exists(shell.breeze.data_directory() + '/scripts/' + plugin.local_path)) {
        return shell.breeze.data_directory() + '/scripts/' + plugin.local_path;
    }
    if (shell.fs.exists(shell.breeze.data_directory() + '/scripts/' + plugin.local_path + '.disabled')) {
        return shell.breeze.data_directory() + '/scripts/' + plugin.local_path + '.disabled';
    }
    return null;
};

export const getPluginVersion = (installPath: string) => {
    const local_version_match = shell.fs.read(installPath).match(/\/\/ @version:\s*(.*)/);
    return local_version_match ? local_version_match[1] : '未安装';
};