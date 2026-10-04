#include "script.h"

#include "binding_qjs.h"
#include "shell/config.h"
#include "shell/contextmenu/contextmenu.h"
#include "shell/diag.h"
#include "shell/logger.h"

#include <algorithm>
#include <exception>
#include <atomic>
#include <optional>
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

std::optional<std::string>
module_rejection(script_context &ctx,
                 const std::expected<qjs::Value, std::string> &res) {
    if (!res)
        return res.error();
    return ctx.post_sync([&]() -> std::optional<std::string> {
        auto *c = ctx.js->ctx;
        if (JS_PromiseState(c, res->v) != JS_PROMISE_REJECTED)
            return std::nullopt;
        qjs::Value reason{c, JS_PromiseResult(c, res->v)};
        auto message = reason.as<std::string>();
        if (reason.isError())
            message += "\n" + reason["stack"].as<std::string>();
        return message;
    });
}

script_context::script_context() {
    on_bind.push_back([this]() {
        auto &module = js->addModule("mshell");
        module.function("println", println);
        mshell_bindAll(module);

        if (auto error = module_rejection(
                *this, eval_string(breeze_script_js, "breeze-script.js"))) {
            diag::report(spdlog::level::err,
                         {.category = "script",
                          .severity = "critical",
                          .title = "Built-in script failed to start",
                          .detail = *error,
                          .source = "breeze-script.js"});
            return;
        }
    });
}

namespace {
// The UCRT keeps std::terminate's handler in per-thread state, so the handler
// installed on the main thread does not apply to the worker threads created here.
// Without one, an uncaught C++ exception aborts the host process - the entire
// desktop shell when injected into explorer.exe - without any diagnostics.
void install_worker_terminate_handler() {
    std::set_terminate([] {
        try {
            if (auto eptr = std::current_exception())
                std::rethrow_exception(eptr);
        } catch (const std::exception &e) {
            spdlog::critical("Uncaught exception in background thread: {}",
                             e.what());
        } catch (...) {
            spdlog::critical("Uncaught non-std exception in background thread");
        }
        spdlog::default_logger()->flush();
        std::abort();
    });
}
}  // namespace

void script_context::watch_folder(const std::filesystem::path &path,
                                  std::function<bool()> on_reload) {
    install_worker_terminate_handler();
    std::atomic_bool has_update = false;

    auto reload_all = [&]() {
        spdlog::info("Reloading all scripts");
        diag::clear_problems("script");

        menu_callbacks_js.clear();
        is_js_ready.store(false);
        module_base = path;
        stop_event_loop_in_time(std::chrono::milliseconds(500));
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
            if (auto error = module_rejection(*this, eval_file(script_path))) {
                diag::report(spdlog::level::err,
                             {.category = "script",
                              .severity = "error",
                              .title = "Failed to load " +
                                       script_path.filename().string(),
                              .detail = *error,
                              .source = script_path.string()});
            }
        }

        is_js_ready.store(true);
        is_js_ready.notify_all();
    };

    while (true) {
        try {
            std::filesystem::create_directories(path);
            reload_all();

            filewatch::FileWatch<std::string> watch(
                path.generic_string(),
                [&](const std::string &changed_path, const filewatch::Event) {
                    if (!changed_path.ends_with(".js")) {
                        return;
                    }

                    spdlog::info("File change detected: {}", changed_path);
                    has_update.store(true, std::memory_order_release);
                });

            while (true) {
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                if (has_update.load(std::memory_order_acquire) && on_reload()) {
                    has_update.store(false, std::memory_order_release);
                    reload_all();
                }
            }
        } catch (const std::exception &e) {
            is_js_ready.store(false);
            spdlog::error("Script folder watcher failed for {}: {}. Retrying.",
                          path.string(), e.what());
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

} // namespace mb_shell
