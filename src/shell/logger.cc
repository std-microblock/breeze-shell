#include "logger.h"
#include "config.h"
#include "diag.h"

#include <Windows.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/spdlog.h>

namespace mb_shell {
void init_logger() {
    try {
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
            (config::data_directory() / "debug.log").string(), true);
        auto msvc_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();

        std::vector<spdlog::sink_ptr> sinks{file_sink, msvc_sink,
                                            diag::memory_sink()};

        auto logger = std::make_shared<spdlog::logger>("breeze", sinks.begin(),
                                                       sinks.end());
        spdlog::set_default_logger(logger);
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
        spdlog::set_level(spdlog::level::debug);
        spdlog::flush_on(spdlog::level::info);
    } catch (const spdlog::spdlog_ex &ex) {
        OutputDebugStringA(ex.what());
    }
}
} // namespace mb_shell
