#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "crash_detection.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>


#include "breeze_ui/animator.h"
#include "breeze_ui/ui.h"
#include "breeze_ui/widget.h"

static unsigned char g_icon_png[] = {
#include "icon-small.png.h"
};

#include <Windows.h>
#include <winver.h>

#include "data_directory.inc"
#include <TlHelp32.h>
#include <comdef.h>
#include <psapi.h>
#include <shellapi.h>
#include <taskschd.h>

#include "Shlobj.h"

namespace fs = std::filesystem;

void init_inject_logger() {
    try {
        auto console_sink =
            std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
            (data_directory() / "inject.log").string(), true);
        auto msvc_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();

        std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink, msvc_sink};
        auto logger = std::make_shared<spdlog::logger>("inject", sinks.begin(),
                                                       sinks.end());
        spdlog::set_default_logger(logger);
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
        spdlog::set_level(spdlog::level::debug);
        spdlog::flush_on(spdlog::level::info);
    } catch (const spdlog::spdlog_ex &ex) {
        printf("Log initialization failed: %s\n", ex.what());
    }
}

std::wstring GetModuleDirectory() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    return fs::path(path).parent_path().wstring();
}

std::wstring GetSelfPath() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    return path;
}

struct instance_config {
    std::wstring id;
    std::wstring target_root;
    std::wstring run_key = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
    std::wstring task_name = L"breeze-shell-startup";
};

instance_config g_instance;

std::string ToUtf8(const std::wstring &wide) {
    int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), (int)wide.size(),
                                   nullptr, 0, nullptr, nullptr);
    std::string out(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), (int)wide.size(), out.data(),
                        size, nullptr, nullptr);
    return out;
}

std::vector<std::wstring> SplitLines(const std::wstring &text) {
    std::vector<std::wstring> lines;
    std::wstring current;
    for (wchar_t c : text) {
        if (c == L'\n') {
            if (!current.empty() && current.back() == L'\r') {
                current.pop_back();
            }
            lines.push_back(current);
            current.clear();
        } else {
            current.push_back(c);
        }
    }
    if (!current.empty()) {
        if (current.back() == L'\r') {
            current.pop_back();
        }
        lines.push_back(current);
    }
    return lines;
}

void LoadInstanceConfig() {
    auto read = [](const wchar_t *name) -> std::wstring {
        wchar_t buf[1024] = {0};
        DWORD n = GetEnvironmentVariableW(name, buf, 1024);
        return n > 0 && n < 1024 ? std::wstring(buf) : L"";
    };
    g_instance.id = read(L"BREEZE_INSTANCE");
    g_instance.target_root = read(L"BREEZE_TARGET_ROOT");
    auto runKey = read(L"BREEZE_RUN_KEY");
    if (!runKey.empty()) {
        g_instance.run_key = runKey;
    }
    auto taskName = read(L"BREEZE_TASK_NAME");
    if (!taskName.empty()) {
        g_instance.task_name = taskName;
    }
}

