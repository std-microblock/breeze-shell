set_project("shell")
set_policy("compatibility.version", "3.0")

set_languages("c++23")
set_warnings("all") 
add_rules("plugin.compile_commands.autoupdate", {outputdir = "build"})
add_rules("mode.debug", "mode.release")

includes("dependencies/blook.lua")
includes("dependencies/glfw.lua")
includes("dependencies/reflect-cpp.lua")
add_requires("blook", "glfw", "nanovg", "glad", "quickjs-ng", "nanosvg", "reflect-cpp")
set_runtimes("MT")
add_rules("mode.releasedbg")
add_rules("mode.minsizerel")

target("ui")
    set_kind("static")
    add_packages("glfw", "glad", "nanovg", "nanosvg", {
        public = true
    })
    add_syslinks("dwmapi")
    add_files("src/ui/*.cc")
    add_headerfiles("src/ui/*.h")
    add_includedirs("src/ui", {
        public = true
    })
    set_encodings("utf-8")

target("ui_test")
    set_kind("binary")
    add_deps("ui")
    add_files("src/ui_test/*.cc")
    set_encodings("utf-8")

target("shell")
    set_kind("shared")
    add_packages("blook", "quickjs-ng", "reflect-cpp")
    add_deps("ui")
    add_syslinks("oleacc", "ole32", "oleaut32", "uuid", "comctl32", "comdlg32", "gdi32", "user32", "shell32", "kernel32", "advapi32", "psapi", "Winhttp")
    add_rules("utils.bin2c", {
        extensions = {".js"}
    })
    add_files("src/shell/script/script.js")

    add_files("src/shell/**/*.cc", "src/shell/*.cc", "src/shell/**/*.c")
    set_encodings("utf-8")

target("inject")
    set_kind("binary")
    add_syslinks("psapi", "user32", "shell32", "kernel32", "advapi32")
    add_files("src/inject/*.cc")
    add_deps("ui")
    set_basename("breeze")
    set_encodings("utf-8")
    add_rules("utils.bin2c", {
        extensions = {".png"}
    })
    add_files("resources/icon-small.png")
    -- set_policy("windows.manifest.uac", "admin")
    add_ldflags("/subsystem:windows")
    