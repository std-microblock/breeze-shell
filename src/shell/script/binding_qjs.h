// This file is generated by scripts/bindgen/index.ts
// Do not modify this file manually!

#pragma once
#include "binding_types.h"
#include "quickjs.h"
#include "quickjspp.hpp"

template <typename T>
struct js_bind {
    static void bind(qjs::Context::Module &mod) {}
};

template <> struct qjs::js_traits<mb_shell::js::example_struct_jni> {
    static mb_shell::js::example_struct_jni unwrap(JSContext *ctx, JSValueConst v) {
        mb_shell::js::example_struct_jni obj;
    
        obj.a = js_traits<int>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "a"));
        
        obj.b = js_traits<int>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "b"));
        
        obj.c = js_traits<std::string>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "c"));
        
        return obj;
    }

    static JSValue wrap(JSContext *ctx, const mb_shell::js::example_struct_jni &val) noexcept {
        JSValue obj = JS_NewObject(ctx);
    
        JS_SetPropertyStr(ctx, obj, "a", js_traits<int>::wrap(ctx, val.a));
        
        JS_SetPropertyStr(ctx, obj, "b", js_traits<int>::wrap(ctx, val.b));
        
        JS_SetPropertyStr(ctx, obj, "c", js_traits<std::string>::wrap(ctx, val.c));
        
        return obj;
    }
};
template<> struct js_bind<mb_shell::js::example_struct_jni> {
    static void bind(qjs::Context::Module &mod) {
        mod.class_<mb_shell::js::example_struct_jni>("example_struct_jni")
            .constructor<>()
                .fun<&mb_shell::js::example_struct_jni::add1>("add1")
                .fun<&mb_shell::js::example_struct_jni::add2>("add2")
                .fun<&mb_shell::js::example_struct_jni::a>("a")
                .fun<&mb_shell::js::example_struct_jni::b>("b")
                .fun<&mb_shell::js::example_struct_jni::c>("c")
            ;
    }

};
    
template <> struct qjs::js_traits<mb_shell::js::folder_view_controller> {
    static mb_shell::js::folder_view_controller unwrap(JSContext *ctx, JSValueConst v) {
        mb_shell::js::folder_view_controller obj;
    
        obj.current_path = js_traits<std::string>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "current_path"));
        
        obj.focused_file_path = js_traits<std::string>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "focused_file_path"));
        
        obj.selected_files = js_traits<std::vector<std::string>>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "selected_files"));
        
        return obj;
    }

    static JSValue wrap(JSContext *ctx, const mb_shell::js::folder_view_controller &val) noexcept {
        JSValue obj = JS_NewObject(ctx);
    
        JS_SetPropertyStr(ctx, obj, "current_path", js_traits<std::string>::wrap(ctx, val.current_path));
        
        JS_SetPropertyStr(ctx, obj, "focused_file_path", js_traits<std::string>::wrap(ctx, val.focused_file_path));
        
        JS_SetPropertyStr(ctx, obj, "selected_files", js_traits<std::vector<std::string>>::wrap(ctx, val.selected_files));
        
        return obj;
    }
};
template<> struct js_bind<mb_shell::js::folder_view_controller> {
    static void bind(qjs::Context::Module &mod) {
        mod.class_<mb_shell::js::folder_view_controller>("folder_view_controller")
            .constructor<>()
                .fun<&mb_shell::js::folder_view_controller::change_folder>("change_folder")
                .fun<&mb_shell::js::folder_view_controller::focus_file>("focus_file")
                .fun<&mb_shell::js::folder_view_controller::open_file>("open_file")
                .fun<&mb_shell::js::folder_view_controller::open_folder>("open_folder")
                .fun<&mb_shell::js::folder_view_controller::scroll_to_file>("scroll_to_file")
                .fun<&mb_shell::js::folder_view_controller::refresh>("refresh")
                .fun<&mb_shell::js::folder_view_controller::select_all>("select_all")
                .fun<&mb_shell::js::folder_view_controller::select_none>("select_none")
                .fun<&mb_shell::js::folder_view_controller::invert_selection>("invert_selection")
                .fun<&mb_shell::js::folder_view_controller::copy>("copy")
                .fun<&mb_shell::js::folder_view_controller::cut>("cut")
                .fun<&mb_shell::js::folder_view_controller::paste>("paste")
                .fun<&mb_shell::js::folder_view_controller::current_path>("current_path")
                .fun<&mb_shell::js::folder_view_controller::focused_file_path>("focused_file_path")
                .fun<&mb_shell::js::folder_view_controller::selected_files>("selected_files")
            ;
    }

};
    
