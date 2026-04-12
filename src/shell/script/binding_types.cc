#include <Windows.h>
#include <WinUser.h>
#include <roapi.h>
#include <roerrorapi.h>
#include <winstring.h>

#include "binding_types.hpp"
#include "async_simple/Promise.h"
#include "binding_types_breeze_ui.h"
#include "breeze-js/quickjspp.hpp"
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <print>
#include <ranges>
#include <regex>
#include <shlobj_core.h>
#include <thread>
#include <variant>

#include "spdlog/spdlog.h"

// Resid
#include "shell/res_string_loader.h"
// Context menu
#include "shell/contextmenu/menu_render.h"
#include "shell/contextmenu/menu_widget.h"
// Compile Information
#include "shell/build_info.h"

#include "shell/contextmenu/hooks.h"

#include "script.h"
#include "shell/utils.h"
#include "winhttp.h"

#include <shellapi.h>

#include "FileWatch.hpp"

#include "wintoastlib.h"
using namespace WinToastLib;

std::vector<
    std::shared_ptr<std::function<void(mb_shell::js::menu_info_basic_js)>>>
    mb_shell::menu_callbacks_js;
namespace mb_shell::js {
bool menu_controller::valid() { return !$menu.expired(); }
std::shared_ptr<mb_shell::js::menu_item_controller>
menu_controller::append_item_after(js_menu_data data, int after_index) {
    if (!valid())
        return nullptr;
    auto m = $menu.lock();
    if (!m)
        return nullptr;

    m->children_dirty = true;
    menu_item item;
    auto new_item = std::make_shared<menu_item_normal_widget>(item);
    auto ctl = std::make_shared<menu_item_controller>(new_item, m);
    new_item->parent = m.get();
    ctl->set_data(data);
    while (after_index < 0) {
        after_index = m->children.size() + after_index + 1;
    }

    if (after_index >= m->children.size()) {
        m->children.push_back(new_item);
    } else {
        m->children.insert(m->children.begin() + after_index, new_item);
    }
    m->update_icon_width();

    if (m->animate_appear_started) {
        new_item->reset_appear_animation(0);
    }

    return ctl;
}
std::function<void()> menu_controller::add_menu_listener(
    std::function<void(menu_info_basic_js)> listener) {
    auto listener_cvt = [listener](menu_info_basic_js info) {
        try {
            listener(info);
        } catch (std::exception &e) {
            spdlog::error("Error in menu listener: {}", e.what());
        }
    };
    auto ptr =
        std::make_shared<std::function<void(menu_info_basic_js)>>(listener_cvt);
    menu_callbacks_js.push_back(ptr);
    return [ptr]() { std::erase(menu_callbacks_js, ptr); };
}
menu_controller::~menu_controller() {}
int menu_item_controller::get_position() const {
    if (!const_cast<menu_item_controller *>(this)->valid())
        return -1;

    auto item = $item.lock();
    if (!item)
        return -1;

    if (auto menu = std::get_if<std::weak_ptr<menu_widget>>(&$parent)) {
        if (auto m = menu->lock()) {
            auto it = std::find(m->children.begin(), m->children.end(), item);
            if (it != m->children.end())
                return static_cast<int>(it - m->children.begin());
        }
    } else if (auto parent =
                   std::get_if<std::weak_ptr<menu_item_parent_widget>>(
                       &$parent);
               auto m = parent->lock()) {
        auto it = std::find(m->children.begin(), m->children.end(), item);
        if (it != m->children.end())
            return static_cast<int>(it - m->children.begin());
    }

    return -1;
}

void menu_item_controller::set_position(int new_index) {
    if (!valid())
        return;

    if (auto $menu = std::get_if<std::weak_ptr<menu_widget>>(&$parent)) {
        auto m = $menu->lock();
        if (!m)
            return;

        if (new_index >= m->children.size())
            return;
        auto item = $item.lock();
        m->children.erase(
            std::remove(m->children.begin(), m->children.end(), item),
            m->children.end());

        m->children.insert(m->children.begin() + new_index, item);
        m->children_dirty = true;
    } else if (auto parent =
                   std::get_if<std::weak_ptr<menu_item_parent_widget>>(
                       &$parent);
               auto m = parent->lock()) {
        if (new_index >= m->children.size())
            return;
        auto item = $item.lock();
        m->children.erase(
            std::remove(m->children.begin(), m->children.end(), item),
            m->children.end());

        m->children.insert(m->children.begin() + new_index, item);
        m->children_dirty = true;
    }
}

static void to_menu_item(menu_item &data, const js_menu_data &js_data) {
    auto get_if_not_reset = [](auto &v) {
        return v.index() == 0 ? &std::get<0>(v) : nullptr;
    };

    if (js_data.type) {
        if (*js_data.type == "spacer") {
            data.type = menu_item::type::spacer;
        }
        if (*js_data.type == "button") {
            data.type = menu_item::type::button;
        }
    }

    if (js_data.name) {
        data.name = *js_data.name;
    }

    if (js_data.action) {
        if (auto action = get_if_not_reset(*js_data.action)) {
            data.action = [action = *action]() { action({}); };
        } else {
            data.action = {};
        }
    }

    if (js_data.submenu) {
        if (auto submenu = get_if_not_reset(*js_data.submenu)) {
            data.submenu = [submenu =
                                *submenu](std::shared_ptr<menu_widget> mw) {
                try {
                    submenu(std::make_shared<menu_controller>(
                        mw->downcast<menu_widget>()));
                } catch (std::exception &e) {
                    spdlog::error("Error in submenu callback: {}", e.what());
                }
            };
        } else {
            data.submenu = {};
        }
    }

    if (js_data.icon_bitmap) {
        if (auto icon_bitmap = get_if_not_reset(*js_data.icon_bitmap)) {
            data.icon_bitmap = *icon_bitmap;
        } else {
            data.icon_bitmap = {};
        }

        data.icon_updated = true;
    }

    if (js_data.icon_svg) {
        if (auto icon_svg = get_if_not_reset(*js_data.icon_svg)) {
            data.icon_svg = *icon_svg;
        } else {
            data.icon_svg = {};
        }

        data.icon_updated = true;
    }

    if (js_data.disabled.has_value()) {
        data.disabled = js_data.disabled.value();
    }
}

void menu_item_controller::set_data(js_menu_data data) {
    if (!valid())
        return;

    auto item = $item.lock();

    to_menu_item(item->item, data);
    if (auto menu = std::get_if<std::weak_ptr<menu_widget>>(&$parent))
        if (auto m = menu->lock()) {
            m->update_icon_width();
        }
}
void menu_item_controller::update_data(js_menu_data data) { set_data(data); }
void menu_item_controller::remove() {
    if (!valid())
        return;
    auto item = $item.lock();

    if (auto $menu = std::get_if<std::weak_ptr<menu_widget>>(&$parent);
        auto m = $menu->lock()) {
        m->children.erase(
            std::remove(m->children.begin(), m->children.end(), item),
            m->children.end());

        m->children_dirty = true;
    } else if (auto parent =
                   std::get_if<std::weak_ptr<menu_item_parent_widget>>(
                       &$parent);
               auto m = parent->lock()) {
        m->children.erase(
            std::remove(m->children.begin(), m->children.end(), item),
            m->children.end());
        m->children_dirty = true;
    }
}
bool menu_item_controller::valid() {
    if (auto a = std::get_if<0>(&$parent); a && a->expired())
        return false;
    else if (auto a = std::get_if<1>(&$parent); a && a->expired())
        return false;

    return !$item.expired();
}
js_menu_data menu_item_controller::data() {
    if (!valid())
        return {};

    auto item = $item.lock();
    if (!item)
        return {};

    auto data = js_menu_data{};

    if (item->item.type == menu_item::type::spacer) {
        data.type = "spacer";
    } else {
        data.type = "button";
    }

    if (item->item.name) {
        data.name = *item->item.name;
    }

    if (item->item.action) {
        data.action = [action = item->item.action.value()](
                          js_menu_action_event_data) { action(); };
    }

    if (item->item.submenu) {
        data.submenu = [submenu = item->item.submenu.value()](
                           std::shared_ptr<menu_controller> ctl) {
            submenu(ctl->$menu.lock());
        };
    }

    if (item->item.icon_bitmap) {
        data.icon_bitmap = item->item.icon_bitmap.value();
    }

    if (item->item.icon_svg) {
        data.icon_svg = item->item.icon_svg.value();
    }

    data.wID = item->item.wID;
    data.name_resid = item->item.name_resid;
    data.disabled = item->item.disabled;
    data.origin_name = item->item.origin_name;

    return data;
}
std::shared_ptr<menu_item_controller> menu_controller::get_item(int index) {
    if (!valid())
        return nullptr;

    auto m = $menu.lock();
    if (!m)
        return nullptr;

    if (index >= m->children.size())
        return nullptr;

    auto item = m->children[index]->downcast<menu_item_widget>();

    auto controller = std::make_shared<menu_item_controller>();
    controller->$item = item;
    controller->$parent = m;

    return controller;
}
std::vector<std::shared_ptr<menu_item_controller>>
menu_controller::get_items() {
    if (!valid())
        return {};
    auto m = $menu.lock();
    if (!m)
        return {};

    std::vector<std::shared_ptr<menu_item_controller>> items;

    for (int i = 0; i < m->children.size(); i++) {
        auto item = m->children[i]->downcast<menu_item_widget>();

        auto controller = std::make_shared<menu_item_controller>();
        controller->$item = item;
        controller->$parent = m;

        items.push_back(controller);
    }

    return items;
}
void menu_controller::close() {
    auto menu = $menu.lock();
    if (!menu)
        return;

    menu->close();
}
std::string clipboard::read_text() {
    if (!OpenClipboard(nullptr))
        return "";

    HANDLE hData = GetClipboardData(CF_TEXT);
    if (hData == nullptr) {
        CloseClipboard();
        return "";
    }

    char *pszText = static_cast<char *>(GlobalLock(hData));
    std::string text(pszText);

    GlobalUnlock(hData);
    CloseClipboard();
    return text;
}

void clipboard::write_text(std::string text) {
    if (!OpenClipboard(nullptr))
        return;

    EmptyClipboard();
    HGLOBAL hData = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
    if (hData == nullptr) {
        CloseClipboard();
        return;
    }
    std::wstring wtext = utf8_to_wstring(text);
    HGLOBAL hDataW =
        GlobalAlloc(GMEM_MOVEABLE, (wtext.size() + 1) * sizeof(wchar_t));
    if (hDataW == nullptr) {
        CloseClipboard();
        return;
    }

    wchar_t *pszDataW = static_cast<wchar_t *>(GlobalLock(hDataW));
    wcscpy_s(pszDataW, wtext.size() + 1, wtext.c_str());
    GlobalUnlock(hDataW);
    SetClipboardData(CF_UNICODETEXT, hDataW);
    CloseClipboard();
}

std::string network::post(std::string url, std::string data) {
    HINTERNET hSession =
        WinHttpOpen(L"BreezeShell", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                    WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        throw std::runtime_error("Failed to initialize WinHTTP");
    }

    URL_COMPONENTS urlComp = {sizeof(URL_COMPONENTS)};
    wchar_t hostName[256] = {0};
    wchar_t urlPath[1024] = {0};
    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = sizeof(hostName) / sizeof(wchar_t);
    urlComp.lpszUrlPath = urlPath;
    urlComp.dwUrlPathLength = sizeof(urlPath) / sizeof(wchar_t);

    std::wstring wideUrl = utf8_to_wstring(url);

    if (!WinHttpCrackUrl(wideUrl.c_str(), wideUrl.length(), 0, &urlComp)) {
        WinHttpCloseHandle(hSession);
        throw std::runtime_error("Invalid URL format");
    }

    HINTERNET hConnect = WinHttpConnect(hSession, hostName,
                                        urlComp.nScheme == INTERNET_SCHEME_HTTPS
                                            ? INTERNET_DEFAULT_HTTPS_PORT
                                            : INTERNET_DEFAULT_HTTP_PORT,
                                        0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        throw std::runtime_error("Failed to connect to server");
    }

    DWORD flags = WINHTTP_FLAG_REFRESH;
    if (urlComp.nScheme == INTERNET_SCHEME_HTTPS) {
        flags |= WINHTTP_FLAG_SECURE;
    }

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect, data.empty() ? L"GET" : L"POST", urlPath, nullptr,
        WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        throw std::runtime_error("Failed to create request");
    }

