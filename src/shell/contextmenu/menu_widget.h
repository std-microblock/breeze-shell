#pragma once
#include "breeze_ui/animator.h"
#include "breeze_ui/extra_widgets.h"
#include "breeze_ui/nanovg_wrapper.h"
#include "breeze_ui/ui.h"
#include "breeze_ui/widget.h"
#include "contextmenu.h"
#include "shell/config.h"
#include "shell/utils.h"
#include "shell/widgets/background_widget.h"
#include <algorithm>
#include <functional>
#include <memory>
#include <optional>

namespace mb_shell {

struct menu_widget;

struct menu_item_widget : public ui::widget {
    using super = ui::widget;
    menu_item item;
    ui::sp_anim_float opacity = anim_float(0, 200);
    menu_item_widget();
    virtual void reset_appear_animation(float delay);
    menu_widget *owner_menu() const;
};

struct menu_item_ownerdraw_widget : public menu_item_widget {
    using super = menu_item_widget;
    owner_draw_menu_info owner_draw;
    std::optional<ui::NVGImage> img{};
    menu_item_ownerdraw_widget(menu_item item);
    void render(ui::nanovg_context ctx) override;
    void reset_appear_animation(float delay) override;
};

struct menu_item_parent_widget : public menu_item_widget {
    using super = menu_item_widget;
    bool lays_out_children() const override { return true; }
    void before_layout() override;
    void reset_appear_animation(float delay) override;
};

struct menu_item_normal_widget : public menu_item_widget {
    using super = menu_item_widget;
    ui::sp_anim_float opacity = anim_float(0, 200);
    float text_padding = config::current->context_menu.theme.text_padding;
    float margin = config::current->context_menu.theme.margin;
    bool has_icon_padding = false;
    bool has_submenu_padding = false;
    float padding = config::current->context_menu.theme.padding;
    float icon_padding = config::current->context_menu.theme.icon_padding;
    float right_icon_padding =
        config::current->context_menu.theme.right_icon_padding;
    menu_item_normal_widget(menu_item item);
    void reset_appear_animation(float delay) override;

    std::optional<ui::NVGImage> icon_img{};
    std::optional<ui::NVGImage> icon_unfold_img{};

    std::shared_ptr<menu_widget> submenu_wid = nullptr;
    float show_submenu_timer = 0.f;

    ui::sp_anim_float bg_opacity = anim_float(0, 200);
    ui::sp_anim_float text_blur = anim_float(0, 200);
    void render(ui::nanovg_context ctx) override;
    void tick(float delta_time) override;
    void before_layout() override;
    bool has_measure() const override { return true; }
    YGSize measure(float width, YGMeasureMode width_mode, float height,
                   YGMeasureMode height_mode) override;
    void handle_mouse_down(ui::mouse_event &e) override;

    void activate();
    void hide_submenu();
    void show_submenu();
    void reload_icon_img(ui::nanovg_context ctx);

  private:
    std::string measure_key;
};

struct menu_item_custom_widget : public menu_item_widget {
    using super = menu_item_widget;
    std::shared_ptr<ui::widget> custom_widget;
    menu_item_custom_widget(std::shared_ptr<ui::widget> custom_widget);
    bool lays_out_children() const override { return true; }
    void before_layout() override;
};

enum class popup_direction {
    top_left,
    top_right,
    bottom_left,
    bottom_right,
};
struct menu_animation_rect {
    float x = 0;
    float y = 0;
    float width = 0;
    float height = 0;
};

struct menu_widget : public ui::flex_widget {
    using super = ui::flex_widget;
    float bg_padding_vertical = 6;

    std::shared_ptr<background_widget> bg;

    std::shared_ptr<menu_widget> current_submenu;
    std::optional<std::weak_ptr<ui::widget>> parent_item_widget;

    menu_widget *parent_menu = nullptr;

    menu menu_data;
    explicit menu_widget(bool is_main = false);
    popup_direction direction = popup_direction::bottom_right;
    bool is_top_level_menu = false;
    bool bg_animation_armed = false;
    bool bg_appear_initialized = false;
    std::optional<menu_animation_rect> bg_start_rect;
    void init_from_data(menu menu_data);
    bool native_content_dirty = false;
    void resync_native_content();
    void arm_background_animation(
        std::optional<menu_animation_rect> initial_rect = std::nullopt);
    bool animate_appear_started = false;
    void reset_animation(bool reverse = false);
    void tick(float delta_time) override;
    void before_layout() override;
    void after_layout() override;
    void handle_key(ui::key_event &e) override;

    void update_icon_width();
    void add_submenu(std::shared_ptr<menu_widget> submenu);
    std::vector<std::shared_ptr<menu_widget>> submenus() const;
    bool directly_hovered() const;

    void render(ui::nanovg_context ctx) override;
    ui::widget *hit_test_tree(float px, float py) override;
    bool hit_test(float px, float py) const override;
    void close();

  private:
    bool keyboard_owner() const;
    bool closing_seen = false;
};

struct screenside_button_group_widget : public ui::flex_widget {
    struct button_widget : public ui::widget {
        using super = ui::widget;
        std::string icon_svg;
        std::optional<ui::NVGImage> icon{};
        std::function<void()> on_click;
        button_widget(std::string icon_svg);

        ui::sp_anim_float bg_opacity = anim_float(0, 200);

        void tick(float delta_time) override;
        void handle_mouse_down(ui::mouse_event &e) override;
        void render(ui::nanovg_context ctx) override;
    };

    using super = ui::flex_widget;
    screenside_button_group_widget();
};

struct mouse_menu_widget_main : public ui::widget {
    float anchor_x = 0, anchor_y = 0;
    mouse_menu_widget_main(menu menu_data, float x, float y);
    bool position_calibrated = false, direction_calibrated = false;
    bool ignore_outside_click_until_mouse_release = false;
    popup_direction direction;
    std::shared_ptr<menu_widget> menu_wid;

    void tick(float delta_time) override;

    static std::pair<float, float>
    calculate_position(menu_widget *menu_wid, ui::render_target &rt,
                       float anchor_x, float anchor_y,
                       popup_direction direction);

    static popup_direction calculate_direction(
        menu_widget *menu_wid, ui::render_target &rt, float anchor_x,
        float anchor_y,
        popup_direction prefer_direction = popup_direction::bottom_right);

    void calibrate_position(bool animated = true);
    void calibrate_direction();

  private:
    std::optional<bool> last_passthrough;
};

} // namespace mb_shell
