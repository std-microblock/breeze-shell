#pragma once

#include <memory>
#include <string>
#include <variant>
namespace mb_shell {
    struct res_string_loader {
        struct res_string_identifier {
            size_t id;
            size_t module;
        };

        using string_id = std::variant<res_string_identifier, size_t>;
        static string_id string_to_id(std::wstring str);
        static std::string string_to_id_string(std::wstring str);
        static void init_hook();
        static void init_known_strings();

        static void init();
    };
}