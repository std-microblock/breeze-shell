#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "breeze_ui/animator.h"
#include "breeze_ui/ui.h"
#include "breeze_ui/widget.h"

static unsigned char g_icon_png[] = {
#include "icon-small.png.h"
};

#include <Windows.h>

#include "data_directory.inc"
#include <TlHelp32.h>
#include <comdef.h>
#include <psapi.h>
#include <shellapi.h>
#include <taskschd.h>


#include "Shlobj.h"

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
                std::string exeFile(pe32.szExeFile);
                if (exeFile == "explorer.exe" ||
                    exeFile == "OneCommander.exe" ||
                    exeFile == "360FileBrowser64.exe" ||
                    exeFile == "DesktopMgr64.exe") {
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
        std::cerr << "LookupPrivilegeValue failed: " << GetLastError()
                  << std::endl;
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

void ShowCrashDialog();

constexpr int MAX_CRASH_COUNT = 3;
int InjectToPID(int targetPID, std::wstring_view dllPath) {
    static int crash_count = 0;

    if (crash_count >= MAX_CRASH_COUNT) {
        return 1;
    }

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

    if (!WriteProcessMemory(hProcess, remoteString, dllPath.data(),
                            (dllPath.size() + 1) * sizeof(wchar_t), NULL)) {
        std::cerr << "WriteProcessMemory failed: " << GetLastError()
                  << std::endl;
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
        std::cerr << "CreateRemoteThread failed: " << GetLastError()
                  << std::endl;
        VirtualFreeEx(hProcess, remoteString, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }

    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);

    std::thread([hProcess]() {
        WaitForSingleObject(hProcess, INFINITE);
        auto exitCode = 0;
        if (!GetExitCodeProcess(hProcess, (LPDWORD)&exitCode)) {
            std::cerr << "GetExitCodeProcess failed: " << GetLastError()
                      << std::endl;
        }
        if (exitCode != 0) {
            std::cerr << "Process exited with code: " << exitCode << std::endl;
            if (++crash_count >= MAX_CRASH_COUNT) {
                ShowCrashDialog();
            }
        } else {
            crash_count = 0;
        }
        CloseHandle(hProcess);
    }).detach();

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

static std::wstring dllPath;

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

static bool english = GetUserDefaultUILanguage() != 2052;

struct inject_ui_title : public ui::flex_widget {
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

struct start_when_startup_switch : public ui::button_widget {
    bool start_when_startup = false;

    static bool check_startup() {
        HKEY hkey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER,
                          L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
                          0, KEY_READ, &hkey) != ERROR_SUCCESS) {
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
                          L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
                          0, KEY_SET_VALUE, &hkey) != ERROR_SUCCESS) {
            return;
        }

        if (startup) {
            wchar_t path[MAX_PATH];
            GetModuleFileNameW(NULL, path, MAX_PATH);
            std::wstring command =
                L"\"" + std::wstring(path) + L"\" inject-consistent";
            RegSetValueExW(hkey, L"breeze-shell", 0, REG_SZ,
                           (BYTE *)command.c_str(),
                           (command.size() + 1) * sizeof(wchar_t));
        } else {
            RegDeleteValueW(hkey, L"breeze-shell");
        }
        RegCloseKey(hkey);
    }

    start_when_startup_switch()
        : button_widget(english ? "Start on boot" : "开机自启") {
        start_when_startup = check_startup();
    }

    void on_click() override {
        set_startup(!start_when_startup);
        start_when_startup = check_startup();

        if (!start_when_startup) {
            RegDeleteKeyValueW(
                HKEY_CURRENT_USER,
                L"Software\\Classes\\CLSID\\{86ca1aa0-34aa-4e8b-a509-"
                L"50c905bae2a2}\\InprocServer32",
                nullptr);
        }
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

enum class StartupPriority { Disabled = 0, Normal = 1, High = 2 };

struct startup_priority_selector : public ui::flex_widget {
    StartupPriority current_priority = StartupPriority::Disabled;

    startup_priority_selector() {
        horizontal = true;
        gap = 0;
        current_priority = get_current_priority();

        const char *labels_en[] = {"Disabled", "Startup", "High Priority"};
        const char *labels_zh[] = {"不自启动", "自启动", "高优先级自启动"};
        const char **labels = english ? labels_en : labels_zh;

        for (int i = 0; i < 3; i++) {
            auto segment =
                std::make_shared<priority_segment_button>(labels[i], i);
            segment->priority = (StartupPriority)i;
            segment->selector = this;
            add_child(segment);
        }
    }

    struct priority_segment_button : public ui::button_widget {
        StartupPriority priority;
        startup_priority_selector *selector = nullptr;
        int position = 0;
        priority_segment_button(const char *label_text, int pos)
            : button_widget(label_text), position(pos) {
            padding_left->reset_to(16);
            padding_right->reset_to(16);
        }

        void on_click() override {
            if (selector) {
                selector->set_priority(priority);
            }
        }

        void update_colors(bool is_active, bool is_hovered) override {
            bool is_current =
                (selector && selector->current_priority == priority);

            if (is_current) {
                if (is_hovered) {
                    bg_color.animate_to({0.3, 0.8, 0.3, 0.7});
                } else {
                    bg_color.animate_to({0.2, 0.7, 0.2, 0.6});
                }
            } else {
                if (is_active) {
                    bg_color.animate_to({0.3, 0.3, 0.3, 0.7});
                } else if (is_hovered) {
                    bg_color.animate_to({0.35, 0.35, 0.35, 0.7});
                } else {
                    bg_color.animate_to({0.3, 0.3, 0.3, 0.6});
                }
            }
        }

        void render(ui::nanovg_context ctx) override {
            ctx.fillColor(bg_color);

            float radius = 6.0f;
            if (position == 0) {
                ctx.beginPath();
                ctx.roundedRectVarying(*x, *y, *width, *height, radius, 0, 0,
                                       radius);
                ctx.fill();
            } else if (position == 2) {
                ctx.beginPath();
                ctx.roundedRectVarying(*x, *y, *width, *height, 0, radius,
                                       radius, 0);
                ctx.fill();
            } else {
                ctx.fillRect(*x, *y, *width, *height);
            }

            float bw = 1.0f;
            float cr = radius - bw / 2;

            if (position == 0) {
                ctx.beginPath();
                ctx.strokeWidth(bw);
                ctx.strokeColor(border_top);
                ctx.moveTo(*x + radius, *y + bw / 2);
                ctx.lineTo(*x + *width, *y + bw / 2);
                ctx.stroke();

                ctx.beginPath();
                ctx.strokeWidth(bw);
                ctx.strokeColor(border_right);
                ctx.moveTo(*x + *width, *y);
                ctx.lineTo(*x + *width, *y + *height);
                ctx.stroke();

                ctx.beginPath();
                ctx.strokeWidth(bw);
                ctx.strokeColor(border_bottom);
                ctx.moveTo(*x + *width, *y + *height - bw / 2);
                ctx.lineTo(*x + radius, *y + *height - bw / 2);
                ctx.stroke();

                ctx.beginPath();
                ctx.strokeWidth(bw);
                ctx.strokeColor(border_left);
                ctx.moveTo(*x + bw / 2, *y + *height - radius);
                ctx.lineTo(*x + bw / 2, *y + radius);
                ctx.stroke();

                ctx.beginPath();
                ctx.strokeWidth(bw);
                ctx.strokeColor(border_top.blend(border_left));
                ctx.moveTo(*x + bw / 2, *y + radius);
                ctx.arcTo(*x + bw / 2, *y + bw / 2, *x + radius, *y + bw / 2,
                          cr);
                ctx.stroke();

                ctx.beginPath();
                ctx.strokeWidth(bw);
                ctx.strokeColor(border_left.blend(border_bottom));
                ctx.moveTo(*x + radius, *y + *height - bw / 2);
                ctx.arcTo(*x + bw / 2, *y + *height - bw / 2, *x + bw / 2,
                          *y + *height - radius, cr);
                ctx.stroke();

            } else if (position == 2) {
                ctx.beginPath();
                ctx.strokeWidth(bw);
                ctx.strokeColor(border_top);
                ctx.moveTo(*x, *y + bw / 2);
                ctx.lineTo(*x + *width - radius, *y + bw / 2);
                ctx.stroke();

                ctx.beginPath();
                ctx.strokeWidth(bw);
                ctx.strokeColor(border_right);
                ctx.moveTo(*x + *width - bw / 2, *y + radius);
                ctx.lineTo(*x + *width - bw / 2, *y + *height - radius);
                ctx.stroke();

                ctx.beginPath();
                ctx.strokeWidth(bw);
                ctx.strokeColor(border_bottom);
                ctx.moveTo(*x + *width - radius, *y + *height - bw / 2);
                ctx.lineTo(*x, *y + *height - bw / 2);
                ctx.stroke();

                ctx.beginPath();
                ctx.strokeWidth(bw);
                ctx.strokeColor(border_left);
                ctx.moveTo(*x, *y);
                ctx.lineTo(*x, *y + *height);
                ctx.stroke();

                ctx.beginPath();
                ctx.strokeWidth(bw);
                ctx.strokeColor(border_right.blend(border_top));
                ctx.moveTo(*x + *width - radius, *y + bw / 2);
                ctx.arcTo(*x + *width - bw / 2, *y + bw / 2,
                          *x + *width - bw / 2, *y + radius, cr);
                ctx.stroke();

                ctx.beginPath();
                ctx.strokeWidth(bw);
                ctx.strokeColor(border_bottom.blend(border_right));
                ctx.moveTo(*x + *width - bw / 2, *y + *height - radius);
                ctx.arcTo(*x + *width - bw / 2, *y + *height - bw / 2,
                          *x + *width - radius, *y + *height - bw / 2, cr);
                ctx.stroke();

            } else {
                ctx.beginPath();
                ctx.strokeWidth(bw);
                ctx.strokeColor(border_top);
                ctx.moveTo(*x, *y + bw / 2);
                ctx.lineTo(*x + *width, *y + bw / 2);
                ctx.stroke();

                ctx.beginPath();
                ctx.strokeWidth(bw);
                ctx.strokeColor(border_right);
                ctx.moveTo(*x + *width, *y);
                ctx.lineTo(*x + *width, *y + *height);
                ctx.stroke();

                ctx.beginPath();
                ctx.strokeWidth(bw);
                ctx.strokeColor(border_bottom);
                ctx.moveTo(*x + *width, *y + *height - bw / 2);
                ctx.lineTo(*x, *y + *height - bw / 2);
                ctx.stroke();

                ctx.beginPath();
                ctx.strokeWidth(bw);
                ctx.strokeColor(border_left);
                ctx.moveTo(*x, *y);
                ctx.lineTo(*x, *y + *height);
                ctx.stroke();
            }

            padding_widget::render(ctx);
        }
    };

    static StartupPriority get_current_priority() {
        HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        bool need_uninit = SUCCEEDED(hr);

        ITaskService *pService = NULL;
        hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER,
                              IID_ITaskService, (void **)&pService);

        if (SUCCEEDED(hr)) {
            hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(),
                                   _variant_t());
            if (SUCCEEDED(hr)) {
                ITaskFolder *pRootFolder = NULL;
                hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
                if (SUCCEEDED(hr)) {
                    IRegisteredTask *pTask = NULL;
                    hr = pRootFolder->GetTask(_bstr_t(L"breeze-shell-startup"),
                                              &pTask);
                    if (SUCCEEDED(hr)) {
                        ITaskDefinition *pTaskDef = NULL;
                        hr = pTask->get_Definition(&pTaskDef);
                        if (SUCCEEDED(hr)) {
                            ITaskSettings *pSettings = NULL;
                            hr = pTaskDef->get_Settings(&pSettings);
                            if (SUCCEEDED(hr)) {
                                int priority = 0;
                                hr = pSettings->get_Priority(&priority);
                                pSettings->Release();
                                pTaskDef->Release();
                                pTask->Release();
                                pRootFolder->Release();
                                pService->Release();
                                if (need_uninit)
                                    CoUninitialize();

                                if (priority <= 4)
                                    return StartupPriority::High;
                                return StartupPriority::Normal;
                            }
                            pTaskDef->Release();
                        }
                        pTask->Release();
                    }
                    pRootFolder->Release();
                }
            }
            pService->Release();
        }

        if (need_uninit)
            CoUninitialize();

        if (start_when_startup_switch::check_startup()) {
            return StartupPriority::Normal;
        }

        return StartupPriority::Disabled;
    }

    static bool is_elevated() {
        BOOL elevated = FALSE;
        HANDLE token = NULL;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
            TOKEN_ELEVATION elevation;
            DWORD size = sizeof(TOKEN_ELEVATION);
            if (GetTokenInformation(token, TokenElevation, &elevation,
                                    sizeof(elevation), &size)) {
                elevated = elevation.TokenIsElevated;
            }
            CloseHandle(token);
        }
        return elevated;
    }

    static void elevate_and_restart() {
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(NULL, path, MAX_PATH);

        SHELLEXECUTEINFOW sei = {sizeof(sei)};
        sei.lpVerb = L"runas";
        sei.lpFile = path;
        sei.lpParameters = L"";
        sei.nShow = SW_SHOW;

        if (ShellExecuteExW(&sei)) {
            ExitProcess(0);
        }
    }

    static bool create_scheduled_task(StartupPriority priority) {
        if (priority == StartupPriority::Disabled ||
            priority == StartupPriority::Normal) {
            return delete_scheduled_task();
        }

        HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        bool need_uninit = SUCCEEDED(hr);

        ITaskService *pService = NULL;
        hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER,
                              IID_ITaskService, (void **)&pService);

        if (FAILED(hr)) {
            if (need_uninit)
                CoUninitialize();
            return false;
        }

        hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(),
                               _variant_t());
        if (FAILED(hr)) {
            pService->Release();
            if (need_uninit)
                CoUninitialize();
            return false;
        }

        ITaskFolder *pRootFolder = NULL;
        hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
        if (FAILED(hr)) {
            pService->Release();
            if (need_uninit)
                CoUninitialize();
            return false;
        }

        pRootFolder->DeleteTask(_bstr_t(L"breeze-shell-startup"), 0);

        ITaskDefinition *pTask = NULL;
        hr = pService->NewTask(0, &pTask);
        if (FAILED(hr)) {
            pRootFolder->Release();
            pService->Release();
            if (need_uninit)
                CoUninitialize();
            return false;
        }

        IRegistrationInfo *pRegInfo = NULL;
        hr = pTask->get_RegistrationInfo(&pRegInfo);
        if (SUCCEEDED(hr)) {
            pRegInfo->put_Author(_bstr_t(L"breeze-shell"));
            pRegInfo->put_Description(
                _bstr_t(L"Start breeze-shell on system startup"));
            pRegInfo->Release();
        }

        IPrincipal *pPrincipal = NULL;
        hr = pTask->get_Principal(&pPrincipal);
        if (SUCCEEDED(hr)) {
            pPrincipal->put_RunLevel(TASK_RUNLEVEL_HIGHEST);
            pPrincipal->Release();
        }

        ITaskSettings *pSettings = NULL;
        hr = pTask->get_Settings(&pSettings);
        if (SUCCEEDED(hr)) {
            pSettings->put_StartWhenAvailable(VARIANT_TRUE);
            pSettings->put_DisallowStartIfOnBatteries(VARIANT_FALSE);
            pSettings->put_StopIfGoingOnBatteries(VARIANT_FALSE);
            pSettings->put_AllowDemandStart(VARIANT_TRUE);
            pSettings->put_Enabled(VARIANT_TRUE);

            pSettings->put_Priority(4);

            pSettings->Release();
        }

        ITriggerCollection *pTriggerCollection = NULL;
        hr = pTask->get_Triggers(&pTriggerCollection);
        if (SUCCEEDED(hr)) {
            ITrigger *pTrigger = NULL;
            hr = pTriggerCollection->Create(TASK_TRIGGER_LOGON, &pTrigger);
            if (SUCCEEDED(hr)) {
                ILogonTrigger *pLogonTrigger = NULL;
                hr = pTrigger->QueryInterface(IID_ILogonTrigger,
                                              (void **)&pLogonTrigger);
                if (SUCCEEDED(hr)) {
                    pLogonTrigger->put_Id(_bstr_t(L"LogonTriggerId"));
                    pLogonTrigger->put_Enabled(VARIANT_TRUE);
                    pLogonTrigger->Release();
                }
                pTrigger->Release();
            }
            pTriggerCollection->Release();
        }

        IActionCollection *pActionCollection = NULL;
        hr = pTask->get_Actions(&pActionCollection);
        if (SUCCEEDED(hr)) {
            IAction *pAction = NULL;
            hr = pActionCollection->Create(TASK_ACTION_EXEC, &pAction);
            if (SUCCEEDED(hr)) {
                IExecAction *pExecAction = NULL;
                hr = pAction->QueryInterface(IID_IExecAction,
                                             (void **)&pExecAction);
                if (SUCCEEDED(hr)) {
                    wchar_t path[MAX_PATH];
                    GetModuleFileNameW(NULL, path, MAX_PATH);
                    pExecAction->put_Path(_bstr_t(path));
                    pExecAction->put_Arguments(_bstr_t(L"inject-consistent"));
                    pExecAction->Release();
                }
                pAction->Release();
            }
            pActionCollection->Release();
        }

        IRegisteredTask *pRegisteredTask = NULL;
        hr = pRootFolder->RegisterTaskDefinition(
            _bstr_t(L"breeze-shell-startup"), pTask, TASK_CREATE_OR_UPDATE,
            _variant_t(), _variant_t(), TASK_LOGON_INTERACTIVE_TOKEN,
            _variant_t(L""), &pRegisteredTask);

        bool success = SUCCEEDED(hr);

        if (pRegisteredTask)
            pRegisteredTask->Release();
        pTask->Release();
        pRootFolder->Release();
        pService->Release();
        if (need_uninit)
            CoUninitialize();

        return success;
    }

    static bool delete_scheduled_task() {
        HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        bool need_uninit = SUCCEEDED(hr);

        ITaskService *pService = NULL;
        hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER,
                              IID_ITaskService, (void **)&pService);

        if (SUCCEEDED(hr)) {
            hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(),
                                   _variant_t());
            if (SUCCEEDED(hr)) {
                ITaskFolder *pRootFolder = NULL;
                hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
                if (SUCCEEDED(hr)) {
                    pRootFolder->DeleteTask(_bstr_t(L"breeze-shell-startup"),
                                            0);
                    pRootFolder->Release();
                }
            }
            pService->Release();
        }

        if (need_uninit)
            CoUninitialize();
        return true;
    }

    void set_priority(StartupPriority priority) {
        bool has_task_scheduler =
            (get_current_priority() == StartupPriority::High);
        bool need_delete_task =
            has_task_scheduler && (priority != StartupPriority::High);

        if (need_delete_task && !is_elevated()) {
            elevate_and_restart();
            return;
        }

        if (priority == StartupPriority::Disabled) {
            start_when_startup_switch::set_startup(false);
            delete_scheduled_task();
        } else if (priority == StartupPriority::Normal) {
            delete_scheduled_task();
            start_when_startup_switch::set_startup(true);
        } else {
            if (!is_elevated()) {
                elevate_and_restart();
                return;
            }
            start_when_startup_switch::set_startup(false);
            create_scheduled_task(priority);
        }

        current_priority = get_current_priority();
    }
};

