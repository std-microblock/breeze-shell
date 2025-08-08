#pragma once
#include "async_simple/Try.h"
#include "breeze_ui/animator.h"
#include "breeze_ui/extra_widgets.h"
#include "breeze_ui/hbitmap_utils.h"
#include "breeze_ui/nanovg_wrapper.h"
#include "breeze_ui/ui.h"
#include "breeze_ui/widget.h"
#include "nanovg.h"
#include "shell/config.h"
#include "taskbar.h"
#include <algorithm>
#include <functional>
#include <memory>
#include <optional>

#include "async_simple/coro/Lazy.h"

#include "shell/widgets/background_widget.h"

namespace mb_shell::taskbar {
struct taskbar_widget : public ui::widget_flex {
    taskbar_widget();
};
} // namespace mb_shell::taskbar