#pragma once
#include "../config.h"
#include "animator.h"
#include "taskbar.h"
#include "extra_widgets.h"
#include "nanovg_wrapper.h"
#include "ui.h"
#include "widget.h"
#include <algorithm>
#include <functional>
#include <memory>
#include <optional>

namespace mb_shell {
    struct taskbar_widget : public ui::widget {
        taskbar_widget() {
            auto text = emplace_child<ui::text_widget>();
            text->text = "Taskbar";
            text->font_size = 50;
        }
    };
} // namespace mb_shell