void restart_explorer() {
    std::vector<DWORD> pids = GetExplorerPIDs();
    for (DWORD pid : pids) {
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
        if (hProcess) {
            TerminateProcess(hProcess, 0);
            CloseHandle(hProcess);
        }
    }

    Sleep(1000);
    if (GetExplorerPIDs().empty()) {
        ShellExecuteW(NULL, L"open", L"explorer.exe", L"", NULL, SW_SHOW);
    }
}

struct restart_explorer_btn : public ui::button_widget {
    restart_explorer_btn()
        : button_widget(english ? "Restart explorer" : "重启资源管理器") {}

    void on_click() override {
        std::thread([]() { restart_explorer(); }).detach();
    }
};

struct injector_ui_main;
struct switch_lang_btn : public ui::button_widget {
    switch_lang_btn() : button_widget(english ? "中文" : "English") {}

    void on_click() override {
        english = !english;

        auto old_i = ctx->rt.root->children.back();
        auto new_i = ctx->rt.root->emplace_child<injector_ui_main>();
        old_i->dying_time = 200;
    }
};

void InjectAllConsistent() {
    GetDebugPrivilege();
    std::vector<DWORD> injected;
    while (true) {
        std::vector<DWORD> pids = GetExplorerPIDs();

        for (DWORD pid : pids) {
            if (!std::ranges::contains(injected, pid) &&
                !IsInjected(pid, dllPath)) {
                InjectToPID(pid, dllPath);
                injected.push_back(pid);
            }
        }
        Sleep(1000);
        MSG msg;
        if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
}

struct inject_all_switch : public ui::button_widget {
    bool injecting_all = false;
    std::chrono::steady_clock::time_point last_check;
    inject_all_switch() : button_widget(english ? "Inject All" : "全局注入") {
        check_is_injecting_all();
    }

    void check_is_injecting_all() {
        HANDLE mutex =
            CreateMutexW(NULL, TRUE, L"breeze-shell-inject-consistent");
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

        if (std::chrono::steady_clock::now() - last_check >
            std::chrono::seconds(1)) {
            check_is_injecting_all();
            last_check = std::chrono::steady_clock::now();
        }
    }
};

struct inject_once_switch : public ui::button_widget {
    inject_once_switch()
        : button_widget(english ? "Inject Once" : "注入一次") {}

    void on_click() override {
        std::thread([]() {
            GetDebugPrivilege();
            NewExplorerProcessAndInject();
        }).detach();
    }
};

struct data_dir_btn : public ui::button_widget {
    data_dir_btn() : button_widget(english ? "Data Folder" : "数据目录") {}

    void on_click() override {
        std::wstring path = data_directory().wstring();
        ShellExecuteW(NULL, L"open", path.c_str(), NULL, NULL, SW_SHOW);
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
            image =
                nvgCreateImageMem(ctx.ctx, 0, g_icon_png, sizeof(g_icon_png));
        }
        auto paint = ctx.imagePattern(*x, *y, *height, *height, 0, *image, 1);
        ctx.fillPaint(paint);
        ctx.fillRect(*x, *y, *height, *height);
    }
};

