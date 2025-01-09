
#include "GLFW/glfw3.h"
#include "blook/blook.h"

#include "entry.h"
#include "menu_widget.h"
#include "script/binding_types.h"
#include "script/quickjspp.hpp"
#include "script/script.h"
#include "shell.h"
#include "ui.h"
#include "utils.h"

#include <codecvt>
#include <condition_variable>
#include <filesystem>
#include <functional>
#include <future>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <consoleapi.h>
#include <debugapi.h>
#include <type_traits>
#include <winuser.h>

#define NOMINMAX
#include <Windows.h>

namespace mb_shell {

struct track_menu_args {
  HMENU hMenu;
  UINT uFlags;
  int x;
  int y;
  HWND hWnd;
  LPTPMPARAMS lptpm;
  void (*tfunc)(HWND, UINT, int, int, LPTPMPARAMS);
};

std::optional<track_menu_args> track_menu_args_opt;
bool track_menu_open = false;
thread_local std::optional<menu_render*> menu_render::current{};
menu_render show_menu(int x, int y, menu menu) {
    std::println("Hello from mb-shell!");
  if (auto res = ui::render_target::init_global(); !res) {
    MessageBoxW(NULL, L"Failed to initialize global render target", L"Error",
                MB_ICONERROR);
    return {nullptr, std::nullopt};
  }

  constexpr int l_pad = 100, t_pad = 100, width = 1200, height = 2400;

  auto rt = std::make_unique<ui::render_target>();

  if (auto res = rt->init(); !res) {
    MessageBoxW(NULL, L"Failed to initialize render target", L"Error",
                MB_ICONERROR);
  }

  rt->set_position(x - l_pad, y - t_pad + 5);
  rt->resize(width, height);

  auto menu_wid = std::make_shared<mouse_menu_widget_main>(menu, l_pad, t_pad);
  rt->root->children.push_back(menu_wid);

  for (auto &listener : menu_callbacks) {
    listener->operator()({menu_info_basic{.from = "menu", .menu = menu_wid}});
  }

  // rt.on_focus_changed = [](bool focused) {
  //   if (!focused) {
  //     glfwSetWindowShouldClose(glfwGetCurrentContext(), GLFW_TRUE);
  //   }
  // };

  nvgCreateFont(rt->nvg, "Yahei", "C:\\WINDOWS\\FONTS\\msyh.ttc");

  return {std::move(rt), std::nullopt};
}

void main() {
  AllocConsole();
  SetConsoleCP(CP_UTF8);
  freopen("CONOUT$", "w", stdout);
  freopen("CONOUT$", "w", stderr);
  freopen("CONIN$", "r", stdin);

  std::thread([]() {
    script_context ctx;

    if (std::filesystem::exists("J:\\Projects\\b-shell\\test.js"))
      ctx.watch_file("J:\\Projects\\b-shell\\test.js");
    else
      ctx.watch_file("D:\\shell\\test.js");
  }).detach();

  auto proc = blook::Process::self();
  auto win32u = proc->module("win32u.dll");
  auto user32 = proc->module("user32.dll");
  // hook NtUserTrackPopupMenu

  auto NtUserTrackPopupMenu = user32.value()->exports("TrackPopupMenu");
  auto NtUserTrackHook = NtUserTrackPopupMenu->inline_hook();

  // We have to post the message to another thread for execution as
  // any winapi call in the hook routine will cause the menu to behave
  // incorrectly for some reason
  NtUserTrackHook->install([=](HMENU hMenu, UINT uFlags, int x, int y,
                               int nReserved, HWND hWnd, const RECT *prcRect) {
    // auto open = NtUserTrackHook->call_trampoline<int>(
    //     hMenu, uFlags, x, y, nReserved, hWnd, prcRect);

    menu menu = menu::construct_with_hmenu(hMenu, hWnd);
    auto menu_render = show_menu(x, y, menu);
    menu_render.rt->start_loop();

    return menu_render.selected_menu.value_or(0);
  });
}
menu_render::menu_render(std::unique_ptr<ui::render_target> rt,
                         std::optional<int> selected_menu)
    : rt(std::move(rt)), selected_menu(selected_menu) {
  current = this;
}
menu_render::~menu_render() { current = std::nullopt; }
} // namespace mb_shell

int APIENTRY DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID lpvReserved) {
  switch (fdwReason) {
  case DLL_PROCESS_ATTACH: {
    auto cmdline = std::string(GetCommandLineA());

    std::ranges::transform(cmdline, cmdline.begin(), tolower);

    if (cmdline.contains("explorer")) {
      mb_shell::main();
    }
    break;
  }
  }
  return 1;
}

int main() {
  if (auto res = ui::render_target::init_global(); !res) {
    MessageBoxW(NULL, L"Failed to initialize global render target", L"Error",
                MB_ICONERROR);
    return 0;
  }

  mb_shell::script_context ctx;

  auto path = []() {
    if (std::filesystem::exists("J:\\Projects\\b-shell\\test.js"))
      return "J:\\Projects\\b-shell\\test.js";
    return "D:\\shell\\test.js";
  }();

  static std::optional<mb_shell::menu_render> menu_render;
  ctx.watch_file(path, []() {
    if (menu_render)
      menu_render->rt->close();
    using namespace mb_shell;
    std::thread([]() {
      mb_shell::menu m{
          .items = {
              {.type = menu_item::type::button, .name = "层叠窗口(D)"},
              {.type = menu_item::type::button, .name = "堆叠显示窗口(E)"},
              {.type = menu_item::type::button, .name = "并排显示窗口(I)"},
              {.type = menu_item::type::spacer},
              {.type = menu_item::type::button, .name = "最小化所有窗口(M)"},
              {.type = menu_item::type::button, .name = "还原所有窗口(R)"},
          }};
      // menu_render = mb_shell::show_menu(0, 0, m);
      menu_render->rt->start_loop();
    }).detach();
  });
  return 0;
}