
#include "GLFW/glfw3.h"
#include "blook/blook.h"

#include "entry.h"
#include "menu_widget.h"
#include "script/binding_types.h"
#include "script/script.h"
#include "shell.h"
#include "ui.h"
#include "utils.h"


#include <codecvt>
#include <condition_variable>
#include <filesystem>
#include <functional>
#include <future>
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

void main() {
  // AllocConsole();
  // SetConsoleCP(CP_UTF8);
  // freopen("CONOUT$", "w", stdout);
  // freopen("CONOUT$", "w", stderr);
  // freopen("CONIN$", "r", stdin);

  std::println("Hello from mb-shell!");

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

  std::thread([&]() {
    while (true) {
      if (!track_menu_open) {
        std::this_thread::yield();
        continue;
      }
      constexpr int l_pad = 100, t_pad = 100, width = 1200, height = 1200;
      if (!track_menu_args_opt) {
        continue;
      }

      auto [hMenu, uFlags, x, y, hWnd, lptpm, tfunc] = *track_menu_args_opt;
      track_menu_args_opt.reset();
      track_menu_open = false;
      menu menu = menu::construct_with_hmenu(hMenu, hWnd);
      std::thread([=]() {
        if (auto res = ui::render_target::init_global(); !res) {
          MessageBoxW(NULL, L"Failed to initialize global render target",
                      L"Error", MB_ICONERROR);
          return;
        }

        ui::render_target rt;

        if (auto res = rt.init(); !res) {
          MessageBoxW(NULL, L"Failed to initialize render target", L"Error",
                      MB_ICONERROR);
        }

        rt.set_position(x - l_pad, y - t_pad + 5);
        rt.resize(width, height);

        auto menu_wid = std::make_shared<menu_widget>(menu, width, height);
        rt.root->children.push_back(menu_wid);

        for (auto &listener : menu_callbacks) {
          listener->operator()(
              {menu_info_basic{.from = "menu", .menu = menu_wid}});
        }

        rt.on_focus_changed = [](bool focused) {
          if (!focused) {
            glfwSetWindowShouldClose(glfwGetCurrentContext(), GLFW_TRUE);
          }
        };

        nvgCreateFont(rt.nvg, "Segui", "C:\\WINDOWS\\FONTS\\msyh.ttc");

        rt.start_loop();
      }).detach();
    }
  }).detach();

  // We have to post the message to another thread for execution as
  // any winapi call in the hook routine will cause the menu to behave
  // incorrectly for some reason
  NtUserTrackHook->install([=](HMENU hMenu, UINT uFlags, int x, int y,
                               int nReserved, HWND hWnd, const RECT *prcRect) {
    track_menu_args_opt =
        track_menu_args{hMenu, uFlags, x, y, hWnd, nullptr, nullptr};
    track_menu_open = true;
    while (track_menu_open) {
      std::this_thread::yield();
    }

    auto open = NtUserTrackHook->call_trampoline<bool>(
        hMenu, uFlags, x, y, nReserved, hWnd, prcRect);

    return open;
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
  mb_shell::main();
  return 0;
}