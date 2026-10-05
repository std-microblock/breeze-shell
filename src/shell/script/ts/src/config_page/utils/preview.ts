type Palette = {
    backdrop: string; backdropTo: string; menu: string; stroke: string; hover: string;
    text: string; textDim: string; accent: string; shadow: string;
};

const palette = (light: boolean): Palette => light
    ? { backdrop: "#DCE8F5", backdropTo: "#EEE4F3", menu: "#FBFBFB", stroke: "#000000", hover: "#000000", text: "#1B1B1B", textDim: "#6E6E6E", accent: "#005FB8", shadow: "#000000" }
    : { backdrop: "#1B2A3A", backdropTo: "#2A2034", menu: "#2C2C2C", stroke: "#FFFFFF", hover: "#FFFFFF", text: "#F2F2F2", textDim: "#A0A0A0", accent: "#60CDFF", shadow: "#000000" };

const n = (v: number) => (Math.round(v * 100) / 100).toString();

const frame = (w: number, h: number, p: Palette, body: string, id: string) =>
    `<svg xmlns="http://www.w3.org/2000/svg" width="${w}" height="${h}" viewBox="0 0 ${w} ${h}">` +
    `<defs><linearGradient id="${id}" x1="0%" y1="0%" x2="100%" y2="100%">` +
    `<stop offset="0" stop-color="${p.backdrop}"/><stop offset="1" stop-color="${p.backdropTo}"/></linearGradient></defs>` +
    `<rect x="0" y="0" width="${w}" height="${h}" rx="6" fill="url(#${id})"/>${body}</svg>`;

export const themePreviewSvg = (t: Record<string, number>, light: boolean, w = 176, h = 112) => {
    const p = palette(light);
    const s = Math.min((h * 0.8) / 120, (w * 0.8) / 168);
    const items = 4;
    const ih = t.item_height * s, gap = t.item_gap * s, pad = t.padding * s, margin = t.margin * s;
    const mw = 168 * s;
    const mh = pad * 2 + items * ih + (items - 1) * gap;
    const mx = (w - mw) / 2, my = (h - mh) / 2;
    const r = Math.min(t.radius * s, mh / 2), ir = Math.min(t.item_radius * s, ih / 2);
    const icon = Math.max(4, Math.min(ih - 4, 9));
    const bars = [0.62, 0.78, 0.5, 0.68];
    let body = `<rect x="${n(mx + 1)}" y="${n(my + 3)}" width="${n(mw)}" height="${n(mh)}" rx="${n(r)}" fill="${p.shadow}" fill-opacity="0.16"/>`;
    body += `<rect x="${n(mx)}" y="${n(my)}" width="${n(mw)}" height="${n(mh)}" rx="${n(r)}" fill="${p.menu}" stroke="${p.stroke}" stroke-opacity="0.12" stroke-width="0.8"/>`;
    for (let i = 0; i < items; i++) {
        const ix = mx + margin, iy = my + pad + i * (ih + gap), iw = mw - margin * 2;
        if (i === 1)
            body += `<rect x="${n(ix)}" y="${n(iy)}" width="${n(iw)}" height="${n(ih)}" rx="${n(ir)}" fill="${p.hover}" fill-opacity="0.09"/>`;
        const cx = ix + t.icon_padding * s * 2;
        const cy = iy + ih / 2;
        body += `<rect x="${n(cx)}" y="${n(cy - icon / 2)}" width="${n(icon)}" height="${n(icon)}" rx="${n(icon / 4)}" fill="${p.accent}" fill-opacity="${i === 1 ? 1 : 0.75}"/>`;
        const tx = cx + icon + t.text_padding * s;
        const right = ix + iw - Math.max(t.right_icon_padding * s, 4) - (i === 2 ? 6 : 0);
        const tw = Math.max(8, (right - tx) * bars[i]);
        const th = Math.max(2.5, Math.min(ih * 0.3, 5));
        body += `<rect x="${n(tx)}" y="${n(cy - th / 2)}" width="${n(tw)}" height="${n(th)}" rx="${n(th / 2)}" fill="${i === 1 ? p.text : p.textDim}"/>`;
        if (i === 2) {
            const ax = ix + iw - t.right_icon_padding * s * 0.6 - 3;
            body += `<path d="M${n(ax)} ${n(cy - 3)} L${n(ax + 3)} ${n(cy)} L${n(ax)} ${n(cy + 3)}" fill="none" stroke="${p.textDim}" stroke-width="1.2" stroke-linecap="round" stroke-linejoin="round"/>`;
        }
    }
    return frame(w, h, p, body, "bg");
};