    BOOL result = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                     data.empty() ? WINHTTP_NO_REQUEST_DATA
                                                  : (LPVOID)data.c_str(),
                                     data.length(), data.length(), 0);

    if (!result || !WinHttpReceiveResponse(hRequest, nullptr)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        throw std::runtime_error("Failed to send/receive request");
    }

    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest,
                        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &statusCode,
                        &statusCodeSize, WINHTTP_NO_HEADER_INDEX);

    if (statusCode >= 400) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        throw std::runtime_error("Server returned error: " +
                                 std::to_string(statusCode));
    }

    std::string response;
    DWORD bytesAvailable;
    do {
        bytesAvailable = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &bytesAvailable))
            break;
        if (!bytesAvailable)
            break;

        std::vector<char> buffer(bytesAvailable);
        DWORD bytesRead = 0;
        if (WinHttpReadData(hRequest, buffer.data(), bytesAvailable,
                            &bytesRead)) {
            response.append(buffer.data(), bytesRead);
        }
    } while (bytesAvailable > 0);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return response;
}

std::string network::get(std::string url) { return post(url, ""); }

void network::get_async(std::string url,
                        std::function<void(std::string)> callback,
                        std::function<void(std::string)> error_callback) {
    std::thread([url, callback, error_callback,
                 &ctx = *qjs::Context::current]() {
        try {
            auto res = get(url);
            ctx.enqueueJob([=]() { callback(res); });
        } catch (std::exception &e) {
            spdlog::error("Error in network::get_async: {}", e.what());
            auto error = std::string(e.what());
            ctx.enqueueJob([=]() { error_callback(error); });
        }
    }).detach();
}

