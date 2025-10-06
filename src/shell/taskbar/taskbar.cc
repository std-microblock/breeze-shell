#include "taskbar.h"
#include "breeze_ui/widget.h"

#include "taskbar_widget.h"

#include <shellapi.h>

#include "shell/config.h"

namespace mb_shell {
std::expected<void, std::string> taskbar_render::init() {
    rt.transparent = true;
    rt.topmost = true;
    rt.decorated = false;
    rt.title = "Breeze Shell Taskbar";
    if (auto res = rt.init(); !res) {
        return std::unexpected(res.error());
    }

    std::println("Taskbar Monitor: {}, {}, {}, {}", monitor.rcMonitor.left,
                 monitor.rcMonitor.top, monitor.rcMonitor.right,
                 monitor.rcMonitor.bottom);

    int height = (monitor.rcMonitor.bottom - monitor.rcMonitor.top) / 20;
    rt.show();
    config::current->apply_fonts_to_nvg(rt.nvg);

    bool top = position == menu_position::top;

    rt.resize(monitor.rcMonitor.right - monitor.rcMonitor.left, height);
    if (top) {
        rt.set_position(monitor.rcMonitor.left, monitor.rcMonitor.top);
    } else {
        rt.set_position(monitor.rcMonitor.left,
                        monitor.rcMonitor.bottom - height);
    }

    APPBARDATA abd = {sizeof(APPBARDATA)};
    abd.hWnd = (HWND)rt.hwnd();
    abd.uEdge = top ? ABE_TOP : ABE_BOTTOM;
    abd.rc =
        top ? RECT{monitor.rcMonitor.left, monitor.rcMonitor.top,
                   monitor.rcMonitor.right, monitor.rcMonitor.top + height}
            : RECT{monitor.rcMonitor.left, monitor.rcMonitor.bottom - height,
                   monitor.rcMonitor.right, monitor.rcMonitor.bottom};

    if (SHAppBarMessage(ABM_NEW, &abd) == 0) {
        return std::unexpected("Failed to register taskbar app");
    }

    SHAppBarMessage(ABM_QUERYPOS, &abd);
    SHAppBarMessage(ABM_SETPOS, &abd);
    rt.set_position(abd.rc.left, abd.rc.top);
    abd.lParam = TRUE;
    SHAppBarMessage(ABM_ACTIVATE, &abd);
    SHAppBarMessage(ABM_WINDOWPOSCHANGED, &abd);

    auto taskbar = rt.root->emplace_child<taskbar::taskbar_widget>();
    taskbar->width->reset_to(
        (monitor.rcMonitor.right - monitor.rcMonitor.left) / rt.dpi_scale);
    taskbar->height->reset_to(height);

    return {};
}
} // namespace mb_shell