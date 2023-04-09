set_xmakever("2.7.2")

set_allowedplats("windows")
set_arch("x64")

set_languages("cxx20")

add_rules("mode.debug", "mode.releasedbg", "mode.release")
add_rules("plugin.vsxmake.autoupdate")
add_rules("c.unity_build")
add_rules("c++.unity_build")

add_defines("UNICODE=1", "_UNICODE=1")

if (is_plat("windows")) then
  add_cxflags("/bigobj", "/MP", "/utf-8")
  add_defines("_CRT_SECURE_NO_WARNINGS=1", "WIN32_LEAN_AND_MEAN=1", "NOMINMAX=1", "WINVER=_WIN32_WINNT_WIN10", "_WIN32_WINNT=_WIN32_WINNT_WIN10", "NTDDI=NTDDI_WIN10_19H1", "ENABLE_SNDFILE_WINDOWS_PROTOTYPES=1")
end

local vsRuntime = "MD"

if is_mode("debug") then
  add_defines("_DEBUG")
  set_symbols("debug")
  set_optimize("none")
  set_warnings("all")
  set_policy("build.optimization.lto", false)
  
  vsRuntime = vsRuntime.."d"
elseif is_mode("releasedbg") then
  add_defines("_DEBUG")
  set_symbols("debug")
  set_optimize("fastest")
  set_warnings("all")
  set_policy("build.optimization.lto", true)

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

add_defines("IMGUI_DISABLE_OBSOLETE_FUNCTIONS=1", "GL_GLEXT_PROTOTYPES=1")

add_vectorexts("mmx", "sse", "sse2")

add_requireconfs("*", { configs = { debug = is_mode("debug"), lto = not is_mode("debug"), shared = false, vs_runtime = vsRuntime } })

add_requires("scnlib 1.1.2", { configs = { header_only = false } })
add_requires("libsdl 2.26.3", { configs = { shared = true, use_sdlmain = true } })
add_requires("libsndfile 1.2.0", { configs = { shared = true } })
add_requires("xxhash v0.8.1")
add_requires("vcpkg::libsamplerate 0.2.2", { configs = { shared = true }, alias = "libsamplerate" })
add_requires("toml++ 3.3.0", { configs = { header_only = true } })

local imguiUserConfig = path.absolute("src/ImGuiConfig.hpp");
add_requires("imgui v1.89.3-docking", { configs = { wchar32 = true, freetype = true, user_config = imguiUserConfig } })

function CopyDataToDirecotry(targetDir)
  os.rm(targetDir .. "/data")
  os.mkdir(targetDir .. "/data")
  os.cp("data/*", targetDir .. "/data")
end

target("HitmanAudioTool")
  set_rundir("$(projectdir)")
  add_defines("IMGUI_USER_CONFIG=\""..imguiUserConfig.."\"")

  add_defines("HAT_SAFE_LOCALIZATION")

  set_kind("binary")
  
  add_files("embed/*")

  add_headerfiles("src/**.h")
  
  add_headerfiles("src/**.hpp")
  add_files("src/**.cpp")
  
  set_configdir("$(buildir)/src/Config")
  add_configfiles("src/Config.h.in")

  add_headerfiles("$(buildir)/src/Config")
  
  set_pcxxheader("src/Precompiled.hpp")
  
  add_packages("scnlib", "libsdl", "libsndfile", "libsamplerate", "imgui", "xxhash", "toml++")
  add_syslinks("comdlg32", "opengl32", "shell32", "icuuc")
  
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
