#include "script.h"

#include "binding_qjs.h"
#include "shell/config.h"
#include "shell/contextmenu/contextmenu.h"
#include "shell/logger.h"

#include <algorithm>
#include <ranges>
#include <thread>

#include "FileWatch.hpp"

extern "C" const uint8_t _binary_script_js_start[];
extern "C" const uint8_t _binary_script_js_end[];

std::string breeze_script_js =
    std::string(reinterpret_cast<const char *>(_binary_script_js_start),
                reinterpret_cast<const char *>(_binary_script_js_end));

namespace mb_shell {

void println(qjs::rest<std::string> args) {
    spdlog::info(std::ranges::fold_left(
        args, std::string(), [](const std::string &a, const std::string &b) {
            return a + b + " ";
        }));
}

script_context::script_context() {
    on_bind.push_back([this]() {
        auto &module = js->addModule("mshell");
        module.function("println", println);
        mshell_bindAll(module);

        if (auto res = eval_string(breeze_script_js, "breeze-script.js");
            !res) {
            spdlog::error("Error in breeze_script_js: {}", res.error());
            return;
        }
    });
}

void script_context::watch_folder(const std::filesystem::path &path,
                                  std::function<bool()> on_reload) {
    bool has_update = false;

    auto reload_all = [&]() {
        spdlog::info("Reloading all scripts");

        menu_callbacks_js.clear();
        is_js_ready.store(false);
        module_base = path;
        reset_runtime();

        std::vector<std::filesystem::path> files;
        std::ranges::copy(std::filesystem::directory_iterator(path) |
                              std::ranges::views::filter([](auto &entry) {
                                  return entry.path().extension() == ".js";
                              }),
                          std::back_inserter(files));

        auto plugin_load_order = config::current->plugin_load_order;
        std::ranges::sort(files, [&](const auto &a, const auto &b) {
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

        for (const auto &script_path : files) {
            if (auto res = eval_file(script_path); !res) {
                spdlog::error("Error evaluating file {}: {}",
                              script_path.string(), res.error());
            }
        }

        is_js_ready.store(true);
        is_js_ready.notify_all();
    };

    reload_all();

    filewatch::FileWatch<std::string> watch(
        path.generic_string(),
        [&](const std::string &changed_path, const filewatch::Event) {
            if (!changed_path.ends_with(".js")) {
                return;
            }

            spdlog::info("File change detected: {}", changed_path);
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
