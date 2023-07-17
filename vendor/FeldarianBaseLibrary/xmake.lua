set_xmakever("2.8.1")

set_allowedplats("windows")
set_arch("x64")

set_languages("cxx20")

add_rules("plugin.vsxmake.autoupdate")
add_rules("c.unity_build")
add_rules("c++.unity_build")

add_defines("UNICODE=1", "_UNICODE=1")

if (is_plat("windows")) then
  add_cxflags("/bigobj", "/utf-8", {tools = {"clang_cl", "cl"}})
  add_cxflags("/MP", {tools = {"cl"}})
  add_defines("_CRT_SECURE_NO_WARNINGS=1", "WIN32_LEAN_AND_MEAN=1", "NOMINMAX=1", "WINVER=_WIN32_WINNT_WIN10", "_WIN32_WINNT=_WIN32_WINNT_WIN10", "NTDDI=NTDDI_WIN10_19H1")
end

local vsRuntime = "MD"

if is_mode("debug") then
  add_defines("_DEBUG")
  set_symbols("debug")
  set_optimize("none")
  set_warnings("all")
  set_policy("build.optimization.lto", false)

  --add_cxflags("/fsanitize=address")
  --add_mxflags("/fsanitize=address")
  --add_ldflags("/fsanitize=address")
  
  vsRuntime = vsRuntime.."d"
elseif is_mode("releasedbg") then
  add_defines("_DEBUG")
  set_symbols("debug")
  set_optimize("fastest")
  set_warnings("all")
  set_policy("build.optimization.lto", true)
  
  --add_cxflags("/fsanitize=address")
  --add_mxflags("/fsanitize=address")
  --add_ldflags("/fsanitize=address")

  vsRuntime = vsRuntime.."d"
elseif is_mode("release") then
  add_defines("NDEBUG")
  set_symbols("hidden")
  set_strip("all")
  set_optimize("fastest")
  set_warnings("all", "error")
  set_policy("build.optimization.lto", true)
end

set_runtimes(vsRuntime);

add_cxflags("-Wno-microsoft-include", "-Wno-unused-command-line-argument", "-Wno-pragma-system-header-outside-header", {tools = {"clang_cl", "clang"}})

add_requireconfs("*", { configs = { debug = is_mode("debug"), lto = not is_mode("debug"), shared = false, vs_runtime = vsRuntime } })

add_requires("libsndfile 1.2.0", { configs = { shared = true } })
add_requires("xxhash v0.8.1")
add_requires("icu4c 73.2", { configs = { shared = true } })

add_requires("toml++ 4a28c36c435d813ddbd39a9a48a79d8c862c547f", { configs = { header_only = true } }) -- 3.3.0 with extra patches
add_requires("libsamplerate 22bd06eb114850ebe31981eb794d150a95439fef") -- 0.2.2 with extra patches

option("fbl-use-pcms16")
    set_default(true)
    set_showmenu(true)
    set_description("Compile PCMS16 module into Feldarian Base Library.")
    set_category("Feldarian Base Library/PCMS16")
    add_defines("FBL_USE_PCMS16=1", { public = true })
option_end()

option("fbl-use-utf")
    set_default(true)
    set_showmenu(true)
    set_description("Compile UTF module into Feldarian Base Library.")
    set_category("Feldarian Base Library/UTF")
    add_defines("FBL_USE_UTF=1", { public = true })
option_end()

option("fbl-pcms16-use-adpcmxq")
    add_deps("fbl-use-pcms16")
    set_default(true)
    set_showmenu(true)
    set_description("Use ADPCM-XQ library for IMA ADPCM instead of libsndfile.")
    set_category("Feldarian Base Library/PCMS16")
    add_defines("FBL_PCMS16_USE_ADPCMXQ=1")
option_end()

if has_config("fbl-pcms16-use-adpcmxq") then
  target("adpcm-xq")
    set_version("0.4.0")
    set_kind("static")

    set_warnings("none")

    add_includedirs("vendor/adpcm-xq", {public = true})

    add_headerfiles("vendor/adpcm-xq/**.h")
    add_files("vendor/adpcm-xq/**.c")
  target_end()
end

if has_config("fbl-use-pcms16") then
  target("FeldarianBaseLibrary_PCMS16")
    set_version("0.1.0")
    set_kind("static")

    set_options("fbl-use-pcms16", "fbl-pcms16-use-adpcmxq")

    if has_config("fbl-pcms16-use-adpcmxq") then
      add_deps("adpcm-xq", {private = true})
    end

    add_defines("ENABLE_SNDFILE_WINDOWS_PROTOTYPES=1")

    add_includedirs("include", {public = false})
  
    add_headerfiles("include/Feldarian/PCMS16/**.hpp")
    add_files("src/PCMS16/**.cpp")

    add_packages("libsndfile", "libsamplerate", "xxhash")
  target_end()
end

if has_config("fbl-use-utf") then
  target("FeldarianBaseLibrary_UTF")
    set_version("0.1.0")
    set_kind("headeronly")

    set_options("fbl-use-utf")

    add_defines("TOML_EXCEPTIONS=0", { public = true })

    add_includedirs("include", {public = false})

    add_headerfiles("include/Feldarian/UTF/**.hpp")
    --add_files("src/UTF/**.cpp")

    add_packages("xxhash", "toml++", "icu4c")
  target_end()
end

target("FeldarianBaseLibrary")
  set_version("0.1.0")
  set_kind("phony")

  set_options("fbl-use-pcms16", "fbl-use-utf")
  
  add_includedirs("include", {public = true})
  
  if has_config("fbl-use-pcms16") then
    add_deps("FeldarianBaseLibrary_PCMS16")
  end
    
  if has_config("fbl-use-utf") then
    add_deps("FeldarianBaseLibrary_UTF")
  end 
target_end()
