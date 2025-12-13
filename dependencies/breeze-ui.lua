package("breeze-glfw")
    set_base("glfw")
    set_urls("https://github.com/breeze-shell/glfw.git")
    add_versions("latest", "master")

local BREEZE_UI_VER = "2025.12.13"
local BREEZE_UI_HASH = "09e83f05a0134dfd881700fcdf8b2954382ed66d"

package("breeze-nanosvg")
    add_urls("https://github.com/std-microblock/breeze-ui.git")
    add_versions(BREEZE_UI_VER, BREEZE_UI_HASH)

    set_kind("library", {headeronly = true})
    set_description("The breeze-nanosvg package")

    on_install("windows", function (package)
        import("package.tools.xmake").install(package)
    end)

package("breeze-nanovg")
    add_urls("https://github.com/std-microblock/breeze-ui.git")
    add_versions(BREEZE_UI_VER, BREEZE_UI_HASH)

    set_description("The breeze-nanovg package")

    add_configs("shared", {description = "Build shared library.", default = false, type = "boolean", readonly = true})

    on_install("windows", function (package)
        import("package.tools.xmake").install(package)
    end)


package("breeze-ui")
    add_urls("https://github.com/std-microblock/breeze-ui.git")
    add_versions(BREEZE_UI_VER, BREEZE_UI_HASH)
    add_deps("breeze-glfw", "glad", "nanovg", "breeze-nanosvg", {
        public = true
    })
    add_configs("shared", {description = "Build shared library.", default = false, type = "boolean", readonly = true})

    if is_plat("windows") then
        add_syslinks("dwmapi", "shcore")
    end

    on_install("windows", function (package)
        import("package.tools.xmake").install(package)
    end)

