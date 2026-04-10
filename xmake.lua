set_project("shell")
local version = "0.1.33"

option("asan")
    set_default(false)
    set_showmenu(true)
    set_description("Enable AddressSanitizer (ASan) support")
option_end()

set_exceptions("cxx")
set_languages("c++2b", "c11")
set_warnings("all") 
add_rules("plugin.compile_commands.autoupdate", {outputdir = "build"})
add_rules("mode.releasedbg")

includes("deps/blook.lua")
includes("deps/breeze-ui.lua")
includes("deps/sentry-native.lua")
includes("deps/yalantinglibs.lua")
includes("deps/breeze-js.lua")

add_requires("sentry-native", {
    configs = {
        backend = "breakpad"
    }
})
add_requires("breeze-glfw", {alias = "glfw"})
add_requires("blook 9c24a0b6e7c7055adcd8f440a558f84f831e4f0f", "glad",
    "reflect-cpp", "wintoast v1.3.1", "breeze-ui", "watcher", "fmt", "spdlog", "breeze-js-runtime")

if has_config("asan") then
    add_defines("_DISABLE_VECTOR_ANNOTATION", "_DISABLE_STRING_ANNOTATION", "_ASAN_")
end

add_requires("yalantinglibs", {
    configs = {
        ssl = true
    }
})

target("ui_test")
    set_default(false)
    set_kind("binary")
    add_packages("breeze-ui")
    add_files("src/ui_test/*.cc")
    set_encodings("utf-8")
    add_tests("defualt")

target("shell")
    set_kind("shared")
    add_headerfiles("src/shell/**.h")
    add_includedirs("src/", {
        public = true
    })

    add_includedirs("src/shell/script/quickjs")

    add_defines("NOMINMAX", "WIN32_LEAN_AND_MEAN")
    add_packages("blook", "reflect-cpp", "wintoast", "yalantinglibs", "breeze-ui", "sentry-native", "watcher", "spdlog", "fmt", "breeze-js-runtime")
    add_syslinks("oleacc", "ole32", "oleaut32", "uuid", "comctl32", "comdlg32", "gdi32", "user32", "shell32", "kernel32", "advapi32", "psapi", "Winhttp", "dbghelp")
    add_rules("utils.bin2obj", {
        extensions = {".js"}
    })
    set_version(version)
    set_configdir("src/shell")
    add_configfiles("src/shell/build_info.h.in")
    on_config(function (package)
        local git_commit_hash = os.iorun("git rev-parse --short HEAD"):trim()
        local git_branch_name = os.iorun("git describe --all"):trim()
        local build_date_time = os.date("%Y-%m-%d %H:%M:%S")
        package:set("configvar", "GIT_COMMIT_HASH", git_commit_hash or "null")
        package:set("configvar", "GIT_BRANCH_NAME", git_branch_name or "null")
        package:set("configvar", "BUILD_DATE_TIME", build_date_time)
    end)
    on_run(function (target)
        if is_host("windows") then
            local cmd = "rundll32.exe " .. target:targetfile() .. ",func"
            os.exec(cmd)
        end
    end)
    add_files("src/shell/script/script.js")
    add_files("src/shell/**.cc", "src/shell/**.c")
    set_encodings("utf-8")

    if has_config("asan") then
        set_policy("build.sanitizer.address", true)
    end

target("asan_test")
    set_kind("binary")
    add_deps("shell")
    add_files("src/asan/asan_main.cc")
    if has_config("asan") then
        set_policy("build.sanitizer.address", true)
    end

target("inject")
    set_kind("binary")
    add_syslinks("psapi", "user32", "shell32", "kernel32", "advapi32", "taskschd", "ole32", "oleaut32", "taskschd", "comsupp")
    add_files("src/inject/*.cc")
    add_packages("breeze-ui", "spdlog", "fmt")
    set_basename("breeze")
    set_encodings("utf-8")
    add_rules("utils.bin2c", {
        extensions = {".png"}
    })
    add_files("resources/icon-small.png")
    add_ldflags("/subsystem:windows")
    
