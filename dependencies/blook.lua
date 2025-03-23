package("blook")
    set_description("A modern C++ library for hacking.")
    set_license("GPL-3.0")

    add_urls("https://github.com/std-microblock/blook.git")

    add_versions("2025.03.22", "b095c0be3817e1912e2eaa8af854d59fcdac6d14")

    add_configs("shared", {description = "Build shared library.", default = false, type = "boolean", readonly = true})

    if is_plat("windows") then
        add_syslinks("advapi32")
    end

    add_deps("zasm edd30ff31d5a1d5f68002a61dca0ebf6e3c10ed0")

    on_install("windows", function (package)
        import("package.tools.xmake").install(package, {}, {target = "blook"})
        os.cp("include", package:installdir())
    end)
