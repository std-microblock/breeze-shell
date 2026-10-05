const EPSILON = 1e-3;

const same = (a: any, b: any) =>
    typeof a === "number" && typeof b === "number" ? Math.abs(a - b) < EPSILON : a === b;

const isObject = (v: any) => v !== null && typeof v === "object" && !Array.isArray(v);

export const presetKeys = (presets: Record<string, any>) => {
    const keys = new Set<string>();
    for (const p of Object.values(presets))
        if (p) Object.keys(p).forEach(k => keys.add(k));
    return [...keys];
};

export const resolveFlat = (value: any, defaults: any, keys: string[]) =>
    Object.fromEntries(keys.map(k => [k, value?.[k] ?? defaults?.[k]]));

export const matchThemePreset = (theme: any, defaults: any, presets: Record<string, any>) => {
    const keys = presetKeys(presets);
    const current = resolveFlat(theme, defaults, keys);
    for (const [name, preset] of Object.entries(presets)) {
        const target = resolveFlat(preset ?? {}, defaults, keys);
        if (keys.every(k => same(current[k], target[k]))) return name;
    }
    return "custom";
};

export const applyThemePreset = (theme: any, preset: any, presets: Record<string, any>) => {
    const keys = new Set(presetKeys(presets));
    const next: Record<string, any> = {};
    for (const [k, v] of Object.entries(theme ?? {}))
        if (!keys.has(k)) next[k] = v;
    return { ...next, ...(preset ?? {}) };
};

export const flatten = (value: any, prefix = "", out: Record<string, any> = {}) => {
    if (!isObject(value)) return out;
    for (const [k, v] of Object.entries(value)) {
        const path = prefix ? `${prefix}.${k}` : k;
        if (isObject(v)) flatten(v, path, out);
        else out[path] = v;
    }
    return out;
};

export const matchAnimationPreset = (animation: any, defaults: any, presets: Record<string, any>) => {
    const base = flatten(defaults);
    const current = { ...base, ...flatten(animation) };
    for (const [name, preset] of Object.entries(presets)) {
        const target = { ...base, ...flatten(preset ?? {}) };
        const paths = new Set([...Object.keys(current), ...Object.keys(target)]);
        if ([...paths].every(p => same(current[p], target[p]))) return name;
    }
    return "custom";
};

export const getPath = (obj: any, path: string) =>
    path.split(".").reduce((o, k) => (o == null ? undefined : o[k]), obj);

export const withPath = (obj: any, path: string, value: any, defaults?: any): any => {
    const [head, ...rest] = path.split(".");
    const base = isObject(obj) ? { ...obj } : {};
    if (rest.length === 0) {
        if (value === undefined || (defaults !== undefined && same(value, defaults?.[head])))
            delete base[head];
        else
            base[head] = value;
    } else {
        const child = withPath(base[head], rest.join("."), value, defaults?.[head]);
        if (Object.keys(child).length === 0) delete base[head];
        else base[head] = child;
    }
    return base;
};