template <> struct qjs::js_traits<mb_shell::js::window_titlebar_controller> {
    static mb_shell::js::window_titlebar_controller unwrap(JSContext *ctx, JSValueConst v) {
        mb_shell::js::window_titlebar_controller obj;
    
        obj.is_click_in_titlebar = js_traits<bool>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "is_click_in_titlebar"));
        
        obj.title = js_traits<std::string>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "title"));
        
        obj.executable_path = js_traits<std::string>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "executable_path"));
        
        obj.hwnd = js_traits<int>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "hwnd"));
        
        obj.x = js_traits<int>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "x"));
        
        obj.y = js_traits<int>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "y"));
        
        obj.width = js_traits<int>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "width"));
        
        obj.height = js_traits<int>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "height"));
        
        obj.maximized = js_traits<bool>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "maximized"));
        
        obj.minimized = js_traits<bool>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "minimized"));
        
        obj.focused = js_traits<bool>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "focused"));
        
        obj.visible = js_traits<bool>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "visible"));
        
        return obj;
    }

    static JSValue wrap(JSContext *ctx, const mb_shell::js::window_titlebar_controller &val) noexcept {
        JSValue obj = JS_NewObject(ctx);
    
        JS_SetPropertyStr(ctx, obj, "is_click_in_titlebar", js_traits<bool>::wrap(ctx, val.is_click_in_titlebar));
        
        JS_SetPropertyStr(ctx, obj, "title", js_traits<std::string>::wrap(ctx, val.title));
        
        JS_SetPropertyStr(ctx, obj, "executable_path", js_traits<std::string>::wrap(ctx, val.executable_path));
        
        JS_SetPropertyStr(ctx, obj, "hwnd", js_traits<int>::wrap(ctx, val.hwnd));
        
        JS_SetPropertyStr(ctx, obj, "x", js_traits<int>::wrap(ctx, val.x));
        
        JS_SetPropertyStr(ctx, obj, "y", js_traits<int>::wrap(ctx, val.y));
        
        JS_SetPropertyStr(ctx, obj, "width", js_traits<int>::wrap(ctx, val.width));
        
        JS_SetPropertyStr(ctx, obj, "height", js_traits<int>::wrap(ctx, val.height));
        
        JS_SetPropertyStr(ctx, obj, "maximized", js_traits<bool>::wrap(ctx, val.maximized));
        
        JS_SetPropertyStr(ctx, obj, "minimized", js_traits<bool>::wrap(ctx, val.minimized));
        
        JS_SetPropertyStr(ctx, obj, "focused", js_traits<bool>::wrap(ctx, val.focused));
        
        JS_SetPropertyStr(ctx, obj, "visible", js_traits<bool>::wrap(ctx, val.visible));
        
        return obj;
    }
};
template<> struct js_bind<mb_shell::js::window_titlebar_controller> {
    static void bind(qjs::Context::Module &mod) {
        mod.class_<mb_shell::js::window_titlebar_controller>("window_titlebar_controller")
            .constructor<>()
                .fun<&mb_shell::js::window_titlebar_controller::set_title>("set_title")
                .fun<&mb_shell::js::window_titlebar_controller::set_icon>("set_icon")
                .fun<&mb_shell::js::window_titlebar_controller::set_position>("set_position")
                .fun<&mb_shell::js::window_titlebar_controller::set_size>("set_size")
                .fun<&mb_shell::js::window_titlebar_controller::maximize>("maximize")
                .fun<&mb_shell::js::window_titlebar_controller::minimize>("minimize")
                .fun<&mb_shell::js::window_titlebar_controller::restore>("restore")
                .fun<&mb_shell::js::window_titlebar_controller::close>("close")
                .fun<&mb_shell::js::window_titlebar_controller::focus>("focus")
                .fun<&mb_shell::js::window_titlebar_controller::show>("show")
                .fun<&mb_shell::js::window_titlebar_controller::hide>("hide")
                .fun<&mb_shell::js::window_titlebar_controller::is_click_in_titlebar>("is_click_in_titlebar")
                .fun<&mb_shell::js::window_titlebar_controller::title>("title")
                .fun<&mb_shell::js::window_titlebar_controller::executable_path>("executable_path")
                .fun<&mb_shell::js::window_titlebar_controller::hwnd>("hwnd")
                .fun<&mb_shell::js::window_titlebar_controller::x>("x")
                .fun<&mb_shell::js::window_titlebar_controller::y>("y")
                .fun<&mb_shell::js::window_titlebar_controller::width>("width")
                .fun<&mb_shell::js::window_titlebar_controller::height>("height")
                .fun<&mb_shell::js::window_titlebar_controller::maximized>("maximized")
                .fun<&mb_shell::js::window_titlebar_controller::minimized>("minimized")
                .fun<&mb_shell::js::window_titlebar_controller::focused>("focused")
                .fun<&mb_shell::js::window_titlebar_controller::visible>("visible")
            ;
    }

};
    
