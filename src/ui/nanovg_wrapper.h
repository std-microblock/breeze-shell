#include "nanovg/nanovg.h"
#include <utility>

namespace ui {
struct nanovg_context {
  NVGcontext *ctx;
  /*
  Codegen:
nanovgSource.matchAll(/nvg(\S+)\(NVGcontext\* ctx,?(.*)\);/g)].map(v=>{
    return `auto ${v[1][0].toLowerCase() + v[1].slice(1)}(${v[2]}) { return nvg${v[1]}(${
        ['ctx',...v[2].split(',').filter(Boolean)].map(v=>v.trim().split(' ').pop()).join(',')
    }); }`
}).join('\n'))
   */
  auto beginFrame(float windowWidth, float windowHeight,
                  float devicePixelRatio) {
    return nvgBeginFrame(ctx, windowWidth, windowHeight, devicePixelRatio);
  }
  auto cancelFrame() { return nvgCancelFrame(ctx); }
  auto endFrame() { return nvgEndFrame(ctx); }
  auto globalCompositeOperation(int op) {
    return nvgGlobalCompositeOperation(ctx, op);
  }
  auto globalCompositeBlendFunc(int sfactor, int dfactor) {
    return nvgGlobalCompositeBlendFunc(ctx, sfactor, dfactor);
  }
  auto globalCompositeBlendFuncSeparate(int srcRGB, int dstRGB, int srcAlpha,
                                        int dstAlpha) {
    return nvgGlobalCompositeBlendFuncSeparate(ctx, srcRGB, dstRGB, srcAlpha,
                                               dstAlpha);
  }
  auto save() { return nvgSave(ctx); }
  auto restore() { return nvgRestore(ctx); }
  auto reset() { return nvgReset(ctx); }
  auto shapeAntiAlias(int enabled) { return nvgShapeAntiAlias(ctx, enabled); }
  auto strokeColor(NVGcolor color) { return nvgStrokeColor(ctx, color); }
  auto strokePaint(NVGpaint paint) { return nvgStrokePaint(ctx, paint); }
  auto fillColor(NVGcolor color) { return nvgFillColor(ctx, color); }
  auto fillPaint(NVGpaint paint) { return nvgFillPaint(ctx, paint); }
  auto miterLimit(float limit) { return nvgMiterLimit(ctx, limit); }
  auto strokeWidth(float size) { return nvgStrokeWidth(ctx, size); }
  auto lineCap(int cap) { return nvgLineCap(ctx, cap); }
  auto lineJoin(int join) { return nvgLineJoin(ctx, join); }
  auto globalAlpha(float alpha) { return nvgGlobalAlpha(ctx, alpha); }
  auto resetTransform() { return nvgResetTransform(ctx); }
  auto transform(float a, float b, float c, float d, float e, float f) {
    return nvgTransform(ctx, a, b, c, d, e, f);
  }
  auto translate(float x, float y) { return nvgTranslate(ctx, x, y); }
  auto rotate(float angle) { return nvgRotate(ctx, angle); }
  auto skewX(float angle) { return nvgSkewX(ctx, angle); }
  auto skewY(float angle) { return nvgSkewY(ctx, angle); }
  auto scale(float x, float y) { return nvgScale(ctx, x, y); }
  auto currentTransform(float *xform) {
    return nvgCurrentTransform(ctx, xform);
  }
  auto createImageRGBA(int w, int h, int imageFlags,
                       const unsigned char *data) {
    return nvgCreateImageRGBA(ctx, w, h, imageFlags, data);
  }
  auto updateImage(int image, const unsigned char *data) {
    return nvgUpdateImage(ctx, image, data);
  }
  auto imageSize(int image, int *w, int *h) {
    return nvgImageSize(ctx, image, w, h);
  }
  auto deleteImage(int image) { return nvgDeleteImage(ctx, image); }
  auto scissor(float x, float y, float w, float h) {
    return nvgScissor(ctx, x, y, w, h);
  }
  auto intersectScissor(float x, float y, float w, float h) {
    return nvgIntersectScissor(ctx, x, y, w, h);
  }
  auto resetScissor() { return nvgResetScissor(ctx); }
  auto beginPath() { return nvgBeginPath(ctx); }
  auto moveTo(float x, float y) { return nvgMoveTo(ctx, x, y); }
  auto lineTo(float x, float y) { return nvgLineTo(ctx, x, y); }
  auto bezierTo(float c1x, float c1y, float c2x, float c2y, float x, float y) {
    return nvgBezierTo(ctx, c1x, c1y, c2x, c2y, x, y);
  }
  auto quadTo(float cx, float cy, float x, float y) {
    return nvgQuadTo(ctx, cx, cy, x, y);
  }
  auto arcTo(float x1, float y1, float x2, float y2, float radius) {
    return nvgArcTo(ctx, x1, y1, x2, y2, radius);
  }
  auto closePath() { return nvgClosePath(ctx); }
  auto pathWinding(int dir) { return nvgPathWinding(ctx, dir); }
  auto arc(float cx, float cy, float r, float a0, float a1, int dir) {
    return nvgArc(ctx, cx, cy, r, a0, a1, dir);
  }
  auto rect(float x, float y, float w, float h) {
    return nvgRect(ctx, x, y, w, h);
  }
  auto roundedRect(float x, float y, float w, float h, float r) {
    return nvgRoundedRect(ctx, x, y, w, h, r);
  }
  auto roundedRectVarying(float x, float y, float w, float h, float radTopLeft,
                          float radTopRight, float radBottomRight,
                          float radBottomLeft) {
    return nvgRoundedRectVarying(ctx, x, y, w, h, radTopLeft, radTopRight,
                                 radBottomRight, radBottomLeft);
  }
  auto ellipse(float cx, float cy, float rx, float ry) {
    return nvgEllipse(ctx, cx, cy, rx, ry);
  }
  auto circle(float cx, float cy, float r) { return nvgCircle(ctx, cx, cy, r); }
  auto fill() { return nvgFill(ctx); }
  auto stroke() { return nvgStroke(ctx); }
  auto createFont(const char *name, const char *filename) {
    return nvgCreateFont(ctx, name, filename);
  }
  auto createFontMem(const char *name, unsigned char *data, int ndata,
                     int freeData) {
    return nvgCreateFontMem(ctx, name, data, ndata, freeData);
  }
  auto findFont(const char *name) { return nvgFindFont(ctx, name); }
  auto addFallbackFontId(int baseFont, int fallbackFont) {
    return nvgAddFallbackFontId(ctx, baseFont, fallbackFont);
  }
  auto addFallbackFont(const char *baseFont, const char *fallbackFont) {
    return nvgAddFallbackFont(ctx, baseFont, fallbackFont);
  }
  auto fontSize(float size) { return nvgFontSize(ctx, size); }
  auto fontBlur(float blur) { return nvgFontBlur(ctx, blur); }
  auto textLetterSpacing(float spacing) {
    return nvgTextLetterSpacing(ctx, spacing);
  }
  auto textLineHeight(float lineHeight) {
    return nvgTextLineHeight(ctx, lineHeight);
  }
  auto textAlign(int align) { return nvgTextAlign(ctx, align); }
  auto fontFaceId(int font) { return nvgFontFaceId(ctx, font); }
  auto fontFace(const char *font) { return nvgFontFace(ctx, font); }
  auto text(float x, float y, const char *string, const char *end) {
    return nvgText(ctx, x, y, string, end);
  }
  auto textBox(float x, float y, float breakRowWidth, const char *string,
               const char *end) {
    return nvgTextBox(ctx, x, y, breakRowWidth, string, end);
  }
  auto textBounds(float x, float y, const char *string, const char *end,
                  float *bounds) {
    return nvgTextBounds(ctx, x, y, string, end, bounds);
  }
  auto textBoxBounds(float x, float y, float breakRowWidth, const char *string,
                     const char *end, float *bounds) {
    return nvgTextBoxBounds(ctx, x, y, breakRowWidth, string, end, bounds);
  }
  auto textGlyphPositions(float x, float y, const char *string, const char *end,
                          NVGglyphPosition *positions, int maxPositions) {
    return nvgTextGlyphPositions(ctx, x, y, string, end, positions,
                                 maxPositions);
  }
  auto textMetrics(float *ascender, float *descender, float *lineh) {
    return nvgTextMetrics(ctx, ascender, descender, lineh);
  }
  auto textBreakLines(const char *string, const char *end, float breakRowWidth,
                      NVGtextRow *rows, int maxRows) {
    return nvgTextBreakLines(ctx, string, end, breakRowWidth, rows, maxRows);
  }
  auto deleteInternal() { return nvgDeleteInternal(ctx); }
  auto internalParams() { return nvgInternalParams(ctx); }
  auto debugDumpPathCache() { return nvgDebugDumpPathCache(ctx); }
};
} // namespace ui