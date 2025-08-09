#include "breeze_ui/widget.h"
#include "breeze_ui/ui.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <print>
#include <ranges>
#include <thread>

void ui::widget::update_child_basic(update_context &ctx,
                                    std::shared_ptr<widget> &w) {
    if (!w)
        return;
    // handle dying time
    if (w->dying_time && w->dying_time.time <= 0) {
        w = nullptr;
        return;
    }
    w->parent = this;
    w->update(ctx);
}

void ui::widget::render_child_basic(nanovg_context ctx,
                                    std::shared_ptr<widget> &w) {
    if (!w)
        return;

    constexpr float big_number = 1e5;
    auto can_render_width =
             enable_child_clipping
                 ? std::max(std::min(**w->width, **width - *w->x), 0.f)
                 : big_number,
         can_render_height =
             enable_child_clipping
                 ? std::max(std::min(**w->height, **height - *w->y), 0.f)
                 : big_number;

    if (can_render_width > 0 && can_render_height > 0) {
        ctx.save();
        w->render(ctx);
        ctx.restore();
    }
}

void ui::widget::render(nanovg_context ctx) {
    if constexpr (false)
        if (_debug_offset_cache[0] != ctx.offset_x ||
            _debug_offset_cache[1] != ctx.offset_y) {
            if (_debug_offset_cache[0] != -1 || _debug_offset_cache[1] != -1) {
                std::println(
                    "[Warning] The offset during render is different from the "
                    "offset during update: (update) {} {} vs (render) {} {} "
                    "({}, {})",
                    _debug_offset_cache[0], _debug_offset_cache[1],
                    ctx.offset_x, ctx.offset_y, (void *)this,
                    typeid(*this).name());
            } else {
                std::println("[Warning] The update function is not called "
                             "before render: {}",
                             (void *)this);
            }
        }

    _debug_offset_cache[0] = -1;
    _debug_offset_cache[1] = -1;

    render_children(ctx.with_offset(*x, *y), children);
}
void ui::widget::update(update_context &ctx) {
    children_dirty = false;
    owner_rt = &ctx.rt;
    for (auto anim : anim_floats) {
        anim->update(ctx.delta_time);

        if (anim->updated()) {
            ctx.need_repaint = true;
        }
    }

    if (this->needs_repaint) {
        ctx.need_repaint = true;
        this->needs_repaint = false;
    }

    dying_time.update(ctx.delta_time);
    update_context upd = ctx.with_offset(*x, *y);
    update_children(upd, children);
    if constexpr (false)
        if (_debug_offset_cache[0] != -1 || _debug_offset_cache[1] != -1) {
            std::println(
                "[Warning] The update function is called twice with different "
                "offsets: {} {} vs {} {} ({})",
                _debug_offset_cache[0], _debug_offset_cache[1], ctx.offset_x,
                ctx.offset_y, (void *)this);
        }
    _debug_offset_cache[0] = ctx.offset_x;
    _debug_offset_cache[1] = ctx.offset_y;

    last_offset_x = ctx.offset_x;
    last_offset_y = ctx.offset_y;
}
void ui::widget::add_child(std::shared_ptr<widget> child) {
    children.push_back(std::move(child));
}

