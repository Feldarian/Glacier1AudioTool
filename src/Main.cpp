//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#include "Precompiled.hpp"

#include "Hitman1Dialog.hpp"
#include "Hitman23Dialog.hpp"
#include "Hitman4Dialog.hpp"

#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl2.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
  std::filesystem::path dataPath = GetProgramPath();
  if (dataPath.empty())
    return -1;

  dataPath /= L"data";
  if (!exists(dataPath))
    return -2;

  std::filesystem::path localizationPath = dataPath / L"localization";
  if (!exists(localizationPath))
    return -3;

  std::filesystem::path fontPath = dataPath / L"fonts";
  if (!exists(localizationPath))
    return -4;

  std::filesystem::path recordsData = dataPath / L"records";
  if (!exists(recordsData))
    return -5;

  LocalizationManager.SetDefaultLanguage("English");
  const auto localizationFilePaths = GetAllFilesInDirectory(localizationPath, L".toml", false);
  if (localizationFilePaths.empty())
    return -6;

  bool foundEnglish = false;
  for (const auto& localizationFilePath : localizationFilePaths)
  {
    if (!LocalizationManager.LoadLocalization(localizationFilePath))
      return -6;

    if (UTFCaseInsensitiveCompare(localizationFilePath.stem().native(), "English") == 0)
      foundEnglish = true;
  }
  if (!foundEnglish)
    return -7;

  fontPath /= L"NotoSans-Regular.ttf";
  if (!exists(fontPath))
    return -7;

  Options::Get().Load();
  Options::Get().Save();

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
  {
    DisplayError(LocalizationManager.LocalizeFormat("ARCHIVE_DIALOG_IMPORT_PROGRESS_IMPORTING_FILE", SDL_GetError()));
    return -8;
  }

  const char *glsl_version = "#version 130";
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

#ifdef SDL_HINT_IME_SHOW_UI
  SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  constexpr auto window_flags =
      static_cast<SDL_WindowFlags>(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  SDL_Window *window =
      SDL_CreateWindow("Hitman Audio Tool", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, window_flags);
  const auto gl_context = SDL_GL_CreateContext(window);
  SDL_GL_MakeCurrent(window, gl_context);
  SDL_GL_SetSwapInterval(1);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
  io.IniFilename = nullptr;
  io.LogFilename = nullptr;

  ImGui::StyleColorsDark();

  ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
  ImGui_ImplOpenGL3_Init(glsl_version);

  ImFontConfig config;
  config.SizePixels = 18;
  config.OversampleH = 3;
  config.OversampleV = 3;
  config.MergeMode = false;

  io.Fonts->AddFontFromFileTTF(ToUTF<char>(fontPath.native()).c_str(), config.SizePixels, &config, GetGlyphRanges());

  constexpr ImVec4 clear_color(0.45f, 0.55f, 0.60f, 1.00f);

  bool done = false;
  while (!done)
  {
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      ImGui_ImplSDL2_ProcessEvent(&event);

      switch (event.type)
      {
        case SDL_QUIT: {
          done = true;
          break;
        }
        case SDL_WINDOWEVENT: {
          done = event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window);
          break;
        }
      }
    }

    done &= !Hitman1Dialog::Get().IsInProgress() && !Hitman23Dialog::Get().IsInProgress();

    if (done)
    {
      switch (Hitman1Dialog::Get().UnsavedChangesPopup())
      {
        case 1: {
          Hitman1Dialog::Get().Save(Hitman1Dialog::Get().GetPath(), false);
          break;
        }
        case 0: {
          break;
        }
        default:
        case -1: {
          done = false;
          break;
        }
      }
    }

    if (done)
    {
      switch (Hitman23Dialog::Get().UnsavedChangesPopup())
      {
        case 1: {
          Hitman23Dialog::Get().Save(Hitman23Dialog::Get().GetPath(), false);
          break;
        }
        case 0: {
          break;
        }
        default:
        case -1: {
          done = false;
          break;
        }
      }
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    int32_t windowWidth = 0;
    int32_t windowHeight = 0;
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);

    ImGui::SetNextWindowPos({}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({static_cast<float>(windowWidth), static_cast<float>(windowHeight)}, ImGuiCond_Always);
    ImGui::Begin("Hitman Audio Tool", nullptr,
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    if (ImGui::BeginTabBar("##ToolsTab"))
    {
      Hitman1Dialog::Get().DrawDialog();
      Hitman23Dialog::Get().DrawDialog();

#ifdef _DEBUG
      Hitman4Dialog::Get().DrawDialog();
#endif

      Options::Get().DrawDialog();

      ImGui::EndTabBar();
    }

    ImGui::End();

    ImGui::Render();
    glViewport(0, 0, static_cast<int32_t>(io.DisplaySize.x), static_cast<int32_t>(io.DisplaySize.y));
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w,
                 clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
      auto *backup_current_window = SDL_GL_GetCurrentWindow();
      const auto backup_current_context = SDL_GL_GetCurrentContext();
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
      SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
    }

    SDL_GL_SwapWindow(window);
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_GL_DeleteContext(gl_context);
  SDL_DestroyWindow(window);
  SDL_Quit();

  Options::Get().Save();

  return 0;
}