template <> struct qjs::js_traits<mb_shell::js::input_box_controller> {
    static mb_shell::js::input_box_controller unwrap(JSContext *ctx, JSValueConst v) {
        mb_shell::js::input_box_controller obj;
    
        obj.text = js_traits<std::string>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "text"));
        
        obj.placeholder = js_traits<std::string>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "placeholder"));
        
        obj.multiline = js_traits<bool>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "multiline"));
        
        obj.password = js_traits<bool>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "password"));
        
        obj.readonly = js_traits<bool>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "readonly"));
        
        obj.disabled = js_traits<bool>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "disabled"));
        
        obj.x = js_traits<int>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "x"));
        
        obj.y = js_traits<int>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "y"));
        
        obj.width = js_traits<int>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "width"));
        
        obj.height = js_traits<int>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "height"));
        
        return obj;
    }

    static JSValue wrap(JSContext *ctx, const mb_shell::js::input_box_controller &val) noexcept {
        JSValue obj = JS_NewObject(ctx);
    
        JS_SetPropertyStr(ctx, obj, "text", js_traits<std::string>::wrap(ctx, val.text));
        
        JS_SetPropertyStr(ctx, obj, "placeholder", js_traits<std::string>::wrap(ctx, val.placeholder));
        
        JS_SetPropertyStr(ctx, obj, "multiline", js_traits<bool>::wrap(ctx, val.multiline));
        
        JS_SetPropertyStr(ctx, obj, "password", js_traits<bool>::wrap(ctx, val.password));
        
        JS_SetPropertyStr(ctx, obj, "readonly", js_traits<bool>::wrap(ctx, val.readonly));
        
        JS_SetPropertyStr(ctx, obj, "disabled", js_traits<bool>::wrap(ctx, val.disabled));
        
        JS_SetPropertyStr(ctx, obj, "x", js_traits<int>::wrap(ctx, val.x));
        
        JS_SetPropertyStr(ctx, obj, "y", js_traits<int>::wrap(ctx, val.y));
        
        JS_SetPropertyStr(ctx, obj, "width", js_traits<int>::wrap(ctx, val.width));
        
        JS_SetPropertyStr(ctx, obj, "height", js_traits<int>::wrap(ctx, val.height));
        
        return obj;
    }
};
template<> struct js_bind<mb_shell::js::input_box_controller> {
    static void bind(qjs::Context::Module &mod) {
        mod.class_<mb_shell::js::input_box_controller>("input_box_controller")
            .constructor<>()
                .fun<&mb_shell::js::input_box_controller::set_text>("set_text")
                .fun<&mb_shell::js::input_box_controller::set_placeholder>("set_placeholder")
                .fun<&mb_shell::js::input_box_controller::set_position>("set_position")
                .fun<&mb_shell::js::input_box_controller::set_size>("set_size")
                .fun<&mb_shell::js::input_box_controller::set_multiline>("set_multiline")
                .fun<&mb_shell::js::input_box_controller::set_password>("set_password")
                .fun<&mb_shell::js::input_box_controller::set_readonly>("set_readonly")
                .fun<&mb_shell::js::input_box_controller::set_disabled>("set_disabled")
                .fun<&mb_shell::js::input_box_controller::focus>("focus")
                .fun<&mb_shell::js::input_box_controller::blur>("blur")
                .fun<&mb_shell::js::input_box_controller::select_all>("select_all")
                .fun<&mb_shell::js::input_box_controller::select_range>("select_range")
                .fun<&mb_shell::js::input_box_controller::set_selection>("set_selection")
                .fun<&mb_shell::js::input_box_controller::insert_text>("insert_text")
                .fun<&mb_shell::js::input_box_controller::delete_text>("delete_text")
                .fun<&mb_shell::js::input_box_controller::clear>("clear")
                .fun<&mb_shell::js::input_box_controller::text>("text")
                .fun<&mb_shell::js::input_box_controller::placeholder>("placeholder")
                .fun<&mb_shell::js::input_box_controller::multiline>("multiline")
                .fun<&mb_shell::js::input_box_controller::password>("password")
                .fun<&mb_shell::js::input_box_controller::readonly>("readonly")
                .fun<&mb_shell::js::input_box_controller::disabled>("disabled")
                .fun<&mb_shell::js::input_box_controller::x>("x")
                .fun<&mb_shell::js::input_box_controller::y>("y")
                .fun<&mb_shell::js::input_box_controller::width>("width")
                .fun<&mb_shell::js::input_box_controller::height>("height")
            ;
    }

};
    
