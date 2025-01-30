#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "animator.h"
#include "ui.h"
#include "widget.h"

static unsigned char g_icon_png[] = {
#include "icon-small.png.h"
};

#include <Windows.h>

#include <TlHelp32.h>
#include <psapi.h>
#include <shellapi.h>

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
  ShellExecuteW(NULL, L"open", L"explorer.exe", L"C:/", NULL, SW_SHOW);
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
  return 0;
}

struct inject_ui_title : public ui::widget_flex {
  inject_ui_title() {
    gap = 10;

    {
      auto title = std::make_shared<ui::text_widget>();
      title->text = "breeze-shell";
      title->font_size = 26;
      title->color.reset_to({1, 1, 1, 1});
      add_child(title);
    }

    {
      auto title = std::make_shared<ui::text_widget>();
      title->text = "Bring fluency & delication back to Windows";
      title->font_size = 14;
      title->color.reset_to({1, 1, 1, 0.8});
      add_child(title);
    }
  }
};

struct button_widget : public ui::padding_widget {
  button_widget(const std::string &button_text) {
    auto text = emplace_child<ui::text_widget>();
    text->text = button_text;
    text->font_size = 14;
    text->color.reset_to({1, 1, 1, 1});

    padding_bottom->reset_to(10);
    padding_top->reset_to(10);
    padding_left->reset_to(22);
    padding_right->reset_to(20);
  }

  ui::animated_color bg_color = {this, 40 / 255.f, 40 / 255.f, 40 / 255.f, 0.6};

  virtual void on_click() = 0;

  void render(ui::nanovg_context ctx) override {
    ctx.fillColor(bg_color.nvg());
    ctx.fillRoundedRect(*x, *y, *width, *height, 6);
    padding_widget::render(ctx);
  }

  virtual void update_colors(bool is_active, bool is_hovered) {
    if (is_active) {
      bg_color.animate_to({0.3, 0.3, 0.3, 0.7});
    } else if (is_hovered) {
      bg_color.animate_to({0.35, 0.35, 0.35, 0.7});
    } else {
      bg_color.animate_to({0.3, 0.3, 0.3, 0.6});
    }
  }

  void update(ui::update_context &ctx) override {
    padding_widget::update(ctx);

    if (ctx.mouse_clicked_on_hit(this)) {
      on_click();
    }

    update_colors(ctx.mouse_down_on(this), ctx.hovered(this));
  }
};

struct start_when_startup_switch : public button_widget {
  bool start_when_startup = false;

  static bool check_startup() {
    HKEY hkey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0,
                      KEY_READ, &hkey) != ERROR_SUCCESS) {
      return false;
    }

    wchar_t path[MAX_PATH];
    DWORD size = sizeof(path);
    bool exists = RegQueryValueExW(hkey, L"breeze-shell", nullptr, nullptr,
                                   (LPBYTE)path, &size) == ERROR_SUCCESS;
    RegCloseKey(hkey);
    return exists;
  }

  static void set_startup(bool startup) {
    HKEY hkey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0,
                      KEY_SET_VALUE, &hkey) != ERROR_SUCCESS) {
      return;
    }

    if (startup) {
      wchar_t path[MAX_PATH];
      GetModuleFileNameW(NULL, path, MAX_PATH);
      std::wstring command =
          L"\"" + std::wstring(path) + L"\" inject-consistent";
      RegSetValueExW(hkey, L"breeze-shell", 0, REG_SZ, (BYTE *)command.c_str(),
                     (command.size() + 1) * sizeof(wchar_t));
    } else {
      RegDeleteValueW(hkey, L"breeze-shell");
    }
    RegCloseKey(hkey);
  }

  start_when_startup_switch() : button_widget("开机自启") {
    start_when_startup = check_startup();
  }

  void on_click() override {
    set_startup(!start_when_startup);
    start_when_startup = check_startup();
  }

  void update_colors(bool is_active, bool is_hovered) override {
    if (start_when_startup) {
      if (is_hovered) {
        bg_color.animate_to({0.3, 0.8, 0.3, 0.7});
      } else {
        bg_color.animate_to({0.2, 0.7, 0.2, 0.6});
      }
    } else {
      button_widget::update_colors(is_active, is_hovered);
    }
  }
};

struct restart_explorer_switch : public button_widget {
  restart_explorer_switch() : button_widget("重启资源管理器") {}

  void on_click() override {
    std::thread([]() {
      std::vector<DWORD> pids = GetExplorerPIDs();
      for (DWORD pid : pids) {
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
        if (hProcess) {
          TerminateProcess(hProcess, 0);
          CloseHandle(hProcess);
        }
      }
      ShellExecuteW(NULL, L"open", L"explorer.exe", L"", NULL, SW_SHOW);
    }).detach();
  }
};

void InjectAllConsistent() {
  GetDebugPrivilege();
  std::vector<DWORD> injected;
  while (true) {
    std::vector<DWORD> pids = GetExplorerPIDs();

    for (DWORD pid : pids) {
      if (!std::ranges::contains(injected, pid) && !IsInjected(pid, dllPath)) {
        InjectToPID(pid, dllPath);
      }
    }
    Sleep(100);
    MSG msg;
    if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }
  }
}

struct inject_all_switch : public button_widget {
  bool injecting_all = false;
  inject_all_switch() : button_widget("全局注入") { check_is_injecting_all(); }

  void check_is_injecting_all() {
    HANDLE mutex = CreateMutexW(NULL, TRUE, L"breeze-shell-inject-consistent");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
      injecting_all = true;
    } else {
      injecting_all = false;
    }