export type AnimationPreviewKind = "default" | "fast" | "none" | "custom";

export const animationPreviewSvg = (kind: AnimationPreviewKind, light: boolean, w = 176, h = 112) => {
    const p = palette(light);
    const rows = 4;
    const gap = 4;
    const mw = w * 0.56, ih = Math.max(8, (h * 0.72 - 16 - gap * (rows - 1)) / rows);
    const mh = 8 * 2 + rows * ih + (rows - 1) * gap;
    const mx = w * 0.07, my = (h - mh) / 2;
    const bar = mw - 34;
    let body = `<rect x="${n(mx)}" y="${n(my)}" width="${n(mw)}" height="${n(mh)}" rx="6" fill="${p.menu}" stroke="${p.stroke}" stroke-opacity="0.12" stroke-width="0.8"/>`;
    for (let i = 0; i < rows; i++) {
        const iy = my + 8 + i * (ih + gap) + ih / 2;
        const lag = kind === "default" ? i * 4 : kind === "custom" ? i * 2 : 0;
        const trail = kind === "none" ? 0 : kind === "fast" ? 2 : 4;
        for (let k = trail; k >= 1; k--) {
            const off = lag + k * (kind === "fast" ? 3 : 4);
            body += `<rect x="${n(mx + 10 + off)}" y="${n(iy - 2.5)}" width="${n(Math.max(4, bar - off))}" height="5" rx="2.5" fill="${p.accent}" fill-opacity="${n(0.1 + 0.08 * (trail - k))}"/>`;
        }
        body += `<rect x="${n(mx + 10)}" y="${n(iy - 2.5)}" width="${n(bar - (i % 2) * 12)}" height="5" rx="2.5" fill="${i === 0 ? p.accent : p.textDim}"/>`;
    }
    const gx = mx + mw + w * 0.07, gw = w - gx - w * 0.07, gy = my + 6, gh = mh - 12;
    let curve: string;
    if (kind === "none")
        curve = `M${gx} ${n(gy + gh)} L${gx} ${gy} L${gx + gw} ${gy}`;
    else if (kind === "fast")
        curve = `M${gx} ${n(gy + gh)} C${n(gx + gw * 0.15)} ${gy} ${n(gx + gw * 0.3)} ${gy} ${gx + gw} ${gy}`;
    else if (kind === "custom")
        curve = `M${gx} ${n(gy + gh)} C${n(gx + gw * 0.2)} ${n(gy + gh * 0.2)} ${n(gx + gw * 0.5)} ${n(gy + gh * 0.9)} ${n(gx + gw * 0.7)} ${n(gy + gh * 0.35)} S${gx + gw} ${gy} ${gx + gw} ${gy}`;
    else
        curve = `M${gx} ${n(gy + gh)} C${n(gx + gw * 0.5)} ${n(gy + gh)} ${n(gx + gw * 0.5)} ${gy} ${gx + gw} ${gy}`;
    body += `<path d="M${gx} ${gy} L${gx} ${n(gy + gh)} L${gx + gw} ${n(gy + gh)}" fill="none" stroke="${p.textDim}" stroke-opacity="0.5" stroke-width="1"/>`;
    body += `<path d="${curve}" fill="none" stroke="${p.accent}" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>`;
    return frame(w, h, p, body, "bg");
};

const EASINGS: Record<string, (t: number) => number> = {
    linear: t => t,
    ease_in: t => t * t,
    ease_out: t => 1 - (1 - t) * (1 - t),
    ease_in_out: t => 0.5 * Math.sin(t * Math.PI - Math.PI / 2) + 0.5,
};

export const easingCurveSvg = (easing: string, color: string, dim: string, w = 40, h = 28) => {
    const pad = 3;
    const fn = EASINGS[easing];
    let d: string;
    if (!fn) {
        d = `M${pad} ${h - pad} L${pad} ${pad} L${w - pad} ${pad}`;
    } else {
        const pts: string[] = [];
        for (let i = 0; i <= 24; i++) {
            const t = i / 24;
            pts.push(`${n(pad + t * (w - pad * 2))} ${n(h - pad - fn(t) * (h - pad * 2))}`);
        }
        d = `M${pts.join(" L")}`;
    }
    return `<svg xmlns="http://www.w3.org/2000/svg" width="${w}" height="${h}" viewBox="0 0 ${w} ${h}">` +
        `<rect x="0.5" y="0.5" width="${w - 1}" height="${h - 1}" rx="4" fill="none" stroke="${dim}" stroke-opacity="0.35"/>` +
        `<path d="${d}" fill="none" stroke="${color}" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"/></svg>`;
};