void network::post_async(std::string url, std::string data,
                         std::function<void(std::string)> callback,
                         std::function<void(std::string)> error_callback) {
    std::thread([url, data, callback, error_callback,
                 &ctx = *qjs::Context::current]() {
        try {
            auto res = post(url, data);
            ctx.enqueueJob([=]() { callback(res); });
        } catch (std::exception &e) {
            spdlog::error("Error in network::post_async: {}", e.what());
            auto error = std::string(e.what());
            ctx.enqueueJob([=]() { error_callback(error); });
        }
    }).detach();
}
subproc_result_data subproc::run(std::string cmd) {
    subproc_result_data result;
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    HANDLE hReadPipe, hWritePipe;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0))
        return result;

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdError = hWritePipe;
    si.hStdOutput = hWritePipe;
    si.dwFlags |= STARTF_USESTDHANDLES;

    if (!CreateProcessW(nullptr, utf8_to_wstring(cmd).data(), nullptr, nullptr,
                        TRUE, 0, nullptr, nullptr, &si, &pi)) {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return result;
    }

    CloseHandle(hWritePipe);

    char buffer[4096];
    DWORD bytesRead;
    std::string out;
    while (ReadFile(hReadPipe, buffer, sizeof(buffer), &bytesRead, nullptr) &&
           bytesRead > 0) {
        out.append(buffer, bytesRead);
    }

    CloseHandle(hReadPipe);

    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    result.out = out;
    result.code = exitCode;

    return result;
}
void subproc::run_async(std::string cmd,
                        std::function<void(subproc_result_data)> callback) {
    std::thread([cmd, callback, &ctx = *qjs::Context::current]() {
        try {
            auto res = run(cmd);
            ctx.enqueueJob([=]() { callback(res); });
        } catch (std::exception &e) {
            spdlog::error("Error in subproc::run_async: {}", e.what());
        }
    }).detach();
}
void menu_controller::clear() {
    if (!valid())
        return;
    auto m = $menu.lock();
    if (!m)
        return;

    m->children_dirty = true;
    m->children.clear();
    m->menu_data.items.clear();
}

void fs::chdir(std::string path) { std::filesystem::current_path(path); }

std::string fs::cwd() { return std::filesystem::current_path().string(); }

bool fs::exists(std::string path) { return std::filesystem::exists(path); }

bool fs::isdir(std::string path) { return std::filesystem::is_directory(path); }

void fs::mkdir(std::string path) {
    try {
        std::filesystem::create_directories(path);
    } catch (std::exception &e) {
        std::filesystem::create_directory(path);
    }
}

void fs::rmdir(std::string path) { std::filesystem::remove(path); }

void fs::rename(std::string old_path, std::string new_path) {
    std::filesystem::rename(old_path, new_path);
}

void fs::remove(std::string path) { std::filesystem::remove(path); }

void fs::copy(std::string src_path, std::string dest_path) {
    std::filesystem::copy_file(
        src_path, dest_path, std::filesystem::copy_options::overwrite_existing);
}

void fs::move(std::string src_path, std::string dest_path) {
    std::filesystem::rename(src_path, dest_path);
}

std::string fs::read(std::string path) {
    std::ifstream file(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
}

void fs::write(std::string path, std::string data) {
    std::ofstream file(path, std::ios::binary);
    file.write(data.c_str(), data.length());
}

std::vector<uint8_t> fs::read_binary(std::string path) {
    std::ifstream file(path, std::ios::binary);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
}

void fs::write_binary(std::string path, std::vector<uint8_t> data) {
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char *>(data.data()), data.size());
}
std::string breeze::version() { return BREEZE_VERSION; }
std::string breeze::data_directory() {
    return config::data_directory().generic_string();
}
std::string breeze::default_config() {
    return config::dump_default_config();
}
bool breeze::should_show_settings_button() {
    return mb_shell::config::current->context_menu.show_settings_button;
}
std::vector<std::string> fs::readdir(std::string path) {
    std::vector<std::string> result;
    std::ranges::copy(std::filesystem::directory_iterator(path) |
                          std::ranges::views::transform([](const auto &entry) {
                              return entry.path().generic_string();
                          }),
                      std::back_inserter(result));
    return result;
}
bool breeze::is_light_theme() { return is_light_mode(); }
void network::download_async(std::string url, std::string path,
                             std::function<void()> callback,
                             std::function<void(std::string)> error_callback) {
    std::thread([url, path, callback, error_callback,
                 &ctx = *qjs::Context::current]() {
        try {
            auto data = get(url);
            fs::write_binary(path,
                             std::vector<uint8_t>(data.begin(), data.end()));
            ctx.enqueueJob([=]() { callback(); });
        } catch (std::exception &e) {
            auto error = std::string(e.what());
            spdlog::error("Error in network::download_async: {}", error);
            ctx.enqueueJob([=]() { error_callback(error); });
        }
    }).detach();
}

