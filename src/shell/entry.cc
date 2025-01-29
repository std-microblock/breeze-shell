
#include "GLFW/glfw3.h"
#include "blook/blook.h"

#include "entry.h"
#include "script/binding_types.h"
#include "script/quickjspp.hpp"
#include "script/script.h"
#include "ui.h"
#include "utils.h"

#include "./contextmenu/menu_render.h"
#include "./contextmenu/menu_widget.h"
#include "./contextmenu/shell.h"

#include <chrono>
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

void main() {
  AllocConsole();
  SetConsoleCP(CP_UTF8);
  freopen("CONOUT$", "w", stdout);
  freopen("CONOUT$", "w", stderr);
  freopen("CONIN$", "r", stdin);

  std::thread([]() {
    script_context ctx;

    if (std::filesystem::exists("J:\\Projects\\b-shell\\test.ts"))
      ctx.watch_file("J:\\Projects\\b-shell\\test.ts");
    else if (std::filesystem::exists("D:\\shell\\test.ts"))
      ctx.watch_file("D:\\shell\\test.ts");
    else if (std::filesystem::exists(
                 (std::filesystem::current_path() / "test.ts")))
      ctx.watch_file((std::filesystem::current_path() / "test.ts"));
  }).detach();

  auto proc = blook::Process::self();
  auto win32u = proc->module("win32u.dll");
  auto user32 = proc->module("user32.dll");
  // hook NtUserTrackPopupMenu

  auto NtUserTrackPopupMenu = win32u.value()->exports("NtUserTrackPopupMenuEx");
  auto NtUserTrackHook = NtUserTrackPopupMenu->inline_hook();

  std::thread([]() {
    if (auto res = ui::render_target::init_global(); !res) {
      MessageBoxW(NULL, L"Failed to initialize global render target", L"Error",
                  MB_ICONERROR);
      return;
    }
  }).detach();

  NtUserTrackHook->install([=](HMENU hMenu, UINT uFlags, int x, int y,
                               HWND hWnd, LPTPMPARAMS lptpm) {
    menu menu = menu::construct_with_hmenu(hMenu, hWnd);
    auto menu_render = menu_render::create(x, y, menu);
    menu_render.rt->last_time = menu_render.rt->clock.now();
    menu_render.rt->render();
    menu_render.rt->last_time = menu_render.rt->clock.now();
    menu_render.rt->start_loop();

    return menu_render.selected_menu.value_or(0);
  });
}
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
  Sleep(1000);
  if (auto res = ui::render_target::init_global(); !res) {
    MessageBoxW(NULL, L"Failed to initialize global render target", L"Error",
                MB_ICONERROR);
    return 0;
  }

  mb_shell::script_context ctx;

  auto path = []() {
    if (std::filesystem::exists("J:\\Projects\\b-shell\\test.ts"))
      return "J:\\Projects\\b-shell\\test.ts";
    return "D:\\shell\\test.ts";
  }();

  static std::optional<mb_shell::menu_render> menu_render;
  ctx.watch_file(path, []() {
    if (menu_render)
      menu_render->rt->close();
    using namespace mb_shell;
    std::thread([]() {
      mb_shell::menu m{
          .items =
              {{.type = menu_item::type::button, .name = "层叠窗口(D)"},
               {.type = menu_item::type::button, .name = "堆叠显示窗口(E)"},
               {.type = menu_item::type::button, .name = "并排显示窗口(I)"},
               {
                   .type = menu_item::type::button,
                   .name = "测试多层菜单",
                   .submenu =
                       [](auto mw) {
                         mw->init_from_data(menu{
                             .items = {
                                 {.type = menu_item::type::button,
                                  .name = "测试多层菜单1",
                                  .submenu =
                                      [](auto mw) {
                                        mw->init_from_data(menu{
                                            .items =
                                                {
                                                    {.type = menu_item::type::
                                                         button,
                                                     .name = "测试多层菜单1-1"},
                                                    {.type = menu_item::type::
                                                         button,
                                                     .name = "测试多层菜单1-2"},
                                                    {.type = menu_item::type::
                                                         button,
                                                     .name = "测试多层菜单1-3"},
                                                },
                                        });
                                      }},
                             }});
                       },
               }},
          .is_top_level = true,
      };
      menu_render = menu_render::create(100, 100, m);
      std::println("Current menu:2 {}", menu_render::current.has_value());
      menu_render->rt->start_loop();
    }).detach();
  });
  return 0;
}