template <> struct qjs::js_traits<mb_shell::js::js_menu_action_event_data> {
    static mb_shell::js::js_menu_action_event_data unwrap(JSContext *ctx, JSValueConst v) {
        mb_shell::js::js_menu_action_event_data obj;
    
        return obj;
    }

    static JSValue wrap(JSContext *ctx, const mb_shell::js::js_menu_action_event_data &val) noexcept {
        JSValue obj = JS_NewObject(ctx);
    
        return obj;
    }
};
template<> struct js_bind<mb_shell::js::js_menu_action_event_data> {
    static void bind(qjs::Context::Module &mod) {
        mod.class_<mb_shell::js::js_menu_action_event_data>("js_menu_action_event_data")
            .constructor<>()
            ;
    }

};
    
template <> struct qjs::js_traits<mb_shell::js::js_menu_data> {
    static mb_shell::js::js_menu_data unwrap(JSContext *ctx, JSValueConst v) {
        mb_shell::js::js_menu_data obj;
    
        obj.type = js_traits<std::optional<std::string>>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "type"));
        
        obj.name = js_traits<std::optional<std::string>>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "name"));
        
        obj.submenu = js_traits<std::optional<std::vector<mb_shell::js::js_menu_data>>>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "submenu"));
        
        obj.action = js_traits<std::optional<std::function<void (mb_shell::js::js_menu_action_event_data)>>>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "action"));
        
        obj.icon_path = js_traits<std::optional<std::string>>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "icon_path"));
        
        return obj;
    }

    static JSValue wrap(JSContext *ctx, const mb_shell::js::js_menu_data &val) noexcept {
        JSValue obj = JS_NewObject(ctx);
    
        JS_SetPropertyStr(ctx, obj, "type", js_traits<std::optional<std::string>>::wrap(ctx, val.type));
        
        JS_SetPropertyStr(ctx, obj, "name", js_traits<std::optional<std::string>>::wrap(ctx, val.name));
        
        JS_SetPropertyStr(ctx, obj, "submenu", js_traits<std::optional<std::vector<mb_shell::js::js_menu_data>>>::wrap(ctx, val.submenu));
        
        JS_SetPropertyStr(ctx, obj, "action", js_traits<std::optional<std::function<void (mb_shell::js::js_menu_action_event_data)>>>::wrap(ctx, val.action));
        
        JS_SetPropertyStr(ctx, obj, "icon_path", js_traits<std::optional<std::string>>::wrap(ctx, val.icon_path));
        
        return obj;
    }
};
template<> struct js_bind<mb_shell::js::js_menu_data> {
    static void bind(qjs::Context::Module &mod) {
        mod.class_<mb_shell::js::js_menu_data>("js_menu_data")
            .constructor<>()
                .fun<&mb_shell::js::js_menu_data::type>("type")
                .fun<&mb_shell::js::js_menu_data::name>("name")
                .fun<&mb_shell::js::js_menu_data::submenu>("submenu")
                .fun<&mb_shell::js::js_menu_data::action>("action")
                .fun<&mb_shell::js::js_menu_data::icon_path>("icon_path")
            ;
    }

};
    
template <> struct qjs::js_traits<mb_shell::js::menu_item_controller> {
    static mb_shell::js::menu_item_controller unwrap(JSContext *ctx, JSValueConst v) {
        mb_shell::js::menu_item_controller obj;
    
        return obj;
    }

    static JSValue wrap(JSContext *ctx, const mb_shell::js::menu_item_controller &val) noexcept {
        JSValue obj = JS_NewObject(ctx);
    
        return obj;
    }
};
template<> struct js_bind<mb_shell::js::menu_item_controller> {
    static void bind(qjs::Context::Module &mod) {
        mod.class_<mb_shell::js::menu_item_controller>("menu_item_controller")
            .constructor<>()
                .fun<&mb_shell::js::menu_item_controller::set_position>("set_position")
                .fun<&mb_shell::js::menu_item_controller::set_data>("set_data")
                .fun<&mb_shell::js::menu_item_controller::get_data>("get_data")
                .fun<&mb_shell::js::menu_item_controller::remove>("remove")
                .fun<&mb_shell::js::menu_item_controller::valid>("valid")
            ;
    }

};
    
