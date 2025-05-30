#pragma once
#include "glad/glad.h"
#include "nanosvg.h"
#include "nanosvgrast.h"
#include "nanovg.h"
#include <functional>
#include <utility>

namespace ui {
struct NVGImage;
struct render_target;
struct nanovg_context {
  NVGcontext *ctx;
  render_target *rt;
  // clang-format off
  /*
  Codegen:

console.log([...nanovgSource.replace(/,\n/gm, ',').replaceAll('\t', '').matchAll(/nvg(\S+)\(NVGcontext\* ctx,?(.*)\);/g)].map(v=>{ if(v[1] === 'Translate') return; return `inline auto ${v[1][0].toLowerCase() + v[1].slice(1)}(${v[2]}) { return nvg${v[1]}(${
        ['ctx',...v[2].split(',').filter(Boolean)].map(v=>v.trim().split(' ').pop()) .map(v=>{ 
        if ('x,y,c1x,c1y,y1,y2,x1,x2,cx,cy,ox,oy'.split(',').includes(v))
         return `${v} + offset_${v.includes('x') ? 'x' : 'y'}`
            return v
          })
        .join(',')
    }); }`
}).join('\n'))
   */

  float offset_x = 0, offset_y = 0;


inline auto beginFrame( float windowWidth, float windowHeight, float devicePixelRatio) { return nvgBeginFrame(ctx,windowWidth,windowHeight,devicePixelRatio); }
inline auto cancelFrame() { return nvgCancelFrame(ctx); }
inline auto endFrame() { return nvgEndFrame(ctx); }
inline auto globalCompositeOperation( int op) { return nvgGlobalCompositeOperation(ctx,op); }
inline auto globalCompositeBlendFunc( int sfactor, int dfactor) { return nvgGlobalCompositeBlendFunc(ctx,sfactor,dfactor); }
inline auto globalCompositeBlendFuncSeparate( int srcRGB, int dstRGB, int srcAlpha, int dstAlpha) { return nvgGlobalCompositeBlendFuncSeparate(ctx,srcRGB,dstRGB,srcAlpha,dstAlpha); }
inline auto save() { return nvgSave(ctx); }
inline auto restore() { return nvgRestore(ctx); }
inline auto reset() { return nvgReset(ctx); }
inline auto shapeAntiAlias( int enabled) { return nvgShapeAntiAlias(ctx,enabled); }
inline auto strokeColor( NVGcolor color) { return nvgStrokeColor(ctx,color); }
inline auto strokePaint( NVGpaint paint) { return nvgStrokePaint(ctx,paint); }
inline auto fillColor( NVGcolor color) { return nvgFillColor(ctx,color); }
inline auto fillPaint( NVGpaint paint) { return nvgFillPaint(ctx,paint); }
inline auto miterLimit( float limit) { return nvgMiterLimit(ctx,limit); }
inline auto strokeWidth( float size) { return nvgStrokeWidth(ctx,size); }
inline auto lineCap( int cap) { return nvgLineCap(ctx,cap); }
inline auto lineJoin( int join) { return nvgLineJoin(ctx,join); }
inline auto globalAlpha( float alpha) { return nvgGlobalAlpha(ctx,alpha); }
inline auto resetTransform() { return nvgResetTransform(ctx); }
inline auto transform( float a, float b, float c, float d, float e, float f) { return nvgTransform(ctx,a,b,c,d,e,f); }
inline auto translate( float x, float y) { return nvgTranslate(ctx,x,y); }
inline auto rotate( float angle) { return nvgRotate(ctx,angle); }
inline auto skewX( float angle) { return nvgSkewX(ctx,angle); }
inline auto skewY( float angle) { return nvgSkewY(ctx,angle); }
inline auto scale( float x, float y) { return nvgScale(ctx,x + offset_x,y + offset_y); }
inline auto currentTransform( float* xform) { return nvgCurrentTransform(ctx,xform); }
inline auto createImage( const char* filename, int imageFlags) { return nvgCreateImage(ctx,filename,imageFlags); }
inline auto createImageMem( int imageFlags, unsigned char* data, int ndata) { return nvgCreateImageMem(ctx,imageFlags,data,ndata); }
inline auto createImageRGBA( int w, int h, int imageFlags, const unsigned char* data) { return nvgCreateImageRGBA(ctx,w,h,imageFlags,data); }
inline auto updateImage( int image, const unsigned char* data) { return nvgUpdateImage(ctx,image,data); }
inline auto imageSize( int image, int* w, int* h) { return nvgImageSize(ctx,image,w,h); }
inline auto deleteImage( int image) { return nvgDeleteImage(ctx,image); }
inline auto linearGradient( float sx, float sy, float ex, float ey,  NVGcolor icol, NVGcolor ocol) { return nvgLinearGradient(ctx,sx,sy,ex,ey,icol,ocol); }
inline auto boxGradient( float x, float y, float w, float h, float r, float f, NVGcolor icol, NVGcolor ocol) { return nvgBoxGradient(ctx,x + offset_x,y + offset_y,w,h,r,f,icol,ocol); }
inline auto radialGradient( float cx, float cy, float inr, float outr,  NVGcolor icol, NVGcolor ocol) { return nvgRadialGradient(ctx,cx + offset_x,cy + offset_y,inr,outr,icol,ocol); }
inline auto imagePattern( float ox, float oy, float ex, float ey,float angle, int image, float alpha) { return nvgImagePattern(ctx,ox + offset_x,oy + offset_y,ex,ey,angle,image,alpha); }
inline auto scissor( float x, float y, float w, float h) { return nvgScissor(ctx,x + offset_x,y + offset_y,w,h); }
inline auto intersectScissor( float x, float y, float w, float h) { return nvgIntersectScissor(ctx,x + offset_x,y + offset_y,w,h); }
inline auto resetScissor() { return nvgResetScissor(ctx); }
inline auto beginPath() { return nvgBeginPath(ctx); }
inline auto moveTo( float x, float y) { return nvgMoveTo(ctx,x + offset_x,y + offset_y); }
inline auto lineTo( float x, float y) { return nvgLineTo(ctx,x + offset_x,y + offset_y); }
inline auto bezierTo( float c1x, float c1y, float c2x, float c2y, float x, float y) { return nvgBezierTo(ctx,c1x + offset_x,c1y + offset_y,c2x,c2y,x + offset_x,y + offset_y); }
inline auto quadTo( float cx, float cy, float x, float y) { return nvgQuadTo(ctx,cx + offset_x,cy + offset_y,x + offset_x,y + offset_y); }
inline auto arcTo( float x1, float y1, float x2, float y2, float radius) { return nvgArcTo(ctx,x1 + offset_x,y1 + offset_y,x2 + offset_x,y2 + offset_y,radius); }
inline auto closePath() { return nvgClosePath(ctx); }
inline auto pathWinding( int dir) { return nvgPathWinding(ctx,dir); }
inline auto arc( float cx, float cy, float r, float a0, float a1, int dir) { return nvgArc(ctx,cx + offset_x,cy + offset_y,r,a0,a1,dir); }
inline auto rect( float x, float y, float w, float h) { return nvgRect(ctx,x + offset_x,y + offset_y,w,h); }
inline auto roundedRect( float x, float y, float w, float h, float r) { return nvgRoundedRect(ctx,x + offset_x,y + offset_y,w,h,r); }
inline auto roundedRectVarying( float x, float y, float w, float h, float radTopLeft, float radTopRight, float radBottomRight, float radBottomLeft) { return nvgRoundedRectVarying(ctx,x + offset_x,y + offset_y,w,h,radTopLeft,radTopRight,radBottomRight,radBottomLeft); }
inline auto ellipse( float cx, float cy, float rx, float ry) { return nvgEllipse(ctx,cx + offset_x,cy + offset_y,rx,ry); }
inline auto circle( float cx, float cy, float r) { return nvgCircle(ctx,cx + offset_x,cy + offset_y,r); }
inline auto fill() { return nvgFill(ctx); }
inline auto stroke() { return nvgStroke(ctx); }
inline auto createFont( const char* name, const char* filename) { return nvgCreateFont(ctx,name,filename); }
inline auto createFontAtIndex( const char* name, const char* filename, const int fontIndex) { return nvgCreateFontAtIndex(ctx,name,filename,fontIndex); }
inline auto createFontMem( const char* name, unsigned char* data, int ndata, int freeData) { return nvgCreateFontMem(ctx,name,data,ndata,freeData); }
inline auto createFontMemAtIndex( const char* name, unsigned char* data, int ndata, int freeData, const int fontIndex) { return nvgCreateFontMemAtIndex(ctx,name,data,ndata,freeData,fontIndex); }
inline auto findFont( const char* name) { return nvgFindFont(ctx,name); }
inline auto addFallbackFontId( int baseFont, int fallbackFont) { return nvgAddFallbackFontId(ctx,baseFont,fallbackFont); }
inline auto addFallbackFont( const char* baseFont, const char* fallbackFont) { return nvgAddFallbackFont(ctx,baseFont,fallbackFont); }
inline auto resetFallbackFontsId( int baseFont) { return nvgResetFallbackFontsId(ctx,baseFont); }
inline auto resetFallbackFonts( const char* baseFont) { return nvgResetFallbackFonts(ctx,baseFont); }
inline auto fontSize( float size) { return nvgFontSize(ctx,size); }
inline auto fontBlur( float blur) { return nvgFontBlur(ctx,blur); }
inline auto textLetterSpacing( float spacing) { return nvgTextLetterSpacing(ctx,spacing); }
inline auto textLineHeight( float lineHeight) { return nvgTextLineHeight(ctx,lineHeight); }
inline auto textAlign( int align) { return nvgTextAlign(ctx,align); }
inline auto fontFaceId( int font) { return nvgFontFaceId(ctx,font); }
inline auto fontFace( const char* font) { return nvgFontFace(ctx,font); }
inline auto text( float x, float y, const char* string, const char* end) { return nvgText(ctx,x + offset_x,y + offset_y,string,end); }
inline auto textBox( float x, float y, float breakRowWidth, const char* string, const char* end) { return nvgTextBox(ctx,x + offset_x,y + offset_y,breakRowWidth,string,end); }
inline auto textBounds( float x, float y, const char* string, const char* end, float* bounds) { return nvgTextBounds(ctx,x + offset_x,y + offset_y,string,end,bounds); }
inline auto textBoxBounds( float x, float y, float breakRowWidth, const char* string, const char* end, float* bounds) { return nvgTextBoxBounds(ctx,x + offset_x,y + offset_y,breakRowWidth,string,end,bounds); }
inline auto textGlyphPositions( float x, float y, const char* string, const char* end, NVGglyphPosition* positions, int maxPositions) { return nvgTextGlyphPositions(ctx,x + offset_x,y + offset_y,string,end,positions,maxPositions); }
inline auto textMetrics( float* ascender, float* descender, float* lineh) { return nvgTextMetrics(ctx,ascender,descender,lineh); }
inline auto textBreakLines( const char* string, const char* end, float breakRowWidth, NVGtextRow* rows, int maxRows) { return nvgTextBreakLines(ctx,string,end,breakRowWidth,rows,maxRows); }
inline auto deleteInternal() { return nvgDeleteInternal(ctx); }
inline auto internalParams() { return nvgInternalParams(ctx); }
inline auto debugDumpPathCache() { return nvgDebugDumpPathCache(ctx); }
  // clang-format on

