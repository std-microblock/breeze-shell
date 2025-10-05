#include "scrollable_widget.h"
#include "nanovg.h"
#include "shell/config.h"
#include "shell/utils.h"
#include <algorithm>

namespace mb_shell {

scrollable_widget::scrollable_widget() : super() {
    scroll_top->set_easing(ui::easing_type::ease_in_out);
}

void scrollable_widget::update(ui::update_context &ctx) {
    // Handle scroll input when hovered
    if (ctx.hovered(this)) {
        scroll_top->animate_to(
            std::clamp(scroll_top->dest() + ctx.scroll_y * 100,
                       height->dest() - actual_height, 0.f));
    }
    widget::update(ctx);
    ctx.hovered_hit(this);

    // Update scrollable children with scroll offset
    update_scrollable_children(ctx);

    // Calculate actual height and apply max_height limit
    actual_height = height->dest();
    height->reset_to(std::min(max_height, height->dest()));
}

void scrollable_widget::render(ui::nanovg_context ctx) {
    super::render(ctx);

    render_scrollable_children(ctx);
    render_scrollbar(ctx);
}

void scrollable_widget::update_scrollable_children(ui::update_context &ctx) {
    auto forkctx = ctx.with_offset(*x, *y + *scroll_top);
    update_children(forkctx, scrollable_children);
    reposition_children_flex(forkctx, scrollable_children);
}

void scrollable_widget::render_scrollable_children(ui::nanovg_context ctx) {
    ctx.transaction([&] {
        ctx.scissor(*x, *y, *width, *height);
        render_children(ctx.with_offset(*x, *y + *scroll_top),
                        scrollable_children);
    });
}

void scrollable_widget::render_scrollbar(ui::nanovg_context ctx) {
    // Only render scrollbar if content height exceeds visible height
    if (height->dest() < actual_height) {
        auto scrollbar_width =
            config::current->context_menu.theme.scrollbar_width;
        auto scrollbar_height = height->dest() * height->dest() / actual_height;
        auto scrollbar_x = width->dest() - scrollbar_width - 2 + *x;
        auto scrollbar_y = *y - *scroll_top / (actual_height - height->dest()) *
                                    (height->dest() - scrollbar_height);

        float c = is_light_mode() ? 0 : 1;
        ctx.fillColor(nvgRGBAf(c, c, c, 0.1));
        ctx.fillRoundedRect(
            scrollbar_x, scrollbar_y, scrollbar_width, scrollbar_height,
            config::current->context_menu.theme.scrollbar_radius);
    }
}

} // namespace mb_shell