#pragma once
#include <format>
#include <iostream>
#include <fstream>

namespace mb_shell {
    void append_debug_string(const std::string &str);
     template <class... types>
     void dbgout(const std::format_string<types...> fmt, types&&... args) {
        std::string str = std::format(fmt, std::forward<types>(args)...);
        append_debug_string(str + "\n");
    }
}