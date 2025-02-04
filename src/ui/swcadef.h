#pragma once
#define NOMINMAX
#include "windows.h"
#include <dwmapi.h>

// Values and names extracted from twinui.pdb (April 2018 update), documented by
// toying with the API. Output from DIA2Dump above the definitions.

// Enum           : ACCENT_STATE, Type: int
// Data           :   constant 0x0, Constant, Type: int, ACCENT_DISABLED
// Data           :   constant 0x1, Constant, Type: int, ACCENT_ENABLE_GRADIENT
// Data           :   constant 0x2, Constant, Type: int,
// ACCENT_ENABLE_TRANSPARENTGRADIENT Data           :   constant 0x3, Constant,
// Type: int, ACCENT_ENABLE_BLURBEHIND Data           :   constant 0x4,
// Constant, Type: int, ACCENT_ENABLE_ACRYLICBLURBEHIND Data           :
// constant 0x5, Constant, Type: int, ACCENT_ENABLE_HOSTBACKDROP Data : constant
// 0x6, Constant, Type: int, ACCENT_INVALID_STATE
enum ACCENT_STATE : INT { // Affects the rendering of the background of a
                          // window.
  ACCENT_DISABLED = 0,    // Default value. Background is black.
  ACCENT_ENABLE_GRADIENT =
      1, // Background is GradientColor, alpha channel ignored.
  ACCENT_ENABLE_TRANSPARENTGRADIENT = 2, // Background is GradientColor.
  ACCENT_ENABLE_BLURBEHIND =
      3, // Background is GradientColor, with blur effect.
  ACCENT_ENABLE_ACRYLICBLURBEHIND =
      4, // Background is GradientColor, with acrylic blur effect.
  ACCENT_ENABLE_HOSTBACKDROP = 5, // Unknown.
  ACCENT_INVALID_STATE =
      6 // Unknown. Seems to draw background fully transparent.
};

enum Flags : UINT {
  NoneBorder = 0x00,
  GradientColor = 0x02,
  Luminosity = 0x02,
  LeftBorder = 0x20,
  TopBorder = 0x40,
  RightBorder = 0x80,
  BottomBorder = 0x100,
  AllBorder = (LeftBorder | TopBorder | RightBorder | BottomBorder),

  AllowSetWindowRgn = 0x10, // 0x10, 0xFE; 16, 25, 140
  FullScreen = 0xFF,
  FullScreen1 = 0xFFFFFFFF
};

inline constexpr DWORD ARGB(DWORD a, DWORD r, DWORD g, DWORD b) {
  return (a << 24) | (r << 16) | (g << 8) | b;
}

// UserDefinedType: ACCENT_POLICY
// Data           :   this+0x0, Member, Type: enum ACCENT_STATE, AccentState
// Data           :   this+0x4, Member, Type: unsigned int, AccentFlags
// Data           :   this+0x8, Member, Type: unsigned long, GradientColor
// Data           :   this+0xC, Member, Type: long, AnimationId
struct ACCENT_POLICY {      // Determines how a window's background is rendered.
  ACCENT_STATE AccentState; // Background effect.
  UINT AccentFlags; // Flags. Set to 2 to tell GradientColor is used, rest is
                    // unknown.
  COLORREF GradientColor; // Background color.
  LONG AnimationId;       // Unknown
};

// Enum           : WINDOWCOMPOSITIONATTRIB, Type: int
// Data           :   constant 0x0, Constant, Type: int, WCA_UNDEFINED
// Data           :   constant 0x1, Constant, Type: int, WCA_NCRENDERING_ENABLED
// Data           :   constant 0x2, Constant, Type: int, WCA_NCRENDERING_POLICY
// Data           :   constant 0x3, Constant, Type: int,
// WCA_TRANSITIONS_FORCEDISABLED Data           :   constant 0x4, Constant,
// Type: int, WCA_ALLOW_NCPAINT Data           :   constant 0x5, Constant, Type:
// int, WCA_CAPTION_BUTTON_BOUNDS Data           :   constant 0x6, Constant,
// Type: int, WCA_NONCLIENT_RTL_LAYOUT Data           :   constant 0x7,
// Constant, Type: int, WCA_FORCE_ICONIC_REPRESENTATION Data           :
// constant 0x8, Constant, Type: int, WCA_EXTENDED_FRAME_BOUNDS Data           :
// constant 0x9, Constant, Type: int, WCA_HAS_ICONIC_BITMAP Data           :
// constant 0xA, Constant, Type: int, WCA_THEME_ATTRIBUTES Data           :
// constant 0xB, Constant, Type: int, WCA_NCRENDERING_EXILED Data           :
// constant 0xC, Constant, Type: int, WCA_NCADORNMENTINFO Data           :
// constant 0xD, Constant, Type: int, WCA_EXCLUDED_FROM_LIVEPREVIEW Data :
// constant 0xE, Constant, Type: int, WCA_VIDEO_OVERLAY_ACTIVE Data           :
// constant 0xF, Constant, Type: int, WCA_FORCE_ACTIVEWINDOW_APPEARANCE Data :
// constant 0x10, Constant, Type: int, WCA_DISALLOW_PEEK Data           :
// constant 0x11, Constant, Type: int, WCA_CLOAK Data           :   constant
// 0x12, Constant, Type: int, WCA_CLOAKED Data           :   constant 0x13,
// Constant, Type: int, WCA_ACCENT_POLICY Data           :   constant 0x14,
// Constant, Type: int, WCA_FREEZE_REPRESENTATION Data           :   constant
// 0x15, Constant, Type: int, WCA_EVER_UNCLOAKED Data           :   constant
// 0x16, Constant, Type: int, WCA_VISUAL_OWNER Data           :   constant 0x17,
// Constant, Type: int, WCA_HOLOGRAPHIC Data           :   constant 0x18,
// Constant, Type: int, WCA_EXCLUDED_FROM_DDA Data           :   constant 0x19,
// Constant, Type: int, WCA_PASSIVEUPDATEMODE Data           :   constant 0x1A,
// Constant, Type: int, WCA_LAST
enum WINDOWCOMPOSITIONATTRIB : INT { // Determines what attribute is being
                                     // manipulated.
  WCA_ACCENT_POLICY =
      0x13 // The attribute being get or set is an accent policy.
};

// UserDefinedType: tagWINDOWCOMPOSITIONATTRIBDATA
// Data           :   this+0x0, Member, Type: enum WINDOWCOMPOSITIONATTRIB,
// Attrib Data           :   this+0x8, Member, Type: void *, pvData Data :
// this+0x10, Member, Type: unsigned int, cbData
struct WINDOWCOMPOSITIONATTRIBDATA { // Options for
                                     // [Get/Set]WindowCompositionAttribute.
  WINDOWCOMPOSITIONATTRIB Attrib;    // Type of what is being get or set.
  LPVOID pvData; // Pointer to memory that will receive what is get or that
                 // contains what will be set.
  UINT cbData;   // Size of the data being pointed to by pvData.
};

typedef BOOL(WINAPI *PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE)(
    HWND, const WINDOWCOMPOSITIONATTRIBDATA *);

static auto pSetWindowCompositionAttribute =
    (PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE)GetProcAddress(
        GetModuleHandleW(L"user32.dll"), "SetWindowCompositionAttribute");