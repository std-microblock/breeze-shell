#include "ui.h"
#include <print>
#include <iostream>

int main() {
    ui::render_target rt;
    if(auto res = rt.init(); !res) {
        std::println("Failed to initialize render target: {}", res.error());
        return 1;
    }

    rt.start_loop();
    
    return 0;
}