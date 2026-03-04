export const getNestedValue = (obj: any, path: string) => {
    return path.split('.').reduce((o, k) => o?.[k], obj);
};

export const setNestedValue = (obj: any, path: string, value: any) => {
    const keys = path.split('.');
    const last = keys.pop()!;
    const target = keys.reduce((o, k) => o[k] = o[k] || {}, obj);
    target[last] = value;
};
