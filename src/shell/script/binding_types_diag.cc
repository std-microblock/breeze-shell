#include "binding_types.hpp"

#include "shell/config.h"
#include "shell/diag.h"

#include <algorithm>
#include <ranges>

namespace mb_shell::js {
namespace {
spdlog::level::level_enum level_from_string(const std::string &level) {
    auto parsed = spdlog::level::from_str(level);
    if (parsed == spdlog::level::off && level != "off")
        return spdlog::level::info;
    return parsed;
}

std::string level_to_string(spdlog::level::level_enum level) {
    if (level == spdlog::level::warn)
        return "warn";
    if (level == spdlog::level::err)
        return "error";
    auto sv = spdlog::level::to_string_view(level);
    return {sv.begin(), sv.end()};
}

std::vector<diagnostic_problem>
to_js(const std::vector<diag::problem> &problems) {
    return problems | std::views::transform([](const diag::problem &p) {
               return diagnostic_problem{.category = p.category,
                                         .severity = p.severity,
                                         .title = p.title,
                                         .detail = p.detail,
                                         .source = p.source,
                                         .time = p.time_ms,
                                         .count = p.count};
           }) |
           std::ranges::to<std::vector>();
}
} // namespace

std::vector<log_entry> diagnostics::logs(int64_t after_id, int max_count) {
    return diag::logs_after(static_cast<uint64_t>(std::max<int64_t>(after_id, 0)),
                            static_cast<size_t>(std::max(max_count, 0))) |
           std::views::transform([](const diag::log_record &r) {
               return log_entry{.id = static_cast<int64_t>(r.id),
                                .time = r.time_ms,
                                .level = level_to_string(r.level),
                                .message = r.message,
                                .thread = static_cast<int64_t>(r.thread_id),
                                .source = r.source,
                                .fields = r.fields};
           }) |
           std::ranges::to<std::vector>();
}

void diagnostics::clear_logs() { diag::clear_logs(); }

void diagnostics::log(std::string level, std::string message,
                      std::string fields, std::string source) {
    diag::log_structured(level_from_string(level), message, fields, source);
}

std::vector<diagnostic_problem> diagnostics::problems() {
    return to_js(diag::problems());
}

std::vector<diagnostic_problem> diagnostics::take_new_problems() {
    return to_js(diag::claim_unnotified());
}

int64_t diagnostics::problems_revision() {
    return static_cast<int64_t>(diag::problems_revision());
}

void diagnostics::report_problem(std::string severity, std::string title,
                                 std::string detail, std::string source) {
    diag::add_problem({.category = "script",
                       .severity = std::move(severity),
                       .title = std::move(title),
                       .detail = std::move(detail),
                       .source = std::move(source)});
}

void diagnostics::clear_problems(std::string category) {
    diag::clear_problems(category);
}

std::string diagnostics::log_file_path() {
    return (config::data_directory() / "debug.log").generic_string();
}

std::string diagnostics::config_file_path() {
    return (config::data_directory() / "config.json").generic_string();
}

std::string diagnostics::effective_config() { return config::dump_config(); }

std::string diagnostics::default_config() { return config::dump_default_config(); }

std::string diagnostics::data_directory() {
    return config::data_directory().generic_string();
}

void diagnostics::reload_config() { config::read_config(); }
} // namespace mb_shell::js
