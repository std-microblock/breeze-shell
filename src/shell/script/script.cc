#include "script.h"
#include "../contextmenu/contextmenu.h"
#include "binding_qjs.h"
#include "quickjs/quickjs-libc.h"

#include "../config.h"
#include "../utils.h"

#include <algorithm>
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
  bool has_update = false;

  std::optional<std::thread> js_thread;
  auto reload_all = [&]() {
    std::println("Reloading all scripts");

    *stop_signal = true;
    stop_signal = std::make_shared<int>(false);

    if (js_thread)
      js_thread->join();

    menu_callbacks_js.clear();

    js_thread = std::thread([&, this, ss = stop_signal]() {
      is_thread_js_main = true;
      set_thread_locale_utf8();
      rt = std::make_shared<qjs::Runtime>();
      JS_UpdateStackTop(rt->rt);
      js = std::make_shared<qjs::Context>(*rt);
      bind();
      try {
        js->eval(breeze_script_js, "breeze-script.js", JS_EVAL_TYPE_MODULE);
      } catch (std::exception &e) {
        std::cerr << "Error in breeze-script.js: " << e.what() << std::endl;
      }

      std::vector<std::filesystem::path> files;
      std::ranges::copy(std::filesystem::directory_iterator(path) |
                            std::ranges::views::filter([](auto &entry) {
                              return entry.path().extension() == ".js";
                            }),
                        std::back_inserter(files));

      // resort files by config
      auto plugin_load_order = config::current->plugin_load_order;
      // if not found, load after all
      std::ranges::sort(files, [&](auto &a, auto &b) {
        auto a_name = a.filename().stem().string();
        auto b_name = b.filename().stem().string();

        auto a_pos = std::ranges::find(plugin_load_order, a_name);
        auto b_pos = std::ranges::find(plugin_load_order, b_name);

        if (a_pos == plugin_load_order.end() &&
            b_pos == plugin_load_order.end()) {
          return a_name < b_name;
        }

        if (a_pos == plugin_load_order.end()) {
          return false;
        }

        if (b_pos == plugin_load_order.end()) {
          return true;
        }

        return a_pos < b_pos;
      });
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

          auto r = [&] {
            __try {
              return js_std_loop(ptr->ctx, ss.get());
            } __except (EXCEPTION_EXECUTE_HANDLER) {
              std::cerr << "Error in JS loop" << std::endl;
              return -999;
            }
          }();

          if (r == -999) {
            has_update = true;
            std::println("JS loop critical error! Restarting...");
            return;
          }
          if (r) {
            js_std_dump_error(ptr->ctx);
          }
          std::lock_guard lock(ptr->js_job_start_mutex);
          ptr->has_pending_job = JS_IsJobPending(rt->rt);
        }

        if (*ss)
          break;

        std::unique_lock lock(js->js_job_start_mutex);
        if (js->has_pending_job)
          continue;
        js->js_job_start_cv.wait(lock);
      }
      is_thread_js_main = false;
    });
  };

  reload_all();

  filewatch::FileWatch<std::string> watch(
      path.generic_string(),
      [&](const std::string &path, const filewatch::Event change_type) {
        if (!path.ends_with(".js")) {
          return;
        }

        std::println("File change detected: {}", path);
        has_update = true;
      });

  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    if (has_update && on_reload()) {
      has_update = false;
      reload_all();
    }
  }
}
} // namespace mb_shell