    CloseHandle(mutex);
  }

  void on_click() override {
    injecting_all = !injecting_all;
    if (injecting_all) {
      wchar_t path[MAX_PATH];
      GetModuleFileNameW(NULL, path, MAX_PATH);
      SHELLEXECUTEINFOW sei = {sizeof(sei)};
      sei.fMask = SEE_MASK_NOCLOSEPROCESS;
      sei.lpFile = path;
      sei.lpParameters = L"inject-consistent";
      sei.nShow = SW_HIDE;
      ShellExecuteExW(&sei);
    } else {
      HANDLE event = CreateEventW(NULL, TRUE, FALSE,
                                  L"breeze-shell-inject-consistent-exit");
      SetEvent(event);
      CloseHandle(event);
    }

    std::thread([this]() {
      Sleep(200);
      check_is_injecting_all();
    }).detach();
  }

  void update_colors(bool is_active, bool is_hovered) override {
    if (injecting_all) {
      if (is_hovered) {
        bg_color.animate_to({0.3, 0.8, 0.3, 0.7});
      } else {
        bg_color.animate_to({0.2, 0.7, 0.2, 0.6});
      }
    } else {
      button_widget::update_colors(is_active, is_hovered);
    }
  }
};

struct inject_once_switch : public button_widget {
  inject_once_switch() : button_widget("注入一次") {}

  void on_click() override {
    std::thread([]() {
      GetDebugPrivilege();
      NewExplorerProcessAndInject();
    }).detach();
  }
};

struct breeze_icon : public ui::widget {
  std::optional<int> image;
  breeze_icon() {
    width->reset_to(50);
    height->reset_to(50);
  }

  void render(ui::nanovg_context ctx) override {
    if (!image) {
      image = nvgCreateImageMem(ctx.ctx, 0, g_icon_png, sizeof(g_icon_png));
    }
    auto paint = nvgImagePattern(ctx.ctx, *x + ctx.offset_x, *y + ctx.offset_y,
                                 *height, *height, 0, *image, 1);
    ctx.fillPaint(paint);
    ctx.fillRect(*x, *y, *height, *height);
  }
};

struct injector_ui_main : public ui::widget_flex {
  injector_ui_main() {
    x->reset_to(20);
    y->reset_to(5);
    gap = 15;
    emplace_child<breeze_icon>();
    emplace_child<inject_ui_title>();

    auto switches_box =  emplace_child<ui::widget_flex>();
    switches_box->gap = 7;
    auto switches = switches_box->emplace_child<ui::widget_flex>();
    switches->gap = 7;
    switches->horizontal = true;

    switches->emplace_child<inject_all_switch>();
    switches->emplace_child<inject_once_switch>();

    switches = switches_box->emplace_child<ui::widget_flex>();
    switches->gap = 7;
    switches->horizontal = true;
    switches->emplace_child<start_when_startup_switch>();
    switches->emplace_child<restart_explorer_switch>();
  }
  void render(ui::nanovg_context ctx) override {
    ctx.fillColor(nvgRGB(32, 32, 32));
    auto gradient_height = 130;
    ctx.fillRect(0, gradient_height, 999, 999);

    // 20->0 linear gradient
    NVGpaint bg = nvgLinearGradient(ctx.ctx, 0, gradient_height, 0, 0,
                                    nvgRGB(32, 32, 32), nvgRGBAf(0, 0, 0, 0));
    ctx.beginPath();
    ctx.moveTo(0, gradient_height);
    ctx.lineTo(999, gradient_height);
    ctx.lineTo(999, 0);
    ctx.lineTo(0, 0);
    ctx.fillPaint(bg);
    ctx.fill();

    widget_flex::render(ctx);
  }
};

void StartInjectUI() {
  if (auto r = ui::render_target::init_global(); !r) {
    std::cerr << "Failed to initialize global render target." << std::endl;
    return;
  }
  ui::render_target rt;
  rt.acrylic = 0.1;
  rt.transparent = true;
  rt.width = 400;
  rt.height = 230;
  if (auto r = rt.init(); !r) {
    std::cerr << "Failed to initialize render target." << std::endl;
    return;
  }
  nvgCreateFont(rt.nvg, "Yahei", "C:\\WINDOWS\\FONTS\\msyh.ttc");

  rt.root->emplace_child<injector_ui_main>();

  rt.start_loop();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nShowCmd) {
  AttachConsole(ATTACH_PARENT_PROCESS);
  freopen("CONOUT$", "w", stdout);
  freopen("CONOUT$", "w", stderr);
  freopen("CONIN$", "r", stdin);

  int argc = 0;
  auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);

  std::vector<std::wstring> args;
  for (int x = 0; x < argc; x++) {
    args.push_back(argv[x]);
  }

  if (args.size() <= 1) {
    StartInjectUI();
  } else {
    if (args[1] == L"new") {
      NewExplorerProcessAndInject();
    } else if (args[1] == L"inject-consistent") {
      HANDLE mutex =
          CreateMutexW(NULL, TRUE, L"breeze-shell-inject-consistent");
      if (GetLastError() == ERROR_ALREADY_EXISTS) {
        std::cerr << "Another instance is running." << std::endl;
        return 1;
      }

      std::thread([]() {
        HANDLE event = CreateEventW(NULL, TRUE, FALSE,
                                    L"breeze-shell-inject-consistent-exit");
        WaitForSingleObject(event, INFINITE);
        CloseHandle(event);
        exit(0);
      }).detach();

      InjectAllConsistent();
    } else {
      std::cerr << "Invalid argument." << std::endl;
    }
  }

  return 0;
}