  // shortcuts
  inline auto fillRect(float x, float y, float w, float h) {
    beginPath();
    rect(x, y, w, h);
    fill();
  }

  inline auto strokeRect(float x, float y, float w, float h) {
    beginPath();
    rect(x, y, w, h);
    stroke();
  }

  inline auto fillCircle(float cx, float cy, float r) {
    beginPath();
    circle(cx, cy, r);
    fill();
  }

  inline auto strokeCircle(float cx, float cy, float r) {
    beginPath();
    circle(cx, cy, r);
    stroke();
  }

  inline auto fillEllipse(float cx, float cy, float rx, float ry) {
    beginPath();
    ellipse(cx, cy, rx, ry);
    fill();
  }

  inline auto strokeEllipse(float cx, float cy, float rx, float ry) {
    beginPath();
    ellipse(cx, cy, rx, ry);
    stroke();
  }

  inline auto fillRoundedRect(float x, float y, float w, float h, float r) {
    beginPath();
    roundedRect(x, y, w, h, r);
    fill();
  }

  inline auto strokeRoundedRect(float x, float y, float w, float h, float r) {
    beginPath();
    roundedRect(x, y, w, h, r);
    stroke();
  }

  inline auto measureText(const char *string) {
    float bounds[4];
    textBounds(0, 0, string, nullptr, bounds);
    return std::make_pair(bounds[2] - bounds[0], bounds[3] - bounds[1]);
  }

