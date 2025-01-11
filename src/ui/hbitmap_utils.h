#pragma once
#include "glad/glad.h"
#include "nanovg_wrapper.h"
#include "widget.h"
namespace ui {
NVGImage LoadBitmapImage(nanovg_context ctx, void* hbitmap);
};