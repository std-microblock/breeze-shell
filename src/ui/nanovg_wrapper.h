#include "nanovg.h"
#include <utility>

namespace ui {
struct nanovg_context {
  NVGcontext *ctx;
  /*
  Codegen:
nanovgSource.matchAll(/nvg(\S+)\(NVGcontext\* ctx,?(.*)\);/g)].map(v=>{
    return `inline auto ${v[1][0].toLowerCase() + v[1].slice(1)}(${v[2]}) { return nvg${v[1]}(${
        ['ctx',...v[2].split(',').filter(Boolean)].map(v=>v.trim().split(' ').pop()).join(',')
    }); }`
}).join('\n'))
   */
  inline auto beginFrame(float windowWidth, float windowHeight,
                  float devicePixelRatio) {
    return nvgBeginFrame(ctx, windowWidth, windowHeight, devicePixelRatio);
  }
  inline auto cancelFrame() { return nvgCancelFrame(ctx); }
  inline auto endFrame() { return nvgEndFrame(ctx); }
  inline auto globalCompositeOperation(int op) {
    return nvgGlobalCompositeOperation(ctx, op);
  }
  inline auto globalCompositeBlendFunc(int sfactor, int dfactor) {
    return nvgGlobalCompositeBlendFunc(ctx, sfactor, dfactor);
  }
  inline auto globalCompositeBlendFuncSeparate(int srcRGB, int dstRGB, int srcAlpha,
                                        int dstAlpha) {
    return nvgGlobalCompositeBlendFuncSeparate(ctx, srcRGB, dstRGB, srcAlpha,
                                               dstAlpha);
  }
  inline auto save() { return nvgSave(ctx); }
  inline auto restore() { return nvgRestore(ctx); }
  inline auto reset() { return nvgReset(ctx); }
  inline auto shapeAntiAlias(int enabled) { return nvgShapeAntiAlias(ctx, enabled); }
  inline auto strokeColor(NVGcolor color) { return nvgStrokeColor(ctx, color); }
  inline auto strokePaint(NVGpaint paint) { return nvgStrokePaint(ctx, paint); }
  inline auto fillColor(NVGcolor color) { return nvgFillColor(ctx, color); }
  inline auto fillPaint(NVGpaint paint) { return nvgFillPaint(ctx, paint); }
  inline auto miterLimit(float limit) { return nvgMiterLimit(ctx, limit); }
  inline auto strokeWidth(float size) { return nvgStrokeWidth(ctx, size); }
  inline auto lineCap(int cap) { return nvgLineCap(ctx, cap); }
  inline auto lineJoin(int join) { return nvgLineJoin(ctx, join); }
  inline auto globalAlpha(float alpha) { return nvgGlobalAlpha(ctx, alpha); }
  inline auto resetTransform() { return nvgResetTransform(ctx); }
  inline auto transform(float a, float b, float c, float d, float e, float f) {
    return nvgTransform(ctx, a, b, c, d, e, f);
  }
  inline auto translate(float x, float y) { return nvgTranslate(ctx, x, y); }
  inline auto rotate(float angle) { return nvgRotate(ctx, angle); }
  inline auto skewX(float angle) { return nvgSkewX(ctx, angle); }
  inline auto skewY(float angle) { return nvgSkewY(ctx, angle); }
  inline auto scale(float x, float y) { return nvgScale(ctx, x, y); }
  inline auto currentTransform(float *xform) {
    return nvgCurrentTransform(ctx, xform);
  }
  inline auto createImageRGBA(int w, int h, int imageFlags,
                       const unsigned char *data) {
    return nvgCreateImageRGBA(ctx, w, h, imageFlags, data);
  }
  inline auto updateImage(int image, const unsigned char *data) {
    return nvgUpdateImage(ctx, image, data);
  }
  inline auto imageSize(int image, int *w, int *h) {
    return nvgImageSize(ctx, image, w, h);
  }
  inline auto deleteImage(int image) { return nvgDeleteImage(ctx, image); }
  inline auto scissor(float x, float y, float w, float h) {
    return nvgScissor(ctx, x, y, w, h);
  }
  inline auto intersectScissor(float x, float y, float w, float h) {
    return nvgIntersectScissor(ctx, x, y, w, h);
  }
  inline auto resetScissor() { return nvgResetScissor(ctx); }
  inline auto beginPath() { return nvgBeginPath(ctx); }
  inline auto moveTo(float x, float y) { return nvgMoveTo(ctx, x, y); }
  inline auto lineTo(float x, float y) { return nvgLineTo(ctx, x, y); }
  inline auto bezierTo(float c1x, float c1y, float c2x, float c2y, float x, float y) {
    return nvgBezierTo(ctx, c1x, c1y, c2x, c2y, x, y);
  }
  inline auto quadTo(float cx, float cy, float x, float y) {
    return nvgQuadTo(ctx, cx, cy, x, y);
  }
  inline auto arcTo(float x1, float y1, float x2, float y2, float radius) {
    return nvgArcTo(ctx, x1, y1, x2, y2, radius);
  }
  inline auto closePath() { return nvgClosePath(ctx); }
  inline auto pathWinding(int dir) { return nvgPathWinding(ctx, dir); }
  inline auto arc(float cx, float cy, float r, float a0, float a1, int dir) {
    return nvgArc(ctx, cx, cy, r, a0, a1, dir);
  }
  inline auto rect(float x, float y, float w, float h) {
    return nvgRect(ctx, x, y, w, h);
  }
  inline auto roundedRect(float x, float y, float w, float h, float r) {
    return nvgRoundedRect(ctx, x, y, w, h, r);
  }
  inline auto roundedRectVarying(float x, float y, float w, float h, float radTopLeft,
                          float radTopRight, float radBottomRight,
                          float radBottomLeft) {
    return nvgRoundedRectVarying(ctx, x, y, w, h, radTopLeft, radTopRight,
                                 radBottomRight, radBottomLeft);
  }
  inline auto ellipse(float cx, float cy, float rx, float ry) {
    return nvgEllipse(ctx, cx, cy, rx, ry);
  }
  inline auto circle(float cx, float cy, float r) { return nvgCircle(ctx, cx, cy, r); }
  inline auto fill() { return nvgFill(ctx); }
  inline auto stroke() { return nvgStroke(ctx); }
  inline auto createFont(const char *name, const char *filename) {
    return nvgCreateFont(ctx, name, filename);
  }
  inline auto createFontMem(const char *name, unsigned char *data, int ndata,
                     int freeData) {
    return nvgCreateFontMem(ctx, name, data, ndata, freeData);
  }
  inline auto findFont(const char *name) { return nvgFindFont(ctx, name); }
  inline auto addFallbackFontId(int baseFont, int fallbackFont) {
    return nvgAddFallbackFontId(ctx, baseFont, fallbackFont);
  }
  inline auto addFallbackFont(const char *baseFont, const char *fallbackFont) {
    return nvgAddFallbackFont(ctx, baseFont, fallbackFont);
  }
  inline auto fontSize(float size) { return nvgFontSize(ctx, size); }
  inline auto fontBlur(float blur) { return nvgFontBlur(ctx, blur); }
  inline auto textLetterSpacing(float spacing) {
    return nvgTextLetterSpacing(ctx, spacing);
  }
  inline auto textLineHeight(float lineHeight) {
    return nvgTextLineHeight(ctx, lineHeight);
  }
  inline auto textAlign(int align) { return nvgTextAlign(ctx, align); }
  inline auto fontFaceId(int font) { return nvgFontFaceId(ctx, font); }
  inline auto fontFace(const char *font) { return nvgFontFace(ctx, font); }
  inline auto text(float x, float y, const char *string, const char *end) {
    return nvgText(ctx, x, y, string, end);
  }
  inline auto textBox(float x, float y, float breakRowWidth, const char *string,
               const char *end) {
    return nvgTextBox(ctx, x, y, breakRowWidth, string, end);
  }
  inline auto textBounds(float x, float y, const char *string, const char *end,
                  float *bounds) {
    return nvgTextBounds(ctx, x, y, string, end, bounds);
  }
  inline auto textBoxBounds(float x, float y, float breakRowWidth, const char *string,
                     const char *end, float *bounds) {
    return nvgTextBoxBounds(ctx, x, y, breakRowWidth, string, end, bounds);
  }
  inline auto textGlyphPositions(float x, float y, const char *string, const char *end,
                          NVGglyphPosition *positions, int maxPositions) {
    return nvgTextGlyphPositions(ctx, x, y, string, end, positions,
                                 maxPositions);
  }
  inline auto textMetrics(float *ascender, float *descender, float *lineh) {
    return nvgTextMetrics(ctx, ascender, descender, lineh);
  }
  inline auto textBreakLines(const char *string, const char *end, float breakRowWidth,
                      NVGtextRow *rows, int maxRows) {
    return nvgTextBreakLines(ctx, string, end, breakRowWidth, rows, maxRows);
  }
  inline auto deleteInternal() { return nvgDeleteInternal(ctx); }
  inline auto internalParams() { return nvgInternalParams(ctx); }
  inline auto debugDumpPathCache() { return nvgDebugDumpPathCache(ctx); }
};
} // namespace ui