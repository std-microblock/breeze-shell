#include "script.h"
#include "../contextmenu/contextmenu.h"
#include "binding_qjs.h"
#include "quickjs/quickjs-libc.h"
#include <debugapi.h>
#include <iostream>
#include <print>
#include <ranges>
#include <thread>
#include <unordered_set>

#include "FileWatch.hpp"

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
  auto last_mod = std::filesystem::file_time_type::min();
  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto new_mod = std::filesystem::last_write_time(path);
    if (new_mod != last_mod) {
      last_mod = new_mod;
      menu_callbacks_js.clear();
      rt = std::make_shared<qjs::Runtime>();
      js = std::make_shared<qjs::Context>(*rt);
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
script_context::script_context() : rt{}, js{} {}
void script_context::watch_folder(const std::filesystem::path &path,
                                  std::function<void()> on_reload) {
  std::unordered_set<std::filesystem::path> watched_files;
  auto reload_all = [&]() {
    std::println("Reloading all scripts");
    menu_callbacks_js.clear();
    rt = std::make_shared<qjs::Runtime>();
    js = std::make_shared<qjs::Context>(*rt);

    auto js_weak = (std::weak_ptr<qjs::Context>)js;
    std::thread([=]() {
      while (auto ptr = js_weak.lock()) {
        js_std_loop(ptr->ctx);
        Sleep(100);
      }
    }).detach();

    bind();
    for (auto &path : watched_files) {
      try {
        std::ifstream file(path);
        std::string script((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        js->eval(script, path.generic_string().c_str(), JS_EVAL_TYPE_MODULE);
      } catch (std::exception &e) {
        std::cerr << "Error in file: " << path << " " << e.what() << std::endl;
      }
    }
    on_reload();
  };

  std::ranges::copy(std::filesystem::directory_iterator(path) |
                        std::ranges::views::filter([](auto &entry) {
                          return entry.path().extension() == ".js";
                        })
                    , std::inserter(watched_files, watched_files.end()));

  reload_all();

  bool has_update = false;

  filewatch::FileWatch<std::string> watch(
      path.generic_string(),
      [&](const std::string &path, const filewatch::Event change_type) {
        if (!path.ends_with(".js")) {
          return;
        }
        if (change_type == filewatch::Event::added ||
            change_type == filewatch::Event::modified) {
          watched_files.insert(path);
        } else if (change_type == filewatch::Event::removed) {
          watched_files.erase(path);
        } else if (change_type == filewatch::Event::renamed_new) {
          watched_files.insert(path);
        } else if (change_type == filewatch::Event::renamed_old) {
          watched_files.erase(path);
        } else {
          std::cerr << "Unknown event type: " << (int)change_type << std::endl;
        }
        has_update = true;
      });

  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    if (has_update) {
      has_update = false;
      reload_all();
    }
  }
}
} // namespace mb_shell