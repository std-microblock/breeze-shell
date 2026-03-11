#pragma once
#include <fmt/format.h>

namespace mb_shell {
void install_error_handlers();
void cleanup_error_handlers();
void report_warning(const char* message);

template<typename... Args>
void report_warning(fmt::format_string<Args...> fmt, Args&&... args) {
    report_warning(fmt::format(fmt, std::forward<Args>(args)...).c_str());
}
}