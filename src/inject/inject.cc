#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "animator.h"
#include "ui.h"
#include "widget.h"

#include <Windows.h>

#include <TlHelp32.h>
#include <psapi.h>

namespace fs = std::filesystem;

std::wstring GetModuleDirectory() {
  wchar_t path[MAX_PATH];
  GetModuleFileNameW(NULL, path, MAX_PATH);
  return fs::path(path).parent_path().wstring();
}

std::vector<DWORD> GetExplorerPIDs() {
  std::vector<DWORD> pids;
  HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (hSnapshot != INVALID_HANDLE_VALUE) {
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(hSnapshot, &pe32)) {
      do {
        if (strcmp(pe32.szExeFile, "explorer.exe") == 0) {
          pids.push_back(pe32.th32ProcessID);
        }
      } while (Process32Next(hSnapshot, &pe32));
    }
    CloseHandle(hSnapshot);
  }
  return pids;
}

void GetDebugPrivilege() {
  HANDLE hToken;
  LUID luid;
  TOKEN_PRIVILEGES tkp;

  if (!OpenProcessToken(GetCurrentProcess(),
                        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
    std::cerr << "OpenProcessToken failed: " << GetLastError() << std::endl;
    return;
  }

  if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)) {
    std::cerr << "LookupPrivilegeValue failed: " << GetLastError() << std::endl;
    CloseHandle(hToken);
    return;
  }

  tkp.PrivilegeCount = 1;
  tkp.Privileges[0].Luid = luid;
  tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

  if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(TOKEN_PRIVILEGES),
                             NULL, NULL)) {
    std::cerr << "AdjustTokenPrivileges failed: " << GetLastError()
              << std::endl;
    CloseHandle(hToken);
    return;
  }

  CloseHandle(hToken);
}

int InjectToPID(int targetPID, std::wstring &dllPath) {

  HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, targetPID);
  if (hProcess == NULL) {
    std::cerr << "OpenProcess failed: " << GetLastError() << std::endl;
    return 1;
  }

  LPVOID remoteString =
      VirtualAllocEx(hProcess, NULL, (dllPath.size() + 1) * sizeof(wchar_t),
                     MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  if (remoteString == NULL) {
    std::cerr << "VirtualAllocEx failed: " << GetLastError() << std::endl;
    CloseHandle(hProcess);
    return 1;
  }

  if (!WriteProcessMemory(hProcess, remoteString, dllPath.c_str(),
                          (dllPath.size() + 1) * sizeof(wchar_t), NULL)) {
    std::cerr << "WriteProcessMemory failed: " << GetLastError() << std::endl;
    VirtualFreeEx(hProcess, remoteString, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    return 1;
  }

  HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
  LPTHREAD_START_ROUTINE loadLibraryW =
      (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32, "LoadLibraryW");

  HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, loadLibraryW,
                                      remoteString, 0, NULL);
  if (hThread == NULL) {
    std::cerr << "CreateRemoteThread failed: " << GetLastError() << std::endl;
    VirtualFreeEx(hProcess, remoteString, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    return 1;
  }

  WaitForSingleObject(hThread, INFINITE);
  CloseHandle(hThread);
  VirtualFreeEx(hProcess, remoteString, 0, MEM_RELEASE);
  CloseHandle(hProcess);

  std::cout << "DLL injected successfully." << std::endl;
  return 0;
}

bool IsInjected(DWORD targetPID, std::wstring &dllPath) {
  HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                                FALSE, targetPID);
  if (hProcess == NULL) {
    std::cerr << "OpenProcess failed: " << GetLastError() << std::endl;
    return false;
  }

  HMODULE hMods[1024];
  DWORD cbNeeded;
  if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
    for (int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
      wchar_t szModName[MAX_PATH];
      if (GetModuleFileNameExW(hProcess, hMods[i], szModName,
                               sizeof(szModName) / sizeof(wchar_t))) {
        if (dllPath.ends_with(
                fs::path(szModName).filename().wstring().c_str())) {
          CloseHandle(hProcess);
          return true;
        }
      }
    }
  }

  CloseHandle(hProcess);
  return false;
}

static std::wstring dllPath = GetModuleDirectory() + L"\\shell.dll";

int NewExplorerProcessAndInject() {
  GetDebugPrivilege();
  std::vector<DWORD> initialPIDs = GetExplorerPIDs();
  system("explorer.exe C:/");
  std::this_thread::sleep_for(std::chrono::seconds(1));
  std::vector<DWORD> newPIDs = GetExplorerPIDs();
  DWORD targetPID = 0;
  for (DWORD pid : newPIDs) {
    bool found = false;
    for (DWORD initialPID : initialPIDs) {
      if (pid == initialPID) {
        found = true;
        break;
      }
    }
    if (!found) {
      targetPID = pid;
      break;
    }
  }

  if (targetPID == 0) {
    std::cerr << "Could not find new explorer.exe process." << std::endl;
    return 1;
  }

  InjectToPID(targetPID, dllPath);
}

