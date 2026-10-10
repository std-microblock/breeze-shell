#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mb_shell::file_usage {

struct usage_process {
    uint32_t pid = 0;
    std::string name;
    std::string path;
    std::string app_name;
    std::string held_file;
    bool working_directory = false;
};

// 查询占用该路径的进程, 空表示没有被占用 (文件走 Restart Manager, 文件夹另外查内容与工作目录)
std::vector<usage_process> query(const std::string &path);
bool in_use(const std::string &path);
bool terminate(uint32_t pid);

} // namespace mb_shell::file_usage
