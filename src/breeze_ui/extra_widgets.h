#pragma once
#include "breeze_ui/animator.h"
#include "breeze_ui/widget.h"
#include "nanovg.h"
#include <condition_variable>
#include <mutex>
#include <thread>


namespace ui {

struct rect_widget : public widget {
    rect_widget();
    ~rect_widget();
    sp_anim_float opacity = anim_float(0, 200);
    sp_anim_float radius = anim_float(0, 0);

    NVGcolor bg_color = nvgRGBAf(0, 0, 0, 0);

    void render(nanovg_context ctx) override;
};

struct acrylic_background_widget : public rect_widget {
    void *hwnd = nullptr;
    bool should_update = true;
    acrylic_background_widget(bool use_dwm = true);
    ~acrylic_background_widget();
    bool use_dwm = true;
    NVGcolor acrylic_bg_color = nvgRGBAf(1, 0, 0, 0);
    std::optional<std::thread> render_thread;
    bool to_close = false;
    float offset_x = 0, offset_y = 0, dpi_scale = 1;
    static thread_local void *last_hwnd;
    void *last_hwnd_self = nullptr;
    void update_color();

    void render(nanovg_context ctx) override;

    void update(update_context &ctx) override;

private:
    int current_xywh[4] = {};
};
} // namespace ui