std::optional<std::wstring> ReadInstanceMarker() {
    std::ifstream file(data_directory() / L"instance", std::ios::binary);
    if (!file.is_open()) {
        return std::nullopt;
    }
    std::string narrow((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    std::wstring marker(narrow.begin(), narrow.end());
    while (!marker.empty() && (marker.back() == L'\r' || marker.back() == L'\n')) {
        marker.pop_back();
    }
    return marker;
}

bool InstanceMarkerMatches() {
    auto marker = ReadInstanceMarker();
    if (!marker) {
        return true;
    }
    return *marker == g_instance.id;
}

std::wstring ScopedName(const std::wstring &name) {
    if (g_instance.id.empty()) {
        return name;
    }
    return name + L"-" + g_instance.id;
}

struct version_number {
    int parts[3] = {0, 0, 0};
    bool valid = false;
};

version_number ParseVersionNumber(const std::wstring &text) {
    version_number version;
    int index = 0;
    int current = -1;
    for (wchar_t c : text) {
        if (c == L'.') {
            if (current < 0 || index >= 3) {
                return version;
            }
            version.parts[index++] = current;
            current = -1;
        } else if (c >= L'0' && c <= L'9') {
            current = current < 0 ? 0 : current * 10;
            current += c - L'0';
        } else if (current >= 0 || index > 0) {
            break;
        }
    }
    if (current >= 0 && index < 3) {
        version.parts[index++] = current;
    }
    version.valid = index == 3;
    return version;
}

std::wstring GetFileProductVersion(const std::wstring &path) {
    DWORD handle = 0;
    DWORD size = GetFileVersionInfoSizeW(path.c_str(), &handle);
    if (!size) {
        return L"";
    }
    std::vector<char> data(size);
    if (!GetFileVersionInfoW(path.c_str(), 0, size, data.data())) {
        return L"";
    }
    struct lang_codepage {
        WORD language;
        WORD code_page;
    } *translate = nullptr;
    UINT length = 0;
    if (!VerQueryValueW(data.data(), L"\\VarFileInfo\\Translation",
                        (LPVOID *)&translate, &length) ||
        length < sizeof(lang_codepage)) {
        return L"";
    }
    wchar_t query[256];
    swprintf_s(query, L"\\StringFileInfo\\%04x%04x\\ProductVersion",
               translate[0].language, translate[0].code_page);
    wchar_t *value = nullptr;
    if (!VerQueryValueW(data.data(), query, (LPVOID *)&value, &length) ||
        !value) {
        return L"";
    }
    return value;
}

int CompareVersions(const version_number &a, const version_number &b) {
    for (int i = 0; i < 3; i++) {
        if (a.parts[i] != b.parts[i]) {
            return a.parts[i] < b.parts[i] ? -1 : 1;
        }
    }
    return 0;
}

fs::path CanonicalBinDir() { return data_directory() / L"bin"; }

fs::path CanonicalExePath() { return CanonicalBinDir() / L"breeze.exe"; }

fs::path TimestampedPath(const fs::path &base, const std::wstring &suffix) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t stamp[64];
    swprintf_s(stamp, L"-%04u%02u%02u-%02u%02u%02u", st.wYear, st.wMonth,
               st.wDay, st.wHour, st.wMinute, st.wSecond);
    return base.wstring() + stamp + suffix;
}

void EnsureCanonicalInstall() {
    std::error_code ec;
    fs::path self(GetSelfPath());
    fs::path canonical = CanonicalExePath();

    auto lower = [](std::wstring s) {
        std::transform(s.begin(), s.end(), s.begin(), ::towlower);
        return s;
    };
    if (lower(self.wstring()) == lower(canonical.wstring())) {
        return;
    }

    fs::create_directories(CanonicalBinDir(), ec);
    ec.clear();

    bool should_sync = !fs::exists(canonical, ec);
    ec.clear();
    if (!should_sync) {
        auto self_version = ParseVersionNumber(GetFileProductVersion(self));
        auto canonical_version =
            ParseVersionNumber(GetFileProductVersion(canonical.wstring()));
        if (self_version.valid && !canonical_version.valid) {
            should_sync = true;
        } else if (self_version.valid && canonical_version.valid &&
                   CompareVersions(self_version, canonical_version) > 0) {
            should_sync = true;
        } else if (!self_version.valid && !canonical_version.valid &&
                   fs::last_write_time(self, ec) >
                       fs::last_write_time(canonical, ec)) {
            should_sync = true;
        }
        ec.clear();
    }

    if (!should_sync) {
        return;
    }

    fs::path staged = CanonicalBinDir() / L"breeze.exe.new";
    fs::remove(staged, ec);
    ec.clear();
    if (!fs::copy_file(self, staged, ec)) {
        spdlog::error("Failed to stage injector: {}", ec.message());
        return;
    }
    ec.clear();

    fs::path backup = CanonicalBinDir() / L"breeze.exe.old";
    fs::remove(backup, ec);
    ec.clear();
    if (fs::exists(canonical, ec)) {
        ec.clear();
        fs::rename(canonical, backup, ec);
        if (ec) {
            spdlog::error("Failed to move current injector: {}", ec.message());
            fs::remove(staged, ec);
            return;
        }
    }
    ec.clear();
    fs::rename(staged, canonical, ec);
    if (ec) {
        spdlog::error("Failed to install canonical injector: {}",
                      ec.message());
        if (!fs::exists(canonical, ec) && fs::exists(backup, ec)) {
            ec.clear();
            fs::rename(backup, canonical, ec);
        }
        return;
    }
    spdlog::info("Canonical injector installed: {}", ToUtf8(canonical.wstring()));
}

bool IsUnderTargetRoot(DWORD pid) {
    if (g_instance.target_root.empty()) {
        return false;
    }
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process) {
        return false;
    }
    wchar_t path[MAX_PATH];
    DWORD size = MAX_PATH;
    bool under = false;
    if (QueryFullProcessImageNameW(process, 0, path, &size)) {
        std::wstring cmp(path);
        std::wstring root = g_instance.target_root;
        std::transform(cmp.begin(), cmp.end(), cmp.begin(), ::towlower);
        std::transform(root.begin(), root.end(), root.begin(), ::towlower);
        while (root.size() > 1 && root.back() == L'\\') {
            root.pop_back();
        }
        under = cmp.rfind(root, 0) == 0;
    }
    CloseHandle(process);
    return under;
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
                    if (!g_instance.target_root.empty() &&
                        !IsUnderTargetRoot(pe32.th32ProcessID)) {
                        continue;
                    }
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
        spdlog::error("OpenProcessToken failed: %d", GetLastError());
        return;
    }

    if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)) {
        spdlog::error("LookupPrivilegeValue failed: %d", GetLastError());
        CloseHandle(hToken);
        return;
    }

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Luid = luid;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(TOKEN_PRIVILEGES),
                               NULL, NULL)) {
        spdlog::error("AdjustTokenPrivileges failed: %d", GetLastError());
        CloseHandle(hToken);
        return;
    }

    CloseHandle(hToken);
}

void ShowCrashDialog();

constexpr int MAX_CRASH_COUNT = 3;
constexpr auto CRASH_WINDOW = std::chrono::minutes(10);
static std::mutex crash_mutex;
static mb_shell::crash_window crash_history(MAX_CRASH_COUNT, CRASH_WINDOW);
static std::atomic<bool> injection_suspended = false;
static std::atomic<bool> crash_dialog_open = false;

fs::path GetKeepInjectingAfterCrashFlagPath() {
    return data_directory() / "keep_injecting_after_crash.flag";
}

