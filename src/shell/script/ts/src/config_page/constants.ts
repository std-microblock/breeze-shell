import * as shell from "mshell";

export const PLUGIN_SOURCES = {
    'Github Raw': 'https://raw.githubusercontent.com/breeze-shell/plugins-packed/refs/heads/main/',
    'Enlysure': 'https://breeze.enlysure.com/',
    'Enlysure Shanghai': 'https://breeze-c.enlysure.com/'
};

export const ICON_CONTEXT_MENU = `<svg viewBox="0 0 24 24"><path d="M3 18h18v-2H3v2zm0-5h18v-2H3v2zm0-7v2h18V6H3z"/></svg>`;
export const ICON_UPDATE = `<svg viewBox="0 0 24 24"><path d="M17 3H5c-1.11 0-2 .9-2 2v14c0 1.1.89 2 2 2h14c1.1 0 2-.9 2-2V7l-4-4zm-5 16c-1.66 0-3-1.34-3-3s1.34-3 3-3 3 1.34 3 3-1.34 3-3 3zm3-10H5V7h10v2z"/></svg>`;
export const ICON_PLUGIN_STORE = `<svg viewBox="0 0 24 24"><path d="M7 2v11h3v9l7-12h-4l4-8z"/></svg>`;
export const ICON_PLUGIN_CONFIG = `<svg viewBox="0 0 24 24"><path d="M19.14,12.94c0.04-0.3,0.06-0.61,0.06-0.94c0-0.32-0.02-0.64-0.07-0.94l2.03-1.58c0.18-0.14,0.23-0.41,0.12-0.61 l-1.92-3.32c-0.12-0.22-0.37-0.29-0.59-0.22l-2.39,0.96c-0.5-0.38-1.03-0.7-1.62-0.94L14.4,2.81c-0.04-0.24-0.24-0.41-0.48-0.41 h-3.84c-0.24,0-0.43,0.17-0.47,0.41L9.25,5.35C8.66,5.59,8.12,5.92,7.63,6.29L5.24,5.33c-0.22-0.08-0.47,0-0.59,0.22L2.74,8.87 C2.62,9.08,2.66,9.34,2.86,9.48l2.03,1.58C4.84,11.36,4.82,11.69,4.82,12s0.02,0.64,0.07,0.94l-2.03,1.58 c-0.18,0.14-0.23,0.41-0.12,0.61l1.92,3.32c0.12,0.22,0.37,0.29,0.59,0.22l2.39-0.96c0.5,0.38,1.03,0.7,1.62,0.94l0.36,2.54 c0.05,0.24,0.24,0.41,0.48,0.41h3.84c0.24,0,0.43-0.17,0.47-0.41l0.36-2.54c0.59-0.24,1.13-0.56,1.62-0.94l2.39,0.96 c0.22,0.08,0.47,0,0.59-0.22l1.92-3.32c0.12-0.22,0.07-0.47-0.12-0.61L19.14,12.94z M12,15.6c-1.98,0-3.6-1.62-3.6-3.6 s1.62-3.6,3.6-3.6s3.6,1.62,3.6,3.6S13.98,15.6,12,15.6z"/></svg>`;
export const ICON_MORE_VERT = `<svg viewBox="0 0 24 24"><path d="M12 8c1.1 0 2-.9 2-2s-.9-2-2-2-2 .9-2 2 .9 2 2 2zm0 2c-1.1 0-2 .9-2 2s.9 2 2 2 2-.9 2-2-.9-2-2-2zm0 6c-1.1 0-2 .9-2 2s.9 2 2 2 2-.9 2-2-.9-2-2-2z"/></svg>`;
export const ICON_BREEZE = `<svg focusable="false" aria-hidden="true" viewBox="0 0 24 24"><path d="M14.5 17c0 1.65-1.35 3-3 3s-3-1.35-3-3h2c0 .55.45 1 1 1s1-.45 1-1-.45-1-1-1H2v-2h9.5c1.65 0 3 1.35 3 3M19 6.5C19 4.57 17.43 3 15.5 3S12 4.57 12 6.5h2c0-.83.67-1.5 1.5-1.5s1.5.67 1.5 1.5S16.33 8 15.5 8H2v2h13.5c1.93 0 3.5-1.57 3.5-3.5m-.5 4.5H2v2h16.5c.83 0 1.5.67 1.5 1.5s-.67 1.5-1.5 1.5v2c1.93 0 3.5-1.57 3.5-3.5S20.43 11 18.5 11"></path></svg>`;
export const ICON_TEST = `<svg viewBox="0 0 24 24"><path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm1 15h-2v-2h2v2zm0-4h-2V7h2v6z"/></svg>`;


// Window dimensions
export const WINDOW_WIDTH = 800;
export const WINDOW_HEIGHT = 600;
export const SIDEBAR_WIDTH = 170;

// Theme presets
export const theme_presets = {
    "default": null,
    "compact": {
        radius: 4.0,
        item_height: 20.0,
        item_gap: 2.0,
        item_radius: 3.0,
        margin: 4.0,
        padding: 4.0,
        text_padding: 6.0,
        icon_padding: 3.0,
        right_icon_padding: 16.0,
        multibutton_line_gap: -4.0
    },
    "loose": {
        radius: 6.0,
        item_height: 24.0,
        item_gap: 4.0,
        item_radius: 8.0,
        margin: 6.0,
        padding: 6.0,
        text_padding: 8.0,
        icon_padding: 4.0,
        right_icon_padding: 20.0,
        multibutton_line_gap: -6.0
    },
    "rounded": {
        radius: 12.0,
        item_radius: 12.0
    },
    "square": {
        radius: 0.0,
        item_radius: 0.0
    }
};

// Animation presets
export const anim_none = {
    easing: "mutation",
};

export const animation_presets = {
    "default": null,
    "fast": {
        "item": {
            "opacity": {
                "delay_scale": 0
            },
            "width": anim_none,
            "x": anim_none,
        },
        "submenu_bg": {
            "opacity": {
                "delay_scale": 0,
                "duration": 100
            }
        },
        "main_bg": {
            "opacity": anim_none,
        }
    },
    "none": {
        "item": {
            "opacity": anim_none,
            "width": anim_none,
            "x": anim_none,
            "y": anim_none
        },
        "submenu_bg": {
            "opacity": anim_none,
            "x": anim_none,
            "y": anim_none,
            "w": anim_none,
            "h": anim_none
        },
        "main_bg": {
            "opacity": anim_none,
            "x": anim_none,
            "y": anim_none,
            "w": anim_none,
            "h": anim_none
        }
    }
};