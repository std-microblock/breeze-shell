#include "menu_render.h"
#include "Windows.h"
#include "menu_widget.h"

#include "../script/binding_types.h"

namespace mb_shell {
thread_local std::optional<menu_render *> menu_render::current{};
menu_render menu_render::create(int x, int y, menu menu) {
  if (auto res = ui::render_target::init_global(); !res) {
    MessageBoxW(NULL, L"Failed to initialize global render target", L"Error",
                MB_ICONERROR);
    return {nullptr, std::nullopt};
  }

  constexpr int l_pad = 100, t_pad = 100;

  auto render =
      menu_render(std::make_unique<ui::render_target>(), std::nullopt);
  auto &rt = render.rt;

  rt->parent = menu.parent_window;

  if (auto res = rt->init(); !res) {
    MessageBoxW(NULL, L"Failed to initialize render target", L"Error",
                MB_ICONERROR);
  }

  // get the monitor in which the menu is being shown
  auto monitor = MonitorFromPoint({x, y}, MONITOR_DEFAULTTONEAREST);
  MONITORINFOEX monitor_info;
  monitor_info.cbSize = sizeof(MONITORINFOEX);
  GetMonitorInfo(monitor, &monitor_info);

  // set the position of the window to fullscreen in this monitor + padding

  std::println("Monitor: {} {} {} {}", monitor_info.rcMonitor.left,
               monitor_info.rcMonitor.top, monitor_info.rcMonitor.right,
               monitor_info.rcMonitor.bottom);

  rt->set_position(monitor_info.rcMonitor.left, monitor_info.rcMonitor.top);
  rt->resize(monitor_info.rcMonitor.right - monitor_info.rcMonitor.left + l_pad,
             monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top +
                 t_pad);

  auto menu_wid = std::make_shared<mouse_menu_widget_main>(
      menu,
      // convert the x and y to the window coordinates
      x - monitor_info.rcMonitor.left, y - monitor_info.rcMonitor.top);

  rt->root->children.push_back(menu_wid);

  for (auto &listener : menu_callbacks) {
    listener->operator()({menu_info_basic{.from = "menu", .menu = menu_wid}});
  }

  rt->on_focus_changed = [](bool focused) {
    if (!focused) {
      glfwSetWindowShouldClose(glfwGetCurrentContext(), GLFW_TRUE);
    }
  };

  nvgCreateFont(rt->nvg, "Yahei", "C:\\WINDOWS\\FONTS\\msyh.ttc");
  std::println("Current menu: {}", menu_render::current.has_value());
  return render;
}
}; // namespace mb_shell