struct injector_ui_main : public ui::flex_widget {
    ui::sp_anim_float opacity = anim_float(100);
    injector_ui_main() {
        x->reset_to(20);
        y->reset_to(5);
        opacity->reset_to(0);
        opacity->set_easing(ui::easing_type::ease_in_out);
        opacity->animate_to(1);
        gap = 15;
        emplace_child<breeze_icon>();
        emplace_child<inject_ui_title>();

        auto switches_box = emplace_child<ui::flex_widget>();
        switches_box->gap = 7;
        auto switches = switches_box->emplace_child<ui::flex_widget>();
        switches->gap = 7;
        switches->horizontal = true;

        switches->emplace_child<inject_all_switch>();
        switches->emplace_child<inject_once_switch>();
        switches->emplace_child<data_dir_btn>();

        switches_box->emplace_child<startup_priority_selector>();

        switches = switches_box->emplace_child<ui::flex_widget>();
        switches->gap = 7;
        switches->horizontal = true;
        switches->emplace_child<restart_explorer_btn>();
        switches->emplace_child<switch_lang_btn>();
    }
    void render(ui::nanovg_context ctx) override {
        auto t = ctx.transaction();
        ctx.globalAlpha(opacity->var());

        ctx.fillColor(nvgRGB(32, 32, 32));
        auto gradient_height = 130;
        ctx.fillRect(0, gradient_height, 999, 999);

        NVGpaint bg =
            nvgLinearGradient(ctx.ctx, 0, gradient_height, 0, 0,
                              nvgRGB(32, 32, 32), nvgRGBAf(0, 0, 0, 0));
        ctx.beginPath();
        ctx.moveTo(0, gradient_height);
        ctx.lineTo(999, gradient_height);
        ctx.lineTo(999, 0);
        ctx.lineTo(0, 0);
        ctx.fillPaint(bg);
        ctx.fill();

        flex_widget::render(ctx);
    }
};

