set_xmakever("2.8.1")

set_allowedplats("windows", "linux")
set_arch("x64")

set_languages("cxx20", "c17")

add_rules("plugin.vsxmake.autoupdate")

if is_plat("windows") then
  add_defines("UNICODE=1", "_UNICODE=1")
  add_cxflags("/bigobj", "/utf-8", {tools = {"clang_cl", "cl"}})
  add_cxflags("/MP", {tools = {"cl"}})
  add_defines("_CRT_SECURE_NO_WARNINGS=1", "WIN32_LEAN_AND_MEAN=1", "NOMINMAX=1", "WINVER=_WIN32_WINNT_WIN10", "_WIN32_WINNT=_WIN32_WINNT_WIN10", "NTDDI=NTDDI_WIN10_19H1")
end

if is_plat("linux") then
  add_cxflags("-stdlib=libc++", {tools = {"clang"}})
end

local vsRuntime = "MD"

set_policy("check.auto_ignore_flags", true)
set_policy("build.across_targets_in_parallel", true)
set_policy("build.ccache", true)
set_policy("build.warning", true)

if is_mode("debug") then
  add_defines("_DEBUG")
  set_symbols("debug")
  set_optimize("none")
  set_warnings("all")
  set_policy("build.optimization.lto", false)

  --add_cxflags("-fsanitize=address -fsanitize=hwaddress -fsanitize=thread -fsanitize=undefined -fsanitize=memory -fsanitize=leak", {tools = {"clang", "gnu"}})
  --add_mxflags("-fsanitize=address -fsanitize=hwaddress -fsanitize=thread -fsanitize=undefined -fsanitize=memory -fsanitize=leak", {tools = {"clang", "gnu"}})
  --add_ldflags("-fsanitize=address -fsanitize=hwaddress -fsanitize=thread -fsanitize=undefined -fsanitize=memory -fsanitize=leak", {tools = {"clang", "gnu"}})
  --add_shflags("-fsanitize=address -fsanitize=hwaddress -fsanitize=thread -fsanitize=undefined -fsanitize=memory -fsanitize=leak", {tools = {"clang", "gnu"}})

  --add_cxflags("/fsanitize=address", {tools = {"clang_cl", "cl"}})
  --add_mxflags("/fsanitize=address", {tools = {"clang_cl", "cl"}})
  --add_ldflags("/fsanitize=address", {tools = {"clang_cl", "cl"}})
  --add_shflags("/fsanitize=address", {tools = {"clang_cl", "cl"}})

  vsRuntime = vsRuntime.."d"
elseif is_mode("releasedbg") then
  add_defines("_DEBUG")
  set_symbols("debug")
  set_optimize("fastest")
  set_warnings("all")
  set_policy("build.optimization.lto", true)

  --add_cxflags("-fsanitize=address -fsanitize=hwaddress -fsanitize=thread -fsanitize=undefined -fsanitize=memory -fsanitize=leak", {tools = {"clang", "gnu"}})
  --add_mxflags("-fsanitize=address -fsanitize=hwaddress -fsanitize=thread -fsanitize=undefined -fsanitize=memory -fsanitize=leak", {tools = {"clang", "gnu"}})
  --add_ldflags("-fsanitize=address -fsanitize=hwaddress -fsanitize=thread -fsanitize=undefined -fsanitize=memory -fsanitize=leak", {tools = {"clang", "gnu"}})
  --add_shflags("-fsanitize=address -fsanitize=hwaddress -fsanitize=thread -fsanitize=undefined -fsanitize=memory -fsanitize=leak", {tools = {"clang", "gnu"}})

  --add_cxflags("/fsanitize=address", {tools = {"clang_cl", "cl"}})
  --add_mxflags("/fsanitize=address", {tools = {"clang_cl", "cl"}})
  --add_ldflags("/fsanitize=address", {tools = {"clang_cl", "cl"}})
  --add_shflags("/fsanitize=address", {tools = {"clang_cl", "cl"}})

  vsRuntime = vsRuntime.."d"
elseif is_mode("release") then
  add_defines("NDEBUG")
  set_symbols("hidden")
  set_strip("all")
  set_optimize("fastest")
  set_warnings("all", "error")
  set_policy("build.optimization.lto", true)
end

add_cxflags("-Wno-microsoft-include", "-Wno-unused-command-line-argument", "-Wno-pragma-system-header-outside-header", {tools = {"clang_cl", "clang"}})

add_requireconfs("*", { configs = { debug = is_mode("debug"), shared = false } })
--add_requireconfs("*.*", { configs = { debug = is_mode("debug"), shared = false } })

if is_plat("windows") then
  set_runtimes(vsRuntime);
  --add_requireconfs("*", { configs = { cxflags = "/fsanitize=address", mxflags = "/fsanitize=address", ldflags = "/fsanitize=address", shflags = "/fsanitize=address" } })
  --add_requireconfs("*.*", { configs = { cxflags = "/fsanitize=address", mxflags = "/fsanitize=address", ldflags = "/fsanitize=address", shflags = "/fsanitize=address" } })
end