  inline nanovg_context with_offset(float x, float y) {
    auto copy = *this;
    copy.offset_x = x + offset_x;
    copy.offset_y = y + offset_y;
    return copy;
  }

  inline nanovg_context with_reset_offset(float x = 0, float y = 0) {
    auto copy = *this;
    copy.offset_x = x;
    copy.offset_y = y;
    return copy;
  }

  struct TransactionScope {
    nanovg_context &ctx;
    TransactionScope(nanovg_context &ctx) : ctx(ctx) { ctx.save(); }
    ~TransactionScope() { ctx.restore(); }
  };

  inline TransactionScope transaction() { return TransactionScope(*this); }

  inline void transaction(std::function<void()> f) {
    save();
    f();
    restore();
  }

  template <typename T> inline T transaction(std::function<T()> f) {
    save();
    auto res = f();
    restore();
    return res;
  }

  inline void drawCubicBezier(float x1, float y1, float cx1, float cy1,
                              float cx2, float cy2, float x2, float y2) {
    beginPath();
    moveTo(x1, y1);
    bezierTo(cx1, cy1, cx2, cy2, x2, y2);
    stroke();
  }

  struct NSVGimageRAII {
    NSVGimage *image;
    NSVGimageRAII(NSVGimage *image) : image(image) {}
    ~NSVGimageRAII() { nsvgDelete(image); }
  };

