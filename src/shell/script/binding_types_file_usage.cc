#include "binding_types.hpp"
#include "shell/file_usage.h"

namespace mb_shell::js {

bool file_usage_process::terminate() {
    return mb_shell::file_usage::terminate((uint32_t)pid, path);
}

bool file_usage::in_use(std::string path) {
    return mb_shell::file_usage::in_use(path);
}

bool file_usage::any_in_use(std::vector<std::string> paths) {
    return mb_shell::file_usage::any_in_use(paths);
}

std::vector<std::shared_ptr<file_usage_process>>
file_usage::query(std::string path) {
    std::vector<std::shared_ptr<file_usage_process>> result;
    for (const auto &entry : mb_shell::file_usage::query(path)) {
        auto process = std::make_shared<file_usage_process>();
        process->pid = (int)entry.pid;
        process->name = entry.name;
        process->path = entry.path;
        process->app_name = entry.app_name;
        process->held_file = entry.held_file;
        process->working_directory = entry.working_directory;
        process->is_current = entry.is_current;
        result.push_back(std::move(process));
    }
    return result;
}

} // namespace mb_shell::js
