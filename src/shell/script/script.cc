#include "script.h"
#include "../contextmenu/contextmenu.h"
#include "binding_qjs.h"
#include "quickjs/quickjs-libc.h"

#include "../utils.h"
#include <future>
#include <iostream>
#include <mutex>
#include <print>
#include <ranges>
#include <sstream>
#include <string_view>
#include <thread>
#include <unordered_set>

#include "FileWatch.hpp"
#include "quickjs/quickjs.h"

thread_local bool is_thread_js_main = false;

static unsigned char script_js_bytes[] = {
#include "script.js.h"
};

std::string breeze_script_js = std::string(
    reinterpret_cast<char *>(script_js_bytes), sizeof(script_js_bytes) - 1);

namespace mb_shell {

void println(qjs::rest<std::string> args) {
  std::stringstream ss;
  for (auto &arg : args) {
    ss << arg << " ";
  }
  ss << std::endl;
  auto ws = utf8_to_wstring(ss.str());
  WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), ws.c_str(), ws.size(), nullptr,
                nullptr);
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
script_context::script_context() : rt{}, js{} {}
void script_context::watch_folder(const std::filesystem::path &path,
                                  std::function<bool()> on_reload) {
  std::unordered_set<std::filesystem::path> files;
  auto reload_all = [&]() {
    std::println("Reloading all scripts");
    menu_callbacks_js.clear();

    *stop_signal = true;
    stop_signal = std::make_shared<int>(false);

    static std::mutex m;
    std::thread([&, this, ss = stop_signal]() {
      std::lock_guard<std::mutex> lock(m);
      is_thread_js_main = true;
      std::setlocale(LC_CTYPE, ".UTF-8");
      std::locale::global(std::locale("en_US.UTF-8"));
      rt = std::make_shared<qjs::Runtime>();
      JS_UpdateStackTop(rt->rt);
      js = std::make_shared<qjs::Context>(*rt);
      bind();

      try {
        js->eval(breeze_script_js, "breeze-script.js", JS_EVAL_TYPE_MODULE);
      } catch (std::exception &e) {
        std::cerr << "Error in breeze-script.js: " << e.what() << std::endl;
      }

      for (auto &path : files) {
        try {
          std::ifstream file(path);
          std::string script((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
          js->eval(script, path.generic_string().c_str(), JS_EVAL_TYPE_MODULE);
        } catch (std::exception &e) {
          std::cerr << "Error in file: " << path << " " << e.what()
                    << std::endl;
        }
      }

      while (auto ptr = js) {

        if (ptr->ctx) {
          auto r = js_std_loop(ptr->ctx, ss.get());
          if (r) {
            js_std_dump_error(ptr->ctx);
          }
        }

        if (*ss)
          break;

        std::unique_lock lock(js->js_job_start_mutex);
        std::this_thread::yield();

        js->js_job_start_cv.wait_for(lock, std::chrono::milliseconds(100));
      }
    }).detach();
  };

  std::ranges::copy(std::filesystem::directory_iterator(path) |
                        std::ranges::views::filter([](auto &entry) {
                          return entry.path().extension() == ".js";
                        }),
                    std::inserter(files, files.end()));

  reload_all();

  bool has_update = false;

  filewatch::FileWatch<std::string> watch(
      path.generic_string(),
      [&](const std::string &path, const filewatch::Event change_type) {
        if (!path.ends_with(".js")) {
          return;
        }
        has_update = true;
      });

  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    if (has_update && on_reload()) {
      has_update = false;
      files.clear();
      std::ranges::copy(std::filesystem::directory_iterator(path) |
                            std::ranges::views::filter([](auto &entry) {
                              return entry.path().extension() == ".js";
                            }),
                        std::inserter(files, files.end()));
      reload_all();
    }
  }
}
} // namespace mb_shell