void network::download_with_progress_async(
    std::string url, std::string path, std::function<void()> callback,
    std::function<void(std::string)> error_callback,
    std::function<void(size_t, size_t)> progress_callback) {
    std::thread([url, path, callback, error_callback,
                 progress_callback, &ctx = *qjs::Context::current]() {
        HINTERNET hSession = nullptr;
        HINTERNET hConnect = nullptr;
        HINTERNET hRequest = nullptr;
        auto close_handles = [&]() {
            if (hRequest) {
                WinHttpCloseHandle(hRequest);
                hRequest = nullptr;
            }
            if (hConnect) {
                WinHttpCloseHandle(hConnect);
                hConnect = nullptr;
            }
            if (hSession) {
                WinHttpCloseHandle(hSession);
                hSession = nullptr;
            }
        };

        try {
            hSession =
                WinHttpOpen(L"BreezeShell", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                            WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
            if (!hSession) {
                throw std::runtime_error("Failed to initialize WinHTTP");
            }

            URL_COMPONENTS urlComp = {sizeof(URL_COMPONENTS)};
            wchar_t hostName[256] = {0};
            wchar_t urlPath[1024] = {0};
            urlComp.lpszHostName = hostName;
            urlComp.dwHostNameLength = sizeof(hostName) / sizeof(wchar_t);
            urlComp.lpszUrlPath = urlPath;
            urlComp.dwUrlPathLength = sizeof(urlPath) / sizeof(wchar_t);

            std::wstring wideUrl = utf8_to_wstring(url);

            if (!WinHttpCrackUrl(wideUrl.c_str(), wideUrl.length(), 0,
                                 &urlComp)) {
                throw std::runtime_error("Invalid URL format");
            }

            hConnect =
                WinHttpConnect(hSession, hostName,
                               urlComp.nScheme == INTERNET_SCHEME_HTTPS
                                   ? INTERNET_DEFAULT_HTTPS_PORT
                                   : INTERNET_DEFAULT_HTTP_PORT,
                               0);
            if (!hConnect) {
                throw std::runtime_error("Failed to connect to server");
            }

            DWORD flags = WINHTTP_FLAG_REFRESH;
            if (urlComp.nScheme == INTERNET_SCHEME_HTTPS) {
                flags |= WINHTTP_FLAG_SECURE;
            }

            hRequest = WinHttpOpenRequest(
                hConnect, L"GET", urlPath, nullptr, WINHTTP_NO_REFERER,
                WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
            if (!hRequest) {
                throw std::runtime_error("Failed to create request");
            }

            if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                    WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
                !WinHttpReceiveResponse(hRequest, nullptr)) {
                throw std::runtime_error("Failed to send/receive request");
            }

            DWORD statusCode = 0;
            DWORD statusCodeSize = sizeof(statusCode);
            WinHttpQueryHeaders(
                hRequest,
                WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusCodeSize,
                WINHTTP_NO_HEADER_INDEX);

            if (statusCode >= 400) {
                throw std::runtime_error("Server returned error: " +
                                         std::to_string(statusCode));
            }

            DWORD contentLength = 0;
            DWORD contentLengthSize = sizeof(contentLength);
            size_t totalBytes = 0;
            if (WinHttpQueryHeaders(
                    hRequest,
                    WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
                    WINHTTP_HEADER_NAME_BY_INDEX, &contentLength,
                    &contentLengthSize, WINHTTP_NO_HEADER_INDEX)) {
                totalBytes = contentLength;
            }

            std::ofstream file(path, std::ios::binary | std::ios::trunc);
            if (!file.is_open()) {
                throw std::runtime_error("Failed to open file for writing");
            }

            size_t downloadedBytes = 0;
            DWORD bytesAvailable = 0;
            do {
                bytesAvailable = 0;
                if (!WinHttpQueryDataAvailable(hRequest, &bytesAvailable)) {
                    throw std::runtime_error("Failed to query download data");
                }
                if (!bytesAvailable)
                    break;

                std::vector<char> buffer(bytesAvailable);
                DWORD bytesRead = 0;
                if (!WinHttpReadData(hRequest, buffer.data(), bytesAvailable,
                                     &bytesRead)) {
                    throw std::runtime_error("Failed to read download data");
                }
                if (!bytesRead)
                    continue;

                file.write(buffer.data(), bytesRead);
                if (!file.good()) {
                    throw std::runtime_error("Failed to write download file");
                }

                downloadedBytes += bytesRead;
                ctx.enqueueJob([=]() {
                    progress_callback(downloadedBytes, totalBytes);
                });
            } while (bytesAvailable > 0);

            file.close();
            close_handles();
            ctx.enqueueJob([=]() { callback(); });
        } catch (std::exception &e) {
            close_handles();
            std::filesystem::remove(path);
            auto error = std::string(e.what());
            spdlog::error("Error in network::download_with_progress_async: {}",
                          error);
            ctx.enqueueJob([=]() { error_callback(error); });
        }
    }).detach();
}
std::string win32::resid_from_string(std::string str) {
    return res_string_loader::string_to_id_string(utf8_to_wstring(str));
}
size_t win32::load_library(std::string path) {
    return reinterpret_cast<size_t>(
        LoadLibraryW(utf8_to_wstring(path).c_str()));
}
std::string breeze::user_language() {
    wchar_t buffer[256];

    /*
    BOOL GetUserPreferredUILanguages(
    [in]            DWORD   dwFlags,
    [out]           PULONG  pulNumLanguages,
    [out, optional] PZZWSTR pwszLanguagesBuffer,
    [in, out]       PULONG  pcchLanguagesBuffer
  );
  */

    ULONG num_langs = 256;
    if (GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &num_langs, buffer,
                                    &num_langs)) {
        return wstring_to_utf8(buffer);
    }

    return "en-US";
}
std::optional<std::string> win32::env(std::string name) {
    return mb_shell::env(name);
}

std::string breeze::hash() { return BREEZE_GIT_COMMIT_HASH; }
std::string breeze::branch() { return BREEZE_GIT_BRANCH_NAME; }
std::vector<std::shared_ptr<mb_shell::js::menu_item_controller>>
menu_item_parent_item_controller::children() {
    if (!valid())
        return {};

    auto item = $item.lock();
    if (!item)
        return {};

    std::vector<std::shared_ptr<mb_shell::js::menu_item_controller>> items;

    for (int i = 0; i < item->children.size(); i++) {
        auto subitem = item->children[i]->downcast<menu_item_widget>();

        auto controller = std::make_shared<menu_item_controller>();
        controller->$item = subitem;
        controller->$parent = item;

        items.push_back(controller);
    }

    return items;
}
int menu_item_parent_item_controller::get_position() const {
    if (!const_cast<menu_item_parent_item_controller *>(this)->valid())
        return -1;

    auto item = $item.lock();
    if (!item || !item->parent)
        return -1;

    auto parent = item->parent->downcast<menu_widget>();
    if (!parent)
        return -1;

    auto it = std::find(parent->children.begin(), parent->children.end(), item);
    if (it == parent->children.end())
        return -1;

    return static_cast<int>(it - parent->children.begin());
}

void menu_item_parent_item_controller::set_position(int new_index) {
    if (!valid())
        return;

    auto item = $item.lock();
    if (!item)
        return;

    auto parent = item->parent->downcast<menu_widget>();

    if (new_index >= parent->children.size())
        return;

    parent->children.erase(
        std::remove(parent->children.begin(), parent->children.end(), item),
        parent->children.end());

    parent->children.insert(parent->children.begin() + new_index, item);
    parent->children_dirty = true;
    parent->update_icon_width();
}
void menu_item_parent_item_controller::remove() {
    if (!valid())
        return;

    auto item = $item.lock();
    if (!item)
        return;

    auto parent = item->parent->downcast<menu_widget>();
    parent->children_dirty = true;
    parent->children.erase(
        std::remove(parent->children.begin(), parent->children.end(), item),
        parent->children.end());
}
bool menu_item_parent_item_controller::valid() {
    return !$item.expired() && !$menu.expired();
}