bool ui::update_context::hovered(widget *w, bool hittest) const {
    auto hit = w->check_hit(*this);
    if (!hit)
        return false;

    if (hittest) {
        if (!hovered_widgets->empty()) {
            // iterate through parent chain
            auto p = w;
            while (p) {
                if (std::ranges::contains(*hovered_widgets, p)) {
                    return true;
                }

                p = p->parent;
            }
            return false;
        }
    }

    return true;
}
float ui::widget::measure_height(update_context &ctx) { return height->dest(); }
float ui::widget::measure_width(update_context &ctx) { return width->dest(); }
void ui::widget_flex::update(update_context &ctx) {
    widget::update(ctx);
    auto forkctx = ctx.with_offset(*x, *y);
    reposition_children_flex(forkctx, children);
}
void ui::widget_flex::reposition_children_flex(
    update_context &ctx, std::vector<std::shared_ptr<widget>> &children) {
    float x = *padding_left, y = *padding_top;
    float target_width = 0, target_height = 0;

    auto children_rev =
        reverse ? children | std::views::reverse |
                      std::ranges::to<std::vector<std::shared_ptr<widget>>>()
                : children;

    auto spacer_count =
        std::ranges::count_if(children_rev, [](const auto &child) {
            return dynamic_cast<const spacer *>(child.get());
        });

    std::vector<std::pair<float, float>> measure_cache;
    measure_cache.reserve(children_rev.size());

    // Cache measure results for all children
    for (auto &child : children_rev) {
        float child_width = child->measure_width(ctx);
        float child_height = child->measure_height(ctx);
        measure_cache.emplace_back(child_width, child_height);
    }

    float total_fixed_size = 0;
    float spacer_size = 0;

    // First pass: calculate total fixed size
    for (size_t i = 0; i < children_rev.size(); ++i) {
        auto &child = children_rev[i];
        if (dynamic_cast<const spacer *>(child.get())) {
            continue; // Skip spacers in first pass
        }
        auto [cached_width, cached_height] = measure_cache[i];
        if (horizontal) {
            total_fixed_size += cached_width;
        } else {
            total_fixed_size += cached_height;
        }
    }

    // Calculate spacer size
    if (spacer_count > 0) {
        float available_space =
            horizontal ? (*width - *padding_left - *padding_right)
                       : (*height - *padding_top - *padding_bottom);
        float gap_space = (children_rev.size() - 1) * gap;
        spacer_size =
            std::max(0.0f, (available_space - total_fixed_size - gap_space) /
                               spacer_count);
    }

    // Second pass: position children
    for (size_t i = 0; i < children_rev.size(); ++i) {
        auto &child = children_rev[i];
        child->x->animate_to(x);
        child->y->animate_to(y);

        float child_size;
        if (dynamic_cast<const spacer *>(child.get())) {
            child_size = spacer_size;
            if (horizontal) {
                child->width->animate_to(spacer_size);
            } else {
                child->height->animate_to(spacer_size);
            }
        } else {
            auto [cached_width, cached_height] = measure_cache[i];
            child_size = horizontal ? cached_width : cached_height;
        }

        if (horizontal) {
            x += child_size + gap;
            auto [cached_width, cached_height] = measure_cache[i];
            target_height = std::max(target_height, cached_height);
        } else {
            y += child_size + gap;
            auto [cached_width, cached_height] = measure_cache[i];
            target_width = std::max(target_width, cached_width);
        }
    }

    if (!auto_size) {
        if (horizontal) {
            target_width = *width;
        } else {
            target_height = *height;
        }
    }

    if (horizontal) {
        width->animate_to(x - gap + *padding_left + *padding_right);
        height->animate_to(target_height + *padding_top + *padding_bottom);

        for (auto &child : children) {
            child->height->animate_to(target_height);
        }
    } else {
        width->animate_to(target_width + *padding_left + *padding_right);
        height->animate_to(y - gap + *padding_top + *padding_bottom);

        for (auto &child : children) {
            child->width->animate_to(target_width);
        }
    }
}

