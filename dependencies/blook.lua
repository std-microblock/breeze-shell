package("blook")
    set_description("A modern C++ library for hacking.")
    set_license("GPL-3.0")

    add_urls("https://github.com/std-microblock/blook.git")

    add_configs("shared", {description = "Build shared library.", default = false, type = "boolean", readonly = true})

    if is_plat("windows") then
        add_syslinks("advapi32")
    end

    add_deps("zasm 2025.03.02")

    on_install("windows", function (package)
        import("package.tools.xmake").install(package, {}, {target = "blook"})
    end)
