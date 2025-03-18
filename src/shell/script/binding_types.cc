#include "binding_types.h"
#include "quickjspp.hpp"
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <print>
#include <ranges>
#include <regex>
#include <shlobj_core.h>
#include <thread>
#include <variant>

// Resid
#include "../res_string_loader.h"
// Context menu
#include "../contextmenu/menu_render.h"
#include "../contextmenu/menu_widget.h"
// Compile Information
#include "../build_info.h"

#include "script.h"
#include "winhttp.h"

std::unordered_set<
    std::shared_ptr<std::function<void(mb_shell::js::menu_info_basic_js)>>>
    mb_shell::menu_callbacks_js;
namespace mb_shell::js {

bool menu_controller::valid() { return !$menu.expired(); }
std::shared_ptr<mb_shell::js::menu_item_controller>
menu_controller::append_item_after(js_menu_data data, int after_index) {
  if (!valid())
    return nullptr;
  auto m = $menu.lock();
  if (!m)
    return nullptr;

  m->children_dirty = true;
  menu_item item;
  auto new_item = std::make_shared<menu_item_normal_widget>(item);
  auto ctl = std::make_shared<menu_item_controller>(new_item, m);
  new_item->parent = m.get();
  ctl->set_data(data);
  while (after_index < 0) {
    after_index = m->item_widgets.size() + after_index + 1;
  }

  if (after_index >= m->item_widgets.size()) {
    m->item_widgets.push_back(new_item);
  } else {
    m->item_widgets.insert(m->item_widgets.begin() + after_index, new_item);
  }
  m->update_icon_width();

  if (m->animate_appear_started) {
    new_item->reset_appear_animation(0);
  }

  return ctl;
}
std::function<void()> menu_controller::add_menu_listener(
    std::function<void(menu_info_basic_js)> listener) {
  auto listener_cvt = [listener](menu_info_basic_js info) {
    try {
      listener(info);
    } catch (std::exception &e) {
      std::cerr << "Error in listener: " << e.what() << std::endl;
    }
  };
  auto ptr =
      std::make_shared<std::function<void(menu_info_basic_js)>>(listener_cvt);
  menu_callbacks_js.insert(ptr);
  return [ptr]() { menu_callbacks_js.erase(ptr); };
}
menu_controller::~menu_controller() {}
void menu_item_controller::set_position(int new_index) {
  if (!valid())
    return;

  if (auto $menu = std::get_if<std::weak_ptr<menu_widget>>(&$parent)) {
    auto m = $menu->lock();
    if (!m)
      return;

    if (new_index >= m->item_widgets.size())
      return;
    auto item = $item.lock();
    m->item_widgets.erase(
        std::remove(m->item_widgets.begin(), m->item_widgets.end(), item),
        m->item_widgets.end());

    m->item_widgets.insert(m->item_widgets.begin() + new_index, item);
    m->children_dirty = true;
  } else if (auto parent =
                 std::get_if<std::weak_ptr<menu_item_parent_widget>>(&$parent);
             auto m = parent->lock()) {
    if (new_index >= m->children.size())
      return;
    auto item = $item.lock();
    m->children.erase(std::remove(m->children.begin(), m->children.end(), item),
                      m->children.end());

    m->children.insert(m->children.begin() + new_index, item);
    m->children_dirty = true;
  }
}

static void to_menu_item(menu_item &data, const js_menu_data &js_data) {
  auto get_if_not_reset = [](auto &v) {
    return v.index() == 0 ? &std::get<0>(v) : nullptr;
  };

  if (js_data.type) {
    if (*js_data.type == "spacer") {
      data.type = menu_item::type::spacer;
    }
    if (*js_data.type == "button") {
      data.type = menu_item::type::button;
    }
  }

  if (js_data.name) {
    data.name = *js_data.name;
  }

  if (js_data.action) {
    if (auto action = get_if_not_reset(*js_data.action)) {
      data.action = [action = *action]() { action({}); };
    } else {
      data.action = {};
    }
  }

  if (js_data.submenu) {
    if (auto submenu = get_if_not_reset(*js_data.submenu)) {
      data.submenu = [submenu = *submenu](std::shared_ptr<menu_widget> mw) {
        try {
          submenu(
              std::make_shared<menu_controller>(mw->downcast<menu_widget>()));
        } catch (std::exception &e) {
          std::cerr << "Error in submenu: " << e.what() << std::endl;
        }
      };
    } else {
      data.submenu = {};
    }
  }

  if (js_data.icon_bitmap) {
    if (auto icon_bitmap = get_if_not_reset(*js_data.icon_bitmap)) {
      data.icon_bitmap = *icon_bitmap;
    } else {
      data.icon_bitmap = {};
    }

    data.icon_updated = true;
  }

  if (js_data.icon_svg) {
    if (auto icon_svg = get_if_not_reset(*js_data.icon_svg)) {
      data.icon_svg = *icon_svg;
    } else {
      data.icon_svg = {};
    }

    data.icon_updated = true;
  }

  if (js_data.disabled.has_value()) {
    data.disabled = js_data.disabled.value();
  }
}

void menu_item_controller::set_data(js_menu_data data) {
  if (!valid())
    return;

  auto item = $item.lock();

  to_menu_item(item->item, data);
  if (auto menu = std::get_if<std::weak_ptr<menu_widget>>(&$parent))
    if (auto m = menu->lock()) {
      m->update_icon_width();
    }
}
void menu_item_controller::remove() {
  if (!valid())
    return;
  auto item = $item.lock();

  if (auto $menu = std::get_if<std::weak_ptr<menu_widget>>(&$parent);
      auto m = $menu->lock()) {
    m->item_widgets.erase(
        std::remove(m->item_widgets.begin(), m->item_widgets.end(), item),
        m->item_widgets.end());

    m->children_dirty = true;
  } else if (auto parent =
                 std::get_if<std::weak_ptr<menu_item_parent_widget>>(&$parent);
             auto m = parent->lock()) {
    m->children.erase(std::remove(m->children.begin(), m->children.end(), item),
                      m->children.end());
    m->children_dirty = true;
  }
}
bool menu_item_controller::valid() {
  if (auto a = std::get_if<0>(&$parent); a && a->expired())
    return false;
  else if (auto a = std::get_if<1>(&$parent); a && a->expired())
    return false;

  return !$item.expired();
}
js_menu_data menu_item_controller::data() {
  if (!valid())
    return {};

  auto item = $item.lock();
  if (!item)
    return {};

  auto data = js_menu_data{};

  if (item->item.type == menu_item::type::spacer) {
    data.type = "spacer";
  } else {
    data.type = "button";
  }

  if (item->item.name) {
    data.name = *item->item.name;
  }

  if (item->item.action) {
    data.action = [action = item->item.action.value()](
                      js_menu_action_event_data) { action(); };
  }

  if (item->item.submenu) {
    data.submenu = [submenu = item->item.submenu.value()](
                       std::shared_ptr<menu_controller> ctl) {
      submenu(ctl->$menu.lock());
    };
  }

  if (item->item.icon_bitmap) {
    data.icon_bitmap = item->item.icon_bitmap.value();
  }

  if (item->item.icon_svg) {
    data.icon_svg = item->item.icon_svg.value();
  }

  data.wID = item->item.wID;
  data.name_resid = item->item.name_resid;

  data.disabled = item->item.disabled;

  return data;
}
std::shared_ptr<menu_item_controller> menu_controller::get_item(int index) {
  if (!valid())
    return nullptr;

  auto m = $menu.lock();
  if (!m)
    return nullptr;

  if (index >= m->item_widgets.size())
    return nullptr;

  auto item = m->item_widgets[index]->downcast<menu_item_widget>();

  auto controller = std::make_shared<menu_item_controller>();
  controller->$item = item;
  controller->$parent = m;

  return controller;
}
std::vector<std::shared_ptr<menu_item_controller>>
menu_controller::get_items() {
  if (!valid())
    return {};
  auto m = $menu.lock();
  if (!m)
    return {};

  std::vector<std::shared_ptr<menu_item_controller>> items;

  for (int i = 0; i < m->item_widgets.size(); i++) {
    auto item = m->item_widgets[i]->downcast<menu_item_widget>();

    auto controller = std::make_shared<menu_item_controller>();
    controller->$item = item;
    controller->$parent = m;

    items.push_back(controller);
  }

  return items;
}
void menu_controller::close() {
  auto menu = $menu.lock();
  if (!menu)
    return;

  menu->close();
}
std::string clipboard::get_text() {
  if (!OpenClipboard(nullptr))
    return "";

  HANDLE hData = GetClipboardData(CF_TEXT);
  if (hData == nullptr) {
    CloseClipboard();
    return "";
  }

  char *pszText = static_cast<char *>(GlobalLock(hData));
  std::string text(pszText);

  GlobalUnlock(hData);
  CloseClipboard();
  return text;
}

void clipboard::set_text(std::string text) {
  if (!OpenClipboard(nullptr))
    return;

  EmptyClipboard();
  HGLOBAL hData = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
  if (hData == nullptr) {
    CloseClipboard();
    return;
  }
  std::wstring wtext = utf8_to_wstring(text);
  HGLOBAL hDataW =
      GlobalAlloc(GMEM_MOVEABLE, (wtext.size() + 1) * sizeof(wchar_t));
  if (hDataW == nullptr) {
    CloseClipboard();
    return;
  }

  wchar_t *pszDataW = static_cast<wchar_t *>(GlobalLock(hDataW));
  wcscpy_s(pszDataW, wtext.size() + 1, wtext.c_str());
  GlobalUnlock(hDataW);
  SetClipboardData(CF_UNICODETEXT, hDataW);
  CloseClipboard();
}

std::string network::post(std::string url, std::string data) {
  HINTERNET hSession =
      WinHttpOpen(L"BreezeShell", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                  WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
  if (!hSession) {
    throw std::runtime_error("Failed to initialize WinHTTP");
  }

  URL_COMPONENTS urlComp = {sizeof(URL_COMPONENTS)};
  wchar_t hostName[256] = {0};
  wchar_t urlPath[1024] = {0};
  urlComp.lpszHostName = hostName;
  urlComp.dwHostNameLength = sizeof(hostName) / sizeof(wchar_t);
  urlComp.lpszUrlPath = urlPath;
  urlComp.dwUrlPathLength = sizeof(urlPath) / sizeof(wchar_t);

  std::wstring wideUrl = utf8_to_wstring(url);

  if (!WinHttpCrackUrl(wideUrl.c_str(), wideUrl.length(), 0, &urlComp)) {
    WinHttpCloseHandle(hSession);
    throw std::runtime_error("Invalid URL format");
  }

  HINTERNET hConnect = WinHttpConnect(hSession, hostName,
                                      urlComp.nScheme == INTERNET_SCHEME_HTTPS
                                          ? INTERNET_DEFAULT_HTTPS_PORT
                                          : INTERNET_DEFAULT_HTTP_PORT,
                                      0);
  if (!hConnect) {
    WinHttpCloseHandle(hSession);
    throw std::runtime_error("Failed to connect to server");
  }

  DWORD flags = WINHTTP_FLAG_REFRESH;
  if (urlComp.nScheme == INTERNET_SCHEME_HTTPS) {
    flags |= WINHTTP_FLAG_SECURE;
  }

  HINTERNET hRequest = WinHttpOpenRequest(
      hConnect, data.empty() ? L"GET" : L"POST", urlPath, nullptr,
      WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
  if (!hRequest) {
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    throw std::runtime_error("Failed to create request");
  }

  BOOL result = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                   data.empty() ? WINHTTP_NO_REQUEST_DATA
                                                : (LPVOID)data.c_str(),
                                   data.length(), data.length(), 0);

  if (!result || !WinHttpReceiveResponse(hRequest, nullptr)) {
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    throw std::runtime_error("Failed to send/receive request");
  }

  DWORD statusCode = 0;
  DWORD statusCodeSize = sizeof(statusCode);
  WinHttpQueryHeaders(hRequest,
                      WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                      WINHTTP_HEADER_NAME_BY_INDEX, &statusCode,
                      &statusCodeSize, WINHTTP_NO_HEADER_INDEX);

  if (statusCode >= 400) {
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    throw std::runtime_error("Server returned error: " +
                             std::to_string(statusCode));
  }

  std::string response;
  DWORD bytesAvailable;
  do {
    bytesAvailable = 0;
    if (!WinHttpQueryDataAvailable(hRequest, &bytesAvailable))
      break;
    if (!bytesAvailable)
      break;

    std::vector<char> buffer(bytesAvailable);
    DWORD bytesRead = 0;
    if (WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead)) {
      response.append(buffer.data(), bytesRead);
    }
  } while (bytesAvailable > 0);

  WinHttpCloseHandle(hRequest);
  WinHttpCloseHandle(hConnect);
  WinHttpCloseHandle(hSession);

  return response;
}

std::string network::get(std::string url) { return post(url, ""); }

void network::get_async(std::string url,
                        std::function<void(std::string)> callback,
                        std::function<void(std::string)> error_callback) {
  std::thread([url, callback, error_callback,
               &lock = menu_render::current.value()->rt->rt_lock]() {
    try {
      auto res = get(url);
      std::lock_guard l(lock);
      callback(res);
    } catch (std::exception &e) {
      std::cerr << "Error in network::get_async: " << e.what() << std::endl;
      error_callback(e.what());
    }
  }).detach();
}

void network::post_async(std::string url, std::string data,
                         std::function<void(std::string)> callback,
                         std::function<void(std::string)> error_callback) {
  std::thread([url, data, callback, error_callback,
               &lock = menu_render::current.value()->rt->rt_lock]() {
    try {
      auto res = post(url, data);
      std::lock_guard l(lock);
      callback(res);
    } catch (std::exception &e) {
      std::cerr << "Error in network::post_async: " << e.what() << std::endl;
      error_callback(e.what());
    }
  }).detach();
}
subproc_result_data subproc::run(std::string cmd) {
  subproc_result_data result;
  SECURITY_ATTRIBUTES sa;
  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.bInheritHandle = TRUE;
  sa.lpSecurityDescriptor = nullptr;

  HANDLE hReadPipe, hWritePipe;
  if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0))
    return result;

  STARTUPINFOW si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  si.hStdError = hWritePipe;
  si.hStdOutput = hWritePipe;
  si.dwFlags |= STARTF_USESTDHANDLES;

  if (!CreateProcessW(nullptr, utf8_to_wstring(cmd).data(), nullptr, nullptr,
                      TRUE, 0, nullptr, nullptr, &si, &pi)) {
    CloseHandle(hReadPipe);
    CloseHandle(hWritePipe);
    return result;
  }

  CloseHandle(hWritePipe);

  char buffer[4096];
  DWORD bytesRead;
  std::string out;
  while (ReadFile(hReadPipe, buffer, sizeof(buffer), &bytesRead, nullptr) &&
         bytesRead > 0) {
    out.append(buffer, bytesRead);
  }

  CloseHandle(hReadPipe);

  DWORD exitCode;
  GetExitCodeProcess(pi.hProcess, &exitCode);

  result.out = out;
  result.code = exitCode;

  return result;
}
void subproc::run_async(std::string cmd,
                        std::function<void(subproc_result_data)> callback) {
  std::thread([cmd, callback,
               &lock = menu_render::current.value()->rt->rt_lock]() {
    try {
      auto res = run(cmd);
      std::lock_guard l(lock);
      callback(res);
    } catch (std::exception &e) {
      std::cerr << "Error in subproc::run_async: " << e.what() << std::endl;
    }
  }).detach();
}
void menu_controller::clear() {
  if (!valid())
    return;
  auto m = $menu.lock();
  if (!m)
    return;

  m->children_dirty = true;
  m->item_widgets.clear();
  m->menu_data.items.clear();
}