std::string getFontPath() {
    char fontsPath[MAX_PATH];
    SHGetSpecialFolderPathA(NULL, fontsPath, CSIDL_FONTS, FALSE);

    const auto p = std::filesystem::path(fontsPath);
    if (std::filesystem::exists(p / "msyh.ttc"))
        return (p / "msyh.ttc").string();
    if (std::filesystem::exists(p / "simsun.ttc"))
        return (p / "simsun.ttc").string();
    if (std::filesystem::exists(p / "segoeui.ttf"))
        return (p / "segoeui.ttf").string();
    throw std::runtime_error("no font found");
}

void StartInjectUI() {
    if (auto r = ui::render_target::init_global(); !r) {
        std::cerr << "Failed to initialize global render target." << std::endl;
        return;
    }
    ui::render_target rt;
    rt.acrylic = 0.1;
    rt.transparent = true;
    rt.width = 400;
    rt.height = 270;
    rt.title = "";
    if (auto r = rt.init(); !r) {
        std::cerr << "Failed to initialize render target." << std::endl;
        return;
    }
    nvgCreateFont(rt.nvg, "main", getFontPath().c_str());
    rt.root->emplace_child<injector_ui_main>();
    rt.start_loop();
}

void ShowCrashDialog() {
    auto show_by_messagebox = []() {
        MessageBoxW(NULL,
                    english ? L"Explorer has crashed too many times after "
                              L"injection.\nBreeze "
                              L"Shell will now exit to prevent further crashes."
                            : L"注入后资源管理器多次崩溃。\nBreeze Shell "
                              L"将退出以防止进一步崩溃。",
                    english ? L"Breeze Shell Error" : L"Breeze Shell 错误",
                    MB_ICONERROR | MB_OK);
    };
    if (auto r = ui::render_target::init_global(); !r) {
        std::cerr << "Failed to initialize global render target." << std::endl;
        show_by_messagebox();
        return;
    }

    ui::render_target rt;
    rt.acrylic = 0.1;
    rt.transparent = true;
    rt.width = 400;
    rt.height = 230;
    rt.title = "";

    if (auto r = rt.init(); !r) {
        std::cerr << "Failed to initialize render target." << std::endl;
        show_by_messagebox();
        return;
    }

    nvgCreateFont(rt.nvg, "main", getFontPath().c_str());

    auto error_ui = rt.root->emplace_child<ui::flex_widget>();
    error_ui->gap = 15;
    error_ui->x->reset_to(20);
    error_ui->y->reset_to(0);

    error_ui->emplace_child<breeze_icon>();

    auto msg_box = error_ui->emplace_child<ui::flex_widget>();
    msg_box->gap = 10;

    auto title = msg_box->emplace_child<ui::text_widget>();
    title->text = "Breeze Shell Error";
    title->font_size = 20;
    title->color.reset_to({1, 0.4, 0.4, 1});

    auto message = msg_box->emplace_child<ui::text_widget>();
    message->text =
        english ? "Explorer has crashed multiple times after injection."
                : "资源管理器在注入后多次崩溃，这可能是 Breeze 的问题。";
    message->font_size = 14;
    message->color.reset_to({1, 1, 1, 0.9});

    auto suggestion = msg_box->emplace_child<ui::text_widget>();
    suggestion->text = english ? "Breeze Shell will be temporarily disabled to "
                                 "prevent further crashes."
                               : "Breeze 将暂时被禁用，以防止持续崩溃。";
    suggestion->font_size = 14;
    suggestion->color.reset_to({1, 1, 1, 0.9});

    auto suggestion2 = msg_box->emplace_child<ui::text_widget>();
    suggestion2->text = english
                            ? "If you want to continue using Breeze Shell, "
                              "please re-enable it in the injector UI."
                            : "如果您想继续使用 Breeze，请在注入器中重新启用。";
    suggestion2->font_size = 14;
    suggestion2->color.reset_to({1, 1, 1, 0.9});

    auto btn_container = error_ui->emplace_child<ui::flex_widget>();
    btn_container->horizontal = true;
    btn_container->gap = 10;

    class close_button : public ui::button_widget {
    public:
        close_button() : button_widget(english ? "Close" : "关闭") {}
        void on_click() override { exit(1); }
    };

    class github_button : public ui::button_widget {
    public:
        github_button()
            : button_widget(english ? "Check GitHub" : "查看 GitHub") {}
        void on_click() override {
            ShellExecuteW(NULL, L"open",
                          L"https://github.com/std-microblock/breeze-shell/"
                          L"releases/latest",
                          NULL, NULL, SW_SHOW);
        }
    };

    btn_container->emplace_child<close_button>();
    btn_container->emplace_child<github_button>();
    rt.start_loop();
    HANDLE event =
        CreateEventW(NULL, TRUE, FALSE, L"breeze-shell-inject-consistent-exit");
    SetEvent(event);
    CloseHandle(event);
}

