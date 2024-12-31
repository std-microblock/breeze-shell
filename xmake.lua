set_project("shell")
set_policy("compatibility.version", "3.0")

set_languages("c++23")
set_warnings("all") 
add_rules("plugin.compile_commands.autoupdate", {outputdir = "build"})
add_rules("mode.debug", "mode.release")

includes("blook.lua")
add_requires("blook", "glfw", "bgfx", "stb")
set_runtimes("MT")
add_rules("mode.releasedbg")
target("ui")
    set_kind("static")
    add_packages("glfw", "bgfx", "stb", {
        public = true
    })
    add_files("src/ui/*.cc", "src/ui/nanovg/*.cpp")
    add_headerfiles("src/ui/*.h")
    add_includedirs("src/ui", {
        public = true
    })

target("ui_test")
    set_kind("binary")
    add_deps("ui")
    add_files("src/ui_test/*.cc")

target("shell")
    set_kind("shared")
    add_packages("blook", "skia", "glfw")
    add_files("src/*.cc")