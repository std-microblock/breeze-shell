#pragma once

#include "widget.h"
#include "extra_widgets.h"
#include "animator.h"
#include "nanovg.h"
#include <memory>

namespace mb_shell {

class background_widget : public ui::widget {
public:
    using super = ui::widget;

    background_widget(bool is_main);

    void render(ui::nanovg_context ctx) override;
    void update(ui::update_context& ctx) override;

    ui::sp_anim_float opacity;
    ui::sp_anim_float x, y, width, height, radius;
    NVGcolor bg_color;

private:
    std::shared_ptr<ui::rect_widget> bg_impl;
};

}