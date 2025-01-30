#include "binding_types.h"
#include "quickjspp.hpp"
#include <iostream>
#include <memory>
#include <mutex>

// Context menu
#include "../contextmenu/menu_render.h"
#include "../contextmenu/menu_widget.h"

#include "wininet.h"

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

  if(m->animate_appear_started) {
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

  if (js_data.disabled) {
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
    data.action = [item](js_menu_action_event_data) {
      item->item.action.value()();
    };
  }

  if (item->item.submenu) {
    data.submenu = [item](std::shared_ptr<menu_controller> ctl) {
      item->item.submenu.value()(ctl->$menu.lock());
    };
  }

  if (item->item.icon_bitmap) {
    data.icon_bitmap = item->item.icon_bitmap.value();
  }

  if (item->item.icon_svg) {
    data.icon_svg = item->item.icon_svg.value();
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
  auto current = menu_render::current;
  if (current) {
    (*current)->rt->close();
  }
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

  char *pszData = static_cast<char *>(GlobalLock(hData));
  strcpy_s(pszData, text.size() + 1, text.c_str());
  GlobalUnlock(hData);

  SetClipboardData(CF_TEXT, hData);
  CloseClipboard();
}

std::string network::post(std::string url, std::string data) {
  HINTERNET hInternet =
      InternetOpenA("HTTP", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
  if (!hInternet)
    return "";

  HINTERNET hConnect =
      InternetOpenUrlA(hInternet, url.c_str(), data.c_str(), data.length(),
                       INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD, 0);
  if (!hConnect) {
    InternetCloseHandle(hInternet);
    return "";
  }

  std::string response;
  char buffer[1024];
  DWORD bytesRead;
  while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) &&
         bytesRead > 0) {
    response.append(buffer, bytesRead);
  }

  InternetCloseHandle(hConnect);
  InternetCloseHandle(hInternet);
  return response;
}

std::string network::get(std::string url) { return post(url, ""); }

void network::get_async(std::string url,
                        std::function<void(std::string)> callback) {
  std::thread([url, callback]() {
    try {
      callback(get(url));
    } catch (std::exception &e) {
      std::cerr << "Error in network::get_async: " << e.what() << std::endl;
    }
  }).detach();
}

void network::post_async(std::string url, std::string data,
                         std::function<void(std::string)> callback) {
  std::thread([url, data, callback]() {
    try {
      callback(post(url, data));
    } catch (std::exception &e) {
      std::cerr << "Error in network::post_async: " << e.what() << std::endl;
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
void fs::chdir(std::string path) {
  SetCurrentDirectoryA(path.c_str());
}

std::string fs::cwd() {
  char buffer[MAX_PATH];
  GetCurrentDirectoryA(MAX_PATH, buffer);
  return std::string(buffer);
}

bool fs::exists(std::string path) {
  return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

void fs::mkdir(std::string path) {
  CreateDirectoryA(path.c_str(), nullptr);
}

void fs::rmdir(std::string path) {
  RemoveDirectoryA(path.c_str());
}

void fs::rename(std::string old_path, std::string new_path) {
  MoveFileA(old_path.c_str(), new_path.c_str());
}

void fs::remove(std::string path) {
  DeleteFileA(path.c_str());
}

void fs::copy(std::string src_path, std::string dest_path) {
  CopyFileA(src_path.c_str(), dest_path.c_str(), FALSE);
}

void fs::move(std::string src_path, std::string dest_path) {
  MoveFileA(src_path.c_str(), dest_path.c_str());
}

std::string fs::read(std::string path) {
  HANDLE hFile = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (hFile == INVALID_HANDLE_VALUE) return "";

  DWORD fileSize = GetFileSize(hFile, nullptr);
  std::string buffer(fileSize, '\0');
  DWORD bytesRead;
  ReadFile(hFile, &buffer[0], fileSize, &bytesRead, nullptr);
  CloseHandle(hFile);
  return buffer;
}

void fs::write(std::string path, std::string data) {
  HANDLE hFile = CreateFileA(path.c_str(), GENERIC_WRITE, 0, nullptr,
               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (hFile == INVALID_HANDLE_VALUE) return;

  DWORD bytesWritten;
  WriteFile(hFile, data.c_str(), data.length(), &bytesWritten, nullptr);
  CloseHandle(hFile);
}

std::vector<uint8_t> fs::read_binary(std::string path) {
  HANDLE hFile = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (hFile == INVALID_HANDLE_VALUE) return {};

  DWORD fileSize = GetFileSize(hFile, nullptr);
  std::vector<uint8_t> buffer(fileSize);
  DWORD bytesRead;
  ReadFile(hFile, buffer.data(), fileSize, &bytesRead, nullptr);
  CloseHandle(hFile);
  return buffer;
}

void fs::write_binary(std::string path, std::vector<uint8_t> data) {
  HANDLE hFile = CreateFileA(path.c_str(), GENERIC_WRITE, 0, nullptr,
               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (hFile == INVALID_HANDLE_VALUE) return;

  DWORD bytesWritten;
  WriteFile(hFile, data.data(), data.size(), &bytesWritten, nullptr);
  CloseHandle(hFile);
}
std::string breeze::version() {
  return "0.1.0";
}
std::string breeze::data_directory() {
  return config::data_directory().generic_string();
}
} // namespace mb_shell::js
