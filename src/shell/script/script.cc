#include "script.h"
#include "../contextmenu/contextmenu.h"
#include "binding_qjs.h"
#include "quickjs/quickjs-libc.h"
#include <iostream>
#include <print>
#include <thread>

namespace mb_shell {

void script_context::eval(const std::string &script) {
  try {
    js->eval(script, "<import>", JS_EVAL_TYPE_MODULE);
  } catch (qjs::exception) {
    auto exc = js->getException();
    std::cerr << (std::string)exc << std::endl;
    if ((bool)exc["stack"])
      std::cerr << (std::string)exc["stack"] << std::endl;
  }
}
void script_context::eval_file(const std::filesystem::path &path) {
  try {
    std::ifstream file(path);
    std::string script((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());

    js->eval(script, path.generic_string().c_str(), JS_EVAL_TYPE_MODULE);

    std::thread([this]() {
      auto ctx = js->ctx;
      while (ctx == js->ctx) {
        js_std_loop(js->ctx);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    }).detach();
  } catch (qjs::exception) {
    auto exc = js->getException();
    std::cerr << (std::string)exc << std::endl;
    if ((bool)exc["stack"])
      std::cerr << (std::string)exc["stack"] << std::endl;
  }
}
void script_context::watch_file(const std::filesystem::path &path,
                                std::function<void()> on_reload) {
  auto last_mod = std::filesystem::last_write_time(path);
  // auto last_mod = std::filesystem::file_time_type::min();
  eval_file(path);
  on_reload();
  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto new_mod = std::filesystem::last_write_time(path);
    if (new_mod != last_mod) {
      last_mod = new_mod;
      menu_callbacks_js.clear();
      rt = std::make_unique<qjs::Runtime>();
      js = std::make_unique<qjs::Context>(*rt);
      bind();
      eval_file(path);
      on_reload();
    }
  }
}

void println(qjs::rest<std::string> args) {
  for (auto const &arg : args)
    std::cout << arg << " ";
  std::cout << "\n";
}

void script_context::bind() {
  auto &module = js->addModule("mshell");

  module.function("println", println);

  bindAll(module);

  js_std_init_handlers(rt->rt);
  js_init_module_std(js->ctx, "qjs:std");
  js_init_module_os(js->ctx, "qjs:os");
  js_init_module_bjson(js->ctx, "qjs:bjson");
}
script_context::script_context()
    : rt{std::make_unique<qjs::Runtime>()},
      js{std::make_unique<qjs::Context>(*rt)} {
  bind();
}
} // namespace mb_shell