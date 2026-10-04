#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <spdlog/common.h>
#include <spdlog/sinks/sink.h>

namespace mb_shell::diag {
struct log_record {
    uint64_t id = 0;
    int64_t time_ms = 0;
    spdlog::level::level_enum level = spdlog::level::info;
    std::string message;
    uint64_t thread_id = 0;
    std::string source;
    std::string fields;
};

struct problem {
    std::string category;
    std::string severity;
    std::string title;
    std::string detail;
    std::string source;
    int64_t time_ms = 0;
    int count = 1;
    bool notified = false;
};

std::shared_ptr<spdlog::sinks::sink> memory_sink();
std::vector<log_record> logs_after(uint64_t after_id, size_t max_count);
void clear_logs();
void log_structured(spdlog::level::level_enum level, const std::string &message,
                    const std::string &fields_json, const std::string &source);

void set_problems(const std::string &category, std::vector<problem> problems);
void add_problem(problem p);
void report(spdlog::level::level_enum level, problem p);
void clear_problems(const std::string &category);
std::vector<problem> problems();
std::vector<problem> claim_unnotified();
uint64_t problems_revision();
int64_t now_ms();
} // namespace mb_shell::diag