std::shared_ptr<mb_shell::js::menu_item_parent_item_controller>
menu_controller::append_parent_item_after(int after_index) {
    if (!valid())
        return nullptr;
    auto m = $menu.lock();
    if (!m)
        return nullptr;
    m->children_dirty = true;

    auto new_item = std::make_shared<menu_item_parent_widget>();
    auto ctl = std::make_shared<menu_item_parent_item_controller>(new_item, m);
    new_item->parent = m.get();

    while (after_index < 0) {
        after_index = m->children.size() + after_index + 1;
    }

    if (after_index >= m->children.size()) {
        m->children.push_back(new_item);
    } else {
        m->children.insert(m->children.begin() + after_index, new_item);
    }

    m->update_icon_width();

    if (m->animate_appear_started) {
        new_item->reset_appear_animation(0);
    }

    return ctl;
}
std::shared_ptr<mb_shell::js::menu_item_controller>
menu_item_parent_item_controller::append_child_after(
    mb_shell::js::js_menu_data data, int after_index) {
    if (!valid())
        return nullptr;

    auto parent = $item.lock();
    if (!parent)
        return nullptr;

    menu_item item;
    auto new_item = std::make_shared<menu_item_normal_widget>(item);
    auto ctl = std::make_shared<menu_item_controller>(
        menu_item_controller{new_item->downcast<menu_item_widget>(), parent});
    new_item->parent = parent.get();
    ctl->set_data(data);

    while (after_index < 0) {
        after_index = parent->children.size() + after_index + 1;
    }

    if (after_index >= parent->children.size()) {
        parent->children.push_back(new_item);
    } else {
        parent->children.insert(parent->children.begin() + after_index,
                                new_item);
    }

    if (parent->parent->downcast<menu_widget>()->animate_appear_started) {
        new_item->reset_appear_animation(0);
    }

    return ctl;
}
void subproc::open(std::string path, std::string args) {
    std::wstring wpath = utf8_to_wstring(path);
    std::wstring wargs = utf8_to_wstring(args);
    ShellExecuteW(nullptr, L"open", wpath.c_str(), wargs.c_str(), nullptr,
                  SW_SHOWNORMAL);
}
void subproc::open_async(std::string path, std::string args,
                         std::function<void()> callback) {
    std::thread([path, callback, args, &ctx = *qjs::Context::current]() {
        try {
            open(path, args);
            ctx.enqueueJob([=]() { callback(); });
        } catch (std::exception &e) {
            spdlog::error("Error in subproc::open_async: {}", e.what());
        }
    }).detach();
}

struct Timer {
    std::function<void()> callback;
    std::weak_ptr<qjs::Context> ctx;
    int delay;
    int elapsed = 0;
    bool repeat;
    int id;
};

std::list<std::unique_ptr<Timer>> timers;
std::optional<std::thread> timer_thread;

void timer_thread_func() {
    while (true) {
        constexpr auto sleep_time = 30;
        Sleep(sleep_time);

        std::vector<std::function<void()>> callbacks;
        for (auto &timer : timers) {
            if (!timer || timer->ctx.expired()) {
                timer = nullptr;
                continue;
            }
            timer->elapsed += sleep_time;
            if (timer->elapsed >= timer->delay) {

                bool repeat = timer->repeat;
                timer->elapsed = 0;
                callbacks.push_back(timer->callback);
                if (!repeat) {
                    timer = nullptr;
                }
            }
        }

        timers.erase(std::remove_if(timers.begin(), timers.end(),
                                    [](const auto &timer) { return !timer; }),
                     timers.end());

        for (const auto &callback : callbacks) {
            try {
                callback();
            } catch (std::exception &e) {
                spdlog::error("Error in timer callback: {}", e.what());
            } catch (...) {
                spdlog::error("Unknown error in timer callback");
            }
        }
    }
}

void ensure_timer_thread() {
    if (!timer_thread) {
        timer_thread = std::thread(timer_thread_func);
    }
}

