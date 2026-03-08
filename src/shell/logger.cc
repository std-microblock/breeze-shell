#include "logger.h"
#include "config.h"
#include "utils.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/msvc_sink.h>
#include <memory>
#include <mutex>

namespace mb_shell {
static std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> console_sink;
static std::mutex logger_mutex;

void init_logger() {
    try {
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
            (config::data_directory() / "debug.log").string(), true);
        auto msvc_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();

        std::vector<spdlog::sink_ptr> sinks{file_sink, msvc_sink};

        auto logger = std::make_shared<spdlog::logger>("breeze", sinks.begin(), sinks.end());
        spdlog::set_default_logger(logger);
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
        spdlog::set_level(spdlog::level::debug);
        spdlog::flush_on(spdlog::level::info);

    } catch (const spdlog::spdlog_ex &ex) {
        printf("Log initialization failed: %s\n", ex.what());
    }
}

void add_console_sink() {
    std::lock_guard<std::mutex> lock(logger_mutex);
    if (!console_sink) {
        console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto logger = spdlog::default_logger();
        if (logger) {
            logger->sinks().push_back(console_sink);
        }
    }
}

void remove_console_sink() {
    std::lock_guard<std::mutex> lock(logger_mutex);
    if (console_sink) {
        auto logger = spdlog::default_logger();
        if (logger) {
            auto &sinks = logger->sinks();
            sinks.erase(std::remove(sinks.begin(), sinks.end(), console_sink), sinks.end());
        }
        console_sink.reset();
    }
}
} // namespace mb_shell