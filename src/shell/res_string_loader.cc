#include "res_string_loader.h"
#include <atomic>
#include <fstream>
#include <mutex>
#include <print>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#include "config.h"
#include "utils.h"

#include "blook/blook.h"

#include "Windows.h"

namespace mb_shell {
static std::mutex lock_str_data;
static std::unordered_map<std::wstring,
                          res_string_loader::res_string_identifier>
    str_data;
static std::unordered_map<size_t, std::string> module_name_cache;

res_string_loader::string_id res_string_loader::string_to_id(std::wstring str) {
  std::lock_guard lock(lock_str_data);
  auto it = str_data.find(str);
  if (it != str_data.end()) {
    return it->second;
  }
  return std::hash<std::wstring>{}(str);
}

std::string get_module_name_from_instance(HINSTANCE hInstance) {
  char buffer[MAX_PATH];
  GetModuleFileNameA(hInstance, buffer, MAX_PATH);
  return std::filesystem::path(buffer).filename().string();
}

size_t store_module_name(HINSTANCE hInstance) {
  std::string module_name = get_module_name_from_instance(hInstance);
  auto mod_hash = std::hash<std::string>{}(module_name);
  module_name_cache[mod_hash] = module_name;
  return mod_hash;
}

void res_string_loader::init_hook() {
  static std::atomic_bool inited = false;
  if (inited.exchange(true)) {
    return;
  }

  auto proc = blook::Process::self();
  auto kernelbase = proc->module("kernelbase.dll").value();

  static auto LoadStringWHook =
      kernelbase->exports("LoadStringW")->inline_hook();
  LoadStringWHook->install(+[](HINSTANCE hInstance, UINT uID, LPWSTR lpBuffer,
                              int cchBufferMax) -> int {
    auto res = LoadStringWHook->call_trampoline<int>(hInstance, uID, lpBuffer,
                                                     cchBufferMax);
    if (res > 0) {
      std::wstring str(lpBuffer, res);
      std::lock_guard lock(lock_str_data);
      if (str_data.find(str) != str_data.end())
        return res;
      str_data[str] = {uID, store_module_name(hInstance)};
    }
    return res;
  });

  static auto LoadStringAHook =
      kernelbase->exports("LoadStringA")->inline_hook();

  LoadStringAHook->install(+[](HINSTANCE hInstance, UINT uID, LPSTR lpBuffer,
                              int cchBufferMax) -> int {
    auto res = LoadStringAHook->call_trampoline<int>(hInstance, uID, lpBuffer,
                                                     cchBufferMax);
    if (res > 0) {
      std::string str(lpBuffer, res);
      std::lock_guard lock(lock_str_data);
      auto s = utf8_to_wstring(str);
      if (str_data.find(s) != str_data.end())
        return res;
      str_data[s] = {uID, store_module_name(hInstance)};
    }

    return res;
  });
}
std::string res_string_loader::string_to_id_string(std::wstring str) {
  auto id = string_to_id(str);
  if (auto *p = std::get_if<res_string_identifier>(&id)) {
    return std::to_string(p->id) + "@" + module_name_cache[p->module];
  } else {
    return std::to_string(std::get<size_t>(id)) + "@0";
  }
}
void res_string_loader::init() {
  std::thread([]() {
    init_known_strings();
    if (config::current->res_string_loader_use_hook)
      init_hook();
  }).detach();
}

void EnumerateStringResources(
    HMODULE mod, std::function<void(std::wstring_view, size_t)> callback) {
  EnumResourceNamesW(
      mod, MAKEINTRESOURCEW(6),
      +[](HMODULE hModule, LPCWSTR /*lpType*/, LPWSTR lpName,
          LONG_PTR lParam) -> BOOL {
        auto &cb =
            *reinterpret_cast<std::function<void(std::wstring_view, size_t)> *>(
                lParam);

        if (!IS_INTRESOURCE(lpName))
          return TRUE;

        HRSRC hRes = FindResourceW(hModule, lpName, MAKEINTRESOURCEW(6));
        if (!hRes)
          return TRUE;

        HGLOBAL hData = LoadResource(hModule, hRes);
        if (!hData)
          return TRUE;

        const BYTE *pData = static_cast<const BYTE *>(LockResource(hData));
        if (!pData)
          return TRUE;

        DWORD dwSize = SizeofResource(hModule, hRes);
        DWORD pos = 0;

        for (int i = 0; i < 16 && pos < dwSize; ++i) {
          WORD len = *reinterpret_cast<const WORD *>(pData + pos);
          pos += sizeof(WORD);

          if (len == 0)
            continue;

          if (pos + len * sizeof(WCHAR) > dwSize)
            break;

          std::wstring_view strView(
              reinterpret_cast<const wchar_t *>(pData + pos), len);
          cb(strView,
             static_cast<size_t>(reinterpret_cast<uintptr_t>(lpName)) * 16 + i);

          pos += len * sizeof(WCHAR);
        }

        return TRUE;
      },
      reinterpret_cast<LONG_PTR>(&callback));
}

void load_all_res_strings(std::string module) {
  HINSTANCE hInstance = GetModuleHandleA(module.data());
  static std::unordered_set<HINSTANCE> loaded_modules;
  if (loaded_modules.contains(hInstance)) {
    return;
  }
  if (!hInstance) {
    return;
  }
  auto mod_hash = store_module_name(hInstance);

  EnumerateStringResources(hInstance, [&](std::wstring_view str, size_t id) {
    std::lock_guard lock(lock_str_data);
    if (str.length() > 60)
      return;
    auto s = std::wstring(str);
    if (str_data.find(s) != str_data.end())
      return;
    str_data[s] = {id, mod_hash};
  });
}

void res_string_loader::init_known_strings() {
  auto res_dlls = {
      "shell32.dll",     "acppage.dll",         "ntshrui.dll",
      "appresolver.dll", "windows.storage.dll", "explorerframe.dll",
      "explorer.exe",    "user32.dll",          "wpdshext.dll",
      "display.dll",     "themecpl.dll",        "regedit.exe",
      "powershell.exe",  "stobject.dll",        "fvecpl.dll",
      "twext.dll",       "twinui.dll",          "twinui.pcshell.dll",
      "isoburn.exe"};

  auto now = std::chrono::high_resolution_clock::now();
  for (auto &dll : res_dlls) {
    load_all_res_strings(dll);
  }
  std::println("[perf] init_known_strings took {}ms",
               std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::high_resolution_clock::now() - now)
                   .count());
}
} // namespace mb_shell