  inline NVGImage imageFromSVG(NSVGimage *image, float dpi_scale = 1);
  inline void drawSVG(NSVGimage *image, float x, float y, float width,
                      float height) {
    auto orig_width = image->width, orig_height = image->height;
    x += offset_x;
    y += offset_y;
    for (auto shape = image->shapes; shape != NULL; shape = shape->next) {
      for (auto path = shape->paths; path != NULL; path = path->next) {
        for (int i = 0; i < path->npts - 1; i += 3) {
          float *p = &path->pts[i * 2];

          drawCubicBezier(
              p[0] * width / orig_width + x, p[1] * height / orig_height + y,
              p[2] * width / orig_width + x, p[3] * height / orig_height + y,
              p[4] * width / orig_width + x, p[5] * height / orig_height + y,
              p[6] * width / orig_width + x, p[7] * height / orig_height + y);
        }
      }
    }
  }

  inline void drawSVG(NSVGimageRAII &image, float x, float y, float width,
                      float height) {
    drawSVG(image.image, x, y, width, height);
  }
};

struct GLTexture {
  GLuint id;
  int width, height;

  inline GLTexture(GLuint id, int width, int height)
      : id(id), width(width), height(height) {}

  GLTexture(GLTexture &&other) = default;
  GLTexture &operator=(GLTexture &&other) = default;

  inline ~GLTexture() { glDeleteTextures(1, &id); }
};

struct NVGImage {
  int id;
  int width, height;
  nanovg_context ctx;

  inline NVGImage(int id, int width, int height, nanovg_context ctx)
      : id(id), width(width), height(height), ctx(ctx) {}

  NVGImage(NVGImage &&other) = default;
  NVGImage &operator=(NVGImage &&other) = default;

  inline ~NVGImage() {
    // if (id != -1)
    //   ctx.deleteImage(id);
  }
};

NVGImage nanovg_context::imageFromSVG(NSVGimage *image, float dpi_scale) {
  static auto rast = nsvgCreateRasterizer();
  auto width = image->width, height = image->height;
  width *= dpi_scale, height *= dpi_scale;

  auto data = (unsigned char *)malloc(width * height * 4);
  nsvgRasterize(rast, image, 0, 0, dpi_scale, data, width, height, width * 4);
  auto id = createImageRGBA(width, height, 0, data);
  free(data);
  return NVGImage(id, width, height, *this);
}

} // namespace ui