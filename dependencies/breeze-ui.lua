package("breeze-glfw")
    set_base("glfw")
    set_urls("https://github.com/breeze-shell/glfw.git")
    add_versions("2026.03.07+1", "a79c32a7d9ef4cd8a15b5f8ccbcdf9510c48da03")

local BREEZE_UI_VER = "2026.03.07+10"
local BREEZE_UI_HASH = "fa18aee44308166f03cefe28a3222057d20c397f"

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