bool ShouldKeepInjectingAfterCrash() {
    return fs::exists(GetKeepInjectingAfterCrashFlagPath());
}

void SetKeepInjectingAfterCrash(bool keep_injecting_after_crash) {
    auto path = GetKeepInjectingAfterCrashFlagPath();
    if (keep_injecting_after_crash) {
        HANDLE file =
            CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL,
                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (file == INVALID_HANDLE_VALUE) {
            spdlog::error("Failed to persist keep-injecting setting: %d",
                          GetLastError());
            return;
        }
        CloseHandle(file);
        return;
    }

    std::error_code ec;
    fs::remove(path, ec);
    if (ec) {
        spdlog::error("Failed to clear keep-injecting setting: %s",
                      ec.message().c_str());
    }
}

void SignalInjectConsistentExit() {
    HANDLE event = CreateEventW(
        NULL, TRUE, FALSE,
        ScopedName(L"breeze-shell-inject-consistent-exit").c_str());
    SetEvent(event);
    CloseHandle(event);
}

const char *BlameName(mb_shell::crash_blame blame) {
    switch (blame) {
    case mb_shell::crash_blame::breeze:
        return "breeze";
    case mb_shell::crash_blame::foreign:
        return "foreign";
    case mb_shell::crash_blame::unknown:
        return "unknown";
    default:
        return "none";
    }
}

