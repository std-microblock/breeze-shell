#pragma once
#include "glad/glad.h"
#include "breeze_ui/nanovg_wrapper.h"
#include "breeze_ui/widget.h"
namespace ui {
NVGImage LoadBitmapImage(nanovg_context ctx, void* hbitmap);
};