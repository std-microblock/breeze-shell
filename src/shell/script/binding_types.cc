#include "binding_types.h"
#include "../contextmenu/menu_widget.h"
#include "quickjspp.hpp"
#include <iostream>
#include <mutex>
namespace mb_shell {
std::unordered_set<std::shared_ptr<std::function<void(menu_info_basic)>>>
    menu_callbacks;
bool js::menu_controller::valid() { return !$menu.expired(); }
bool js::menu_controller::add_menu_item_after(js_menu_data data,
                                              int after_index) {
  if (!valid())
    return false;
  auto m = $menu.lock();
  if (!m)
    return false;

  std::unique_lock lock(m->data_lock, std::defer_lock);
  std::ignore = lock.try_lock();

  mb_shell::menu_item item;

  if (data.type) {
    if (*data.type == "spacer") {
      item.type = mb_shell::menu_item::type::spacer;
    }
    if (*data.type == "button") {
      item.type = mb_shell::menu_item::type::button;
    }
  } else {
    item.type = mb_shell::menu_item::type::button;
  }

  if (data.name) {
    item.name = *data.name;
  }

  if (data.action) {
    item.action = [data]() { data.action.value()({}); };
  }

  auto new_item = std::make_shared<mb_shell::menu_item_widget>(item, m.get());

  if (after_index >= m->children.size()) {
    m->children.push_back(new_item);
  } else {
    m->children.insert(m->children.begin() + after_index, new_item);
  }

  return true;
}
bool js::menu_controller::set_menu_item(int index,
                                        mb_shell::js::js_menu_data data) {
  if (!valid())
    return false;
  auto m = $menu.lock();
  if (!m)
    return false;

  std::unique_lock lock(m->data_lock, std::defer_lock);
  std::ignore = lock.try_lock();

  if (index >= m->children.size())
    return false;

  auto item = m->children[index]->downcast<menu_item_widget>();

  if (data.type) {
    if (*data.type == "spacer") {
      item->item.type = mb_shell::menu_item::type::spacer;
    }
    if (*data.type == "button") {
      item->item.type = mb_shell::menu_item::type::button;
    }
  }

  if (data.name) {
    item->item.name = *data.name;
  }

  if (data.action) {
    item->item.action = [data]() { data.action.value()({}); };
  }

  return true;
}
bool js::menu_controller::set_menu_item_position(int index, int new_index) {
  if (!valid())
    return false;
  auto m = $menu.lock();
  if (!m)
    return false;

  std::unique_lock lock(m->data_lock, std::defer_lock);
  std::ignore = lock.try_lock();

  if (index >= m->children.size() || new_index >= m->children.size())
    return false;

  auto item = m->children[index];
  m->children.erase(m->children.begin() + index);
  m->children.insert(m->children.begin() + new_index, item);

  return true;
}
bool js::menu_controller::remove_menu_item(int index) {
  if (!valid())
    return false;
  auto m = $menu.lock();
  if (!m)
    return false;

  std::unique_lock lock(m->data_lock, std::defer_lock);
  std::ignore = lock.try_lock();

  if (index >= m->children.size())
    return false;

  m->children.erase(m->children.begin() + index);
  return true;
}
std::function<void()> js::menu_controller::add_menu_listener(
    std::function<void(mb_shell::js::menu_info_basic_js)> listener) {
  auto listener_cvt = [listener](mb_shell::menu_info_basic info) {
    try {
      menu_info_basic_js m{
          .from = info.from,
          .menu = std::make_shared<mb_shell::js::menu_controller>(info.menu->menu_wid)};
      listener(m);
    } catch (qjs::exception e) {
      auto js = &e.context();
      auto exc = js->getException();
      std::cerr << "JS Error: " << (std::string)exc << std::endl;
      if ((bool)exc["stack"])
        std::cerr << (std::string)exc["stack"] << std::endl;
    }
  };
  auto ptr =
      std::make_shared<std::function<void(menu_info_basic)>>(listener_cvt);
  menu_callbacks.insert(ptr);
    // $listeners_to_dispose.insert(ptr.get());
  return [ptr]() {
    menu_callbacks.erase(ptr);
    // $listeners_to_dispose.erase(ptr.get());
  };
}
js::menu_controller::~menu_controller() {
  //   for (auto &listener : $listeners_to_dispose) {
  //     for (auto it = menu_callbacks.begin(); it != menu_callbacks.end();
  //     ++it) {
  //       if (it->get() == listener) {
  //         menu_callbacks.erase(it);
  //         break;
  //       }
  //     }
  //   }
}
void js::menu_item_controller::set_position(int new_index) {
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
void js::menu_item_controller::set_data(mb_shell::js::js_menu_data data) {
  if (!valid())
    return;

  auto m = $menu.lock();
  if (!m)
    return;

  std::unique_lock lock(m->data_lock, std::defer_lock);
  std::ignore = lock.try_lock();

  auto item = $item.lock();

  if (data.type) {
    if (*data.type == "spacer") {
      item->item.type = mb_shell::menu_item::type::spacer;
    }
    if (*data.type == "button") {
      item->item.type = mb_shell::menu_item::type::button;
    }
  }

  if (data.name) {
    item->item.name = *data.name;
  }

  if (data.action) {
    item->item.action = [data]() { data.action.value()({}); };
  }
}
void js::menu_item_controller::remove() {
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
bool js::menu_item_controller::valid() {
  return !$item.expired() && !$menu.expired();
}
js::js_menu_data js::menu_item_controller::get_data() {
  if (!valid())
    return {};

  auto item = $item.lock();
  if (!item)
    return {};

  auto data = js_menu_data{};

  if (item->item.type == mb_shell::menu_item::type::spacer) {
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
std::shared_ptr<mb_shell::js::menu_item_controller>
js::menu_controller::get_menu_item(int index) {
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

  auto controller = std::make_shared<mb_shell::js::menu_item_controller>();
  controller->$item = item;
  controller->$menu = m;

  return controller;
}
std::vector<std::shared_ptr<mb_shell::js::menu_item_controller>>
js::menu_controller::get_menu_items() {
  if (!valid())
    return {};
  auto m = $menu.lock();
  if (!m)
    return {};

  std::unique_lock lock(m->data_lock, std::defer_lock);
  std::ignore = lock.try_lock();

  std::vector<std::shared_ptr<mb_shell::js::menu_item_controller>> items;

  for (int i = 0; i < m->children.size(); i++) {
    auto item = m->children[i]->downcast<menu_item_widget>();

    auto controller = std::make_shared<mb_shell::js::menu_item_controller>();
    controller->$item = item;
    controller->$menu = m;

    items.push_back(controller);
  }

  return items;
}
} // namespace mb_shell
