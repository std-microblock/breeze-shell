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
  file.flush();

  printf("%s\n", str.c_str());
  OutputDebugStringA(str.c_str());
}
} // namespace mb_shell