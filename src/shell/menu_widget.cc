#include "menu_widget.h"
#include "nanovg.h"
#include "shell.h"
#include <vector>

mb_shell::menu_item_widget::menu_item_widget(menu_item item, size_t index)
    : super() {
  opacity->reset_to(0);

  this->y->before_animate = [this, index](float dest) {
    this->y->from = std::max(0.f, dest - 40 - index * 10);
  };
  opacity->animate_to(255);

  auto delay = std::min(index * 10.f, 100.f);
  this->y->set_delay(delay);
  opacity->set_delay(delay);
  if (item.type == menu_item::type::spacer) {
    height->animate_to(1);
  } else {
    height->animate_to(25);
  }
  this->item = item;
}

struct bitmap_dump {
  int width;
  int height;
  std::vector<uint8_t> data;
};

bitmap_dump dump_bitmap_rgb(HBITMAP bmp) {
  BITMAP bm;
  GetObject(bmp, sizeof(bm), &bm);

  BITMAPINFOHEADER bi = {0};
  bi.biSize = sizeof(BITMAPINFOHEADER);
  bi.biWidth = bm.bmWidth;
  bi.biHeight = bm.bmHeight;
  bi.biPlanes = 1;
  bi.biBitCount = 32;
  bi.biCompression = BI_RGB;

  std::vector<uint8_t> bits(bm.bmWidth * bm.bmHeight * 4);
  HDC hdc = GetDC(nullptr);
  GetDIBits(hdc, bmp, 0, bm.bmHeight, bits.data(),
            reinterpret_cast<BITMAPINFO *>(&bi), DIB_RGB_COLORS);
  ReleaseDC(nullptr, hdc);
  return {bm.bmWidth, bm.bmHeight, bits};
}

std::vector<uint8_t> rgb_to_rgba(std::vector<uint8_t> rgb) {
  std::vector<uint8_t> rgba(rgb.size() * 4 / 3);
  for (size_t i = 0; i < rgb.size(); i += 3) {
    rgba[i * 4 / 3] = rgb[i];
    rgba[i * 4 / 3 + 1] = rgb[i + 1];
    rgba[i * 4 / 3 + 2] = rgb[i + 2];
    rgba[i * 4 / 3 + 3] = 255;
  }
  return rgba;
}

void mb_shell::menu_item_widget::render(ui::nanovg_context ctx) {
  super::render(ctx);
  if (item.type == menu_item::type::spacer) {
    ctx.fillColor(nvgRGBAf(1, 1, 1, 0.1));
    ctx.fillRect(*x, *y, *width, *height);
    return;
  }

  ctx.fillColor(nvgRGBAf(1, 1, 1, *bg_opacity / 255.f));
  ctx.fillRoundedRect(*x + margin, *y, *width - margin * 2, *height, 4);

  ctx.fillColor(nvgRGBAf(1, 1, 1, *opacity / 255.f));
  ctx.fontFace("Segui");
  ctx.fontSize(14);

  if (item.icon) {
    if (icon_img_id == -1) {
      auto bits = dump_bitmap_rgb(item.icon.value());
      auto rgba = rgb_to_rgba(bits.data);
      icon_img_id =
          ctx.createImageRGBA(bits.width, bits.height, 0, rgba.data());
    }

    auto paint = nvgImagePattern(ctx.ctx, *x + icon_padding, *y + icon_padding,
                                 icon_width, icon_width, 0, icon_img_id, 1);
    ctx.beginPath();
    ctx.rect(*x + icon_padding, *y + icon_padding, icon_width, icon_width);
    ctx.fillPaint(paint);
    ctx.fill();
  }

  ctx.text(floor(*x + text_padding + icon_width + icon_padding * 2),
           floor(*y + 2 + 14), item.name->c_str(), nullptr);
}
void mb_shell::menu_item_widget::update(ui::UpdateContext &ctx) {
  super::update(ctx);
  if (ctx.mouse_down_on(this)) {
    bg_opacity->animate_to(40);
  } else if (ctx.hovered(this)) {
    bg_opacity->animate_to(20);
  } else {
    bg_opacity->animate_to(0);
  }

  if (ctx.mouse_clicked_on(this)) {
    if (item.action) {
      item.action.value()();
    }
  }
}
float mb_shell::menu_item_widget::measure_width(ui::UpdateContext &ctx) {
  if (item.type == menu_item::type::spacer) {
    return 1;
  }
  return ctx.vg.measureText(item.name->c_str()).first + text_padding * 2 +
         margin * 2 + icon_width + icon_padding * 2;
}
mb_shell::menu_widget::menu_widget(menu menu, float wid_x, float wid_y)
    : super(), menu_data(menu) {
  gap = 5;
  anchor_x = wid_x;
  anchor_y = wid_y;
  this->x->reset_to(wid_x);
  this->y->reset_to(wid_y);

  bg = std::make_unique<ui::acrylic_background_widget>();
  bg->acrylic_bg_color = nvgRGBAf(0, 0, 0, 0.5);
  bg->update_color();
  bg->radius->reset_to(6);
  bg->use_dwm = false;

  bg->opacity->reset_to(0);
  bg->opacity->animate_to(255);

  auto init_items = menu_data.items;
  for (size_t i = 0; i < init_items.size(); i++) {
    auto &item = init_items[i];
    auto mi = new menu_item_widget(item, i);
    children.push_back(std::unique_ptr<menu_item_widget>(mi));
  }
}
void mb_shell::menu_widget::update(ui::UpdateContext &ctx) {
  super::update(ctx);

  x->reset_to(anchor_x / ctx.rt.dpi_scale);
  y->reset_to(anchor_y / ctx.rt.dpi_scale);
  bg->x->reset_to(anchor_x / ctx.rt.dpi_scale);
  bg->y->reset_to(anchor_y / ctx.rt.dpi_scale - bg_padding_vertical);

  bg->width->reset_to(width->dest());
  bg->height->reset_to(height->dest() + bg_padding_vertical * 2);
  bg->update(ctx);
}
void mb_shell::menu_widget::render(ui::nanovg_context ctx) {
  bg->render(ctx);
  super::render(ctx);
}