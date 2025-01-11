#pragma once
#include <string>

namespace mb_shell {
std::string wstring_to_utf8(std::wstring const &str);
std::wstring utf8_to_wstring(std::string const &str);
bool is_win11_or_later();
} // namespace mb_shell