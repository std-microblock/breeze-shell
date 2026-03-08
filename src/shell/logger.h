#pragma once
#include <spdlog/spdlog.h>
#include <fmt/format.h>

namespace mb_shell {
void init_logger();
void add_console_sink();
void remove_console_sink();
}