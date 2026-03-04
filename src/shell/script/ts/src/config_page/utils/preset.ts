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

export const checkPresetMatch = (current: any, preset: any, excludeKeys: string[] = []) => {
    if (!current) return false;
    if (!preset) return false;
    return Object.keys(preset).every(key => {
        if (excludeKeys.includes(key)) return true;
        return JSON.stringify(current[key]) === JSON.stringify(preset[key]);
    });
};

export const getCurrentPreset = (current: any, presets: any, excludeKeys: string[] = []) => {
    if (!current) return "默认";
    for (const [name, preset] of Object.entries(presets)) {
        if (preset && checkPresetMatch(current, preset, excludeKeys)) {
            return name;
        }
    }
    return "自定义";
};