void ui::update_context::set_hit_hovered(widget *w) {
    hovered_widgets->push_back(w);
}
bool ui::update_context::mouse_clicked_on(widget *w, bool hittest) const {
    return mouse_clicked && hovered(w, hittest);
}
bool ui::update_context::mouse_down_on(widget *w, bool hittest) const {
    return mouse_down && hovered(w, hittest);
}
bool ui::update_context::mouse_clicked_on_hit(widget *w, bool hittest) {
    if (mouse_clicked_on(w, hittest)) {
        set_hit_hovered(w);
        return true;
    }
    return false;
}
bool ui::update_context::hovered_hit(widget *w, bool hittest) {
    if (hovered(w, hittest)) {
        set_hit_hovered(w);
        return true;
    } else {
        return false;
    }
}
bool ui::widget::check_hit(const update_context &ctx) {
    return ctx.mouse_x >= (x->dest() + ctx.offset_x) &&
           ctx.mouse_x <= (x->dest() + width->dest() + ctx.offset_x) &&
           ctx.mouse_y >= (y->dest() + ctx.offset_y) &&
           ctx.mouse_y <= (y->dest() + height->dest() + ctx.offset_y);
}
void ui::widget::update_children(
    update_context &ctx, std::vector<std::shared_ptr<widget>> &children) {
    for (auto &child : children) {
        update_child_basic(ctx, child);
        if (children_dirty)
            break;
    }

    // Remove dead children
    std::erase_if(children, [](auto &child) { return !child; });
}
void ui::widget::render_children(
    nanovg_context ctx, std::vector<std::shared_ptr<widget>> &children) {
    for (auto &child : children) {
        render_child_basic(ctx, child);
    }
}
void ui::text_widget::render(nanovg_context ctx) {
    widget::render(ctx);
    ctx.fontSize(font_size);
    ctx.fillColor(color.nvg());
    ctx.textAlign(NVG_ALIGN_TOP | NVG_ALIGN_LEFT);
    ctx.fontFace(font_family.c_str());

    ctx.text(*x, *y, text.c_str(), nullptr);
}
void ui::text_widget::update(update_context &ctx) {
    widget::update(ctx);
    ctx.vg.fontSize(font_size);
    ctx.vg.fontFace(font_family.c_str());
    auto text = ctx.vg.measureText(this->text.c_str());

    if (strink_horizontal) {
        width->animate_to(text.first);
    }

    if (strink_vertical) {
        height->animate_to(text.second);
    }
}
void ui::padding_widget::update(update_context &ctx) {
    auto off = ctx.with_offset(*padding_left, *padding_top);
    widget::update(off);

    float max_width = 0, max_height = 0;
    for (auto &child : children) {
        max_width = std::max(max_width, child->measure_width(ctx));
        max_height = std::max(max_height, child->measure_height(ctx));
    }

    width->animate_to(max_width + *padding_left + *padding_right);
    height->animate_to(max_height + *padding_top + *padding_bottom);
}
void ui::padding_widget::render(nanovg_context ctx) {
    ctx.transaction();
    ctx.translate(**padding_left, **padding_top);
    widget::render(ctx);
}
ui::update_context ui::update_context::within(widget *w) const {
    auto copy = *this;
    copy.offset_x = w->last_offset_x;
    copy.offset_y = w->last_offset_y;
    return copy;
}
void ui::update_context::print_hover_info(widget *w) const {
    std::printf(
        "widget(%p)\n\t hovered: %d x: %f y: %f width: %f height: %f \n\t"
        "mouse_x: %f mouse_y: %f (x: %d %d y: %d %d)\n",
        w, hovered(w), w->x->dest(), w->y->dest(), w->width->dest(),
        w->height->dest(), mouse_x, mouse_y,
        mouse_x >= (w->x->dest() + offset_x),
        mouse_x <= (w->x->dest() + w->width->dest() + offset_x),
        mouse_y >= (w->y->dest() + offset_y),
        mouse_y <= (w->y->dest() + w->height->dest() + offset_y));
}
bool ui::widget::focused() {
    return owner_rt && !owner_rt->focused_widget->expired() &&
           owner_rt->focused_widget->lock().get() == this;
}
bool ui::widget::focus_within() {
    return focused() || std::ranges::any_of(children, [](const auto &child) {
               return child->focus_within();
           });
}
void ui::widget::set_focus(bool focused) {
    if (owner_rt) {
        if (focused) {
            owner_rt->focused_widget = this->shared_from_this();
        } else {
            if (owner_rt->focused_widget &&
                owner_rt->focused_widget->lock().get() == this) {
                owner_rt->focused_widget.reset();
            }
        }
    }
}

