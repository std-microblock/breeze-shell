#pragma once

#include <cstddef>
#include <functional>
#include <future>
#include <queue>
#include <vector>
namespace mb_shell {
    struct window_proc_hook {
        void* hwnd;
        void* original_proc;
        void* hooked_proc = nullptr;
        bool installed = false;

        std::vector<std::function<std::optional<int>(void*, void*, size_t, size_t, size_t)>> hooks;
        std::queue<std::function<void()>> tasks;

        void send_null();
        auto add_task(auto&& f) -> std::future<std::invoke_result_t<decltype(f)>> {
            if (!installed) throw std::runtime_error("Hook not installed");
            using return_type = std::invoke_result_t<decltype(f)>;
            auto task = std::make_shared<std::packaged_task<return_type()>>(std::forward<decltype(f)>(f));
            std::future<return_type> res = task->get_future();
            tasks.emplace([task]() { (*task)(); });
            send_null();
            return res;
        }

        void install(void* hwnd);
        void uninstall();
        ~window_proc_hook();
    };
}