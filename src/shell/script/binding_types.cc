#include "binding_types.h"
#include "../menu_widget.h"
#include "quickjspp.hpp"
#include <iostream>
namespace mb_shell {
std::unordered_set<std::shared_ptr<std::function<void(menu_info_basic)>>>
    menu_callbacks;
bool js::menu_controller::valid() { return !$menu.expired(); }
bool js::menu_controller::add_menu_item_after(js_menu_data data,
                                              int after_index) {
  if (!valid())
    return false;
  auto m = $menu.lock()->get_child<menu_widget>();
  if (!m)
    return false;

  std::lock_guard lock(m->data_lock);

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

  auto new_item = std::make_shared<mb_shell::menu_item_widget>(item);

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
  auto m = $menu.lock()->get_child<menu_widget>();
  if (!m)
    return false;

  std::lock_guard lock(m->data_lock);

  if (index >= m->children.size())
    return false;

  auto item = m->get_child<menu_item_widget>();

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
  auto m = $menu.lock()->get_child<menu_widget>();
  if (!m)
    return false;

  std::lock_guard lock(m->data_lock);

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
  auto m = $menu.lock()->get_child<menu_widget>();
  if (!m)
    return false;

  std::lock_guard lock(m->data_lock);

  if (index >= m->children.size())
    return false;

  m->children.erase(m->children.begin() + index);
  return true;
}
std::vector<std::shared_ptr<mb_shell::js::menu_item_data>>
js::menu_controller::get_menu_items() {
  if (!valid())
    return {};
  auto m = $menu.lock()->get_child<menu_widget>();
  if (!m)
    return {};

  std::lock_guard lock(m->data_lock);

  std::vector<std::shared_ptr<mb_shell::js::menu_item_data>> items;

  for (int i = 0; i < m->children.size(); i++) {
    auto item = m->children[i]->downcast<menu_item_widget>();
    auto data = std::make_shared<mb_shell::js::menu_item_data>();

    if (item->item.type == mb_shell::menu_item::type::spacer) {
      data->type = "spacer";
    } else {
      data->type = "button";
    }

    if (item->item.name) {
      data->name = *item->item.name;
    }

    items.push_back(data);
  }

  return items;
}
std::shared_ptr<mb_shell::js::menu_item_data>
js::menu_controller::get_menu_item(int index) {
  if (!valid())
    return nullptr;

  auto m = $menu.lock()->get_child<menu_widget>();
  if (!m)
    return nullptr;

  std::lock_guard lock(m->data_lock);

  if (index >= m->children.size())
    return nullptr;

  auto item = m->children[index]->downcast<menu_item_widget>();

  auto data = std::make_shared<mb_shell::js::menu_item_data>();

  if (item->item.type == mb_shell::menu_item::type::spacer) {
    data->type = "spacer";
  } else {
    data->type = "button";
  }

  if (item->item.name) {
    data->name = *item->item.name;
  }

  return data;
}
std::function<void()> js::menu_controller::add_menu_listener(
    std::function<void(mb_shell::js::menu_info_basic_js)> listener) {
  auto listener_cvt = [listener](mb_shell::menu_info_basic info) {
    try {
      listener(
          {.from = info.from,
           .menu = std::make_shared<mb_shell::js::menu_controller>(info.menu)});
    } catch (qjs::exception e) {
      auto js = &e.context();
      auto exc = js->getException();
      std::cerr << (std::string)exc << std::endl;
      if ((bool)exc["stack"])
        std::cerr << (std::string)exc["stack"] << std::endl;
    }
  };
  auto ptr =
      std::make_shared<std::function<void(menu_info_basic)>>(listener_cvt);
  menu_callbacks.insert(ptr);
  //   $listeners_to_dispose.insert(ptr.get());
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
} // namespace mb_shell