void UpdateDllPath() {
    auto dllPathNew = data_directory().wstring() + L"\\shell.dll";
    auto dllPathPacked = GetModuleDirectory() + L"\\shell.dll";

    auto updateDllFile = [&](const std::wstring &packed,
                             const std::wstring &target) {
        std::error_code ec;
        fs::remove(target, ec);

        if (ec) {
            ec.clear();
            std::wstring oldPath =
                fs::path(target).parent_path() / L"shell_old.dll";
            if (fs::exists(oldPath)) {
                fs::remove(oldPath, ec);
            }

            if (!ec) {
                fs::rename(target, oldPath, ec);
                if (!ec) {
                    fs::copy(packed, target,
                             fs::copy_options::overwrite_existing);
                }
            }
        } else {
            fs::copy(packed, target, fs::copy_options::overwrite_existing);
        }
    };

    if (fs::exists(dllPathNew)) {
        if (fs::exists(dllPathPacked)) {
            auto packedTime = fs::last_write_time(dllPathPacked);
            auto newTime = fs::last_write_time(dllPathNew);

            if (packedTime > newTime) {
                updateDllFile(dllPathPacked, dllPathNew);
            }
        }
    } else {
        fs::create_directories(fs::path(dllPathNew).parent_path());

        if (fs::exists(dllPathPacked)) {
            fs::copy(dllPathPacked, dllPathNew);
        }
    }

    dllPath = dllPathNew;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nShowCmd) {
    int argc = 0;
    auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    std::vector<std::wstring> args;
    for (int x = 0; x < argc; x++) {
        args.push_back(argv[x]);
    }

    UpdateDllPath();

    if (args.size() <= 1) {
        if (false) {
            AttachConsole(ATTACH_PARENT_PROCESS);
            freopen("CONOUT$", "w", stdout);
            freopen("CONOUT$", "w", stderr);
            freopen("CONIN$", "r", stdin);
        }

        try {
            std::println("breeze-shell injector started.");
        } catch (std::exception &) {
            freopen("NUL", "w", stdout);
            freopen("NUL", "w", stderr);
        }

        StartInjectUI();
    } else {
        freopen("NUL", "w", stdout);
        freopen("NUL", "w", stderr);

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
                HANDLE event = CreateEventW(
                    NULL, TRUE, FALSE, L"breeze-shell-inject-consistent-exit");
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