bool TryRollbackPendingShellUpdate() {
    fs::path pending = data_directory() / L"update-pending";
    fs::path updateDir = data_directory() / L"update";

    std::wstring backupName;
    std::wstring version;
    {
        std::ifstream file(pending, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
        std::string narrow((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        std::wstring content(narrow.begin(), narrow.end());
        auto lines = SplitLines(content);
        if (lines.size() < 2 || lines[0].empty() || lines[1].empty()) {
            return false;
        }
        backupName = lines[0];
        version = lines[1];
    }

    fs::path backup = updateDir / backupName;
    fs::path current = data_directory() / L"shell.dll";

    if (!fs::exists(backup)) {
        std::error_code ec;
        fs::remove(pending, ec);
        return false;
    }

    std::error_code ec;
    fs::path quarantined = TimestampedPath(
        updateDir / (L"shell-bad-" + version), L".dll");
    fs::rename(current, quarantined, ec);
    if (ec) {
        spdlog::error("Rollback: cannot move current shell.dll: {}",
                      ec.message());
        return false;
    }
    ec.clear();
    fs::rename(backup, current, ec);
    if (ec) {
        spdlog::error("Rollback: cannot restore backup: {}", ec.message());
        ec.clear();
        fs::rename(quarantined, current, ec);
        return false;
    }

    fs::remove(pending, ec);
    ec.clear();
    std::ofstream marker(data_directory() / L"update-rolledback");
    marker << ToUtf8(version);
    spdlog::warn("Shell {} crashed repeatedly after update, rolled back to {}",
                 ToUtf8(version), ToUtf8(backupName));
    return true;
}

void OnInjectedProcessExit(DWORD pid, DWORD exitCode) {
    auto marker_path = data_directory() / "crashes" /
                       (std::to_wstring(pid) + L".txt");
    auto marker = mb_shell::read_crash_marker(marker_path);
    auto blame = mb_shell::blame_crash(exitCode, marker);

    if (blame == mb_shell::crash_blame::none)
        return;

    spdlog::error("Process {} exited with 0x{:08x}, blame={}, module={}, "
                  "stack={}",
                  pid, exitCode, BlameName(blame),
                  marker ? marker->module : "-",
                  marker ? marker->stack_modules : "-");

    if (marker) {
        std::error_code ec;
        auto archived = marker_path;
        archived.replace_extension(
            std::to_string(std::chrono::system_clock::now()
                               .time_since_epoch()
                               .count()) +
            ".log");
        fs::rename(marker_path, archived, ec);
    }

    if (!mb_shell::counts_toward_limit(blame))
        return;

    bool limit_reached;
    {
        std::lock_guard lock(crash_mutex);
        limit_reached =
            crash_history.record(mb_shell::crash_window::clock::now());
        if (limit_reached)
            crash_history.reset();
    }
    if (!limit_reached)
        return;

    if (TryRollbackPendingShellUpdate()) {
        return;
    }

    if (!ShouldKeepInjectingAfterCrash())
        injection_suspended = true;
    if (!crash_dialog_open.exchange(true)) {
        ShowCrashDialog();
        crash_dialog_open = false;
    }}

int InjectToPID(int targetPID, std::wstring_view dllPath) {
    if (injection_suspended.load()) {
        return 1;
    }

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, targetPID);
    if (hProcess == NULL) {
        spdlog::error("OpenProcess failed: %d", GetLastError());
        return 1;
    }

    LPVOID remoteString =
        VirtualAllocEx(hProcess, NULL, (dllPath.size() + 1) * sizeof(wchar_t),
                       MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (remoteString == NULL) {
        spdlog::error("VirtualAllocEx failed: %d", GetLastError());
        CloseHandle(hProcess);
        return 1;
    }

    if (!WriteProcessMemory(hProcess, remoteString, dllPath.data(),
                            (dllPath.size() + 1) * sizeof(wchar_t), NULL)) {
        spdlog::error("WriteProcessMemory failed: %d", GetLastError());
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
        spdlog::error("CreateRemoteThread failed: %d", GetLastError());
        VirtualFreeEx(hProcess, remoteString, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }

    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);

    std::thread([hProcess, targetPID]() {
        WaitForSingleObject(hProcess, INFINITE);
        DWORD exitCode = 0;
        if (!GetExitCodeProcess(hProcess, &exitCode)) {
            spdlog::error("GetExitCodeProcess failed: {}", GetLastError());        }
        CloseHandle(hProcess);
        OnInjectedProcessExit(targetPID, exitCode);
    }).detach();

    spdlog::info("DLL injected successfully.");
    return 0;
}

bool IsInjected(DWORD targetPID, std::wstring &dllPath) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                                  FALSE, targetPID);
    if (hProcess == NULL) {
        spdlog::error("OpenProcess failed: {}", GetLastError());
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
        spdlog::error("Could not find new explorer.exe process.");
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
        if (RegOpenKeyExW(HKEY_CURRENT_USER, g_instance.run_key.c_str(), 0,
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
        if (RegOpenKeyExW(HKEY_CURRENT_USER, g_instance.run_key.c_str(), 0,
                          KEY_SET_VALUE, &hkey) != ERROR_SUCCESS) {
            return;
        }

        if (startup) {
            std::error_code ec;
            std::wstring exe = fs::exists(CanonicalExePath(), ec)
                                   ? CanonicalExePath().wstring()
                                   : GetSelfPath();
            std::wstring command = L"\"" + exe + L"\" inject-consistent";
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
                    hr = pRootFolder->GetTask(_bstr_t(g_instance.task_name.c_str()),
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

        pRootFolder->DeleteTask(_bstr_t(g_instance.task_name.c_str()), 0);

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
                    std::error_code ec;
                    std::wstring exe = fs::exists(CanonicalExePath(), ec)
                                           ? CanonicalExePath().wstring()
                                           : GetSelfPath();
                    pExecAction->put_Path(_bstr_t(exe.c_str()));
                    pExecAction->put_Arguments(_bstr_t(L"inject-consistent"));
                    pExecAction->Release();
                }
                pAction->Release();
            }
            pActionCollection->Release();
        }

        IRegisteredTask *pRegisteredTask = NULL;
        hr = pRootFolder->RegisterTaskDefinition(
            _bstr_t(g_instance.task_name.c_str()), pTask, TASK_CREATE_OR_UPDATE,
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
                    pRootFolder->DeleteTask(_bstr_t(g_instance.task_name.c_str()),
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
    // TerminateProcess leaves exit code != 0, which the injector's crash
    // watcher would count as a shell crash. Ask the shell to exit itself
    // instead, so restarts are never mistaken for bad builds.
    HWND tray = g_instance.id.empty()
                    ? FindWindowW(L"Shell_TrayWnd", nullptr)
                    : nullptr;
    if (tray) {
        DWORD trayPid = 0;
        GetWindowThreadProcessId(tray, &trayPid);
        auto pids = GetExplorerPIDs();
        if (trayPid && std::ranges::contains(pids, trayPid)) {
            SendMessageTimeoutW(tray, WM_COMMAND, 516, 0, SMTO_ABORTIFHUNG,
                                3000, nullptr);
            for (int i = 0; i < 40; i++) {
                Sleep(250);
                if (!std::ranges::contains(GetExplorerPIDs(), trayPid)) {
                    break;
                }
            }
        }
    }

    for (DWORD pid : GetExplorerPIDs()) {
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
        if (hProcess) {
            TerminateProcess(hProcess, 0);
            CloseHandle(hProcess);
        }
    }

    Sleep(1000);
    if (g_instance.id.empty() && GetExplorerPIDs().empty()) {
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
        if (!owner_rt || !owner_rt->root)
            return;

        auto &root = owner_rt->root;
        auto old_i = root->children.back();
        root->emplace_child<injector_ui_main>();
        old_i->dying_time = 200;
    }
};

struct keep_injecting_after_crash_switch : public ui::button_widget {
    bool keep_injecting_after_crash = false;

    keep_injecting_after_crash_switch()
        : button_widget(english ? "Stay enabled after crash"
                                : "崩溃后不自动禁用") {
        keep_injecting_after_crash = ShouldKeepInjectingAfterCrash();
    }

    void on_click() override {
        SetKeepInjectingAfterCrash(!keep_injecting_after_crash);
        keep_injecting_after_crash = ShouldKeepInjectingAfterCrash();
    }

    void update_colors(bool is_active, bool is_hovered) override {
        if (keep_injecting_after_crash) {
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

void InjectAllConsistent() {
    GetDebugPrivilege();
    std::vector<DWORD> injected;
    while (true) {
        std::vector<DWORD> pids = GetExplorerPIDs();

        for (DWORD pid : pids) {
            if (!injection_suspended.load() &&
                !std::ranges::contains(injected, pid) &&
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
        HANDLE mutex = CreateMutexW(
            NULL, TRUE, ScopedName(L"breeze-shell-inject-consistent").c_str());
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
            std::error_code ec;
            std::wstring exe = fs::exists(CanonicalExePath(), ec)
                                   ? CanonicalExePath().wstring()
                                   : GetSelfPath();
            SHELLEXECUTEINFOW sei = {sizeof(sei)};
            sei.fMask = SEE_MASK_NOCLOSEPROCESS;
            sei.lpFile = exe.c_str();
            sei.lpParameters = L"inject-consistent";
            sei.nShow = SW_HIDE;
            ShellExecuteExW(&sei);
        } else {
            SignalInjectConsistentExit();
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

        auto crash_options = switches_box->emplace_child<ui::flex_widget>();
        crash_options->gap = 7;
        crash_options->horizontal = true;
        crash_options->emplace_child<keep_injecting_after_crash_switch>();
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
        spdlog::error("Failed to initialize global render target.");
        return;
    }
    ui::render_target rt;
    rt.acrylic = 0.1;
    rt.transparent = true;
    rt.width = 400;
    rt.height = 320;
    rt.title = "";
    if (auto r = rt.init(); !r) {
        spdlog::error("Failed to initialize render target.");
        return;
    }
    nvgCreateFont(rt.nvg, "main", getFontPath().c_str());
    rt.root->emplace_child<injector_ui_main>();
    rt.start_loop();
}

void ShowCrashDialog() {
    auto auto_disable_after_crash = !ShouldKeepInjectingAfterCrash();
    std::function<void()> show_by_messagebox = [auto_disable_after_crash]() {
        MessageBoxW(
            NULL,
            auto_disable_after_crash
                ? (english ? L"Explorer has crashed too many times after "
                             L"injection.\nBreeze "
                             L"Shell will now exit to prevent further crashes."
                           : L"注入后资源管理器多次崩溃。\nBreeze Shell "
                             L"将退出以防止进一步崩溃。")
                : (english ? L"Explorer has crashed multiple times after "
                             L"injection.\nInject All is configured to stay "
                             L"enabled."
                           : L"注入后资源管理器多次崩溃。\n当前已设置为崩溃后不"
                             L"自动禁用全局注入。"),
            auto_disable_after_crash
                ? (english ? L"Breeze Shell Error" : L"Breeze Shell 错误")
                : (english ? L"Breeze Shell Warning" : L"Breeze Shell 警告"),
            (auto_disable_after_crash ? MB_ICONERROR : MB_ICONWARNING) | MB_OK);
    };
    if (auto r = ui::render_target::init_global(); !r) {
        spdlog::error("Failed to initialize global render target.");
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
        spdlog::error("Failed to initialize render target.");
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
    title->text = auto_disable_after_crash ? "Breeze Shell Error"
                                           : "Breeze Shell Warning";
    title->font_size = 20;
    title->color.reset_to({1, 0.4, 0.4, 1});

    auto message = msg_box->emplace_child<ui::text_widget>();
    message->text =
        english ? "Explorer has crashed multiple times after injection."
                : "资源管理器在注入后多次崩溃，这可能是 Breeze 的问题。";
    message->font_size = 14;
    message->color.reset_to({1, 1, 1, 0.9});

    auto suggestion = msg_box->emplace_child<ui::text_widget>();
    suggestion->text =
        auto_disable_after_crash
            ? (english ? "Breeze Shell will be temporarily disabled to "
                         "prevent further crashes."
                       : "Breeze 将暂时被禁用，以防止持续崩溃。")
            : (english ? "Inject All is configured to stay enabled after "
                         "crashes."
                       : "当前已设置为崩溃后不自动禁用全局注入。");
    suggestion->font_size = 14;
    suggestion->color.reset_to({1, 1, 1, 0.9});

    auto suggestion2 = msg_box->emplace_child<ui::text_widget>();
    suggestion2->text =
        auto_disable_after_crash
            ? (english ? "If you want to continue using Breeze Shell, "
                         "please re-enable it in the injector UI."
                       : "如果您想继续使用 Breeze，请在注入器中重新启用。")
            : (english ? "If crashes continue, disable Inject All manually in "
                         "the injector UI."
                       : "如果崩溃持续，请在注入器里手动关闭全局注入。");
    suggestion2->font_size = 14;
    suggestion2->color.reset_to({1, 1, 1, 0.9});

    auto btn_container = error_ui->emplace_child<ui::flex_widget>();
    btn_container->horizontal = true;
    btn_container->gap = 10;

    class close_button : public ui::button_widget {
    public:
        close_button(bool auto_disable_after_crash)
            : button_widget(auto_disable_after_crash
                                ? (english ? "Close" : "关闭")
                                : (english ? "Continue" : "继续")) {}
        void on_click() override {
            if (owner_rt)
                owner_rt->hide_as_close();
        }
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

    btn_container->emplace_child<close_button>(auto_disable_after_crash);
    btn_container->emplace_child<github_button>();
    rt.start_loop();
    if (auto_disable_after_crash) {
        SignalInjectConsistentExit();
    }
}

void UpdateDllPath() {
    std::error_code ec;
    fs::path dataDll = data_directory() / L"shell.dll";
    fs::path packedDll = fs::path(GetModuleDirectory()) / L"shell.dll";
    fs::path binDll = CanonicalBinDir() / L"shell.dll";

    fs::create_directories(CanonicalBinDir(), ec);
    ec.clear();
    bool packedSameAsBin = false;
    if (fs::exists(packedDll, ec) && fs::exists(binDll, ec)) {
        ec.clear();
        packedSameAsBin = fs::equivalent(packedDll, binDll, ec);
        ec.clear();
    }

    if (fs::exists(packedDll, ec) && !packedSameAsBin) {
        ec.clear();
        fs::copy_file(packedDll, binDll, fs::copy_options::overwrite_existing,
                      ec);
        if (ec) {
            spdlog::warn("Failed to sync packed shell.dll into bin: {}",
                         ec.message());
        }
        ec.clear();
    }

    if (!fs::exists(dataDll, ec)) {
        ec.clear();
        fs::create_directories(dataDll.parent_path(), ec);
        ec.clear();
        if (fs::exists(binDll, ec)) {
            fs::copy_file(binDll, dataDll, ec);
        } else if (fs::exists(packedDll, ec)) {
            fs::copy_file(packedDll, dataDll, ec);
        }
        dllPath = dataDll.wstring();
        return;
    }

    fs::path source;
    if (fs::exists(binDll, ec)) {
        source = binDll;
    } else if (fs::exists(packedDll, ec) && !packedSameAsBin) {
        source = packedDll;
    }
    ec.clear();
    if (source.empty()) {
        dllPath = dataDll.wstring();
        return;
    }

    bool sameFile = false;
    if (fs::exists(source, ec)) {
        ec.clear();
        sameFile = fs::equivalent(source, dataDll, ec);
        ec.clear();
    }
    if (sameFile) {
        dllPath = dataDll.wstring();
        return;
    }

    auto packedVersion = ParseVersionNumber(GetFileProductVersion(source.wstring()));
    auto dataVersion = ParseVersionNumber(GetFileProductVersion(dataDll.wstring()));

    bool packedNewer = false;
    if (packedVersion.valid && !dataVersion.valid) {
        packedNewer = true;
    } else if (packedVersion.valid && dataVersion.valid &&
               CompareVersions(packedVersion, dataVersion) > 0) {
        packedNewer = true;
    } else if (!packedVersion.valid && !dataVersion.valid &&
               fs::last_write_time(source, ec) >
                   fs::last_write_time(dataDll, ec)) {
        packedNewer = true;
    }
    ec.clear();

    if (!packedNewer) {
        dllPath = dataDll.wstring();
        return;
    }

    fs::path backup = TimestampedPath(data_directory() / L"shell.dll",
                                      L".bak");
    fs::rename(dataDll, backup, ec);
    if (ec) {
        spdlog::error("Cannot move current shell.dll: {}", ec.message());
        dllPath = dataDll.wstring();
        return;
    }
    ec.clear();
    if (!fs::copy_file(source, dataDll, ec)) {
        spdlog::error("Cannot install newer shell.dll: {}", ec.message());
        ec.clear();
        fs::rename(backup, dataDll, ec);
    } else {
        spdlog::info("shell.dll updated from packed copy: {}",
                     ToUtf8(source.wstring()));
    }
    dllPath = dataDll.wstring();
}

std::optional<std::wstring> ReadRunKeyValue() {
    HKEY hkey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, g_instance.run_key.c_str(), 0,
                      KEY_READ, &hkey) != ERROR_SUCCESS) {
        return std::nullopt;
    }
    wchar_t value[1024];
    DWORD size = sizeof(value);
    DWORD type = 0;
    bool ok = RegQueryValueExW(hkey, L"breeze-shell", nullptr, &type,
                               (LPBYTE)value, &size) == ERROR_SUCCESS &&
              type == REG_SZ;
    RegCloseKey(hkey);
    if (!ok) {
        return std::nullopt;
    }
    return std::wstring(value, size / sizeof(wchar_t) - 1);
}

void WriteRunKeyValue(const std::wstring &command) {
    HKEY hkey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, g_instance.run_key.c_str(), 0,
                      KEY_SET_VALUE, &hkey) != ERROR_SUCCESS) {
        spdlog::error("Cannot open Run key: {}", GetLastError());
        return;
    }
    RegSetValueExW(hkey, L"breeze-shell", 0, REG_SZ,
                   (BYTE *)command.c_str(),
                   (command.size() + 1) * sizeof(wchar_t));
    RegCloseKey(hkey);
}

std::optional<std::wstring> ReadScheduledTaskPath() {
    std::optional<std::wstring> result;
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    bool need_uninit = SUCCEEDED(hr);
    ITaskService *pService = NULL;
    hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER,
                          IID_ITaskService, (void **)&pService);
    if (SUCCEEDED(hr)) {
        if (SUCCEEDED(pService->Connect(_variant_t(), _variant_t(),
                                        _variant_t(), _variant_t()))) {
            ITaskFolder *pRootFolder = NULL;
            if (SUCCEEDED(pService->GetFolder(_bstr_t(L"\\"), &pRootFolder))) {
                IRegisteredTask *pTask = NULL;
                if (SUCCEEDED(pRootFolder->GetTask(
                        _bstr_t(g_instance.task_name.c_str()), &pTask))) {
                    ITaskDefinition *pTaskDef = NULL;
                    if (SUCCEEDED(pTask->get_Definition(&pTaskDef))) {
                        IActionCollection *pActions = NULL;
                        if (SUCCEEDED(pTaskDef->get_Actions(&pActions))) {
                            IAction *pAction = NULL;
                            if (SUCCEEDED(pActions->get_Item(1, &pAction))) {
                                IExecAction *pExec = NULL;
                                if (SUCCEEDED(pAction->QueryInterface(
                                        IID_IExecAction, (void **)&pExec))) {
                                    BSTR path = NULL;
                                    if (SUCCEEDED(pExec->get_Path(&path)) &&
                                        path) {
                                        result = std::wstring(path);
                                        SysFreeString(path);
                                    }
                                    pExec->Release();
                                }
                                pAction->Release();
                            }
                            pActions->Release();
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
    if (need_uninit) {
        CoUninitialize();
    }
    return result;
}

int RunHidden(const std::wstring &cmdline) {
    STARTUPINFOW si = {sizeof(si)};
    PROCESS_INFORMATION pi = {};
    if (!CreateProcessW(nullptr, const_cast<LPWSTR>(cmdline.c_str()),
                        nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr,
                        nullptr, &si, &pi)) {
        return -1;
    }
    CloseHandle(pi.hThread);
    WaitForSingleObject(pi.hProcess, 15000);
    DWORD code = 0;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess);
    return (int)code;
}

void ReplaceLegacyInjectorInPlace(const std::wstring &legacyExe) {
    std::error_code ec;
    fs::path self(GetSelfPath());
    fs::path legacy(legacyExe);

    auto lower = [](std::wstring s) {
        std::transform(s.begin(), s.end(), s.begin(), ::towlower);
        return s;
    };
    if (lower(self.wstring()) == lower(legacy.wstring())) {
        return;
    }
    if (!fs::exists(CanonicalExePath(), ec)) {
        return;
    }
    ec.clear();

    fs::path staged(legacyExe + L".new");
    fs::remove(staged, ec);
    ec.clear();
    if (!fs::copy_file(CanonicalExePath(), staged, ec)) {
        spdlog::error("Cannot stage replacement for legacy injector: {}",
                      ec.message());
        return;
    }
    ec.clear();

    fs::path backup(legacyExe + L".old");
    fs::remove(backup, ec);
    ec.clear();
    fs::rename(legacy, backup, ec);
    if (ec) {
        spdlog::error("Cannot move legacy injector: {}", ec.message());
        fs::remove(staged, ec);
        return;
    }
    ec.clear();
    fs::rename(staged, legacy, ec);
    if (ec) {
        spdlog::error("Cannot install replacement injector: {}",
                      ec.message());
        ec.clear();
        if (!fs::exists(legacy, ec) && fs::exists(backup, ec)) {
            ec.clear();
            fs::rename(backup, legacy, ec);
        }
        return;
    }
    spdlog::info("Legacy injector replaced in place: {}",
                 ToUtf8(legacy.wstring()));
}

bool ConsistentInjectorRunning() {
    SetLastError(0);
    HANDLE mutex = CreateMutexW(
        NULL, TRUE, ScopedName(L"breeze-shell-inject-consistent").c_str());
    bool running = GetLastError() == ERROR_ALREADY_EXISTS ||
                   GetLastError() == ERROR_ACCESS_DENIED;
    CloseHandle(mutex);
    return running;
}

void MigrateStartupRegistrations() {
    std::error_code ec;
    fs::path canonical = CanonicalExePath();
    if (!fs::exists(canonical, ec)) {
        return;
    }
    ec.clear();

    std::wstring canonicalStr = canonical.wstring();
    std::wstring wantCommand = L"\"" + canonicalStr + L"\" inject-consistent";

    auto pathFromCommand = [](const std::wstring &command) {
        if (command.size() >= 2 && command[0] == L'"') {
            auto end = command.find(L'"', 1);
            if (end != std::wstring::npos) {
                return command.substr(1, end - 1);
            }
        }
        return command;
    };

    auto lower = [](std::wstring s) {
        std::transform(s.begin(), s.end(), s.begin(), ::towlower);
        return s;
    };

    if (auto runValue = ReadRunKeyValue()) {
        auto currentExe = pathFromCommand(*runValue);
        if (lower(currentExe) != lower(canonicalStr)) {
            WriteRunKeyValue(wantCommand);
            spdlog::info("Run entry migrated: {} -> {}",
                         ToUtf8(currentExe), ToUtf8(canonicalStr));
        }
    }

    if (auto taskPath = ReadScheduledTaskPath()) {
        if (lower(*taskPath) == lower(canonicalStr)) {
            return;
        }
        spdlog::info("Scheduled task points to {}", ToUtf8(*taskPath));

        std::wstring change = L"schtasks.exe /Change /TN \"" +
                              g_instance.task_name + L"\" /TR \"\"" +
                              wantCommand + L"\"\"";
        int code = RunHidden(change);
        if (code == 0) {
            spdlog::info("Scheduled task retargeted via schtasks");
            return;
        }
        spdlog::warn("schtasks /Change failed ({}), trying task scheduler API",
                     code);

        HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        bool need_uninit = SUCCEEDED(hr);
        bool retargeted = false;
        ITaskService *pService = NULL;
        hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER,
                              IID_ITaskService, (void **)&pService);
        if (SUCCEEDED(hr)) {
            if (SUCCEEDED(pService->Connect(_variant_t(), _variant_t(),
                                            _variant_t(), _variant_t()))) {
                ITaskFolder *pRootFolder = NULL;
                if (SUCCEEDED(pService->GetFolder(_bstr_t(L"\\"),
                                                  &pRootFolder))) {
                    IRegisteredTask *pTask = NULL;
                    if (SUCCEEDED(pRootFolder->GetTask(
                            _bstr_t(g_instance.task_name.c_str()), &pTask))) {
                        ITaskDefinition *pTaskDef = NULL;
                        if (SUCCEEDED(pTask->get_Definition(&pTaskDef))) {
                            IActionCollection *pActions = NULL;
                            if (SUCCEEDED(pTaskDef->get_Actions(&pActions))) {
                                IAction *pAction = NULL;
                                if (SUCCEEDED(pActions->get_Item(1,
                                                                 &pAction))) {
                                    IExecAction *pExec = NULL;
                                    if (SUCCEEDED(pAction->QueryInterface(
                                            IID_IExecAction,
                                            (void **)&pExec))) {
                                        pExec->put_Path(
                                            _bstr_t(canonicalStr.c_str()));
                                        pExec->put_Arguments(
                                            _bstr_t(L"inject-consistent"));
                                        pExec->Release();
                                        retargeted = true;
                                    }
                                    pAction->Release();
                                }
                            }
                            if (retargeted) {
                                IRegisteredTask *pRegistered = NULL;
                                hr = pRootFolder->RegisterTaskDefinition(
                                    _bstr_t(g_instance.task_name.c_str()),
                                    pTaskDef, TASK_CREATE_OR_UPDATE,
                                    _variant_t(), _variant_t(),
                                    TASK_LOGON_INTERACTIVE_TOKEN,
                                    _variant_t(L""), &pRegistered);
                                retargeted = SUCCEEDED(hr);
                                if (pRegistered) {
                                    pRegistered->Release();
                                }
                            }
                            pActions->Release();
                        }
                        pTaskDef->Release();
                    }
                    pTask->Release();
                }
                pRootFolder->Release();
            }
        }
        pService->Release();

        if (need_uninit) {
            CoUninitialize();
        }

        if (retargeted) {
            spdlog::info("Scheduled task retargeted via task scheduler API");
        } else {
            spdlog::warn(
                "Scheduled task migration failed, replacing legacy injector "
                "in place");
            ReplaceLegacyInjectorInPlace(*taskPath);
        }
    }
}

bool AnyStartupRegistrationPresent() {
    if (ReadRunKeyValue()) {
        return true;
    }
    return ReadScheduledTaskPath().has_value();
}

void WaitForConsistentInjectorExit(int timeoutMs) {
    SignalInjectConsistentExit();
    for (int waited = 0; waited < timeoutMs; waited += 250) {
        if (!ConsistentInjectorRunning()) {
            return;
        }
        Sleep(250);
    }
}

void StartConsistentInjection() {
    HANDLE mutex = CreateMutexW(
        NULL, TRUE, ScopedName(L"breeze-shell-inject-consistent").c_str());
    if (GetLastError() == ERROR_ALREADY_EXISTS ||
        GetLastError() == ERROR_ACCESS_DENIED) {
        spdlog::warn("Another consistent injector is still running.");
        CloseHandle(mutex);
        return;
    }

    std::thread([]() {
        HANDLE event = CreateEventW(
            NULL, TRUE, FALSE,
            ScopedName(L"breeze-shell-inject-consistent-exit").c_str());
        WaitForSingleObject(event, INFINITE);
        CloseHandle(event);
        exit(0);
    }).detach();

    InjectAllConsistent();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nShowCmd) {
    init_inject_logger();
    LoadInstanceConfig();

    if (!InstanceMarkerMatches()) {
        spdlog::error(
            "Refusing to run: data directory belongs to instance '{}' but "
            "this process is instance '{}'",
            ToUtf8(ReadInstanceMarker().value_or(L"<missing>")),
            ToUtf8(g_instance.id));
        return 1;
    }

    int argc = 0;
    auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    std::vector<std::wstring> args;
    for (int x = 0; x < argc; x++) {
        args.push_back(argv[x]);
    }

    UpdateDllPath();
    EnsureCanonicalInstall();

    if (args.size() <= 1) {
        MigrateStartupRegistrations();

        if (false) {
            AttachConsole(ATTACH_PARENT_PROCESS);
            freopen("CONOUT$", "w", stdout);
            freopen("CONOUT$", "w", stderr);
            freopen("CONIN$", "r", stdin);
        }

        try {
            spdlog::info("breeze-shell injector started.");
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
            StartConsistentInjection();
        } else if (args[1] == L"restart-consistent") {
            WaitForConsistentInjectorExit(10000);
            StartConsistentInjection();
        } else if (args[1] == L"migrate") {
            bool hadRegistration = AnyStartupRegistrationPresent();
            bool wasRunning = ConsistentInjectorRunning();
            MigrateStartupRegistrations();
            WaitForConsistentInjectorExit(10000);
            if (hadRegistration || wasRunning) {
                StartConsistentInjection();
            }
        } else if (args[1] == L"restart-explorer") {
            restart_explorer();
        } else {
            spdlog::error("Invalid argument.");
        }
    }

    return 0;
}
