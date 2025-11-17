package("breeze-glfw")
    set_base("glfw")
    set_urls("https://github.com/breeze-shell/glfw.git")
    add_versions("latest", "master")

package("breeze-ui")
    add_urls("https://github.com/std-microblock/breeze-ui.git")
    add_versions("2025.11.17", "29a3969461bb4feea3e6364db220c37282e44366")
    add_deps("breeze-glfw", "nanovg", "glad", "nanosvg")
    add_configs("shared", {description = "Build shared library.", default = false, type = "boolean", readonly = true})

    if is_plat("windows") then
        add_syslinks("dwmapi", "shcore")
    end

    on_install("windows", function (package)
        import("package.tools.xmake").install(package, {}, {target = "breeze_ui"})
    end)
