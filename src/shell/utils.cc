#include "utils.h"
#include <dwmapi.h>
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#include <codecvt>
#include <iostream>

#include "windows.h"
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
bool get_personalize_dword_value(const wchar_t* value_name) {
  auto key = HKEY_CURRENT_USER;
  auto subkey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
  DWORD data = 0;
  DWORD size = sizeof(data);
  auto res = RegGetValueW(key, subkey, value_name, RRF_RT_REG_DWORD, nullptr, &data, &size);
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
  DWORD mask = PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
  return (mbi.Protect & mask) != 0;
}
