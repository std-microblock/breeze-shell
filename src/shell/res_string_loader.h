#pragma once

#include <memory>
#include <string>
#include <variant>
#include <vector>
namespace mb_shell {
    struct res_string_loader {
        struct res_string_identifier {
            size_t id;
            size_t module;

            bool operator<=>(const res_string_identifier &other) const {
                return std::tie(module, id) < std::tie(other.module, other.id);
            }

            bool operator==(const res_string_identifier &other) const {
                return module == other.module && id == other.id;
            }
        };

        using string_id = std::variant<res_string_identifier, size_t>;
        static string_id string_to_id(std::wstring str);
        static std::string string_to_id_string(std::wstring str);
        static std::string string_from_id_string(const std::string &str);
        static std::vector<std::string>
        get_all_ids_of_string(const std::wstring &str);
        static void init_hook();
        static void init_known_strings();

        static void init();
    };
}

namespace std {
    template <>
    struct hash<mb_shell::res_string_loader::res_string_identifier> {
        size_t operator()(const mb_shell::res_string_loader::res_string_identifier &id) const noexcept {
            return std::hash<size_t>{}(id.id) ^ std::hash<size_t>{}(id.module);
        }
    };
}