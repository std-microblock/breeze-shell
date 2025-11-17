package("quickjs-ng")
    set_homepage("https://github.com/quickjs-ng/quickjs")
    set_description("QuickJS, the Next Generation: a mighty JavaScript engine")
    set_license("MIT")

    add_urls("https://github.com/quickjs-ng/quickjs/archive/refs/tags/$(version).tar.gz",
             "https://github.com/quickjs-ng/quickjs.git", {submodules = false})

    add_versions("v0.11.0", "v0.11.0")

    add_configs("libc", {description = "Build standard library modules as part of the library", default = false, type = "boolean"})

    if is_plat("linux", "bsd") then
        add_syslinks("m", "pthread")
    end

    add_deps("cmake")
    if on_check then
        on_check("windows", function (package)
            local vs_toolset = package:toolchain("msvc"):config("vs_toolset")
            if vs_toolset then
                local vs_toolset_ver = import("core.base.semver").new(vs_toolset)
                local minor = vs_toolset_ver:minor()
                assert(minor and minor >= 30, "package(quickjs-ng) require vs_toolset >= 14.3")
            end
        end)
        on_check("wasm", "cross", function (package)
            if package:version():eq("0.11.0") then
                raise("package(quickjs-ng v0.11.0) unsupported platform")
            end
        end)
    end

    on_install("!iphoneos and (!windows or windows|!x86)", function (package)
        io.replace("CMakeLists.txt", "xcheck_add_c_compiler_flag(-Werror)", "", {plain = true})
        io.replace("CMakeLists.txt", "if(NOT WIN32 AND NOT EMSCRIPTEN)", "if(0)", {plain = true})

        local configs = {}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=Release")
        -- if package:is_plat("windows") then
        --     if package:is_debug() then
        --         -- add /debug to link flags
        --         table.insert(configs, "-DCMAKE_EXE_LINKER_FLAGS_RELEASE=\"/DEBUG\"")
        --         table.insert(configs, "-DCMAKE_SHARED_LINKER_FLAGS_RELEASE=\"/DEBUG\"")

        --         -- also add /DEBUG to cflags, and /Zi to cxxflags
        --         table.insert(configs, "-DCMAKE_C_FLAGS_RELEASE=/Zi /DEBUG")
        --         table.insert(configs, "-DCMAKE_CXX_FLAGS_RELEASE=/Zi /DEBUG")
        --     end
        -- end

        table.insert(configs, "-DBUILD_SHARED_LIBS=" .. (package:config("shared") and "ON" or "OFF"))
        table.insert(configs, "-DQJS_CONFIG_ASAN=" .. (package:config("asan") and "ON" or "OFF"))
        table.insert(configs, "-DQJS_CONFIG_MSAN=" .. (package:config("msan") and "ON" or "OFF"))
        table.insert(configs, "-DQJS_CONFIG_UBSAN=" .. (package:config("ubsan") and "ON" or "OFF"))
        table.insert(configs, "-DQJS_BUILD_LIBC=" .. (package:config("libc") and "ON" or "OFF"))

        if package:config("shared") and package:is_plat("windows") then
            table.insert(configs, "-DCMAKE_WINDOWS_EXPORT_ALL_SYMBOLS=ON")
        end
        import("package.tools.cmake").install(package, configs)

        if package:is_plat("windows") and package:is_debug() then
            local dir = package:installdir(package:config("shared") and "bin" or "lib")
            os.vcp(path.join(package:buildir(), "*.pdb"), dir)
        end
    end)

    on_test(function (package)
        assert(package:has_cfuncs("JS_NewRuntime", {includes = "quickjs.h"}))
    end)