bool ui::update_context::key_pressed(int key) const {
    return (bool)(rt.key_states.get()[key] & key_state::pressed);
}
bool ui::update_context::key_down(int key) const {
    return glfwGetKey((GLFWwindow *)window, key) == GLFW_PRESS;
}
void ui::update_context::stop_key_propagation(int key) {
    if (key >= 0 && key < GLFW_KEY_LAST + 1) {
        rt.key_states.get()[key] = key_state::none;
    }
}
ui::button_widget::button_widget(const std::string &button_text)
    : button_widget() {
    auto text = emplace_child<ui::text_widget>();
    text->text = button_text;
    text->font_size = 14;
    text->color.reset_to({1, 1, 1, 0.95});
}
void ui::button_widget::render(ui::nanovg_context ctx) {

    ctx.fillColor(bg_color);
    ctx.fillRoundedRect(*x, *y, *width, *height, 6);

    float bw = 1.0f;

    float radius = 6.0f;
    // 4 edges
    ctx.beginPath();
    ctx.strokeWidth(bw);
    ctx.strokeColor(border_top);
    ctx.moveTo(*x + radius, *y + bw / 2);
    ctx.lineTo(*x + *width - radius, *y + bw / 2);
    ctx.stroke();

    ctx.beginPath();
    ctx.strokeWidth(bw);
    ctx.strokeColor(border_right);
    ctx.moveTo(*x + *width - bw / 2, *y + radius);
    ctx.lineTo(*x + *width - bw / 2, *y + *height - radius);
    ctx.stroke();

    ctx.beginPath();
    ctx.strokeWidth(bw);
    ctx.strokeColor(border_bottom);
    ctx.moveTo(*x + *width - radius, *y + *height - bw / 2);
    ctx.lineTo(*x + radius, *y + *height - bw / 2);
    ctx.stroke();

    ctx.beginPath();
    ctx.strokeWidth(bw);
    ctx.strokeColor(border_left);
    ctx.moveTo(*x + bw / 2, *y + *height - radius);
    ctx.lineTo(*x + bw / 2, *y + radius);
    ctx.stroke();

    // 4 corners
    float cr = radius - bw / 2;
    ctx.beginPath();
    ctx.strokeWidth(bw);
    ctx.strokeColor(border_right.blend(border_top));
    ctx.moveTo(*x + *width - radius, *y + bw / 2);
    ctx.arcTo(*x + *width - bw / 2, *y + bw / 2, *x + *width - bw / 2,
              *y + radius, cr);
    ctx.stroke();

    ctx.beginPath();
    ctx.strokeWidth(bw);
    ctx.strokeColor(border_bottom.blend(border_right));
    ctx.moveTo(*x + *width - bw / 2, *y + *height - radius);
    ctx.arcTo(*x + *width - bw / 2, *y + *height - bw / 2, *x + *width - radius,
              *y + *height - bw / 2, cr);
    ctx.stroke();

    ctx.beginPath();
    ctx.strokeWidth(bw);
    ctx.strokeColor(border_left.blend(border_bottom));
    ctx.moveTo(*x + radius, *y + *height - bw / 2);
    ctx.arcTo(*x + bw / 2, *y + *height - bw / 2, *x + bw / 2,
              *y + *height - radius, cr);
    ctx.stroke();

    ctx.beginPath();
    ctx.strokeWidth(bw);
    ctx.strokeColor(border_top.blend(border_left));
    ctx.moveTo(*x + bw / 2, *y + radius);
    ctx.arcTo(*x + bw / 2, *y + bw / 2, *x + radius, *y + bw / 2, cr);
    ctx.stroke();

    padding_widget::render(ctx);
}
void ui::button_widget::update_colors(bool is_active, bool is_hovered) {
    if (is_active) {
        bg_color.animate_to({0.3, 0.3, 0.3, 0.7});
    } else if (is_hovered) {
        bg_color.animate_to({0.35, 0.35, 0.35, 0.7});
    } else {
        bg_color.animate_to({0.3, 0.3, 0.3, 0.6});
    }
}
void ui::button_widget::on_click() {}
void ui::button_widget::update(ui::update_context &ctx) {
    padding_widget::update(ctx);

    if (ctx.mouse_clicked_on_hit(this)) {
        this->ctx = &ctx;
        on_click();
        this->ctx = nullptr;
    }

    update_colors(ctx.mouse_down_on(this), ctx.hovered(this));
}
ui::button_widget::button_widget() {
    padding_bottom->reset_to(10);
    padding_top->reset_to(10);
    padding_left->reset_to(22);
    padding_right->reset_to(20);

    border_top.reset_to({1, 1, 1, 0.12});
    border_right.reset_to({1, 1, 1, 0.04});
    border_bottom.reset_to({1, 1, 1, 0.02});
    border_left.reset_to({1, 1, 1, 0.04});
}
void ui::widget::remove_child(std::shared_ptr<widget> child) {
    child->parent = nullptr;
    children.erase(std::remove(children.begin(), children.end(), child),
                   children.end());
    children_dirty = true;
}
