package("thorvg")
set_homepage("https://www.thorvg.org")
set_description(
    "Thor Vector Graphics is a lightweight portable library used for drawing vector-based scenes and animations including SVG and Lottie. It can be freely utilized across various software platforms and applications to visualize graphical contents.")
set_license("MIT")

add_urls("https://github.com/thorvg/thorvg/archive/refs/tags/$(version).tar.gz",
    "https://github.com/thorvg/thorvg.git")

add_versions("v1.0-pre10", "0dcd466d25275a76a84a6a58f798d098e72e7555b4c5d17cdec822afc38212dc")
add_versions("v0.15.8", "d7c0961c872086aa64db7b5bb6c58e7a2f81202aecd164c835ac77e74d95d12b")
add_versions("v0.15.2", "98fcd73567c003a33fad766a7dbb9244c61e9b4721397d42e7fa04fc2e499dce")
add_versions("v0.15.1", "4935228bb11e1a5303fc98d2a4b355c94eb82bff10cff581821b0b3c41368049")
add_versions("v0.14.10", "e11e2e27ef26ed018807e828cce3bca1fb9a7f25683a152c9cd1f7aac9f3b67a")
add_versions("v0.14.6", "13d7778968ce10f4f7dd1ea1f66861d4ee8ff22f669566992b4ac00e050496cf")
add_versions("v0.14.3", "302e7bb2082a5c4528b6ec9b95d500b2c0f54f4333870a709cc122b5b393dcfe")
add_versions("v0.14.2", "04202e8f5e17b427c3b16ae3b3d4be5d3f3cdac96d5c64ed3efd7b6db3ad731f")
add_versions("v0.14.1", "9c0346fda8b62a3b13a764dda5784e0465c8cab54fb5342d0240c7fb40e415bd")
add_versions("v0.13.8", "ce49929a94d1686d4f1436da6ef5fa7a8439901c22b5fa0879d7d5879b8ba2bd")
add_versions("v0.13.6", "f24fd3647e1a309dec00f6455b32258c0dd0e0dbd1133233169467571f188bad")
add_versions("v0.13.5", "977ed74c3846c9a6acd5765aff776745d40e3c91507b22e51177d59c69afd198")
add_versions("v0.13.2", "03b5dbb4c73ff221a4bd7243cc0ad377aecff4c3077f5a57ee2902e4122d3218")

add_configs("engines", { description = "Enable Rasterizer Engine in thorvg", default = { "sw", "gl" }, type = "table" })
add_configs("loaders", { description = "Enable File Loaders", type = "table" })
add_configs("savers", { description = "Enable File Savers", type = "table" })
add_configs("tools", { description = "Enable building thorvg tools", type = "table" })
add_configs("extra", { description = "Enable support for exceptionally advanced features", type = "table" })
add_configs("threads",
    { description = "Enable the multi-threading task scheduler in thorvg", default = false, type = "boolean" })
add_configs("simd", { description = "Enable CPU Vectorization(SIMD) in thorvg", default = false, type = "boolean" })
add_configs("log", { description = "Enable log message", default = false, type = "boolean" })
add_configs("c_api", { description = "Enable API bindings", default = false, type = "boolean" })
if is_plat("wasm") then
    add_configs("shared", { description = "Build shared library.", default = false, type = "boolean", readonly = true })
end

if is_plat("linux", "bsd") then
    add_syslinks("pthread", "m")
end

add_deps("meson", "ninja", "glad")
if is_plat("windows") then
    add_deps("pkgconf")
end

if on_check then
    on_check("android", function(package)
        local ndk = package:toolchain("ndk")
        local ndkver = ndk:config("ndkver")
        assert(ndkver and tonumber(ndkver) > 22, "package(thorvg) require ndk version > 22")
        if package:is_arch("armeabi-v7a") then
            local ndk_sdkver = ndk:config("ndk_sdkver")
            assert(ndk_sdkver and tonumber(ndk_sdkver) > 21, "package(thorvg/armeabi-v7a) require ndk api level > 21")
        end
    end)
end

on_load(function(package)
    import("core.base.hashset")

    local loaders = package:config("loaders")
    if loaders then
        local loaders = hashset.from(loaders)
        if loaders then
            local deps = { jpg = "libjpeg-turbo", png = "libpng", webp = "libwebp" }
            if loaders:has("all") then
                for _, dep in pairs(deps) do
                    package:add("deps", dep)
                end
            else
                for name, dep in pairs(deps) do
                    if loaders:has(name) then
                        package:add("deps", dep)
                    end
                end
            end
        end
    end

    if not package:config("shared") then
        package:add("defines", "TVG_STATIC")
    end
end)

on_install(function(package)
    if package:is_plat("mingw") then
        io.replace("src/loaders/svg/tvgSvgLoader.cpp", "float_t", "float", { plain = true })
    end


    io.replace("src/renderer/gl_engine/tvgGlCommon.h", "#include <GL/gl.h>",
        "#include <glad/glad.h>\n#define M_PI 3.1415926535897", { plain = true })

    io.replace("meson.build", "include_directories('.')", "include_directories('.'), include_directories('" ..
        package:dep("glad"):installdir():gsub("\\", "/")
        .. "/include')", { plain = true })

    local configs = {}
    table.insert(configs, "-Ddefault_library=" .. (package:config("shared") and "shared" or "static"))

    local loaders = package:config("loaders")
    local savers = package:config("savers")
    local tools = package:config("tools")
    local extra = package:config("extra")
    table.insert(configs, "-Dengines=" .. table.concat(package:config("engines"), ","))
    table.insert(configs, "-Dloaders=" .. (loaders and table.concat(loaders, ",") or ""))
    table.insert(configs, "-Dsavers=" .. (savers and table.concat(savers, ",") or ""))
    table.insert(configs, "-Dtools=" .. (tools and table.concat(tools, ",") or ""))
    table.insert(configs, "-Dextra=" .. (extra and table.concat(extra, ",") or ""))


    if package:config("c_api") then
        table.insert(configs, "-Dbindings=capi")
    end
    table.insert(configs, "-Dthreads=" .. (package:config("threads") and "true" or "false"))
    table.insert(configs, "-Dsimd=" .. (package:config("simd") and "true" or "false"))
    table.insert(configs, "-Dlog=" .. (package:config("log") and "true" or "false"))
    import("package.tools.meson").install(package, configs)
end)

on_test(function(package)
    assert(package:has_cxxfuncs("tvg::Initializer::init(0)", { includes = "thorvg.h", configs = { languages = "c++14" } }))
    if package:config("c_api") then
        assert(package:has_cxxfuncs("tvg_engine_init", { includes = "thorvg_capi.h" }))
    end
end)
