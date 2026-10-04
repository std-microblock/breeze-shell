#pragma once

#include "script/script.h"
#include "window_proc_hook.h"

namespace mb_shell {
struct entry {
    static window_proc_hook main_window_loop_hook;
};

script_context &main_script_context();
} // namespace mb_shell