struct inject_ui_title : public ui::widget_flex {
  inject_ui_title() {
    {
      auto title = std::make_shared<ui::text_widget>();
      title->text = "breeze-shell";
      title->font_size = 14;
      title->color.reset_to({1, 1, 1, 1});
      add_child(title);
    }

    {
      auto title = std::make_shared<ui::text_widget>();
      title->text = "Bring fluency & delication back to Windows";
      title->font_size = 8;
      title->color.reset_to({1, 1, 1, 0.8});
      add_child(title);
    }
  }
};

struct inject_all_switch : public ui::widget {
  bool inject_all = false;
  std::optional<std::thread> inject_thread;

  inject_all_switch() {
    height->reset_to(20);
    width->reset_to(50);
  }

  ui::animated_color bg_color = {this, 40 / 255.f, 40 / 255.f, 40 / 255.f, 0.4};
  void render(ui::nanovg_context ctx) override {
    widget::render(ctx);

    ctx.fillColor(bg_color.nvg());
    ctx.fillRoundedRect(*x, *y, 50, *height, 5);

    ctx.fontSize(9);
    ctx.fillColor(nvgRGB(255, 255, 255));
    ctx.text(*x + 7, *y + 13, "全局注入", nullptr);
  }

  void update(ui::update_context &ctx) override {
    widget::update(ctx);

    if (ctx.mouse_clicked_on_hit(this)) {
      inject_all = !inject_all;
      if (inject_all && !inject_thread) {
        inject_thread = std::thread([this]() {
          GetDebugPrivilege();
          std::vector<DWORD> injected;
          while (true) {
            std::vector<DWORD> pids = GetExplorerPIDs();
            if (inject_all) {
              for (DWORD pid : pids) {
                if (!std::ranges::contains(injected, pid) &&
                    !IsInjected(pid, dllPath)) {
                  InjectToPID(pid, dllPath);
                }
              }
            }
            Sleep(100);
          }
        });
      }
    }

    if (inject_all) {
      if (ctx.hovered(this)) {
        bg_color.animate_to({0.4, 0.6, 0.4, 0.4});
      } else {
        bg_color.animate_to({0.2, 0.6, 0.2, 0.4});
      }
    } else {
      if (ctx.hovered(this)) {
        bg_color.animate_to({0.3, 0.3, 0.3, 0.4});
      } else {
        bg_color.animate_to({0.4, 0.4, 0.4, 0.4});
      }
    }
  }
};

struct inject_once_switch : public ui::widget {
  inject_once_switch() {
    height->reset_to(20);
    width->reset_to(50);
  }

  ui::animated_color bg_color = {this, 40 / 255.f, 40 / 255.f, 40 / 255.f, 0.4};
  void render(ui::nanovg_context ctx) override {
    widget::render(ctx);

    ctx.fillColor(bg_color.nvg());
    ctx.fillRoundedRect(*x, *y, 50, *height, 5);

    ctx.fontSize(9);
    ctx.fillColor(nvgRGB(255, 255, 255));
    ctx.text(*x + 7, *y + 13, "测试注入", nullptr);
  }

  void update(ui::update_context &ctx) override {
    widget::update(ctx);

    if (ctx.mouse_clicked_on_hit(this)) {
      std::thread([]() {
        GetDebugPrivilege();
        NewExplorerProcessAndInject();
      }).detach();
    }

    if (ctx.mouse_down_on(this)) {
      bg_color.animate_to({0.2, 0.2, 0.2, 0.4});
    } else if (ctx.hovered(this)) {
      bg_color.animate_to({0.3, 0.3, 0.3, 0.4});
    } else {
      bg_color.animate_to({0.4, 0.4, 0.4, 0.4});
    }
  }
};

struct injector_ui_main : public ui::widget_flex {
  injector_ui_main() {
    x->reset_to(10);
    y->reset_to(15);
    gap = 10;
    emplace_child<inject_ui_title>();

    auto switches = emplace_child<ui::widget_flex>();

    switches->gap = 7;
    switches->horizontal = true;

    switches->emplace_child<inject_all_switch>();
    switches->emplace_child<inject_once_switch>();
  }
};

void StartInjectUI() {
  if (auto r = ui::render_target::init_global(); !r) {
    std::cerr << "Failed to initialize global render target." << std::endl;
    return;
  }
  ui::render_target rt;
  rt.transparent = true;
  rt.acrylic = true;
  rt.width = 400;
  rt.height = 200;
  if (auto r = rt.init(); !r) {
    std::cerr << "Failed to initialize render target." << std::endl;
    return;
  }
  nvgCreateFont(rt.nvg, "Yahei", "C:\\WINDOWS\\FONTS\\msyh.ttc");

  rt.root->emplace_child<injector_ui_main>();

  rt.start_loop();
}

int main(int argc, char **argv) {
  std::vector<std::string> args(argv, argv + argc);

  if (args.size() <= 1) {
    StartInjectUI();
  } else {
    if (args[1] == "new") {
      NewExplorerProcessAndInject();
    } else {
      std::cerr << "Invalid argument." << std::endl;
    }
  }

  return 0;
}