void fs::chdir(std::string path) { std::filesystem::current_path(path); }

std::string fs::cwd() { return std::filesystem::current_path().string(); }

bool fs::exists(std::string path) { return std::filesystem::exists(path); }

bool fs::isdir(std::string path) { return std::filesystem::is_directory(path); }

void fs::mkdir(std::string path) {
  try {
    std::filesystem::create_directories(path);
  } catch (std::exception &e) {
    std::filesystem::create_directory(path);
  }
}

void fs::rmdir(std::string path) { std::filesystem::remove(path); }

void fs::rename(std::string old_path, std::string new_path) {
  std::filesystem::rename(old_path, new_path);
}

void fs::remove(std::string path) { std::filesystem::remove(path); }

void fs::copy(std::string src_path, std::string dest_path) {
  std::filesystem::copy_file(src_path, dest_path,
                             std::filesystem::copy_options::overwrite_existing);
}

void fs::move(std::string src_path, std::string dest_path) {
  std::filesystem::rename(src_path, dest_path);
}

std::string fs::read(std::string path) {
  std::ifstream file(path, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

void fs::write(std::string path, std::string data) {
  std::ofstream file(path, std::ios::binary);
  file.write(data.c_str(), data.length());
}

std::vector<uint8_t> fs::read_binary(std::string path) {
  std::ifstream file(path, std::ios::binary);
  return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>());
}

void fs::write_binary(std::string path, std::vector<uint8_t> data) {
  std::ofstream file(path, std::ios::binary);
  file.write(reinterpret_cast<const char *>(data.data()), data.size());
}
std::string breeze::version() { return BREEZE_VERSION; }
std::string breeze::data_directory() {
  return config::data_directory().generic_string();
}
std::vector<std::string> fs::readdir(std::string path) {
  std::vector<std::string> result;
  std::ranges::copy(std::filesystem::directory_iterator(path) |
                        std::ranges::views::transform([](const auto &entry) {
                          return entry.path().generic_string();
                        }),
                    std::back_inserter(result));
  return result;
}
bool breeze::is_light_theme() { return is_light_mode(); }
void network::download_async(std::string url, std::string path,
                             std::function<void()> callback,
                             std::function<void(std::string)> error_callback) {

  std::thread([url, path, callback, error_callback,
               &lock = menu_render::current.value()->rt->rt_lock]() {
    try {
      auto data = get(url);
      fs::write_binary(path, std::vector<uint8_t>(data.begin(), data.end()));
      std::lock_guard l(lock);
      callback();
    } catch (std::exception &e) {
      error_callback(e.what());
    }
  }).detach();
}
std::string win32::resid_from_string(std::string str) {
  return res_string_loader::string_to_id_string(utf8_to_wstring(str));
}
size_t win32::load_library(std::string path) {
  return reinterpret_cast<size_t>(LoadLibraryW(utf8_to_wstring(path).c_str()));
}
std::string breeze::user_language() {
  wchar_t lang[10];
  GetUserDefaultLocaleName(lang, 10);
  return wstring_to_utf8(lang);
}
std::optional<std::string> win32::env(std::string name) {
  return mb_shell::env(name);
}

std::string breeze::hash() { return BREEZE_GIT_COMMIT_HASH; }
std::string breeze::branch() { return BREEZE_GIT_BRANCH_NAME; }
std::string breeze::build_date() { return BREEZE_BUILD_DATE_TIME; }
std::vector<std::shared_ptr<mb_shell::js::menu_item_controller>>
menu_item_parent_item_controller::children() {
  if (!valid())
    return {};

  auto item = $item.lock();
  if (!item)
    return {};

  std::vector<std::shared_ptr<mb_shell::js::menu_item_controller>> items;

  for (int i = 0; i < item->children.size(); i++) {
    auto subitem = item->children[i]->downcast<menu_item_widget>();

    auto controller = std::make_shared<menu_item_controller>();
    controller->$item = subitem;
    controller->$parent = item;

    items.push_back(controller);
  }

  return items;
}
void menu_item_parent_item_controller::set_position(int new_index) {
  if (!valid())
    return;

  auto item = $item.lock();
  if (!item)
    return;

  auto parent = item->parent->downcast<menu_widget>();

  if (new_index >= parent->item_widgets.size())
    return;

  parent->item_widgets.erase(std::remove(parent->item_widgets.begin(),
                                         parent->item_widgets.end(), item),
                             parent->item_widgets.end());

  parent->item_widgets.insert(parent->item_widgets.begin() + new_index, item);
  parent->children_dirty = true;
  parent->update_icon_width();
}
void menu_item_parent_item_controller::remove() {
  if (!valid())
    return;

  auto item = $item.lock();
  if (!item)
    return;

  auto parent = item->parent->downcast<menu_widget>();
  parent->children_dirty = true;
  parent->item_widgets.erase(std::remove(parent->item_widgets.begin(),
                                         parent->item_widgets.end(), item),
                             parent->item_widgets.end());
}
bool menu_item_parent_item_controller::valid() {
  return !$item.expired() && !$menu.expired();
}

std::shared_ptr<mb_shell::js::menu_item_parent_item_controller>
menu_controller::append_parent_item_after(int after_index) {
  if (!valid())
    return nullptr;
  auto m = $menu.lock();
  if (!m)
    return nullptr;
  m->children_dirty = true;

  auto new_item = std::make_shared<menu_item_parent_widget>();
  auto ctl = std::make_shared<menu_item_parent_item_controller>(new_item, m);
  new_item->parent = m.get();

  while (after_index < 0) {
    after_index = m->item_widgets.size() + after_index + 1;
  }

  if (after_index >= m->item_widgets.size()) {
    m->item_widgets.push_back(new_item);
  } else {
    m->item_widgets.insert(m->item_widgets.begin() + after_index, new_item);
  }

  m->update_icon_width();

  if (m->animate_appear_started) {
    new_item->reset_appear_animation(0);
  }

  return ctl;
}
std::shared_ptr<mb_shell::js::menu_item_controller>
menu_item_parent_item_controller::append_child_after(
    mb_shell::js::js_menu_data data, int after_index) {
  if (!valid())
    return nullptr;

  auto parent = $item.lock();
  if (!parent)
    return nullptr;

  menu_item item;
  auto new_item = std::make_shared<menu_item_normal_widget>(item);
  auto ctl = std::make_shared<menu_item_controller>(
      menu_item_controller{new_item->downcast<menu_item_widget>(), parent});
  new_item->parent = parent.get();
  ctl->set_data(data);

  while (after_index < 0) {
    after_index = parent->children.size() + after_index + 1;
  }

  if (after_index >= parent->children.size()) {
    parent->children.push_back(new_item);
  } else {
    parent->children.insert(parent->children.begin() + after_index, new_item);
  }

  if (parent->parent->downcast<menu_widget>()->animate_appear_started) {
    new_item->reset_appear_animation(0);
  }

  return ctl;
}
void subproc::open(std::string path, std::string args) {
  std::wstring wpath = utf8_to_wstring(path);
  std::wstring wargs = utf8_to_wstring(args);
  ShellExecuteW(nullptr, L"open", wpath.c_str(), wargs.c_str(), nullptr,
                SW_SHOWNORMAL);
}
void subproc::open_async(std::string path, std::string args,
                         std::function<void()> callback) {
  std::thread([path, callback, args,
               &lock = menu_render::current.value()->rt->rt_lock]() {
    try {
      open(path, args);
      std::lock_guard l(lock);
      callback();
    } catch (std::exception &e) {
      std::cerr << "Error in subproc::open_async: " << e.what() << std::endl;
    }
  }).detach();
}

struct Timer {
  std::function<void()> callback;
  std::weak_ptr<qjs::Context> ctx;
  int delay;
  int elapsed = 0;
  bool repeat;
  int id;
};

std::vector<std::shared_ptr<Timer>> timers;
std::optional<std::thread> timer_thread;

void timer_thread_func() {
  while (true) {
    constexpr auto sleep_time = 30;
    Sleep(sleep_time);
    std::vector<std::shared_ptr<Timer>> to_remove;

    for (auto &timer : timers) {
      if (timer->ctx.expired()) {
        to_remove.push_back(timer);
        continue;
      }
      timer->elapsed += sleep_time;
      if (timer->elapsed >= timer->delay) {
        try {
          timer->callback();
          if (!timer->repeat) {
            to_remove.push_back(timer);
          } else {
            timer->elapsed = 0;
          }
        } catch (std::exception &e) {
          std::cerr << "Error in timer callback: " << e.what() << std::endl;
          to_remove.push_back(timer);
        }
      }
    }

    for (auto &timer : to_remove) {
      timers.erase(std::remove(timers.begin(), timers.end(), timer),
                   timers.end());
    }
  }
}

void ensure_timer_thread() {
  if (!timer_thread) {
    timer_thread = std::thread(timer_thread_func);
  }
}

int infra::setTimeout(std::function<void()> callback, int delay) {
  ensure_timer_thread();

  auto timer = std::make_shared<Timer>();
  timer->callback = callback;
  timer->delay = delay;
  timer->repeat = false;
  timer->ctx = qjs::Context::current->weak_from_this();
  timer->id = timers.size() + 1;
  timers.push_back(timer);

  return timer->id;
};
void infra::clearTimeout(int id) {
  timers.erase(
      std::remove_if(timers.begin(), timers.end(),
                     [id](const auto &timer) { return timer->id == id; }),
      timers.end());
};
int infra::setInterval(std::function<void()> callback, int delay) {
  ensure_timer_thread();

  auto timer = std::make_shared<Timer>();
  timer->callback = callback;
  timer->delay = delay;
  timer->repeat = true;
  timer->ctx = qjs::Context::current->weak_from_this();
  timer->id = timers.size() + 1;
  timers.push_back(timer);

  return timer->id;
};
void infra::clearInterval(int id) {
  timers.erase(
      std::remove_if(timers.begin(), timers.end(),
                     [id](const auto &timer) { return timer->id == id; }),
      timers.end());
};
std::string infra::atob(std::string base64) {
  std::string result;
  result.reserve(base64.length() * 3 / 4);

  const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz"
                                   "0123456789+/";

  int val = 0, valb = -8;
  for (unsigned char c : base64) {
    if (c == '=')
      break;

    if (isspace(c))
      continue;

    val = (val << 6) + base64_chars.find(c);
    valb += 6;
    if (valb >= 0) {
      result.push_back(char((val >> valb) & 0xFF));
      valb -= 8;
    }
  }
  return result;
}

std::string infra::btoa(std::string str) {
  const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz"
                                   "0123456789+/";

  std::string result;
  int val = 0, valb = -6;
  for (unsigned char c : str) {
    val = (val << 8) + c;
    valb += 8;
    while (valb >= 0) {
      result.push_back(base64_chars[(val >> valb) & 0x3F]);
      valb -= 6;
    }
  }
  if (valb > -6) {
    result.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
  }
  while (result.size() % 4) {
    result.push_back('=');
  }
  return result;
}

void fs::copy_shfile(std::string src_path, std::string dest_path,
                      std::function<void(bool, std::string)> callback) {
   std::thread([=, &lock = menu_render::current.value()->rt->rt_lock] {
     SHFILEOPSTRUCTW FileOp = {GetForegroundWindow()};
     std::wstring wsrc = utf8_to_wstring(src_path);
     std::wstring wdest = utf8_to_wstring(dest_path);
 
     std::vector<wchar_t> from_buf(wsrc.size() + 2, 0);
     std::vector<wchar_t> to_buf(wdest.size() + 2, 0);
     wcsncpy_s(from_buf.data(), from_buf.size(), wsrc.c_str(), _TRUNCATE);
     wcsncpy_s(to_buf.data(), to_buf.size(), wdest.c_str(), _TRUNCATE);
 
     FileOp.wFunc = FO_COPY;
     FileOp.pFrom = from_buf.data();
     FileOp.pTo = to_buf.data();
     FileOp.fFlags = FOF_RENAMEONCOLLISION | FOF_ALLOWUNDO | FOF_NOCONFIRMMKDIR | FOF_NOCOPYSECURITYATTRIBS | FOF_WANTMAPPINGHANDLE;
 
     auto res = SHFileOperationW(&FileOp);
     std::wstring final_path;
     bool success = (res == 0) && !FileOp.fAnyOperationsAborted;
     if (success) {
       if (FileOp.hNameMappings) {
        struct file_operation_collision_mapping
        {
            int index;
            SHNAMEMAPPINGW* mapping;
        };
    
        file_operation_collision_mapping* mapping = static_cast<file_operation_collision_mapping*>(FileOp.hNameMappings);
        SHNAMEMAPPINGW* map = &mapping->mapping[0];
        final_path = map->pszNewPath;
    
        SHFreeNameMappings(FileOp.hNameMappings);
       } else {
         std::filesystem::path dest(wdest);
         std::filesystem::path src(wsrc);
         final_path = (dest / src.filename()).wstring();
       }

       SHChangeNotify(SHCNE_CREATE, SHCNF_PATH | SHCNF_FLUSH, final_path.c_str(), nullptr);
     }
 
     std::string utf8_path = wstring_to_utf8(final_path);
     std::lock_guard l(lock);
     callback(success, utf8_path);
   }).detach();
 }

void fs::move_shfile(std::string src_path, std::string dest_path,
                     std::function<void(bool)> callback) {
  std::thread([=, &lock = menu_render::current.value()->rt->rt_lock] {
    SHFILEOPSTRUCTW FileOp = {GetForegroundWindow()};
    std::wstring wsrc = utf8_to_wstring(src_path);
    std::wstring wdest = utf8_to_wstring(dest_path);

    FileOp.wFunc = FO_MOVE;
    FileOp.pFrom = wsrc.c_str();
    FileOp.pTo = wdest.c_str();

    auto res = SHFileOperationW(&FileOp);
    std::lock_guard l(lock);
    callback(res == 0);
  }).detach();
}
size_t win32::load_file_icon(std::string path) {
  SHFILEINFOW sfi = {0};
  DWORD_PTR ret =
      SHGetFileInfoW(utf8_to_wstring(path).c_str(), 0, &sfi,
                     sizeof(SHFILEINFOW), SHGFI_ICON | SHGFI_SMALLICON);
  if (ret == 0)
    return 0;

  ICONINFO iconinfo;
  GetIconInfo(sfi.hIcon, &iconinfo);

  return (size_t)iconinfo.hbmColor;
}
} // namespace mb_shell::js