template <> struct qjs::js_traits<mb_shell::js::menu_item_data> {
    static mb_shell::js::menu_item_data unwrap(JSContext *ctx, JSValueConst v) {
        mb_shell::js::menu_item_data obj;
    
        obj.type = js_traits<std::string>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "type"));
        
        obj.name = js_traits<std::optional<std::string>>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "name"));
        
        return obj;
    }

    static JSValue wrap(JSContext *ctx, const mb_shell::js::menu_item_data &val) noexcept {
        JSValue obj = JS_NewObject(ctx);
    
        JS_SetPropertyStr(ctx, obj, "type", js_traits<std::string>::wrap(ctx, val.type));
        
        JS_SetPropertyStr(ctx, obj, "name", js_traits<std::optional<std::string>>::wrap(ctx, val.name));
        
        return obj;
    }
};
template<> struct js_bind<mb_shell::js::menu_item_data> {
    static void bind(qjs::Context::Module &mod) {
        mod.class_<mb_shell::js::menu_item_data>("menu_item_data")
            .constructor<>()
                .fun<&mb_shell::js::menu_item_data::type>("type")
                .fun<&mb_shell::js::menu_item_data::name>("name")
            ;
    }

};
    
template <> struct qjs::js_traits<mb_shell::js::js_menu_context> {
    static mb_shell::js::js_menu_context unwrap(JSContext *ctx, JSValueConst v) {
        mb_shell::js::js_menu_context obj;
    
        obj.folder_view = js_traits<std::optional<std::shared_ptr<mb_shell::js::folder_view_controller>>>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "folder_view"));
        
        obj.window_titlebar = js_traits<std::optional<std::shared_ptr<mb_shell::js::window_titlebar_controller>>>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "window_titlebar"));
        
        obj.input_box = js_traits<std::optional<std::shared_ptr<mb_shell::js::input_box_controller>>>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "input_box"));
        
        return obj;
    }

    static JSValue wrap(JSContext *ctx, const mb_shell::js::js_menu_context &val) noexcept {
        JSValue obj = JS_NewObject(ctx);
    
        JS_SetPropertyStr(ctx, obj, "folder_view", js_traits<std::optional<std::shared_ptr<mb_shell::js::folder_view_controller>>>::wrap(ctx, val.folder_view));
        
        JS_SetPropertyStr(ctx, obj, "window_titlebar", js_traits<std::optional<std::shared_ptr<mb_shell::js::window_titlebar_controller>>>::wrap(ctx, val.window_titlebar));
        
        JS_SetPropertyStr(ctx, obj, "input_box", js_traits<std::optional<std::shared_ptr<mb_shell::js::input_box_controller>>>::wrap(ctx, val.input_box));
        
        return obj;
    }
};
template<> struct js_bind<mb_shell::js::js_menu_context> {
    static void bind(qjs::Context::Module &mod) {
        mod.class_<mb_shell::js::js_menu_context>("js_menu_context")
            .constructor<>()
                .fun<&mb_shell::js::js_menu_context::folder_view>("folder_view")
                .fun<&mb_shell::js::js_menu_context::window_titlebar>("window_titlebar")
                .fun<&mb_shell::js::js_menu_context::input_box>("input_box")
            ;
    }

};
    
template <> struct qjs::js_traits<mb_shell::js::menu_info_basic_js> {
    static mb_shell::js::menu_info_basic_js unwrap(JSContext *ctx, JSValueConst v) {
        mb_shell::js::menu_info_basic_js obj;
    
        obj.menu = js_traits<std::shared_ptr<mb_shell::js::menu_controller>>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "menu"));
        
        obj.context = js_traits<std::shared_ptr<mb_shell::js::js_menu_context>>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "context"));
        
        return obj;
    }

    static JSValue wrap(JSContext *ctx, const mb_shell::js::menu_info_basic_js &val) noexcept {
        JSValue obj = JS_NewObject(ctx);
    
        JS_SetPropertyStr(ctx, obj, "menu", js_traits<std::shared_ptr<mb_shell::js::menu_controller>>::wrap(ctx, val.menu));
        
        JS_SetPropertyStr(ctx, obj, "context", js_traits<std::shared_ptr<mb_shell::js::js_menu_context>>::wrap(ctx, val.context));
        
        return obj;
    }
};
template<> struct js_bind<mb_shell::js::menu_info_basic_js> {
    static void bind(qjs::Context::Module &mod) {
        mod.class_<mb_shell::js::menu_info_basic_js>("menu_info_basic_js")
            .constructor<>()
                .fun<&mb_shell::js::menu_info_basic_js::menu>("menu")
                .fun<&mb_shell::js::menu_info_basic_js::context>("context")
            ;
    }

};
    
