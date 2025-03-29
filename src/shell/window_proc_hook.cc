#include "window_proc_hook.h"
#include "blook/blook.h"

#include <Windows.h>
#include <unordered_set>

namespace mb_shell {
static std::unordered_set<HWND> hooked_windows;

void window_proc_hook::install(void *hwnd) {
  if (installed)
    return;
  this->hwnd = hwnd;
  this->original_proc = (void *)GetWindowLongPtrW((HWND)hwnd, GWLP_WNDPROC);

  this->hooked_proc = (void *)blook::Function::into_function_pointer(
      [this](HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) -> LRESULT {
        SetWindowLongPtrW((HWND)hwnd, GWLP_WNDPROC,
                          (LONG_PTR)this->original_proc);

        for (auto &f : this->hooks) {
          f(hwnd, this->original_proc, msg, wp, lp);
        }

        while (!this->tasks.empty()) {
          this->tasks.front()();
          this->tasks.pop();
        }

        SetWindowLongPtrW((HWND)hwnd, GWLP_WNDPROC,
                          (LONG_PTR)this->hooked_proc);

        return CallWindowProcW((WNDPROC)this->original_proc, hwnd, msg, wp, lp);
      });

  SetWindowLongPtrW((HWND)hwnd, GWLP_WNDPROC, (LONG_PTR)this->hooked_proc);
  installed = true;
}

void window_proc_hook::uninstall() {
  SetWindowLongPtrW((HWND)hwnd, GWLP_WNDPROC, (LONG_PTR)original_proc);
  installed = false;
}
window_proc_hook::~window_proc_hook() {
  if (installed) {
    uninstall();
  }
}
} // namespace mb_shell