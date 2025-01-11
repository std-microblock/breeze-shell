#include "hbitmap_utils.h"
#include "Windows.h"
#include <cstdio>
#include <vector>
ui::NVGImage ui::LoadBitmapImage(nanovg_context ctx, void *hbitmap) {
  HBITMAP hBitmap = (HBITMAP)hbitmap;
  BITMAP bm;

  auto dc = CreateCompatibleDC(NULL);

  GetObject(hBitmap, sizeof(bm), &bm);

  BITMAPINFO bi = {0};
  bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

  if (!GetDIBits(dc, hBitmap, 0, 0, NULL, &bi, DIB_RGB_COLORS)) {
    printf("Failed to get DIB bits\n");
    return NVGImage(-1, 0, 0, ctx);
  }

  std::vector<unsigned char> bits(bi.bmiHeader.biSizeImage);

  bi.bmiHeader.biBitCount = 32;
  bi.bmiHeader.biCompression = BI_RGB;

  if (!GetDIBits(dc, hBitmap, 0, bm.bmHeight, bits.data(), &bi,
                 DIB_RGB_COLORS)) {
    printf("Failed to get DIB bits\n");
    return NVGImage(-1, 0, 0, ctx);
  }

  DeleteDC(dc);

  std::vector<uint8_t> rgba(bm.bmWidth * bm.bmHeight * 4);

    for (int i = 0; i < bm.bmWidth * bm.bmHeight; i++) {
        rgba[i * 4 + 0] = bits[i * 4 + 2];
        rgba[i * 4 + 1] = bits[i * 4 + 1];
        rgba[i * 4 + 2] = bits[i * 4 + 0];
        rgba[i * 4 + 3] = bits[i * 4 + 3];
    }

    // hbitmap is in reverse order (bottom to top)
    for (int y = 0; y < bm.bmHeight / 2; y++) {
        for (int x = 0; x < bm.bmWidth; x++) {
            for (int i = 0; i < 4; i++) {
                std::swap(rgba[(y * bm.bmWidth + x) * 4 + i],
                          rgba[((bm.bmHeight - y - 1) * bm.bmWidth + x) * 4 + i]);
            }
        }
    }

  auto id = ctx.createImageRGBA(bm.bmWidth, bm.bmHeight, 0, rgba.data());
  if (id == -1) {
    printf("Failed to create image\n");
    return NVGImage(-1, 0, 0, ctx);
  }
  return NVGImage(id, bm.bmWidth, bm.bmHeight, ctx);
}
