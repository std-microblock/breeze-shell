#pragma once

#include <Windows.h>

#include <chrono>
#include <deque>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

namespace mb_shell {
inline bool is_crash_exit_code(DWORD exit_code) {
    return exit_code >= 0xC0000000u;
}

struct crash_marker {
    DWORD code = 0;
    std::string module;
    int first_breeze_frame = -1;
    int breeze_frames = 0;
    unsigned long long uptime_ms = 0;
    std::string stack_modules;
};

inline std::optional<crash_marker>
read_crash_marker(const std::filesystem::path &path) {
    std::ifstream file(path);
    if (!file)
        return std::nullopt;

    crash_marker marker;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        auto eq = line.find('=');
        if (eq == std::string::npos)
            continue;
        auto key = line.substr(0, eq);
        auto value = line.substr(eq + 1);
        if (key == "code")
            marker.code = std::stoul(value, nullptr, 16);
        else if (key == "module")
            marker.module = value;
        else if (key == "first_breeze_frame")
            marker.first_breeze_frame = std::stoi(value);
        else if (key == "breeze_frames")
            marker.breeze_frames = std::stoi(value);
        else if (key == "uptime_ms")
            marker.uptime_ms = std::stoull(value);
        else if (key == "stack_modules")
            marker.stack_modules = value;
    }
    return marker;
}

enum class crash_blame { none, breeze, foreign, unknown };

constexpr int kBreezeBlameFrameDepth = 8;

inline crash_blame blame_crash(DWORD exit_code,
                               const std::optional<crash_marker> &marker) {
    if (marker) {
        if (_stricmp(marker->module.c_str(), "shell.dll") == 0 ||
            (marker->first_breeze_frame >= 0 &&
             marker->first_breeze_frame < kBreezeBlameFrameDepth))
            return crash_blame::breeze;
        return crash_blame::foreign;
    }
    return is_crash_exit_code(exit_code) ? crash_blame::unknown
                                         : crash_blame::none;
}

class crash_window {
  public:
    using clock = std::chrono::steady_clock;

    crash_window(int limit, clock::duration window)
        : limit_(limit), window_(window) {}

    bool record(clock::time_point now) {
        crashes_.push_back(now);
        while (!crashes_.empty() && now - crashes_.front() > window_)
            crashes_.pop_front();
        return static_cast<int>(crashes_.size()) >= limit_;
    }

    void reset() { crashes_.clear(); }

  private:
    int limit_;
    clock::duration window_;
    std::deque<clock::time_point> crashes_;
};
} // namespace mb_shell
