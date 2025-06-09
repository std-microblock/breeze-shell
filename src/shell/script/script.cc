#include "script.h"
#include "../contextmenu/contextmenu.h"
#include "binding_qjs.h"

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
#include "quickjs.h"
#include "quickjspp.hpp"

#include "../logger.h"

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

  auto g = js->global();
  g["console"] = js->newObject();
  qjs::Value println_fn = 
  qjs::js_traits<std::function<void(qjs::rest<std::string>)>>::wrap(
      js->ctx, println);
  g["console"]["log"] = println_fn;
  g["console"]["info"] = println_fn;
  g["console"]["warn"] = println_fn;
  g["console"]["error"] = println_fn;
  g["console"]["debug"] = println_fn;
}
script_context::script_context() : rt{}, js{} {}
void script_context::watch_folder(const std::filesystem::path &path,
                                  std::function<bool()> on_reload) {
  bool has_update = false;

  std::optional<std::thread> js_thread;
  auto reload_all = [&]() {
    dbgout("Reloading all scripts");

    *stop_signal = true;
    stop_signal = std::make_shared<int>(false);

    if (js_thread)
      js_thread->join();
    dbgout("Creating JS thread");
    menu_callbacks_js.clear();

    js_thread = std::thread([&, this, ss = stop_signal]() {
      is_thread_js_main = true;
      set_thread_locale_utf8();
      rt = std::make_shared<qjs::Runtime>();
      JS_UpdateStackTop(rt->rt);
      js = std::make_shared<qjs::Context>(*rt);
      bind();
      try {
        JS_UpdateStackTop(rt->rt);
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

          js->moduleLoader = [&](std::string_view module_name) {
            auto module_path =
                path.parent_path() / (std::string(module_name) + ".js");
            if (!std::filesystem::exists(module_path)) {
              return qjs::Context::ModuleData{};
            }
            std::ifstream file(module_path);
            std::string script((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
            return qjs::Context::ModuleData{script};
          };
          auto func = JS_Eval(js->ctx, script.c_str(), script.size(),
                              path.generic_string().c_str(),
                              JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);

          if (JS_IsException(func)) {
            std::cerr << "Error in file: " << path << std::endl;
            JS_FreeValue(js->ctx, func);
            continue;
          }

          JSModuleDef *m = (JSModuleDef *)JS_VALUE_GET_PTR(func);
          auto meta_obj = JS_GetImportMeta(js->ctx, m);

          JS_DefinePropertyValueStr(
              js->ctx, meta_obj, "url",
              JS_NewString(js->ctx, path.generic_string().c_str()),
              JS_PROP_C_W_E);

          JS_DefinePropertyValueStr(
              js->ctx, meta_obj, "name",
              JS_NewString(js->ctx, path.filename().generic_string().c_str()),
              JS_PROP_C_W_E);

          JS_FreeValue(js->ctx, meta_obj);

          auto val = qjs::Value{js->ctx, JS_EvalFunction(js->ctx, func)};
          if (val.isError()) {
            std::cerr << "Error in file: " << path << (std::string)val << (std::string)val["stack"]
                      << std::endl;
          }
        } catch (std::exception &e) {
          std::cerr << "Error in file: " << path << " " << e.what()
                    << std::endl;
        }
      }

      while (auto ptr = js) {
        if (ptr->ctx) {
          while (JS_IsJobPending(rt->rt) && !*ss) {
            auto ctx = ptr->ctx;
            auto ctx1 = ctx;
            auto res = JS_ExecutePendingJob(rt->rt, &ctx1);
            if (res == -999) {
              has_update = true;
              dbgout("JS loop critical error! Restarting...");
              return;
            }
            std::lock_guard lock(ptr->js_job_start_mutex);
            ptr->has_pending_job = JS_IsJobPending(rt->rt);
          }
        }
        if (*ss)
          break;
        std::unique_lock lock(js->js_job_start_mutex);
        if (js->has_pending_job)
          continue;
        js->js_job_start_cv.wait_for(
            lock, std::chrono::milliseconds(100),
            [&]() { return js->has_pending_job || *ss; });
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

        dbgout("File change detected: {}", path);
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
