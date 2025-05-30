#pragma once
#include "nanovg.h"
#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>
#include <chrono>

namespace mb_shell {
std::string wstring_to_utf8(std::wstring const &str);
std::wstring utf8_to_wstring(std::string const &str);
bool is_win11_or_later();
bool is_light_mode();
bool is_acrylic_available();
std::optional<std::string> env(const std::string &name);
bool is_memory_readable(const void *ptr);
NVGcolor parse_color(const std::string &str);
std::string format_color(NVGcolor color);
void set_thread_locale_utf8();

std::vector<std::string> split_string(const std::string &str, char delimiter);

struct task_queue {
public:
  task_queue();

  ~task_queue();

  template <typename F, typename... Args>
  auto add_task(F &&f, Args &&...args)
      -> std::future<std::invoke_result_t<F, Args...>> {
    using return_type = std::invoke_result_t<F, Args...>;

    if (stop) {
      throw std::runtime_error("add_task called on stopped task_queue");
    }

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<return_type> res = task->get_future();

    {
      std::lock_guard<std::mutex> lock(queue_mutex);
      tasks.emplace([task]() { (*task)(); });
    }

    condition.notify_one();
    return res;
  }

private:
  void run();

  std::thread worker;
  std::queue<std::function<void()>> tasks;
  std::mutex queue_mutex;
  std::condition_variable condition;
  bool stop;
};

struct perf_counter {
    std::chrono::high_resolution_clock::time_point start;
    std::chrono::high_resolution_clock::time_point last_end;
    std::string name;
    void end(std::optional<std::string> block_name = {});
    perf_counter(std::string name);
};
} // namespace mb_shell