#include "utils.h"
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#include <codecvt>
#include <iostream>
#include <print>
#include <sstream>
#include <vector>


#include <locale>

#include "windows.h"
#include <dwmapi.h>

#include "logger.h"

std::wstring mb_shell::utf8_to_wstring(std::string const &str) {
  std::wstring_convert<
      std::conditional_t<sizeof(wchar_t) == 4, std::codecvt_utf8<wchar_t>,
                         std::codecvt_utf8_utf16<wchar_t>>>
      converter;
  return converter.from_bytes(str);
}
std::string mb_shell::wstring_to_utf8(std::wstring const &str) {
  std::wstring_convert<
      std::conditional_t<sizeof(wchar_t) == 4, std::codecvt_utf8<wchar_t>,
                         std::codecvt_utf8_utf16<wchar_t>>>
      converter;
  return converter.to_bytes(str);
}

bool mb_shell::is_win11_or_later() {
  using rtl_get_nt_version_numbers =
      void(NTAPI *)(uint32_t *, uint32_t *, uint32_t *);
  uint32_t major, minor, build;

  auto ntdll = GetModuleHandleW(L"ntdll.dll");
  if (!ntdll) {
    return false;
  }

  auto RtlGetNtVersionNumbers = (rtl_get_nt_version_numbers)GetProcAddress(
      ntdll, "RtlGetNtVersionNumbers");

  if (!RtlGetNtVersionNumbers) {
    return false;
  }

  RtlGetNtVersionNumbers(&major, &minor, &build);
  build &= 0xFFFF;
  return (major >= 10 && build >= 22000);
}
bool get_personalize_dword_value(const wchar_t *value_name) {
  auto key = HKEY_CURRENT_USER;
  auto subkey =
      L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
  DWORD data = 0;
  DWORD size = sizeof(data);
  auto res = RegGetValueW(key, subkey, value_name, RRF_RT_REG_DWORD, nullptr,
                          &data, &size);
  return res == ERROR_SUCCESS && data == 1;
}

bool mb_shell::is_light_mode() {
  return get_personalize_dword_value(L"AppsUseLightTheme");
}

bool mb_shell::is_acrylic_available() {
  return get_personalize_dword_value(L"EnableTransparency");
}
std::optional<std::string> mb_shell::env(const std::string &name) {
  wchar_t buffer[32767];
  GetEnvironmentVariableW(utf8_to_wstring(name).c_str(), buffer, 32767);
  if (buffer[0] == 0) {
    return std::nullopt;
  }
  return wstring_to_utf8(buffer);
}
bool mb_shell::is_memory_readable(const void *ptr) {
  MEMORY_BASIC_INFORMATION mbi;
  if (!VirtualQuery(ptr, &mbi, sizeof(mbi))) {
    return false;
  }
  DWORD mask = PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY |
               PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE |
               PAGE_EXECUTE_WRITECOPY;
  return (mbi.Protect & mask) != 0;
}
NVGcolor mb_shell::parse_color(const std::string &str) {
  // allowed:
  // #RRGGBB
  // #RRGGBBAA
  // #RGB
  // #RGBA
  // RRGGBB
  // RRGGBBAA
  // RGB
  // RGBA
  // r, g,b
  // r,g, b,a

  std::string s = str;
  if (s.empty())
    return nvgRGBA(0, 0, 0, 255);

  // Remove leading '#' if present
  if (s[0] == '#')
    s = s.substr(1);

  // Handle comma-separated values
  if (s.find(',') != std::string::npos) {
    std::vector<int> components;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, ',')) {
      components.push_back(std::stoi(item));
    }

    if (components.size() == 3) {
      return nvgRGB(components[0], components[1], components[2]);
    }
    if (components.size() == 4) {
      return nvgRGBA(components[0], components[1], components[2],
                     components[3]);
    }
  }

  // Handle hex values
  switch (s.length()) {
  case 3: // RGB
    return nvgRGB(17 * std::stoi(s.substr(0, 1), nullptr, 16),
                  17 * std::stoi(s.substr(1, 1), nullptr, 16),
                  17 * std::stoi(s.substr(2, 1), nullptr, 16));
  case 4: // RGBA
    return nvgRGBA(17 * std::stoi(s.substr(0, 1), nullptr, 16),
                   17 * std::stoi(s.substr(1, 1), nullptr, 16),
                   17 * std::stoi(s.substr(2, 1), nullptr, 16),
                   17 * std::stoi(s.substr(3, 1), nullptr, 16));
  case 6: // RRGGBB
    return nvgRGB(std::stoi(s.substr(0, 2), nullptr, 16),
                  std::stoi(s.substr(2, 2), nullptr, 16),
                  std::stoi(s.substr(4, 2), nullptr, 16));
  case 8: // RRGGBBAA
    return nvgRGBA(std::stoi(s.substr(0, 2), nullptr, 16),
                   std::stoi(s.substr(2, 2), nullptr, 16),
                   std::stoi(s.substr(4, 2), nullptr, 16),
                   std::stoi(s.substr(6, 2), nullptr, 16));
  }

  return nvgRGBA(0, 0, 0, 255); // Default black
}
void mb_shell::set_thread_locale_utf8() {
  std::setlocale(LC_CTYPE, ".UTF-8");
  std::locale::global(std::locale("en_US.UTF-8"));
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);

  SetThreadLocale(
      MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT));
}
mb_shell::task_queue::task_queue() : stop(false) {
  worker = std::thread(&task_queue::run, this);
}
mb_shell::task_queue::~task_queue() {
  {
    std::lock_guard<std::mutex> lock(queue_mutex);
    stop = true;
  }
  condition.notify_all();
  if (worker.joinable()) {
    worker.join();
  }
}
void mb_shell::task_queue::run() {
  while (true) {
    std::function<void()> task;
    {
      std::unique_lock<std::mutex> lock(queue_mutex);
      condition.wait(lock, [this]() { return stop || !tasks.empty(); });

      if (stop && tasks.empty()) {
        return;
      }

      task = std::move(tasks.front());
      tasks.pop();
    }

    task();
  }
}
mb_shell::perf_counter::perf_counter(std::string name) : name(name) {
  start = std::chrono::high_resolution_clock::now();
  last_end = start;
}
void mb_shell::perf_counter::end(std::optional<std::string> block_name) {
  auto now = std::chrono::high_resolution_clock::now();

  if (block_name) {
    dbgout(
        "[perf] {}: {}ms / {} {}ms", block_name.value(),
        std::chrono::duration_cast<std::chrono::milliseconds>(now - last_end)
            .count(),
        name,
        std::chrono::duration_cast<std::chrono::milliseconds>(now - start)
            .count());
  } else {
    dbgout(
        "[perf] {}: {}ms", name,
        std::chrono::duration_cast<std::chrono::milliseconds>(now - start)
            .count());
  }
  last_end = now;
}
std::vector<std::string> mb_shell::split_string(const std::string &str,
                                                char delimiter) {
  std::vector<std::string> result;
  std::string token;
  std::istringstream tokenStream(str);
  while (std::getline(tokenStream, token, delimiter)) {
    result.push_back(token);
  }
  return result;
}
std::string mb_shell::format_color(NVGcolor color) {
  return std::format(
      "#{0:02x}{1:02x}{2:02x}{3:02x}", static_cast<int>(color.r * 255),
      static_cast<int>(color.g * 255), static_cast<int>(color.b * 255),
      static_cast<int>(color.a * 255));
}
