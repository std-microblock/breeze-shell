#pragma once
#include "quickjspp.hpp"
#include <chrono>
#include <expected>
#include <filesystem>
#include <fstream>
#include <print>
#include <thread>

namespace mb_shell {

struct example_struct_jni {
  int a;
  int b;

  int add();

  ~example_struct_jni();

  example_struct_jni();
};

struct script_context {
  std::unique_ptr<qjs::Runtime> rt;
  std::unique_ptr<qjs::Context> js;

  script_context();

  void bind();

  void eval(const std::string &script);

  void eval_file(const std::filesystem::path &path);

  void watch_file(const std::filesystem::path &path);
};
} // namespace mb_shell