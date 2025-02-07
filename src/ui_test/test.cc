#include "GLFW/glfw3.h"
#include "animator.h"
#include "extra_widgets.h"
#include "nanovg_wrapper.h"
#include "ui.h"
#include "widget.h"
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <print>
#include <thread>
#include <vector>

struct test_widget : public ui::acrylic_background_widget {
  using super = ui::acrylic_background_widget;
  test_widget() : super() {
    x->animate_to(100);
    y->animate_to(100);
    width->animate_to(100);
    height->animate_to(100);

    acrylic_bg_color = nvgRGBAf(0, 0.5, 0.5, 0.5);
    update_color();
    radius->animate_to(10);
  }

  ui::sp_anim_float color_transition = anim_float(256, 3000);
  void render(ui::nanovg_context ctx) override {
    super::render(ctx);
    ctx.fontFace("main");
    ctx.fontSize(24);
    ctx.text(*x + 10, *y + 30, "Button", nullptr);
  }

  void update(ui::update_context &ctx) override {
    super::update(ctx);
    if (ctx.mouse_down_on(this)) {
      color_transition->animate_to(255);
      width->animate_to(200);
      height->animate_to(200);
    } else if (ctx.hovered(this)) {
      color_transition->animate_to(0);
      width->animate_to(150);
      height->animate_to(150);
    } else {
      color_transition->animate_to(128);
      width->animate_to(100);
      height->animate_to(100);
    }

    if (ctx.mouse_clicked_on(this)) {
      if (x->dest() == 100) {
        x->animate_to(200);
        y->animate_to(200);
      } else {
        x->animate_to(100);
        y->animate_to(100);
      }
    }
  }
};

struct dying_widget_test : public ui::widget {
  using super = ui::widget;
  
  ui::sp_anim_float opacity = anim_float(0, 200);

  dying_widget_test() : super() {
    x->animate_to(100);
    y->animate_to(100);
    width->animate_to(100);
    height->animate_to(100);
    opacity->animate_to(255);
  }

  void render(ui::nanovg_context ctx) override {
    super::render(ctx);

  static std::string s = R"#(<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" viewBox="0 0 16 16"><path fill="red" d="M1 8a7 7 0 1 1 14 0A7 7 0 0 1 1 8m7.5-3a.5.5 0 0 0-1 0v2.5H5a.5.5 0 0 0 0 1h2.5V11a.5.5 0 0 0 1 0V8.5H11a.5.5 0 0 0 0-1H8.5z"/></svg>)#";
    static auto svg = nsvgParse(s.data(),
      "px", 96
    );
    ctx.fillColor(nvgRGBAf(0.5, 0.5, 0, *opacity / 255.f));
    ctx.fillRect(*x, *y, *width, *height);
    ctx.drawSVG(svg, *x, *y, *width, *height);
    // std::println("Rendering dying widget");
  }


  void update(ui::update_context &ctx) override {
    super::update(ctx);
    if (ctx.mouse_down_on(this)) {
      dying_time = 200;
    }

    if (dying_time) {
      opacity->animate_to(0);
    } else if (ctx.hovered(this)) {
      opacity->animate_to(128);
    } else {
      opacity->animate_to(255);
    }    
  }
};

int main() {

  if (auto res = ui::render_target::init_global(); !res) {
    std::println("Failed to initialize global render target: {}", res.error());
    return 1;
  }

  ui::render_target rt;
  rt.decorated = true;
  // rt.topmost = true;
  // rt.transparent = false;

  if (auto res = rt.init(); !res) {
    std::println("Failed to initialize render target: {}", res.error());
    return 1;
  }
  nvgCreateFont(rt.nvg, "main", "C:\\WINDOWS\\FONTS\\msyh.ttc");

  rt.start_loop();
  return 0;

  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
  glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
  glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
  glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

  auto window = glfwCreateWindow(800, 600, "UI", nullptr, nullptr);
  glfwMakeContextCurrent(window);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cerr << "Failed to initialize GLAD" << std::endl;
    return -1;
  }
  
  while (!glfwWindowShouldClose(window)) {
    glClearColor(0, 0, 0, 0.3);
    glClear(GL_COLOR_BUFFER_BIT);
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  return 0;
}