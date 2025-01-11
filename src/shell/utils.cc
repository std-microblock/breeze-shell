#include "utils.h"
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