template <> struct qjs::js_traits<mb_shell::js::menu_controller> {
    static mb_shell::js::menu_controller unwrap(JSContext *ctx, JSValueConst v) {
        mb_shell::js::menu_controller obj;
    
        return obj;
    }

    static JSValue wrap(JSContext *ctx, const mb_shell::js::menu_controller &val) noexcept {
        JSValue obj = JS_NewObject(ctx);
    
        return obj;
    }
};
template<> struct js_bind<mb_shell::js::menu_controller> {
    static void bind(qjs::Context::Module &mod) {
        mod.class_<mb_shell::js::menu_controller>("menu_controller")
            .constructor<>()
                .fun<&mb_shell::js::menu_controller::valid>("valid")
                .fun<&mb_shell::js::menu_controller::add_menu_item_after>("add_menu_item_after")
                .fun<&mb_shell::js::menu_controller::close>("close")
                .fun<&mb_shell::js::menu_controller::get_menu_items>("get_menu_items")
                .fun<&mb_shell::js::menu_controller::get_menu_item>("get_menu_item")
                .static_fun<&mb_shell::js::menu_controller::add_menu_listener>("add_menu_listener")
            ;
    }

};
    
template <> struct qjs::js_traits<mb_shell::js::clipboard> {
    static mb_shell::js::clipboard unwrap(JSContext *ctx, JSValueConst v) {
        mb_shell::js::clipboard obj;
    
        return obj;
    }

    static JSValue wrap(JSContext *ctx, const mb_shell::js::clipboard &val) noexcept {
        JSValue obj = JS_NewObject(ctx);
    
        return obj;
    }
};
template<> struct js_bind<mb_shell::js::clipboard> {
    static void bind(qjs::Context::Module &mod) {
        mod.class_<mb_shell::js::clipboard>("clipboard")
            .constructor<>()
                .static_fun<&mb_shell::js::clipboard::get_text>("get_text")
                .static_fun<&mb_shell::js::clipboard::set_text>("set_text")
            ;
    }

};
    
template <> struct qjs::js_traits<mb_shell::js::network> {
    static mb_shell::js::network unwrap(JSContext *ctx, JSValueConst v) {
        mb_shell::js::network obj;
    
        return obj;
    }

    static JSValue wrap(JSContext *ctx, const mb_shell::js::network &val) noexcept {
        JSValue obj = JS_NewObject(ctx);
    
        return obj;
    }
};
template<> struct js_bind<mb_shell::js::network> {
    static void bind(qjs::Context::Module &mod) {
        mod.class_<mb_shell::js::network>("network")
            .constructor<>()
                .static_fun<&mb_shell::js::network::get>("get")
                .static_fun<&mb_shell::js::network::post>("post")
                .static_fun<&mb_shell::js::network::get_async>("get_async")
                .static_fun<&mb_shell::js::network::post_async>("post_async")
            ;
    }

};
    
inline void bindAll(qjs::Context::Module &mod) {

    js_bind<mb_shell::js::example_struct_jni>::bind(mod);

    js_bind<mb_shell::js::folder_view_controller>::bind(mod);

    js_bind<mb_shell::js::window_titlebar_controller>::bind(mod);

    js_bind<mb_shell::js::input_box_controller>::bind(mod);

    js_bind<mb_shell::js::js_menu_action_event_data>::bind(mod);

    js_bind<mb_shell::js::js_menu_data>::bind(mod);

    js_bind<mb_shell::js::menu_item_controller>::bind(mod);

    js_bind<mb_shell::js::menu_item_data>::bind(mod);

    js_bind<mb_shell::js::js_menu_context>::bind(mod);

    js_bind<mb_shell::js::menu_info_basic_js>::bind(mod);

    js_bind<mb_shell::js::menu_controller>::bind(mod);

    js_bind<mb_shell::js::clipboard>::bind(mod);

    js_bind<mb_shell::js::network>::bind(mod);

}
