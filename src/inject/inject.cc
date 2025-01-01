#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <vector>
#include <string>
#include <chrono>
#include <filesystem>
#include <thread>

namespace fs = std::filesystem;

// 获取程序所在目录
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

int main() {
    // 获取程序目录下的 shell.dll 路径
    std::wstring dllPath = GetModuleDirectory() + L"\\shell.dll";

    // 获取 SeDebugPrivilege 权限
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        std::cerr << "OpenProcessToken failed: " << GetLastError() << std::endl;
        return 1;
    }

    LUID luid;
    if (!LookupPrivilegeValueA(NULL, SE_DEBUG_NAME, &luid)) {
        std::cerr << "LookupPrivilegeValue failed: " << GetLastError() << std::endl;
        CloseHandle(hToken);
        return 1;
    }

    TOKEN_PRIVILEGES tp;
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL)) {
        std::cerr << "AdjustTokenPrivileges failed: " << GetLastError() << std::endl;
        CloseHandle(hToken);
        return 1;
    }

    CloseHandle(hToken);

    // 获取初始 explorer.exe 进程列表
    std::vector<DWORD> initialPIDs = GetExplorerPIDs();

    // 启动 explorer.exe
    system("explorer.exe C:/");

    // 等待一秒
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 获取新的 explorer.exe 进程列表
    std::vector<DWORD> newPIDs = GetExplorerPIDs();

    // 找到新创建的 explorer.exe 进程
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

    // 注入 DLL
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, targetPID);
    if (hProcess == NULL) {
        std::cerr << "OpenProcess failed: " << GetLastError() << std::endl;
        return 1;
    }

    LPVOID remoteString = VirtualAllocEx(hProcess, NULL, (dllPath.size() + 1) * sizeof(wchar_t), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (remoteString == NULL) {
        std::cerr << "VirtualAllocEx failed: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return 1;
    }


    if (!WriteProcessMemory(hProcess, remoteString, dllPath.c_str(), (dllPath.size() + 1) * sizeof(wchar_t), NULL)) {
        std::cerr << "WriteProcessMemory failed: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, remoteString, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }

    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    LPTHREAD_START_ROUTINE loadLibraryW = (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32, "LoadLibraryW");

    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, loadLibraryW, remoteString, 0, NULL);
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
