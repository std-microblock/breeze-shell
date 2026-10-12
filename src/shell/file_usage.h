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
    // 是否是调用方自己的进程 (例如注入在资源管理器里的 shell)
    bool is_current = false;
};

// 查询占用该路径的进程, 空表示没有被占用 (文件走 Restart Manager, 文件夹另外查内容与工作目录)
std::vector<usage_process> query(const std::string &path);
bool in_use(const std::string &path);
// 这些路径里有没有被占用的 (一次会话查完, 比逐个 query 快得多)
bool any_in_use(const std::vector<std::string> &paths);
// 结束占用进程; 进程镜像路径必须与 expected_path 一致 (为空直接拒绝), 免得 pid 被复用后杀错
bool terminate(uint32_t pid, const std::string &expected_path);

} // namespace mb_shell::file_usage
