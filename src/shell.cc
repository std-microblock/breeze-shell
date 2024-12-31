#include "blook/blook.h"

#include <ranges>
#include <string>
#include <string_view>

#include <Windows.h>

namespace mb_shell {
void main() { MessageBoxA(NULL, "Hello, World!", "mb_shell", MB_OK); }
} // namespace mb_shell

int WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID lpvReserved) {
  switch (fdwReason) {
  case DLL_PROCESS_ATTACH: {

    auto cmdline = std::string(GetCommandLineA());

    std::ranges::transform(cmdline, cmdline.begin(), tolower);
    if (cmdline.contains("rundll")) {
      PROCESS_INFORMATION info;
      STARTUPINFOA startup_info;
      ZeroMemory(&info, sizeof(info));
      ZeroMemory(&startup_info, sizeof(startup_info));

      auto res = CreateProcessA("explorer.exe", NULL, NULL, NULL, FALSE, 0,
                                NULL, NULL, &startup_info, &info);

      auto proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, info.dwProcessId);
      auto blook_proc = blook::Process::attach(proc);

      auto path = blook_proc->memo().malloc(0x1000);
      char module_path[MAX_PATH];
      GetModuleFileNameA(NULL, module_path, MAX_PATH);

      if (auto res =
              path.write(module_path, std::string_view(module_path).size() + 1);
          res) {
        CreateRemoteThread(proc, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA,
                           path.data(), 0, NULL);
      }
    }
    if (cmdline.contains("explorer")) {
      mb_shell::main();
    }
    break;
  }
  }
  return 1;
}