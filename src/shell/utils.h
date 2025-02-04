#pragma once
#include <optional>
#include <string>

namespace mb_shell {
std::string wstring_to_utf8(std::wstring const &str);
std::wstring utf8_to_wstring(std::string const &str);
bool is_win11_or_later();
bool is_light_mode();
bool is_acrylic_available();
std::optional<std::string> env(const std::string &name);
bool is_memory_readable(const void *ptr);
} // namespace mb_shell