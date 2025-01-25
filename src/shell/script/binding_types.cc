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
menu_controller::add_menu_item_after(js_menu_data data, int after_index) {
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

  if (after_index >= m->children.size()) {
    m->children.push_back(new_item);
  } else {
    m->children.insert(m->children.begin() + after_index, new_item);
  }

  $update_icon_width();

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
    data.submenu = [js_data]() {
      auto items = std::vector<menu_item>{};
      for (auto &i : js_data.submenu.value()) {
        auto item = menu_item{};
        to_menu_item(item, i);
        items.push_back(item);
      }

      return menu{.items = items};
    };
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
  menu_controller{m}.$update_icon_width();
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
js_menu_data menu_item_controller::get_data() {
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

  return data;
}
std::shared_ptr<menu_item_controller>
menu_controller::get_menu_item(int index) {
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
menu_controller::get_menu_items() {
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
void menu_controller::$update_icon_width() {
  if (!valid())
    return;

  auto m = $menu.lock();

  bool has_icon = std::ranges::any_of(m->children, [](auto &item) {
    auto i = item->template downcast<menu_item_widget>()->item;
    return i.icon_bitmap.has_value() || i.type == menu_item::type::toggle;
  });

  for (auto &item : m->children) {
    auto mi = item->template downcast<menu_item_widget>();
    if (!has_icon) {
      mi->icon_width = 0;
    } else {
      mi->icon_width = 16;
    }
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
} // namespace mb_shell::js
