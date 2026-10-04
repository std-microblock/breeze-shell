export const parseVersion = (version: string): number[] =>
    (version || "")
        .replace(/^v/i, "")
        .split(".")
        .map(part => parseInt(part, 10) || 0);

export const compareVersions = (a: string, b: string): number => {
    const pa = parseVersion(a);
    const pb = parseVersion(b);
    for (let i = 0; i < 3; i++) {
        if (pa[i] !== pb[i]) {
            return pa[i] < pb[i] ? -1 : 1;
        }
    }
    return 0;
};
