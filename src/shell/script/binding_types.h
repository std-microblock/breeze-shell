#pragma once
#include <string>
#include <variant>

namespace mb_shell {
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
  static void test();
};
} // namespace mb_shell