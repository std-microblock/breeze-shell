#include "quickjspp.hpp"
#include <print>

namespace qjs {
thread_local Context *Context::current;
void wait_with_msgloop(std::function<void()> f) {
  auto this_thread = GetCurrentThreadId();
  bool completed_flag = false;
  auto thread_wait = std::thread([=, &completed_flag]() {
    f();
    completed_flag = true;
    PostThreadMessageW(this_thread, WM_NULL, 0, 0);
  });

  MSG msg;
  while (GetMessageW(&msg, nullptr, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);

    if (completed_flag) {
      break;
    }
  }
  thread_wait.join();
}
} // namespace qjs