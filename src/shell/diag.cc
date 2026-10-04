#include "diag.h"

#include <algorithm>
#include <chrono>
#include <deque>
#include <mutex>

#include <spdlog/sinks/base_sink.h>
#include <spdlog/spdlog.h>

namespace mb_shell::diag {
namespace {
constexpr size_t max_records = 4000;
constexpr size_t max_problems_per_category = 50;

struct pending_structured {
    const std::string *message = nullptr;
    const std::string *fields = nullptr;
    const std::string *source = nullptr;
};
thread_local pending_structured pending;
thread_local bool capture_suppressed = false;

struct problem_store {
    std::mutex mutex;
    std::vector<problem> items;
    uint64_t revision = 0;
};

problem_store &store() {
    static auto *s = new problem_store();
    return *s;
}

class memory_sink_impl : public spdlog::sinks::base_sink<std::mutex> {
  public:
    std::vector<log_record> after(uint64_t after_id, size_t max_count) {
        std::lock_guard lock(mutex_);
        auto it = std::upper_bound(
            records.begin(), records.end(), after_id,
            [](uint64_t id, const log_record &r) { return id < r.id; });
        auto available = static_cast<size_t>(records.end() - it);
        if (available > max_count)
            it += available - max_count;
        return {it, records.end()};
    }

    void clear() {
        std::lock_guard lock(mutex_);
        records.clear();
    }

  protected:
    void sink_it_(const spdlog::details::log_msg &msg) override {
        log_record r;
        r.id = ++next_id;
        r.time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        msg.time.time_since_epoch())
                        .count();
        r.level = msg.level;
        r.thread_id = msg.thread_id;
        if (pending.message) {
            r.message = *pending.message;
            r.fields = *pending.fields;
            r.source = *pending.source;
        } else {
            r.message.assign(msg.payload.begin(), msg.payload.end());
        }
        if (r.source.empty() && !msg.source.empty()) {
            r.source = std::string(msg.source.filename) + ":" +
                       std::to_string(msg.source.line);
        }

        if (!capture_suppressed && (msg.level == spdlog::level::err ||
                                    msg.level == spdlog::level::critical)) {
            add_problem({.category = "runtime",
                         .severity = msg.level == spdlog::level::critical
                                         ? "critical"
                                         : "error",
                         .title = r.message.substr(0, r.message.find('\n')),
                         .detail = r.message,
                         .source = r.source,
                         .time_ms = r.time_ms});
        }

        records.push_back(std::move(r));
        if (records.size() > max_records)
            records.pop_front();
    }

    void flush_() override {}

  private:
    std::deque<log_record> records;
    uint64_t next_id = 0;
};

std::shared_ptr<memory_sink_impl> &sink_instance() {
    static auto *sink = new std::shared_ptr<memory_sink_impl>(
        std::make_shared<memory_sink_impl>());
    return *sink;
}
} // namespace

int64_t now_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::shared_ptr<spdlog::sinks::sink> memory_sink() { return sink_instance(); }

std::vector<log_record> logs_after(uint64_t after_id, size_t max_count) {
    return sink_instance()->after(after_id, max_count);
}

void clear_logs() { sink_instance()->clear(); }

void log_structured(spdlog::level::level_enum level, const std::string &message,
                    const std::string &fields_json, const std::string &source) {
    struct reset {
        ~reset() { pending = {}; }
    } guard;
    pending = {&message, &fields_json, &source};
    if (fields_json.empty())
        spdlog::log(level, "{}", message);
    else
        spdlog::log(level, "{} {}", message, fields_json);
}

void set_problems(const std::string &category, std::vector<problem> problems) {
    auto &s = store();
    std::lock_guard lock(s.mutex);
    std::vector<std::string> notified_titles;
    for (auto &p : s.items) {
        if (p.category == category && p.notified)
            notified_titles.push_back(p.title + '\n' + p.detail);
    }
    std::erase_if(s.items,
                  [&](const problem &p) { return p.category == category; });
    for (auto &p : problems) {
        p.category = category;
        if (!p.time_ms)
            p.time_ms = now_ms();
        p.notified = std::ranges::contains(notified_titles,
                                           p.title + '\n' + p.detail);
        s.items.push_back(std::move(p));
    }
    ++s.revision;
}

void add_problem(problem p) {
    if (!p.time_ms)
        p.time_ms = now_ms();
    auto &s = store();
    std::lock_guard lock(s.mutex);
    auto same = std::ranges::find_if(s.items, [&](const problem &e) {
        return e.category == p.category && e.title == p.title;
    });
    if (same != s.items.end()) {
        same->count++;
        same->time_ms = p.time_ms;
        same->detail = std::move(p.detail);
    } else {
        if (std::ranges::count(s.items, p.category, &problem::category) >=
            max_problems_per_category) {
            auto oldest = std::ranges::find(s.items, p.category,
                                            &problem::category);
            s.items.erase(oldest);
        }
        s.items.push_back(std::move(p));
    }
    ++s.revision;
}

void report(spdlog::level::level_enum level, problem p) {
    {
        struct reset {
            ~reset() { capture_suppressed = false; }
        } guard;
        capture_suppressed = true;
        if (p.detail.empty() || p.detail == p.title)
            spdlog::log(level, "{}", p.title);
        else
            spdlog::log(level, "{}: {}", p.title, p.detail);
    }
    add_problem(std::move(p));
}

void clear_problems(const std::string &category) {
    auto &s = store();
    std::lock_guard lock(s.mutex);
    std::erase_if(s.items,
                  [&](const problem &p) { return p.category == category; });
    ++s.revision;
}

std::vector<problem> problems() {
    auto &s = store();
    std::lock_guard lock(s.mutex);
    return s.items;
}

std::vector<problem> claim_unnotified() {
    auto &s = store();
    std::lock_guard lock(s.mutex);
    std::vector<problem> out;
    for (auto &p : s.items) {
        if (!p.notified) {
            p.notified = true;
            out.push_back(p);
        }
    }
    return out;
}

uint64_t problems_revision() {
    auto &s = store();
    std::lock_guard lock(s.mutex);
    return s.revision;
}
} // namespace mb_shell::diag
