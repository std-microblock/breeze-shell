#pragma once
#include <memory>
#include <string>
#include <variant>
#include "../menu_widget.h"

namespace mb_shell_js {
struct example_struct_jni {
  int a;
  int b;

  int add1(int a, int b) { return a + b; }
  std::variant<int, std::string> add2(std::string a, std::string b) {
    return a + b;
  }
  std::string c;
};

struct menu_controller {
  std::shared_ptr<mb_shell::menu_widget> menu;
  
};
} // namespace mb_shell