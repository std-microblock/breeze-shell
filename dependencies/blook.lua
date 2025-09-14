package("blook")
    set_description("A modern C++ library for hacking.")
    set_license("GPL-3.0")

    add_urls("https://github.com/std-microblock/blook.git")

    add_versions("2025.09.14", "966cd3269d055f04b725e40fc0670b8c212bd97a")

    add_configs("shared", {description = "Build shared library.", default = false, type = "boolean", readonly = true})

    if is_plat("windows") then
        add_syslinks("advapi32")
    end

    add_deps("zasm c239a78b51c1b0060296193174d78b802f02a618")

    on_install("windows", function (package)
        import("package.tools.xmake").install(package, {}, {target = "blook"})
    end)