if is_plat("linux") then
  --add_requireconfs("*", { configs = { cxflags = "-fsanitize=address -fsanitize=hwaddress -fsanitize=thread -fsanitize=undefined -fsanitize=memory -fsanitize=leak", mxflags = "-fsanitize=address -fsanitize=hwaddress -fsanitize=thread -fsanitize=undefined -fsanitize=memory -fsanitize=leak", ldflags = "-fsanitize=address -fsanitize=hwaddress -fsanitize=thread -fsanitize=undefined -fsanitize=memory -fsanitize=leak", shflags = "-fsanitize=address -fsanitize=hwaddress -fsanitize=thread -fsanitize=undefined -fsanitize=memory -fsanitize=leak" } })
  --add_requireconfs("*.*", { configs = { cxflags = "-fsanitize=address -fsanitize=hwaddress -fsanitize=thread -fsanitize=undefined -fsanitize=memory -fsanitize=leak", mxflags = "-fsanitize=address -fsanitize=hwaddress -fsanitize=thread -fsanitize=undefined -fsanitize=memory -fsanitize=leak", ldflags = "-fsanitize=address -fsanitize=hwaddress -fsanitize=thread -fsanitize=undefined -fsanitize=memory -fsanitize=leak", shflags = "-fsanitize=address -fsanitize=hwaddress -fsanitize=thread -fsanitize=undefined -fsanitize=memory -fsanitize=leak" } })
end

add_requires("libsndfile 1.2.2", { configs = { shared = true } })
add_requires("xxhash v0.8.2")
add_requires("icu4c 73.2", { configs = { shared = true } })

--add_requires("catch2 v3.4.0")
--add_requires("tracy v0.9.1")

add_requires("toml++ v3.4.0", { configs = { header_only = true } })
add_requires("libsamplerate 22bd06eb114850ebe31981eb794d150a95439fef") -- 0.2.2 with extra patches

option("fbl-vendor-use-std-ranges")
  set_default(true)
  set_showmenu(true)
  set_description("Use std::ranges")
  set_category("Feldarian Base Library/Vendor")
option_end()

option("fbl-vendor-use-range-v3")
  set_default(false)
  set_showmenu(true)
  set_description("Use range-v3")
  set_category("Feldarian Base Library/Vendor")
option_end()

if has_config("fbl-use-utf-range-v3") then
  add_requires("range-v3 0.12.0")
end

option("fbl-vendor-use-std-format")
  set_default(true)
  set_showmenu(true)
  set_description("Use std::format")
  set_category("Feldarian Base Library/Vendor")
option_end()

option("fbl-vendor-use-fmt")
  set_default(false)
  set_showmenu(true)
  set_description("Use fmt")
  set_category("Feldarian Base Library/Vendor")
option_end()

if has_config("fbl-use-utf-fmt") then
  add_requires("fmt 10.1.1", { configs = { header_only = false } })
end

option("fbl-use-pcms16")
  set_default(true)
  set_showmenu(true)
  set_description("Compile PCMS16 module into Feldarian Base Library.")
  set_category("Feldarian Base Library/PCMS16")
option_end()

option("fbl-use-utf")
  set_default(true)
  set_showmenu(true)
  set_description("Compile UTF module into Feldarian Base Library.")
  set_category("Feldarian Base Library/UTF")
option_end()

add_requires("spdlog v1.12.0", { configs = { header_only = false, std_format = not has_config("fbl-use-utf-fmt"), fmt_external = has_config("fbl-use-utf-fmt"), noexcept = true } })

option("fbl-pcms16-use-adpcmxq")
  add_deps("fbl-use-pcms16")
  set_default(true)
  set_showmenu(true)
  set_description("Use ADPCM-XQ library for IMA ADPCM instead of libsndfile.")
  set_category("Feldarian Base Library/PCMS16")
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
      add_defines("FBL_PCMS16_USE_ADPCMXQ=1")
      add_deps("adpcm-xq", {private = true})
    end

    add_defines("FBL_USE_PCMS16=1", { public = true })
    add_defines("ENABLE_SNDFILE_WINDOWS_PROTOTYPES=1")

    add_includedirs("include", {public = false})
  
    add_headerfiles("include/Feldarian/PCMS16/**.hpp")
    add_files("src/PCMS16/**.cpp")

    add_packages("libsndfile", "libsamplerate", "xxhash")

    if has_config("fbl-vendor-use-std-ranges") then
      add_defines("FBL_VENDOR_USE_STD_RANGES=1", { public = true })
    end

    if has_config("fbl-vendor-use-range-v3") then
      add_defines("FBL_VENDOR_USE_RANGE_V3=1", { public = true })
      add_packages("range-v3", { public = true })
    end
  target_end()
end

if has_config("fbl-use-utf") then
  target("FeldarianBaseLibrary_UTF")
    set_version("0.1.0")
    set_kind("headeronly")

    set_options("fbl-use-utf", "fbl-use-utf-std-format", "fbl-use-utf-fmt", "fbl-use-utf-std-ranges", "fbl-use-utf-range-v3")

    add_defines("FBL_USE_UTF=1", { public = true })
    add_defines("TOML_EXCEPTIONS=0", { public = true })

    add_includedirs("include", {public = false})

    add_headerfiles("include/Feldarian/UTF/**.hpp")
    --add_files("src/UTF/**.cpp")

    add_packages("xxhash", "toml++", "icu4c", { public = true })

    if has_config("fbl-vendor-use-std-format") then
      add_defines("FBL_VENDOR_USE_STD_FORMAT=1", { public = true })
    end

    if has_config("fbl-vendor-use-fmt") then
      add_defines("FBL_VENDOR_USE_FMT=1", { public = true })
      add_packages("fmt", { public = true })
    end

    if has_config("fbl-vendor-use-std-ranges") then
      add_defines("FBL_VENDOR_USE_STD_RANGES=1", { public = true })
    end

    if has_config("fbl-vendor-use-range-v3") then
      add_defines("FBL_VENDOR_USE_RANGE_V3=1", { public = true })
      add_packages("range-v3", { public = true })
    end

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
