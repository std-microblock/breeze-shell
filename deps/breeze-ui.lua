package("breeze-glfw")
    set_base("glfw")
    set_urls("https://github.com/breeze-shell/glfw.git")
    add_versions("2026.03.07+1", "a79c32a7d9ef4cd8a15b5f8ccbcdf9510c48da03")

local BREEZE_UI_VER = "2026.10.05+1"
local BREEZE_UI_HASH = "e2248ddfafb12b5ef977060b93f3b053ad81c9ec"
local BREEZE_UI_LOCAL_PATH = path.join(os.scriptdir(), "..", "..", "breeze-ui")
local USE_LOCAL_BREEZE_UI = os.exists(BREEZE_UI_LOCAL_PATH)

package("breeze-nanosvg")
    if USE_LOCAL_BREEZE_UI then
        set_sourcedir(BREEZE_UI_LOCAL_PATH)
    else
        add_urls("https://github.com/std-microblock/breeze-ui.git")
        add_versions(BREEZE_UI_VER, BREEZE_UI_HASH)
    end

    set_kind("library", {headeronly = true})
    set_description("The breeze-nanosvg package")

    on_install("windows", function (package)
        import("package.tools.xmake").install(package)
    end)

package("breeze-nanovg")
    if USE_LOCAL_BREEZE_UI then
        set_sourcedir(BREEZE_UI_LOCAL_PATH)
    else
        add_urls("https://github.com/std-microblock/breeze-ui.git")
        add_versions(BREEZE_UI_VER, BREEZE_UI_HASH)
    end

    set_description("The breeze-nanovg package")

    add_configs("shared", {description = "Build shared library.", default = false, type = "boolean", readonly = true})

    on_install("windows", function (package)
        import("package.tools.xmake").install(package)
    end)


package("breeze-ui")
    if USE_LOCAL_BREEZE_UI then
        set_sourcedir(BREEZE_UI_LOCAL_PATH)
    else
        add_urls("https://github.com/std-microblock/breeze-ui.git")
        add_versions(BREEZE_UI_VER, BREEZE_UI_HASH)
    end
    add_deps("breeze-glfw", "glad 0.1.36", "nanovg", "breeze-nanosvg", "simdutf", {
        public = true
    })
    add_configs("shared", {description = "Build shared library.", default = false, type = "boolean", readonly = true})

    if is_plat("windows") then
        add_syslinks("dwmapi", "imm32", "shcore", "windowsapp", "CoreMessaging")
    end

    on_install("windows", function (package)
        import("package.tools.xmake").install(package)
    end)

