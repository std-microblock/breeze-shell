#include <chrono>
#include <thread>
extern "C" __declspec(dllimport) void func();

int main() {
    func();
    while (1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}