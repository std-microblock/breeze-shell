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
