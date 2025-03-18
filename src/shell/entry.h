#pragma once

#include "window_proc_hook.h"

namespace mb_shell {
struct entry {
  static window_proc_hook main_window_loop_hook;
};
} // namespace mb_shell