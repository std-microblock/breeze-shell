#include "window_proc_hook.h"
#include "blook/blook.h"

#include <Windows.h>

namespace mb_shell {
void window_proc_hook::install(void *hwnd) {
  if (installed)
    uninstall();
  this->hwnd = hwnd;
  this->original_proc = (void *)GetWindowLongPtrW((HWND)hwnd, GWLP_WNDPROC);
  SetWindowLongPtrW(
      (HWND)hwnd, GWLP_WNDPROC,
      (LONG_PTR)blook::Function::into_function_pointer(
          [this](HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) -> LRESULT {
            for (auto &f : this->hooks) {
              f(hwnd, this->original_proc, msg, wp, lp);
            }

            while (!this->tasks.empty()) {
              this->tasks.front()();
              this->tasks.pop();
            }

            return CallWindowProcW((WNDPROC)this->original_proc, hwnd, msg, wp,
                                   lp);
          }));
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