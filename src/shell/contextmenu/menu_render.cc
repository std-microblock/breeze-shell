#include "menu_render.h"
#include "Windows.h"
#include "menu_widget.h"

#include "../script/binding_types.h"
#include "ui.h"
#include <mutex>
#include <thread>

#include "../logger.h"

namespace mb_shell {
std::optional<menu_render *> menu_render::current{};
menu_render menu_render::create(int x, int y, menu menu) {
  if (auto res = ui::render_target::init_global(); !res) {
    MessageBoxW(NULL, L"Failed to initialize global render target", L"Error",
                MB_ICONERROR);
    return {nullptr, std::nullopt};
  }

  constexpr int l_pad = 100, t_pad = -1;
  static auto rt = []() {
    auto rt = std::make_shared<ui::render_target>();
    rt->transparent = true;
    rt->no_focus = true;
    rt->capture_all_input = true;
    rt->decorated = false;
    rt->topmost = true;
    rt->vsync = config::current->context_menu.vsync;

    if (config::current->avoid_resize_ui) {
      rt->width = 3840;
      rt->height = 2159;
    }

    if (auto res = rt->init(); !res) {
      MessageBoxW(NULL, L"Failed to initialize render target", L"Error",
                  MB_ICONERROR);
    }

    nvgCreateFont(rt->nvg, "main",
                  config::current->font_path_main.string().c_str());
    nvgCreateFont(rt->nvg, "fallback",
                  config::current->font_path_fallback.string().c_str());
    nvgAddFallbackFont(rt->nvg, "main", "fallback");
    return rt;
  }();
  auto render = menu_render(rt, std::nullopt);
  auto current_js_context = std::make_shared<js::js_menu_context>(
      js::js_menu_context::$from_window(menu.parent_window));

  rt->parent = menu.parent_window;

  // get the monitor in which the menu is being shown
  auto monitor = MonitorFromPoint({x, y}, MONITOR_DEFAULTTONEAREST);
  MONITORINFOEX monitor_info;
  monitor_info.cbSize = sizeof(MONITORINFOEX);
  GetMonitorInfo(monitor, &monitor_info);

  // set the position of the window to fullscreen in this monitor + padding

  dbgout("Monitor: {} {} {} {}", monitor_info.rcMonitor.left,
               monitor_info.rcMonitor.top, monitor_info.rcMonitor.right,
               monitor_info.rcMonitor.bottom);

  rt->set_position(monitor_info.rcMonitor.left, monitor_info.rcMonitor.top);
  if (!config::current->avoid_resize_ui)
    rt->resize(
        monitor_info.rcMonitor.right - monitor_info.rcMonitor.left + l_pad,
        monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top + t_pad);

  glfwMakeContextCurrent(rt->window);
  glfwSwapInterval(config::current->context_menu.vsync ? 1 : 0);

  rt->show();
  auto menu_wid = std::make_shared<mouse_menu_widget_main>(
      menu,
      // convert the x and y to the window coordinates
      x - monitor_info.rcMonitor.left, y - monitor_info.rcMonitor.top);
  rt->root->children.push_back(menu_wid);
  js::menu_info_basic_js menu_info{
      .menu = std::make_shared<js::menu_controller>(menu_wid->menu_wid),
      .context = current_js_context};

  dbgout("[perf] JS plugins start");
  auto before_js = rt->clock.now();
  for (auto &listener : menu_callbacks_js) {
    listener->operator()(menu_info);
  }
  dbgout("[perf] JS plugins costed {}ms",
               std::chrono::duration_cast<std::chrono::milliseconds>(
                   rt->clock.now() - before_js)
                   .count());

  rt->on_focus_changed = [](bool focused) {
    if (!focused) {
    }
  };

  dbgout("Current menu: {}", menu_render::current.has_value());
  return render;
}

menu_render::menu_render(std::shared_ptr<ui::render_target> rt,
                         std::optional<int> selected_menu)
    : rt(std::move(rt)), selected_menu(selected_menu) {
  current = this;
}
menu_render::~menu_render() {
  if (this->rt) {
    current = nullptr;
  }
}
menu_render::menu_render(menu_render &&t) {
  current = this;

  rt = std::move(t.rt);
  selected_menu = std::move(t.selected_menu);
}
menu_render &menu_render::operator=(menu_render &&t) {
  current = this;
  rt = std::move(t.rt);
  selected_menu = std::move(t.selected_menu);
  return *this;
}
}; // namespace mb_shell