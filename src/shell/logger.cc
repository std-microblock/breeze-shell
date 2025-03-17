#include "logger.h"
#include "config.h"
#include "utils.h"

#include <fstream>
#include <iostream>
#include <string>

#include <windows.h>

namespace mb_shell {
void append_debug_string(const std::string &str) {
  static std::ofstream file(config::data_directory() / "debug.log",
                            std::ios::app);
  file << str;
  file.close();

  auto handle = GetStdHandle(STD_OUTPUT_HANDLE);
  if (handle == INVALID_HANDLE_VALUE) {
    return;
  }

  DWORD written;
  std::wstring wstr = utf8_to_wstring(str);
  WriteConsoleW(handle, wstr.c_str(), wstr.size(), &written, nullptr);
}
} // namespace mb_shell