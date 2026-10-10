#include "file_usage.h"
#include "shell/utils.h"

#include <windows.h>
#include <restartmanager.h>
#include <tlhelp32.h>
#include <winternl.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <ranges>
#include <unordered_map>

namespace mb_shell::file_usage {

namespace {

namespace fs = std::filesystem;

constexpr size_t max_scan_files = 2048;
constexpr size_t max_detail_files = 64;
constexpr size_t max_rm_batch = 256;
constexpr auto scan_budget = std::chrono::milliseconds(800);

std::wstring downcase(std::wstring str) {
    std::ranges::transform(str, str.begin(),
                           [](wchar_t c) { return (wchar_t)towlower(c); });
    return str;
}

std::wstring image_path_of(HANDLE process) {
    std::wstring buffer(32768, L'\0');
    DWORD size = (DWORD)buffer.size();
    const BOOL ok = QueryFullProcessImageNameW(process, 0, buffer.data(), &size);
    return ok ? std::wstring(buffer.data(), size) : std::wstring{};
}

std::wstring exe_path_of(uint32_t pid) {
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process)
        return {};
    auto path = image_path_of(process);
    CloseHandle(process);
    return path;
}

struct restart_manager_hit {
    uint32_t pid = 0;
    std::wstring app_name;
};

std::vector<restart_manager_hit>
restart_manager_probe(const std::vector<std::wstring> &paths) {
    std::vector<restart_manager_hit> hits;

    // 一次会话里注册太多资源会失败, 分批查
    for (size_t offset = 0; offset < paths.size(); offset += max_rm_batch) {
        const auto end = std::min(offset + max_rm_batch, paths.size());
        DWORD session = 0;
        wchar_t key[CCH_RM_SESSION_KEY + 1] = {};
        if (RmStartSession(&session, 0, key) != ERROR_SUCCESS)
            break;

        std::vector<LPCWSTR> raw;
        raw.reserve(end - offset);
        for (auto i = offset; i < end; i++)
            raw.push_back(paths[i].c_str());

        if (RmRegisterResources(session, (UINT)raw.size(), raw.data(), 0,
                                nullptr, 0, nullptr) == ERROR_SUCCESS) {
            UINT needed = 0, count = 0;
            DWORD reasons = 0;
            if (RmGetList(session, &needed, &count, nullptr, &reasons) ==
                    ERROR_MORE_DATA &&
                needed > 0) {
                std::vector<RM_PROCESS_INFO> info(needed);
                count = needed;
                if (RmGetList(session, &needed, &count, info.data(), &reasons) ==
                    ERROR_SUCCESS) {
                    for (UINT i = 0; i < count; i++)
                        hits.push_back({(uint32_t)info[i].Process.dwProcessId,
                                        info[i].strAppName});
                }
            }
        }

        RmEndSession(session);
    }

    return hits;
}

// RTL_USER_PROCESS_PARAMETERS 在 winternl.h 里是残缺的, 只取到工作目录为止的头部
struct process_parameters_head {
    ULONG maximum_length;
    ULONG length;
    ULONG flags;
    ULONG debug_flags;
    HANDLE console_handle;
    ULONG console_flags;
    HANDLE standard_input;
    HANDLE standard_output;
    HANDLE standard_error;
    struct {
        UNICODE_STRING dos_path;
        HANDLE handle;
    } current_directory;
};

std::wstring current_directory_of(uint32_t pid) {
    HANDLE process = OpenProcess(
        PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!process)
        return {};

    std::wstring result;
    PROCESS_BASIC_INFORMATION basic{};
    ULONG returned = 0;
    PEB peb{};
    process_parameters_head params{};
    SIZE_T read = 0;
    const auto &dos_path = params.current_directory.dos_path;

    if (NtQueryInformationProcess(process, ProcessBasicInformation, &basic,
                                  sizeof(basic), &returned) >= 0 &&
        ReadProcessMemory(process, basic.PebBaseAddress, &peb, sizeof(peb),
                          &read) &&
        ReadProcessMemory(process, peb.ProcessParameters, &params,
                          sizeof(params), &read) &&
        dos_path.Buffer && dos_path.Length > 0) {
        std::wstring buffer(dos_path.Length / sizeof(wchar_t), L'\0');
        if (ReadProcessMemory(process, dos_path.Buffer, buffer.data(),
                              dos_path.Length, &read))
            result = std::move(buffer);
    }

    CloseHandle(process);
    return result;
}

std::wstring as_directory_prefix(const std::wstring &path) {
    std::wstring prefix = downcase(path);
    if (prefix.starts_with(L"\\\\?\\"))
        prefix.erase(0, 4);
    while (prefix.size() > 3 && (prefix.back() == L'\\' || prefix.back() == L'/'))
        prefix.pop_back();
    if (!prefix.empty() && prefix.back() != L'\\')
        prefix += L'\\';
    return prefix;
}

bool is_inside(const std::wstring &prefix, std::wstring path) {
    if (path.size() <= 3)
        return false;
    if (path.starts_with(L"\\\\?\\"))
        path.erase(0, 4);
    return downcase(std::move(path)).starts_with(prefix);
}

// 直接枚举句柄需要管理员权限, 所以文件夹只能查里面的文件和别人的工作目录
std::vector<std::wstring> files_inside(const std::wstring &dir) {
    std::vector<std::wstring> files;
    std::error_code ec;
    const auto start = std::chrono::steady_clock::now();
    try {
        for (const auto &entry : fs::recursive_directory_iterator(
                 dir, fs::directory_options::skip_permission_denied, ec)) {
            if (files.size() >= max_scan_files ||
                std::chrono::steady_clock::now() - start > scan_budget)
                break;
            if (entry.is_regular_file(ec))
                files.push_back(entry.path().wstring());
        }
    } catch (const std::exception &) {
    }
    return files;
}

void collect_hits(std::vector<usage_process> &result,
                  std::unordered_map<uint32_t, size_t> &known, uint32_t pid,
                  const std::wstring &app_name, const std::wstring &held_file,
                  bool working_directory, const std::wstring &name_hint = {}) {
    auto [it, inserted] = known.try_emplace(pid, result.size());
    if (inserted) {
        auto path = exe_path_of(pid);
        usage_process process;
        process.pid = pid;
        process.path = wstring_to_utf8(path);
        process.name = wstring_to_utf8(
            path.empty() ? name_hint : fs::path(path).filename().wstring());
        process.app_name = wstring_to_utf8(app_name);
        process.is_current = pid == GetCurrentProcessId();
        result.push_back(std::move(process));
    }

    auto &process = result[it->second];
    if (process.held_file.empty() && !held_file.empty())
        process.held_file = wstring_to_utf8(held_file);
    process.working_directory = process.working_directory || working_directory;
}

std::vector<usage_process> query_directory(const std::wstring &dir) {
    std::vector<usage_process> result;
    std::unordered_map<uint32_t, size_t> known;

    auto files = files_inside(dir);
    if (files.size() <= max_detail_files) {
        for (const auto &file : files)
            for (const auto &hit : restart_manager_probe({file}))
                collect_hits(result, known, hit.pid, hit.app_name, file, false);
    } else {
        for (const auto &hit : restart_manager_probe(files))
            collect_hits(result, known, hit.pid, hit.app_name, {}, false);
    }

    const auto prefix = as_directory_prefix(dir);
    if (prefix.size() > 3) {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32W entry{};
            entry.dwSize = sizeof(entry);
            if (Process32FirstW(snapshot, &entry)) {
                do {
                    if (is_inside(prefix, current_directory_of(entry.th32ProcessID)))
                        collect_hits(result, known, entry.th32ProcessID, {}, {},
                                     true, entry.szExeFile);
                } while (Process32NextW(snapshot, &entry));
            }
            CloseHandle(snapshot);
        }
    }

    return result;
}

} // namespace

std::vector<usage_process> query(const std::string &path) {
    const fs::path target{utf8_to_wstring(path)};
    if (target.empty())
        return {};

    std::error_code ec;
    if (fs::is_directory(target, ec))
        return query_directory(target.wstring());

    std::vector<usage_process> result;
    std::unordered_map<uint32_t, size_t> known;
    ec.clear();
    const auto absolute = fs::absolute(target, ec).wstring();
    for (const auto &hit : restart_manager_probe({absolute}))
        collect_hits(result, known, hit.pid, hit.app_name, absolute, false);
    return result;
}

bool in_use(const std::string &path) { return !query(path).empty(); }

bool terminate(uint32_t pid, const std::string &expected_path) {
    if (pid == 0 || pid == 4 || pid == GetCurrentProcessId() ||
        expected_path.empty())
        return false;

    HANDLE process = OpenProcess(
        PROCESS_TERMINATE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process)
        return false;

    const auto current = image_path_of(process);
    const bool terminated =
        !current.empty() &&
        downcase(current) == downcase(utf8_to_wstring(expected_path)) &&
        TerminateProcess(process, 0) != FALSE;
    CloseHandle(process);
    return terminated;
}

} // namespace mb_shell::file_usage
