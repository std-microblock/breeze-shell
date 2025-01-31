#pragma once
#include "quickjspp.hpp"
#include <chrono>
#include <expected>
#include <filesystem>
#include <fstream>
#include <memory>
#include <print>
#include <thread>
#include <threads.h>

extern thread_local bool is_thread_js_main;
namespace mb_shell {
struct script_context {
  std::shared_ptr<qjs::Runtime> rt;
  std::shared_ptr<qjs::Context> js;
  std::shared_ptr<int> stop_signal = std::make_shared<int>(0);

  script_context();

  void bind();

  void eval(const std::string &script);

  void eval_file(const std::filesystem::path &path);

  void watch_file(
      const std::filesystem::path &path,
      std::function<void()> on_reload = []() {});

  void watch_folder(
      const std::filesystem::path &path,
      std::function<bool()> on_reload = []() { return true; });
};
} // namespace mb_shell