int infra::setTimeout(std::function<void()> callback, int delay) {
    ensure_timer_thread();

    auto timer = std::make_unique<Timer>();
    timer->callback = callback;
    timer->delay = delay;
    timer->repeat = false;
    timer->ctx = qjs::Context::current->weak_from_this();
    timer->id = timers.size() + 1;
    auto id = timer->id;
    timers.push_back(std::move(timer));

    return id;
};
void infra::clearTimeout(int id) {
    if (auto it = std::find_if(
            timers.begin(), timers.end(),
            [id](const auto &timer) { return timer && timer->id == id; });
        it != timers.end()) {
        timers.erase(it);
    }
};
int infra::setInterval(std::function<void()> callback, int delay) {
    ensure_timer_thread();

    auto timer = std::make_unique<Timer>();
    timer->callback = callback;
    timer->delay = delay;
    timer->repeat = true;
    timer->ctx = qjs::Context::current->weak_from_this();
    timer->id = timers.size() + 1;
    auto id = timer->id;
    timers.push_back(std::move(timer));

    return id;
};
void infra::clearInterval(int id) { clearTimeout(id); };
std::string infra::atob(std::string base64) {
    std::string result;
    result.reserve(base64.length() * 3 / 4);

    const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                     "abcdefghijklmnopqrstuvwxyz"
                                     "0123456789+/";

    int val = 0, valb = -8;
    for (unsigned char c : base64) {
        if (c == '=')
            break;

        if (isspace(c))
            continue;

        val = (val << 6) + base64_chars.find(c);
        valb += 6;
        if (valb >= 0) {
            result.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return result;
}

std::string infra::btoa(std::string str) {
    const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                     "abcdefghijklmnopqrstuvwxyz"
                                     "0123456789+/";

    std::string result;
    int val = 0, valb = -6;
    for (unsigned char c : str) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            result.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) {
        result.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    while (result.size() % 4) {
        result.push_back('=');
    }
    return result;
}

void fs::copy_shfile(std::string src_path, std::string dest_path,
                     std::function<void(bool, std::string)> callback) {
    std::thread([=, &ctx = *qjs::Context::current] {
        SHFILEOPSTRUCTW FileOp = {GetForegroundWindow()};
        std::wstring wsrc = utf8_to_wstring(src_path);
        std::wstring wdest = utf8_to_wstring(dest_path);

        std::vector<wchar_t> from_buf(wsrc.size() + 2, 0);
        std::vector<wchar_t> to_buf(wdest.size() + 2, 0);
        wcsncpy_s(from_buf.data(), from_buf.size(), wsrc.c_str(), _TRUNCATE);
        wcsncpy_s(to_buf.data(), to_buf.size(), wdest.c_str(), _TRUNCATE);

        FileOp.wFunc = FO_COPY;
        FileOp.pFrom = from_buf.data();
        FileOp.pTo = to_buf.data();
        FileOp.fFlags = FOF_RENAMEONCOLLISION | FOF_ALLOWUNDO |
                        FOF_NOCONFIRMMKDIR | FOF_NOCOPYSECURITYATTRIBS |
                        FOF_WANTMAPPINGHANDLE;

        auto res = SHFileOperationW(&FileOp);
        std::wstring final_path;
        bool success = (res == 0) && !FileOp.fAnyOperationsAborted;
        if (success) {
            if (FileOp.hNameMappings) {
                struct file_operation_collision_mapping {
                    int index;
                    SHNAMEMAPPINGW *mapping;
                };

                file_operation_collision_mapping *mapping =
                    static_cast<file_operation_collision_mapping *>(
                        FileOp.hNameMappings);
                SHNAMEMAPPINGW *map = &mapping->mapping[0];
                final_path = map->pszNewPath;

                SHFreeNameMappings(FileOp.hNameMappings);
            } else {
                std::filesystem::path dest(wdest);
                std::filesystem::path src(wsrc);
                final_path = (dest / src.filename()).wstring();
            }

            SHChangeNotify(SHCNE_CREATE, SHCNF_PATH | SHCNF_FLUSH,
                           final_path.c_str(), nullptr);
        }

        std::string utf8_path = wstring_to_utf8(final_path);

        ctx.enqueueJob([=]() { callback(success, utf8_path); });
    }).detach();
}

void fs::move_shfile(std::string src_path, std::string dest_path,
                     std::function<void(bool)> callback) {
    std::thread([=, &ctx = *qjs::Context::current] {
        SHFILEOPSTRUCTW FileOp = {GetForegroundWindow()};
        std::wstring wsrc = utf8_to_wstring(src_path);
        std::wstring wdest = utf8_to_wstring(dest_path);

        FileOp.wFunc = FO_MOVE;
        FileOp.pFrom = wsrc.c_str();
        FileOp.pTo = wdest.c_str();

        auto res = SHFileOperationW(&FileOp);
        ctx.enqueueJob([=]() { callback(res == 0); });
    }).detach();
}
size_t win32::load_file_icon(std::string path) {
    SHFILEINFOW sfi = {0};
    DWORD_PTR ret =
        SHGetFileInfoW(utf8_to_wstring(path).c_str(), 0, &sfi,
                       sizeof(SHFILEINFOW), SHGFI_ICON | SHGFI_SMALLICON);
    if (ret == 0)
        return 0;

    ICONINFO iconinfo;
    GetIconInfo(sfi.hIcon, &iconinfo);

    return (size_t)iconinfo.hbmColor;
}
std::function<void()>
fs::watch(std::string path, std::function<void(std::string, int)> callback) {
    // TODO: fix memory leak

    auto dispose = std::make_shared<bool>(false);
    auto fw = new filewatch::FileWatch<std::string>(path);

    fw->set_callback(
        [dispose, callback, fw](const std::string &path,
                                const filewatch::Event change_type) {
            if (*dispose)
                return;
            try {
                callback(path, (int)change_type);
            } catch (qjs::qjs_context_destroyed_exception &e) {
                *dispose = true;
            } catch (std::exception &e) {
                spdlog::error("Error in file watch callback: {}", e.what());
            }
        });

    return [dispose] { *dispose = true; };
}
std::shared_ptr<mb_shell::js::menu_controller>
menu_controller::create_detached() {
    auto m = std::make_shared<menu_widget>(true);
    auto ctl = std::make_shared<menu_controller>(m);
    ctl->$menu = m;
    ctl->$menu_detached = m; // to keep it alive
    m->parent = m.get();
    m->menu_data.is_top_level = true;

    return ctl;
}
int32_t win32::reg_get_dword(std::string key, std::string name) {
    HKEY hKey;
    std::wstring wkey = utf8_to_wstring(key);
    std::wstring wname = utf8_to_wstring(name);

    if (RegOpenKeyExW(HKEY_CURRENT_USER, wkey.c_str(), 0, KEY_READ, &hKey) !=
        ERROR_SUCCESS) {
        return 0;
    }

    DWORD value = 0;
    DWORD dataSize = sizeof(DWORD);
    DWORD dataType = REG_DWORD;

    if (RegQueryValueExW(hKey, wname.c_str(), nullptr, &dataType,
                         reinterpret_cast<LPBYTE>(&value),
                         &dataSize) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return 0;
    }

    RegCloseKey(hKey);
    return static_cast<int32_t>(value);
}

std::string win32::reg_get_string(std::string key, std::string name) {
    HKEY hKey;
    std::wstring wkey = utf8_to_wstring(key);
    std::wstring wname = utf8_to_wstring(name);

    if (RegOpenKeyExW(HKEY_CURRENT_USER, wkey.c_str(), 0, KEY_READ, &hKey) !=
        ERROR_SUCCESS) {
        return "";
    }

    DWORD dataSize = 0;
    DWORD dataType = REG_SZ;

    // Get the size needed
    if (RegQueryValueExW(hKey, wname.c_str(), nullptr, &dataType, nullptr,
                         &dataSize) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return "";
    }

    std::vector<wchar_t> buffer(dataSize / sizeof(wchar_t) + 1, 0);

    if (RegQueryValueExW(hKey, wname.c_str(), nullptr, &dataType,
                         reinterpret_cast<LPBYTE>(buffer.data()),
                         &dataSize) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return "";
    }

    RegCloseKey(hKey);
    return wstring_to_utf8(buffer.data());
}

int64_t win32::reg_get_qword(std::string key, std::string name) {
    HKEY hKey;
    std::wstring wkey = utf8_to_wstring(key);
    std::wstring wname = utf8_to_wstring(name);

    if (RegOpenKeyExW(HKEY_CURRENT_USER, wkey.c_str(), 0, KEY_READ, &hKey) !=
        ERROR_SUCCESS) {
        return 0;
    }

    ULONGLONG value = 0;
    DWORD dataSize = sizeof(ULONGLONG);
    DWORD dataType = REG_QWORD;

    if (RegQueryValueExW(hKey, wname.c_str(), nullptr, &dataType,
                         reinterpret_cast<LPBYTE>(&value),
                         &dataSize) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return 0;
    }

    RegCloseKey(hKey);
    return static_cast<int64_t>(value);
}

void win32::reg_set_dword(std::string key, std::string name, int32_t value) {
    HKEY hKey;
    std::wstring wkey = utf8_to_wstring(key);
    std::wstring wname = utf8_to_wstring(name);

    // Create the key if it doesn't exist
    if (RegCreateKeyExW(HKEY_CURRENT_USER, wkey.c_str(), 0, nullptr,
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey,
                        nullptr) != ERROR_SUCCESS) {
        return;
    }

    DWORD dwValue = static_cast<DWORD>(value);
    RegSetValueExW(hKey, wname.c_str(), 0, REG_DWORD,
                   reinterpret_cast<const BYTE *>(&dwValue), sizeof(DWORD));

    RegCloseKey(hKey);
}

void win32::reg_set_string(std::string key, std::string name,
                           std::string value) {
    HKEY hKey;
    std::wstring wkey = utf8_to_wstring(key);
    std::wstring wname = utf8_to_wstring(name);
    std::wstring wvalue = utf8_to_wstring(value);

    // Create the key if it doesn't exist
    if (RegCreateKeyExW(HKEY_CURRENT_USER, wkey.c_str(), 0, nullptr,
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey,
                        nullptr) != ERROR_SUCCESS) {
        return;
    }

    RegSetValueExW(hKey, wname.c_str(), 0, REG_SZ,
                   reinterpret_cast<const BYTE *>(wvalue.c_str()),
                   static_cast<DWORD>((wvalue.size() + 1) * sizeof(wchar_t)));

    RegCloseKey(hKey);
}

void win32::reg_set_qword(std::string key, std::string name, int64_t value) {
    HKEY hKey;
    std::wstring wkey = utf8_to_wstring(key);
    std::wstring wname = utf8_to_wstring(name);

    // Create the key if it doesn't exist
    if (RegCreateKeyExW(HKEY_CURRENT_USER, wkey.c_str(), 0, nullptr,
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey,
                        nullptr) != ERROR_SUCCESS) {
        return;
    }

    ULONGLONG qwValue = static_cast<ULONGLONG>(value);
    RegSetValueExW(hKey, wname.c_str(), 0, REG_QWORD,
                   reinterpret_cast<const BYTE *>(&qwValue), sizeof(ULONGLONG));

    RegCloseKey(hKey);
}

static WORD get_scancode(std::string key) {
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);

    static const std::unordered_map<std::string, WORD> scancodes = {
        {"escape", 0x01},
        {"1", 0x02},
        {"2", 0x03},
        {"3", 0x04},
        {"4", 0x05},
        {"5", 0x06},
        {"6", 0x07},
        {"7", 0x08},
        {"8", 0x09},
        {"9", 0x0A},
        {"0", 0x0B},
        {"minus", 0x0C},
        {"equal", 0x0D},
        {"backspace", 0x0E},
        {"tab", 0x0F},
        {"q", 0x10},
        {"w", 0x11},
        {"e", 0x12},
        {"r", 0x13},
        {"t", 0x14},
        {"y", 0x15},
        {"u", 0x16},
        {"i", 0x17},
        {"o", 0x18},
        {"p", 0x19},
        {"bracket_left", 0x1A},
        {"bracket_right", 0x1B},
        {"enter", 0x1C},
        {"ctrl", 0x1D},
        {"a", 0x1E},
        {"s", 0x1F},
        {"d", 0x20},
        {"f", 0x21},
        {"g", 0x22},
        {"h", 0x23},
        {"j", 0x24},
        {"k", 0x25},
        {"l", 0x26},
        {"semicolon", 0x27},
        {"quote", 0x28},
        {"backtick", 0x29},
        {"shift", 0x2A},
        {"backslash", 0x2B},
        {"z", 0x2C},
        {"x", 0x2D},
        {"c", 0x2E},
        {"v", 0x2F},
        {"b", 0x30},
        {"n", 0x31},
        {"m", 0x32},
        {"comma", 0x33},
        {"period", 0x34},
        {"slash", 0x35},
        {"alt", 0x38},
        {"space", 0x39},
        {"capslock", 0x3A},
        {"f1", 0x3B},
        {"f2", 0x3C},
        {"f3", 0x3D},
        {"f4", 0x3E},
        {"f5", 0x3F},
        {"f6", 0x40},
        {"f7", 0x41},
        {"f8", 0x42},
        {"f9", 0x43},
        {"f10", 0x44},
        {"numlock", 0x45},
        {"scrolllock", 0x46},
        {"home", 0x47},
        {"up", 0x48},
        {"pageup", 0x49},
        {"minus_pad", 0x4A},
        {"left", 0x4B},
        {"center", 0x4C},
        {"right", 0x4D},
        {"plus_pad", 0x4E},
        {"end", 0x4F},
        {"down", 0x50},
        {"pagedown", 0x51},
        {"insert", 0x52},
        {"delete", 0x53},
        {"f11", 0x57},
        {"f12", 0x58},
        {"win", 0xE05B},
        {"context", 0xE05D},
        {"printscreen", 0xE037},
        {"pause", 0xE11D45}};

    auto it = scancodes.find(key);
    if (it != scancodes.end()) {
        return it->second;
    }
    return 0;
}

bool win32::is_key_down(std::string key) {
    auto key_lower =
        key | std::views::transform(::tolower) | std::ranges::to<std::string>();

    WORD scancode = get_scancode(key_lower);
    if (scancode != 0) {
        SHORT state =
            GetAsyncKeyState(MapVirtualKeyW(scancode & 0xFF, MAPVK_VSC_TO_VK));
        return (state & 0x8000) != 0;
    }

    return false;
}

static bool is_extended_key(WORD scancode) {
    return (scancode & 0xFF00) == 0xE000 || (scancode & 0xFF0000) == 0xE10000;
}

void win32::simulate_hotkeys(std::vector<std::string> keys) {
    if (keys.empty())
        return;

    for (const auto &key : keys) {
        simulate_key_down(key);
    }
    for (auto it = keys.rbegin(); it != keys.rend(); ++it) {
        simulate_key_up(*it);
    }
}

void win32::simulate_key_press(std::string key) {
    simulate_key_down(key);
    simulate_key_up(key);
}

void win32::simulate_key_down(std::string key) {
    WORD sc = get_scancode(key);
    if (sc == 0)
        return;

    INPUT input = {0};
    input.type = INPUT_KEYBOARD;
    input.ki.wScan = sc & 0xFF;
    input.ki.dwFlags = KEYEVENTF_SCANCODE;

    if (is_extended_key(sc)) {
        input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    }

    SendInput(1, &input, sizeof(INPUT));
}

void win32::simulate_key_up(std::string key) {
    WORD sc = get_scancode(key);
    if (sc == 0)
        return;

    INPUT input = {0};
    input.type = INPUT_KEYBOARD;
    input.ki.wScan = sc & 0xFF;
    input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;

    if (is_extended_key(sc)) {
        input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    }

    SendInput(1, &input, sizeof(INPUT));
}

void win32::simulate_text_input(std::string text) {
    std::wstring wtext = utf8_to_wstring(text);

    for (wchar_t c : wtext) {
        INPUT input[2] = {0};

        input[0].type = INPUT_KEYBOARD;
        input[0].ki.wScan = c;
        input[0].ki.dwFlags = KEYEVENTF_UNICODE;

        input[1].type = INPUT_KEYBOARD;
        input[1].ki.wScan = c;
        input[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;

        SendInput(2, input, sizeof(INPUT));
    }
}

void win32::simulate_mouse_move(int x, int y) {
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dx = (x * 65535) / (GetSystemMetrics(SM_CXSCREEN) - 1);
    input.mi.dy = (y * 65535) / (GetSystemMetrics(SM_CYSCREEN) - 1);
    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
    SendInput(1, &input, sizeof(INPUT));
}

static void get_mouse_flags(std::string button, bool down, DWORD &flags,
                            DWORD &data) {
    std::transform(button.begin(), button.end(), button.begin(), ::tolower);
    flags = 0;
    data = 0;

    if (button == "left") {
        flags = down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
    } else if (button == "right") {
        flags = down ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
    } else if (button == "middle") {
        flags = down ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
    } else if (button == "x1") {
        flags = down ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP;
        data = XBUTTON1;
    } else if (button == "x2") {
        flags = down ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP;
        data = XBUTTON2;
    }
}

void win32::simulate_mouse_click(std::string button) {
    simulate_mouse_down(button);
    simulate_mouse_up(button);
}

void win32::simulate_mouse_down(std::string button) {
    DWORD flags, data;
    get_mouse_flags(button, true, flags, data);

    if (flags == 0)
        return;

    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = flags;
    input.mi.mouseData = data;
    SendInput(1, &input, sizeof(INPUT));
}

void win32::simulate_mouse_up(std::string button) {
    DWORD flags, data;
    get_mouse_flags(button, false, flags, data);

    if (flags == 0)
        return;

    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = flags;
    input.mi.mouseData = data;
    SendInput(1, &input, sizeof(INPUT));
}

struct WinToastEventHandler : public IWinToastHandler {
    std::function<void(int)> on_activate = [](int) {};
    std::function<void(WinToastDismissalReason)> on_dismiss =
        [](WinToastDismissalReason) {};
    void toastActivated() const override {}
    void toastActivated(int actionIndex) const override {
        on_activate(actionIndex);
    }
    void toastActivated(const char *) const override {}

    void toastDismissed(WinToastDismissalReason state) const override {
        on_dismiss(state);
    }

    void toastFailed() const override {}
} winToastEventHandler;

static void wintoast_init() {
    static bool initialized = false;
    if (initialized)
        return;
    initialized = true;
    WinToast::instance()->setAppName(L"Breeze");
    WinToast::instance()->setAppUserModelId(L"breeze-shell");
    WinToast::instance()->initialize();
}

void notification::send_basic(std::string message) {
    wintoast_init();

    WinToastTemplate templ(WinToastTemplate::ImageAndText02);
    templ.setTextField(utf8_to_wstring(message), WinToastTemplate::FirstLine);
    WinToast::instance()->showToast(templ, &winToastEventHandler);
}
void notification::send_with_image(std::string message, std::string icon_path) {
    wintoast_init();

    WinToastTemplate templ(WinToastTemplate::ImageAndText02);
    templ.setTextField(utf8_to_wstring(message), WinToastTemplate::FirstLine);
    templ.setImagePath(utf8_to_wstring(icon_path));
    WinToast::instance()->showToast(templ, &winToastEventHandler);
}
void notification::send_title_text(std::string title, std::string message,
                                   std::string image_path) {
    wintoast_init();
    WinToastTemplate templ(WinToastTemplate::ImageAndText02);
    templ.setTextField(utf8_to_wstring(title), WinToastTemplate::FirstLine);
    templ.setTextField(utf8_to_wstring(message), WinToastTemplate::SecondLine);
    if (!image_path.empty())
        templ.setImagePath(utf8_to_wstring(image_path));
    WinToast::instance()->showToast(templ, &winToastEventHandler);
}
void notification::send_with_buttons(
    std::string title, std::string message,
    std::vector<std::pair<std::string, std::function<void()>>> buttons) {
    wintoast_init();
    WinToastTemplate templ(WinToastTemplate::Text02);
    templ.setTextField(utf8_to_wstring(title), WinToastTemplate::FirstLine);
    templ.setTextField(utf8_to_wstring(message), WinToastTemplate::SecondLine);

    for (const auto &[button_text, callback] : buttons) {
        templ.addAction(utf8_to_wstring(button_text));
    }

    auto *handler = new WinToastEventHandler();
    handler->on_activate = [buttons, handler](int actionIndex) {
        if (actionIndex >= 0 && actionIndex < buttons.size()) {
            buttons[actionIndex].second();
        }
        delete handler;
    };
    handler->on_dismiss = [=](WinToastEventHandler::WinToastDismissalReason) {};

    WinToast::instance()->showToast(templ, handler);
}

void menu_controller::append_widget_after(
    std::shared_ptr<mb_shell::js::breeze_ui::js_widget> widget,
    int after_index) {
    if (!valid())
        return;
    auto m = $menu.lock();
    if (!m)
        return;
    m->children_dirty = true;
    while (after_index < 0) {
        after_index = m->children.size() + after_index + 1;
    }

    auto widget_wrapper =
        std::make_shared<mb_shell::menu_item_custom_widget>(widget->$widget);

    if (after_index >= m->children.size()) {
        m->children.push_back(widget_wrapper);
    } else {
        m->children.insert(m->children.begin() + after_index, widget_wrapper);
    }

    m->update_icon_width();
}
std::string win32::string_from_resid(std::string str) {
    return res_string_loader::string_from_id_string(str);
}
std::vector<std::string> win32::all_resids_from_string(std::string str) {
    return res_string_loader::get_all_ids_of_string(utf8_to_wstring(str));
}
void menu_controller::show_at(int x, int y) {
    if (!valid() || !$menu_detached)
        return;

    std::promise<void> prom;
    auto fut = prom.get_future();
    std::thread([x, y, this, &prom]() mutable {
        std::ignore = mb_shell::track_popup_menu(
            menu{
                .is_top_level = true,
            },
            x, y,
            [this, &prom](menu_render &render) {
                auto menu_new =
                    render.rt->root->get_child<mouse_menu_widget_main>()
                        ->menu_wid;
                menu_new->children = $menu_detached->children;
                menu_new->children = $menu_detached->children;
                $menu = menu_new;
                $menu_detached = nullptr;
                prom.set_value();
            },
            false);
    }).detach();
    fut.wait();
}
void menu_controller::show_at_cursor() {
    POINT p;
    if (GetCursorPos(&p)) {
        show_at(p.x, p.y);
    }
}
void breeze::allow_js_reload(bool can) {
    mb_shell::context_menu_hooks::block_js_reload.fetch_add(can ? -1 : 1);
}
std::optional<std::shared_ptr<breeze_ui::js_widget>>
menu_controller::get_widget() {
    if (!valid())
        return std::nullopt;
    auto m = $menu.lock();
    if (!m)
        return std::nullopt;

    return std::make_shared<breeze_ui::js_widget>(m);
}
void screenside_button_controller::add_button(std::string icon_svg,
                                              std::function<void()> on_click) {
    if ($menu.expired())
        return;
    auto menu = $menu.lock();
    auto button =
        std::make_shared<screenside_button_group_widget::button_widget>(
            icon_svg);
    button->on_click = on_click;
    if (auto group = menu->get_child<screenside_button_group_widget>()) {
        group->children.push_back(button);
        group->children_dirty = true;
    }
}
std::string breeze::current_process_name() {
    static std::string process_name = []() {
        wchar_t buffer[MAX_PATH];
        GetModuleFileNameW(NULL, buffer, MAX_PATH);
        return wstring_to_utf8(
            std::filesystem::path(buffer).filename().wstring());
    }();

    return process_name;
}
std::string breeze::current_process_path() {
    static std::string process_path = []() {
        wchar_t buffer[MAX_PATH];
        GetModuleFileNameW(NULL, buffer, MAX_PATH);
        return wstring_to_utf8(std::filesystem::path(buffer).wstring());
    }();

    return process_path;
}
bool breeze::can_reload_js() {
    return mb_shell::context_menu_hooks::block_js_reload.load() <= 0;
}
void breeze::crash_cpu_exception() {
    int *x = (int *)0xffff;
    (*x)++;
}
void breeze::crash_cpp_exception() {
    throw std::runtime_error("Crash!");
}
} // namespace mb_shell::js
