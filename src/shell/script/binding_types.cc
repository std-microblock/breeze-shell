#include "binding_types.h"
#include "quickjspp.hpp"
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <ranges>
#include <regex>

// Context menu
#include "../contextmenu/menu_render.h"
#include "../contextmenu/menu_widget.h"

#include "winhttp.h"

std::unordered_set<
    std::shared_ptr<std::function<void(mb_shell::js::menu_info_basic_js)>>>
    mb_shell::menu_callbacks_js;
namespace mb_shell::js {

bool menu_controller::valid() { return !$menu.expired(); }
std::shared_ptr<mb_shell::js::menu_item_controller>
menu_controller::append_menu_after(js_menu_data data, int after_index) {
  if (!valid())
    return nullptr;
  auto m = $menu.lock();
  if (!m)
    return nullptr;

  std::unique_lock lock(m->data_lock, std::defer_lock);
  std::ignore = lock.try_lock();

  menu_item item;
  auto new_item = std::make_shared<menu_item_widget>(item, m.get());
  auto ctl = std::make_shared<menu_item_controller>(new_item, m);

  ctl->set_data(data);

  while (after_index < 0) {
    after_index = m->children.size() + after_index + 1;
  }

  if (after_index >= m->children.size()) {
    m->children.push_back(new_item);
  } else {
    m->children.insert(m->children.begin() + after_index, new_item);
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

  auto m = $menu.lock();
  if (!m)
    return;

  std::unique_lock lock(m->data_lock, std::defer_lock);
  std::ignore = lock.try_lock();

  if (new_index >= m->children.size())
    return;

  auto item = $item.lock();

  m->children.erase(std::remove(m->children.begin(), m->children.end(), item),
                    m->children.end());

  m->children.insert(m->children.begin() + new_index, item);
}

static void to_menu_item(menu_item &data, const js_menu_data &js_data) {
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
    data.action = [js_data]() { js_data.action.value()({}); };
  }

  if (js_data.submenu) {
    data.submenu = [js_data](std::shared_ptr<menu_widget> mw) {
      try {
        js_data.submenu.value()(
            std::make_shared<menu_controller>(mw->downcast<menu_widget>()));
      } catch (std::exception &e) {
        std::cerr << "Error in submenu: " << e.what() << std::endl;
      }
    };
  }

  if (js_data.icon_bitmap) {
    data.icon_bitmap = js_data.icon_bitmap.value();
    data.icon_updated = true;
  }

  if (js_data.icon_svg) {
    data.icon_svg = js_data.icon_svg.value();
    data.icon_updated = true;
  }

  if (js_data.disabled.has_value()) {
    data.disabled = js_data.disabled.value();
  }
}

void menu_item_controller::set_data(js_menu_data data) {
  if (!valid())
    return;

  auto m = $menu.lock();
  if (!m)
    return;

  std::unique_lock lock(m->data_lock, std::defer_lock);
  std::ignore = lock.try_lock();

  auto item = $item.lock();

  to_menu_item(item->item, data);
  m->update_icon_width();
}
void menu_item_controller::remove() {
  if (!valid())
    return;

  auto m = $menu.lock();
  if (!m)
    return;

  std::unique_lock lock(m->data_lock, std::defer_lock);
  std::ignore = lock.try_lock();

  auto item = $item.lock();

  m->children.erase(std::remove(m->children.begin(), m->children.end(), item),
                    m->children.end());
}
bool menu_item_controller::valid() {
  return !$item.expired() && !$menu.expired();
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

  if (item->item.wID) {
    data.wID = item->item.wID.value();
  }

  data.disabled = item->item.disabled;

  return data;
}
std::shared_ptr<menu_item_controller> menu_controller::get_item(int index) {
  if (!valid())
    return nullptr;

  auto m = $menu.lock();
  if (!m)
    return nullptr;

  std::unique_lock lock(m->data_lock, std::defer_lock);
  std::ignore = lock.try_lock();

  if (index >= m->children.size())
    return nullptr;

  auto item = m->children[index]->downcast<menu_item_widget>();

  auto controller = std::make_shared<menu_item_controller>();
  controller->$item = item;
  controller->$menu = m;

  return controller;
}
std::vector<std::shared_ptr<menu_item_controller>>
menu_controller::get_items() {
  if (!valid())
    return {};
  auto m = $menu.lock();
  if (!m)
    return {};

  std::unique_lock lock(m->data_lock, std::defer_lock);
  std::ignore = lock.try_lock();

  std::vector<std::shared_ptr<menu_item_controller>> items;

  for (int i = 0; i < m->children.size(); i++) {
    auto item = m->children[i]->downcast<menu_item_widget>();

    auto controller = std::make_shared<menu_item_controller>();
    controller->$item = item;
    controller->$menu = m;

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
  std::thread([url, callback, error_callback]() {
    try {
      callback(get(url));
    } catch (std::exception &e) {
      std::cerr << "Error in network::get_async: " << e.what() << std::endl;
      error_callback(e.what());
    }
  }).detach();
}

void network::post_async(std::string url, std::string data,
                         std::function<void(std::string)> callback,
                         std::function<void(std::string)> error_callback) {
  std::thread([url, data, callback, error_callback]() {
    try {
      callback(post(url, data));
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

  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  si.hStdError = hWritePipe;
  si.hStdOutput = hWritePipe;
  si.dwFlags |= STARTF_USESTDHANDLES;

  if (!CreateProcessA(nullptr, (LPSTR)cmd.c_str(), nullptr, nullptr, TRUE, 0,
                      nullptr, nullptr, &si, &pi)) {
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
  std::thread([cmd, callback]() {
    try {
      callback(run(cmd));
    } catch (std::exception &e) {
      std::cerr << "Error in subproc::run_async: " << e.what() << std::endl;
    }
  }).detach();
}
std::shared_ptr<mb_shell::js::menu_item_controller>
menu_controller::prepend_menu(mb_shell::js::js_menu_data data) {
  return append_menu_after(data, 0);
}
std::shared_ptr<mb_shell::js::menu_item_controller>
menu_controller::append_menu(mb_shell::js::js_menu_data data) {
  return append_menu_after(data, -1);
}
void menu_controller::clear() {
  if (!valid())
    return;
  auto m = $menu.lock();
  if (!m)
    return;

  std::unique_lock lock(m->data_lock, std::defer_lock);
  std::ignore = lock.try_lock();

  m->children.clear();
  m->menu_data.items.clear();
}

void fs::chdir(std::string path) { std::filesystem::current_path(path); }

std::string fs::cwd() { return std::filesystem::current_path().string(); }

bool fs::exists(std::string path) { return std::filesystem::exists(path); }

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
std::string breeze::version() { return "0.1.0"; }
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
} // namespace mb_shell::js
