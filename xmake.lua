set_project("shell")
local version = "0.1.32"

option("asan")
    set_default(false)
    set_showmenu(true)
    set_description("Enable AddressSanitizer (ASan) support")
option_end()

set_exceptions("cxx")
set_languages("c++2b")
set_warnings("all") 
add_rules("plugin.compile_commands.autoupdate", {outputdir = "build"})
add_rules("mode.releasedbg")

includes("dependencies/blook.lua")
includes("dependencies/breeze-ui.lua")

set_runtimes("MT")
add_requires("breeze-glfw", {alias = "glfw"})
add_requires("blook 3524a931af49be471840e5312fb0c18e888706fd", "glad",
    "reflect-cpp", "wintoast v1.3.1", "cpptrace v0.8.3", "breeze-ui")

if has_config("asan") then
    add_defines("_DISABLE_VECTOR_ANNOTATION", "_DISABLE_STRING_ANNOTATION", "_ASAN_")
end

add_requires("yalantinglibs b82a21925958b6c50deba3aa26a2737cdb814e27", {
    configs = {
        ssl = true
    }
})

add_requireconfs("**.cinatra", {
    override = true,
    version = "e329293f6705649a6f1e8847ec845a7631179bb8"
})

add_requireconfs("**.async_simple", {
    override = true,
    version = "18f3882be354d407af0f0674121dcddaeff36e26"
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
    add_packages("blook", "reflect-cpp", "wintoast", "cpptrace", "yalantinglibs", "breeze-ui")
    add_syslinks("oleacc", "ole32", "oleaut32", "uuid", "comctl32", "comdlg32", "gdi32", "user32", "shell32", "kernel32", "advapi32", "psapi", "Winhttp", "dbghelp")
    add_rules("utils.bin2c", {
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
    add_syslinks("psapi", "user32", "shell32", "kernel32", "advapi32")
    add_files("src/inject/*.cc")
    add_packages("breeze-ui")
    set_basename("breeze")
    set_encodings("utf-8")
    add_rules("utils.bin2c", {
        extensions = {".png"}
    })
    add_files("resources/icon-small.png")
    add_ldflags("/subsystem:windows")
    
