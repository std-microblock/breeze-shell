package("sentry-native")
    set_homepage("https://sentry.io/welcome/")
    set_description("Sentry SDK for C, C++ and native applications.")
    set_license("MIT")

    set_urls("https://github.com/getsentry/sentry-native/releases/download/$(version)/sentry-native.zip",
             "https://github.com/getsentry/sentry-native.git")

    add_versions("0.13.1", "430bcf38465f29b5b167c4bbeb1d955cda0e40efb73da224b52675e89c7a984e")
    add_versions("0.12.8", "d668da4c13052d98b3920e3731c7d2166f9b0b7113b603c751c660eb567f3248")
    add_versions("0.12.2", "d265d26e761dfdfc3ce3b2f1916c48da316fe2147981e23182ce933e4b0835b6")
    add_versions("0.12.0", "3bf6eebb7dcc9c99267746324734a15164ba0058d67f690e315d47ee0bd8e953")
    add_versions("0.11.3", "6a4ccd2bf91320ca84169b322cbbfe5a0d13f0b4ee45bb4adf93bd1c4c59a08a")
    add_versions("0.11.2", "3f6a5ca384096fa1a9cc9624ec24fe5490f0630bb11302d9006cd522f4f6c5a3")
    add_versions("0.11.1", "04c80503cfaf0904f3adf43f97cea4cc6bdd4c21707c093ee0ed34e7a3f8e3e7")
    add_versions("0.10.1", "ab49c03879d83462cfca95abeaf0cb08fb2b54f6c2bbc1962dcded272b009272")
    add_versions("0.9.1", "e5349b1a233ac52291e54cba3a6d028781d8173e8b3cd759f17cd27769f02eab")
    add_versions("0.8.3", "26a3f2118b5fde469659f5c48eb8cdc70b7a43aea8d2bdf9efb0d6fa6ac36cb6")
    add_versions("0.8.1", "a7fe694b36fa61903704f93c6aff79b0bb5b27726b1075a47855b6ed58028108")
    add_versions("0.7.20", "bf8afca08506cd3f48c273ccf75bee37b030392369317afc40188bf478aa6902")
    add_versions("0.7.17", "c1341a0ac02440db65f41b968a46979ceab8de765c2407efb61a99511346e098")
    add_versions("0.7.16", "410bf23c894c5d3a43945c3ab015e314584753efab05ba8f56756dfe3cecf6da")
    add_versions("0.7.15", "9880614984c75fc6ed1967b7aa29aebbea2f0c88f2d7c707b18391b5632091c0")
    add_versions("0.7.12", "03c99ef84992fddd37f79c63ae78a69ec49b1b1d7598c7a7c5d8e6742b97ea0a")
    add_versions("0.7.11", "7fb41a8e5270168958d867504f503beb014035f382edaa07132be65348df27e0")
    add_versions("0.7.10", "b7f7b5002cf7a4c614736ac294351da499db4f7fe155a452d527e69badc672bc")
    add_versions("0.7.9", "d01f66125e1fb80c02668d2ea6b908987323d3f477d69332ef21506a62606d40")
    add_versions("0.7.6", "42180ad933a3a2bd86a1649ed0f1a41df20e783ce17c5cb1f86775f7caf06bc1")
    add_versions("0.7.5", "d9f1b44753fae3e9462aa1e6fd3021cb0ff2f51c1a1fa02b34510e3bc311f429")
    add_versions("0.7.2", "afb44d5cc4e0ec5f2e8068132c68256959188f6bf015e1837e7cc687014b9c70")
    add_versions("0.7.1", "c450a064b0dbb1883a355455db2b6469abef59c04686a53719384bbc7ff507d3")
    add_versions("0.7.0", "4dfccc879a81771b9da1c335947ffc9e5987ca3d16b3035efa2c66a06f727543")
    add_versions("0.6.7", "37d7880f837c85d0b19cac106b631c7b4524ff13f11cd31e8337da10842ea779")
    add_versions("0.6.6", "7a98467c0b2571380a3afc5e681cb13aa406a709529be12d74610b0015ccde0c")
    add_versions("0.6.5", "5f74a5c5c3abc6e1e7825d3306be9e3b3fd4e0f586f3cf7e86607d6f56a71995")
    add_versions("0.6.4", "e00278bf9a4821bb4008985a5a552a84aba6ebb06d3f9e828082fcbf06b04a38")
    add_versions("0.6.3", "6b515c17a9b860ea47c6a5fd7abdfdc89b4b8cbc654c23a8bb42a39bfcb87ad9")
    add_versions("0.5.0", "87e67ad783a7ec4476b0eb4742bd40fe5a1e2435")
    add_versions("0.4.15", "ae3ac4efa76d431d8734d7b0b1bf9bbedaf2cbdb18dfc5c95e2411a67808cf29")

    add_deps("cmake")

    add_configs("backend", {description = "Set the backend of sentry to use", type = "string"})

    if is_plat("windows") then
        add_syslinks("dbghelp", "winhttp", "shlwapi", "advapi32", "version", "Gdi32", "User32")
    elseif is_plat("linux") then
        add_deps("libcurl")
        add_syslinks("dl", "pthread", "rt")
    elseif is_plat("android") then
        add_syslinks("dl", "log")
    elseif is_plat("macosx") then
        add_deps("libcurl")
        add_frameworks("CoreText", "CoreGraphics", "SystemConfiguration", "CoreFoundation", "Foundation", "IOKit")
        add_syslinks("bsm")
    end

    on_check(function (package)
        assert(package:check_cxxsnippets({test = [[
            #include <ranges>
            template<typename T>
            concept CR = std::ranges::contiguous_range<T>;
            void test() {}
        ]]}, {configs = {languages = "c++20"}}), "package(sentry-native) Require at least C++20.")
    end)

    on_load("windows", "linux", "macosx", function (package)
        if not package:config("shared") then
            package:add("defines", "SENTRY_BUILD_STATIC")
        end

        local backend
        if package:is_plat("linux") then -- linux defaults to breakpad before 0.7 and then defaults to crashpad
            backend = package:version() and package:version():ge("0.7") and "crashpad" or "breakpad"
        end
        if package:config("backend") == "crashpad" then
            backend = "crashpad"
        elseif package:config("backend") == "breakpad" then
            backend = "breakpad"
        end

        if backend == "crashpad" then
            package:add("links", "sentry", "crashpad_client", "crashpad_util", "crashpad_minidump", "crashpad_handler_lib", "mini_chromium", "crashpad_tools", "crashpad_compat", "crashpad_snapshot")
            if package:version():ge("0.12.8") then
                package:add("links", "crashpad_mpack")
            end
            package:add("deps", "zlib")
        elseif backend == "breadpad" then
            package:add("links", "sentry", "breakpad_client")
        end

        if package:is_plat("linux") and package:version():ge("0.13.0") then
            package:add("deps", "libunwind")
        end
    end)

    on_install("windows|x86", "windows|x64", "linux", "macosx|x86_64", function (package) -- TODO: to enable android you will need to figure out the order of libs
        -- Patch out -Werror to fix build with newer clang versions (22+)
        local sourcedir = path.join(package:cachedir(), "source")
        io.replace(path.join(sourcedir, "CMakeLists.txt"), "-Werror", "", {plain = true})

        local opt = {}
        local configs = {}
        table.insert(configs, "-DSENTRY_BUILD_EXAMPLES=OFF")
        table.insert(configs, "-DSENTRY_BUILD_TESTS=OFF")
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        if package:config("shared") then
            table.insert(configs, "-DBUILD_SHARED_LIBS=ON")
            table.insert(configs, "-DSENTRY_BUILD_SHARED_LIBS=ON")
        else
            table.insert(configs, "-DBUILD_SHARED_LIBS=OFF")
            table.insert(configs, "-DSENTRY_BUILD_SHARED_LIBS=OFF")
        end
        if package:config("backend") then
            table.insert(configs, "-DSENTRY_BACKEND=" .. package:config("backend"))
        end
        if package:is_plat("windows") then
            local vs_runtime = package:config("vs_runtime")
            table.insert(configs, "-DSENTRY_BUILD_RUNTIMESTATIC=" .. ((vs_runtime == "MT" or vs_runtime == "MTd") and "ON" or "OFF"))
        elseif package:is_plat("macosx") then
            opt.shflags = {"-framework", "SystemConfiguration"}
        end
        import("package.tools.cmake").install(package, configs, opt)
   end)

    on_test(function (package)
        assert(package:check_cxxsnippets({test = [[
            void test(int args, char** argv) {
                sentry_options_t* options = sentry_options_new();
                sentry_init(options);
                sentry_close();
            }
        ]]}, {includes = {"sentry.h"}}))
    end)