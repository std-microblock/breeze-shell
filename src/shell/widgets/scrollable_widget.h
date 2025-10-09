#pragma once
#include "breeze_ui/animator.h"
#include "breeze_ui/nanovg_wrapper.h"
#include "breeze_ui/ui.h"
#include "breeze_ui/widget.h"
#include <memory>
#include <vector>

namespace mb_shell {

/**
 * A widget that provides scrollable content with vertical scrollbar
 * Handles scroll input and renders a scrollbar when content exceeds max_height
 */
struct scrollable_widget : public ui::flex_widget {
    using super = ui::flex_widget;

    float max_height = 9999999;
    float actual_height = 0;
    ui::sp_anim_float scroll_top =
        anim_float(0, 200, ui::easing_type::ease_in_out);
    std::vector<std::shared_ptr<ui::widget>> scrollable_children;

    scrollable_widget();

    void update(ui::update_context &ctx) override;
    void render(ui::nanovg_context ctx) override;
    void update_scrollable_children(ui::update_context &ctx);
    void render_scrollable_children(ui::nanovg_context ctx);
    void render_scrollbar(ui::nanovg_context ctx);
};

} // namespace mb_shell