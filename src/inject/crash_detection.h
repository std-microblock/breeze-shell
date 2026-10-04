#pragma once

#include <Windows.h>

namespace mb_shell {
/* 只有 Windows 的严重错误码 (0xC0000000 起, 含 0xC0000005 访问冲突 /
   0xC0000409 fail-fast / 0xC0000374 堆损坏等) 才算"进程崩溃退出"。
   普通非零退出码 (用户手动结束、正常退出码 1/2/3 等) 不算。 */
inline bool is_crash_exit_code(DWORD exit_code) {
    return exit_code >= 0xC0000000u;
}
} // namespace mb_shell
