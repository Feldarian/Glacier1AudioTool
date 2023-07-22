set_xmakever("2.8.1")

includes("vendor/FeldarianBaseLibrary")

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

add_cxflags("/wd4200", {tools = {"cl"}})

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

add_defines("IMGUI_DISABLE_OBSOLETE_FUNCTIONS=1")

add_requireconfs("*", { configs = { debug = is_mode("debug"), lto = not is_mode("debug"), shared = false, vs_runtime = vsRuntime } })

add_requires("scnlib 1.1.2", { configs = { header_only = false } })
add_requires("libsdl 2.28.1", { configs = { shared = true, use_sdlmain = false } })
add_requires("spdlog v1.12.0", { configs = { header_only = true, std_format = true, noexcept = true } })
add_requires("catch2 v3.4.0")
add_requires("tracy v0.9.1")

local imguiUserConfig = path.absolute("src/ImGuiConfig.hpp");
add_requires("imgui v1.89.7-docking", { configs = { wchar32 = true, freetype = true, user_config = imguiUserConfig } })

function CopyDataToDirecotry(targetDir)
  os.rm(targetDir .. "/data")
  os.mkdir(targetDir .. "/data")
  os.cp("data/*", targetDir .. "/data")
end

target("imgui-backends")
  set_version("1.89.7")
  set_kind("static")

  add_includedirs("vendor/imgui-backends/include", {public = true})

  add_headerfiles("vendor/imgui-backends/include/**.h")
  add_files("vendor/imgui-backends/src/**.cpp")

  add_packages("imgui", "libsdl")
target_end()

target("Glacier1AudioTool")
  set_version("1.2.0")
  set_kind("binary")
  
  set_rundir("$(projectdir)")
  add_defines("IMGUI_USER_CONFIG=\""..imguiUserConfig.."\"")

  --add_defines("G1AT_BUILD_TESTS")
  --add_defines("G1AT_ENABLE_PROFILING")

  add_deps("FeldarianBaseLibrary", "imgui-backends", { private = true })
  
  add_files("manifests/**.manifest")

  add_includedirs("src")

  add_headerfiles("src/*.h")
  --add_files("src/*.c")
  add_headerfiles("src/*.hpp")
  --add_files("src/*.cpp")

  --add_headerfiles("src/App/**.h")
  --add_files("src/App/**.c")
  add_headerfiles("src/App/**.hpp")
  add_files("src/App/**.cpp")
  
  --add_headerfiles("src/Config/**.h")
  --add_files("src/Config/**.c")
  add_headerfiles("src/Config/**.hpp")
  --add_files("src/Config/**.cpp")
  
  --add_headerfiles("src/Core/**.h")
  --add_files("src/Core/**.c")
  add_headerfiles("src/Core/**.hpp")
  add_files("src/Core/**.cpp")
  
  --add_headerfiles("src/G1AT/**.h")
  --add_files("src/G1AT/**.c")
  add_headerfiles("src/G1AT/**.hpp")
  add_files("src/G1AT/**.cpp")

  if (is_plat("windows")) then
    --add_headerfiles("src/Platform/Windows/**.h")
    --add_files("src/Platform/Windows/**.c")
    add_headerfiles("src/Platform/Windows/**.hpp")
    add_files("src/Platform/Windows/**.cpp")
  elseif (is_plat("linux")) then
    --add_headerfiles("src/Platform/Linux/**.h")
    --add_files("src/Platform/Linux/**.c")
    add_headerfiles("src/Platform/Linux/**.hpp")
    add_files("src/Platform/Linux/**.cpp")
  elseif (is_plat("macosx")) then
    --add_headerfiles("src/Platform/Mac/**.h")
    --add_files("src/Platform/Mac/**.c")
    add_headerfiles("src/Platform/Mac/**.hpp")
    add_files("src/Platform/Mac/**.cpp")
  end
  
  set_configvar("G1AT_COMPANY_NAMESPACE", "Feldarian")
  set_configvar("G1AT_COMPANY_NAME", "Feldarian Softworks")
  set_configvar("G1AT_NAME", "Glacier 1 Audio Tool")
  set_configvar("G1AT_DESCRIPTION", "Tool able to read, write, import and export selected Glacier 1 sound-related formats.")
  set_configvar("G1AT_HOMEPAGE", "https://github.com/WSSDude/Glacier1AudioTool")

  set_configdir("$(buildir)/generated/Config")
  add_configfiles("src/Config/Config.h.in")
  add_includedirs("$(buildir)/generated")
  add_headerfiles("$(buildir)/generated/Config/**.h")
  
  set_pcxxheader("src/Precompiled.hpp")
  
  add_syslinks("comdlg32", "opengl32", "shell32")
  add_packages("scnlib", "libsdl", "imgui", "spdlog", "xxhash", "toml++", "icu4c")
  
  before_build(function (target)
    os.rm(target:targetdir() .. "/data")
  end)

  after_build(function (target)
    os.mkdir(target:targetdir() .. "/data")
    os.cp("data/*", target:targetdir() .. "/data")
  end)

  before_install(function (target)
    os.rm(target:installdir())
  end)

  after_install(function (target)
    os.mkdir(target:installdir() .. "/data")
    os.cp("data/*", target:installdir() .. "/data")

    os.mkdir(target:installdir() .. "/EUPL-1.2")
    os.cp("EUPL-1.2/*", target:installdir() .. "/EUPL-1.2")

    os.cp("README.md", target:installdir())
    os.cp("LICENSE.md", target:installdir())
    os.cp("LICENSE_ThirdParty.md", target:installdir())

    os.mv(target:installdir() .. "/bin/*", target:installdir())
    os.rm(target:installdir() .. "/bin")
  end)
target_end()
