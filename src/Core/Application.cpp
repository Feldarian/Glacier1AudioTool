//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#include <Precompiled.hpp>

#include "Application.hpp"

#include "Debug/Instrumentor.hpp"
#include "DPIHandler.hpp"
#include "Resources.hpp"

#include <Config/Config.hpp>

#include <G1AT/Hitman1Dialog.hpp>
#include <G1AT/Hitman23Dialog.hpp>
#include <G1AT/Hitman4Dialog.hpp>
#include <G1AT/Utils.hpp>

namespace
{

std::shared_ptr<HitmanDialog> s_SelectedDialog;
OrderedSet<std::shared_ptr<HitmanDialog>> s_Dialogs;
String8CI s_OpenFilters;

}

bool BuildFontAtlas()
{
  if (!g_GlyphRangesBuilder.NeedsBuild())
    return true;

  auto fontsPath = GetProgramPath().path();
  fontsPath /= L"data";
  fontsPath /= L"fonts";
  if (!exists(fontsPath))
    return false;

  ImFontGlyphRangesBuilder builder;

  const auto glyphRanges = g_GlyphRangesBuilder.Build();
  for (const auto& glyphRange : glyphRanges)
  {
    const uint32_t translatedGlyphRange[3]{glyphRange.first, glyphRange.second, 0};
    builder.AddRanges(translatedGlyphRange);
  }

  static ImVector<ImWchar> imguiRanges;
  imguiRanges.clear();

  builder.BuildRanges(&imguiRanges);

  ImFontConfig config;
  config.SizePixels = 18 * App::DPIHandler::get_scale();
  config.OversampleH = 3;
  config.OversampleV = 1;
  config.MergeMode = false;

  const ImGuiIO &io = ImGui::GetIO();
  io.Fonts->Clear();

  const auto fonts = GetAllFilesInDirectory(String8CI(fontsPath), "", false);
  for (const auto& font : fonts)
  {
    const auto fontExtension = font.path().extension();
    if (fontExtension != StringViewWCI(L".ttf") && fontExtension != StringViewWCI(L".otf"))
      continue;

    io.Fonts->AddFontFromFileTTF(font.c_str(), config.SizePixels, &config, imguiRanges.Data);
    config.MergeMode = true;
  }

  if (!config.MergeMode)
    io.Fonts->AddFontDefault(&config);

  ImGui_ImplSDLRenderer2_DestroyFontsTexture();
  ImGui_ImplSDLRenderer2_CreateFontsTexture();

  return true;
}

void ReloadOriginalData()
{
  for (auto& hitmanDialog : s_Dialogs)
    hitmanDialog->ReloadOriginalData();
}

void InitializeOpenFilters()
{
  if (!s_OpenFilters.empty())
    return;

  auto hitmanFilters = Hitman1Dialog::GetOpenFilter();
  ranges::copy(Hitman23Dialog::GetOpenFilter(), std::back_inserter(hitmanFilters));
  ranges::copy(Hitman4Dialog::GetOpenFilter(), std::back_inserter(hitmanFilters));

  s_OpenFilters = MakeFileDialogFilter(hitmanFilters);
}

namespace App {

Application::Application(const std::string& title) {
  APP_PROFILE_FUNCTION();

  auto dataPath = GetProgramPath().path();
  if (dataPath.empty())
  {
    m_exit_status = ExitStatus::FAILURE;
    return;
  }

  dataPath /= L"data";
  if (!exists(dataPath))
  {
    m_exit_status = ExitStatus::FAILURE;
    return;
  }

  const auto localizationPath = dataPath / L"localization";
  if (!exists(localizationPath))
  {
    m_exit_status = ExitStatus::FAILURE;
    return;
  }

  const auto fontsPath = dataPath / L"fonts";
  if (!exists(fontsPath))
  {
    m_exit_status = ExitStatus::FAILURE;
    return;
  }

  const auto localizationFilePaths = GetAllFilesInDirectory(String8CI(localizationPath), ".toml", false);
  if (localizationFilePaths.empty())
  {
    m_exit_status = ExitStatus::FAILURE;
    return;
  }

  bool foundEnglish = false;
  for (const auto& localizationFilePath : localizationFilePaths)
  {
    if (!g_LocalizationManager.LoadLocalization(localizationFilePath))
    {
      m_exit_status = ExitStatus::FAILURE;
      return;
    }

    if (localizationFilePath.path().stem() == StringViewWCI(L"English"))
      foundEnglish = true;
  }
  if (!foundEnglish)
  {
    m_exit_status = ExitStatus::FAILURE;
    return;
  }

  if (!g_LocalizationManager.SetDefaultLanguage("English"))
  {
    m_exit_status = ExitStatus::FAILURE;
    return;
  }

  Options::Get().Load();
  Options::Get().Save();

  const unsigned int init_flags{SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER};
  if (SDL_Init(init_flags) != 0) {
    APP_ERROR("Error: %s\n", SDL_GetError());
    m_exit_status = ExitStatus::FAILURE;
    return;
  }

  SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");

  m_window = std::make_unique<Window>(Window::Settings{title});

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io{ImGui::GetIO()};

  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable |
                    ImGuiConfigFlags_ViewportsEnable;

  const std::string user_config_path{SDL_GetPrefPath(G1AT_COMPANY_NAMESPACE, G1AT_NAME)};
  APP_DEBUG("User config path: {}", user_config_path);

  //// Absolute imgui.ini path to preserve settings independent of app location.
  //static const std::string imgui_ini_filename{user_config_path + "imgui.ini"};
  //io.IniFilename = imgui_ini_filename.c_str();
  io.IniFilename = nullptr;
  io.LogFilename = nullptr;

  //// ImGUI font
  //const float font_scaling_factor{DPIHandler::get_scale()};
  //const float font_size{18.0F * font_scaling_factor};
  //const std::string font_path{Resources::font_path("Manrope.ttf").generic_string()};
  //
  //io.Fonts->AddFontFromFileTTF(font_path.c_str(), font_size);
  //io.FontDefault = io.Fonts->AddFontFromFileTTF(font_path.c_str(), font_size);
  DPIHandler::set_global_font_scaling(&io);

  // Setup Platform/Renderer backends
  ImGui_ImplSDL2_InitForSDLRenderer(m_window->get_native_window(), m_window->get_native_renderer());
  ImGui_ImplSDLRenderer2_Init(m_window->get_native_renderer());
}

Application::~Application() {
  APP_PROFILE_FUNCTION();

  ImGui_ImplSDLRenderer2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  m_window.reset();

  SDL_Quit();

  Options::Get().Save();
}

ExitStatus App::Application::run() {
  APP_PROFILE_FUNCTION();

  if (m_exit_status == ExitStatus::FAILURE) {
    return m_exit_status;
  }

  m_running = true;
  while (m_running)
  {
    APP_PROFILE_SCOPE("MainLoop");

    SDL_Event event{};
    while (SDL_PollEvent(&event) == 1) {
      APP_PROFILE_SCOPE("EventPolling");

      ImGui_ImplSDL2_ProcessEvent(&event);

      if (event.type == SDL_QUIT) {
        stop();
      }

      if (event.type == SDL_WINDOWEVENT &&
          event.window.windowID == SDL_GetWindowID(m_window->get_native_window())) {
        on_event(event.window);
      }
    }

    if (!m_running)
    {
      for (auto& hitmanDialog : s_Dialogs)
      {
        m_running |= hitmanDialog->IsInProgress();
        if (m_running)
          break;
      }
    }

    if (!m_running)
    {
      for (auto& hitmanDialog : s_Dialogs)
      {
        switch (hitmanDialog->UnsavedChangesPopup())
        {
          case 1: {
            hitmanDialog->Save(hitmanDialog->GetPath(), false);
            break;
          }
          case 0: {
            break;
          }
          default:
          case -1: {
            m_running = true;
            break;
          }
        }

        if (m_running)
          break;
      }
    }

    if (!m_running)
      break;

    BuildFontAtlas();

    // Start the Dear ImGui frame
    ImGui_ImplSDL2_NewFrame();
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui::NewFrame();

    if (!m_minimized)
    {
      const auto dockID = ImGui::DockSpaceOverViewport();

      const bool isSelectedDialog = s_SelectedDialog != nullptr;
      const bool inProgress = s_SelectedDialog && s_SelectedDialog->IsInProgress();
      const bool isSavingAllowed = s_SelectedDialog && s_SelectedDialog->IsSaveAllowed();
      const bool isExportAllowed = s_SelectedDialog && s_SelectedDialog->IsExportAllowed();
      const bool isImportAllowed = s_SelectedDialog && s_SelectedDialog->IsImportAllowed();

      if (ImGui::BeginMainMenuBar())
      {
        if (ImGui::BeginMenu(g_LocalizationManager.Localize("ARCHIVE_DIALOG_FILE_MENU").c_str()))
        {
          if (ImGui::MenuItem(g_LocalizationManager.Localize("ARCHIVE_DIALOG_OPEN").c_str()))
          {
            InitializeOpenFilters();

            auto [archivePath, archiveType] = OpenFileDialog(s_OpenFilters);

            auto alreadyOpened = false;
            for (const auto& dialog : s_Dialogs)
            {
              if (dialog->GetPath() == archivePath.path())
              {
                alreadyOpened = true;
                break;
              }
            }

            // TODO - some error messages/warnings wouldn't hurt...
            if (!alreadyOpened && !archivePath.empty() && archiveType != 0)
            {
              switch (archiveType)
              {
                case 1:
                {

                  const auto* originalSelectedDialog = s_SelectedDialog.get();

                  for (const auto& filter : Hitman1Dialog::GetOpenFilter() | ranges::views::values)
                  {
                    if (archivePath.path().extension() != StringViewWCI(filter.path().extension()))
                      continue;

                    s_SelectedDialog = *s_Dialogs.insert(std::make_shared<Hitman1Dialog>()).first;
                    s_SelectedDialog->Load(archivePath);
                    break;
                  }

                  if (s_SelectedDialog.get() != originalSelectedDialog)
                    break;

                  for (const auto& filter : Hitman23Dialog::GetOpenFilter() | ranges::views::values)
                  {
                    if (archivePath.path().extension() != StringViewWCI(filter.path().extension()))
                      continue;

                    s_SelectedDialog = *s_Dialogs.insert(std::make_shared<Hitman23Dialog>()).first;
                    s_SelectedDialog->Load(archivePath);
                    break;
                  }

                  if (s_SelectedDialog.get() != originalSelectedDialog)
                    break;

                  for (const auto& filter : Hitman4Dialog::GetOpenFilter() | ranges::views::values)
                  {
                    if (archivePath.path().extension() != StringViewWCI(filter.path().extension()))
                      continue;

                    s_SelectedDialog = *s_Dialogs.insert(std::make_shared<Hitman4Dialog>()).first;
                    s_SelectedDialog->Load(archivePath);
                    break;
                  }

                  break;
                }
                case 2:
                {
                  s_SelectedDialog = *s_Dialogs.insert(std::make_unique<Hitman1Dialog>()).first;
                  s_SelectedDialog->Load(archivePath);
                  break;
                }
                case 3:
                {
                  s_SelectedDialog = *s_Dialogs.insert(std::make_unique<Hitman23Dialog>()).first;
                  s_SelectedDialog->Load(archivePath);
                  break;
                }
                case 4:
                {
                  s_SelectedDialog = *s_Dialogs.insert(std::make_unique<Hitman4Dialog>()).first;
                  s_SelectedDialog->Load(archivePath);
                  break;
                }
              }
            }
          }

          if (!isSelectedDialog || inProgress)
            ImGui::BeginDisabled();

          if (ImGui::MenuItem(g_LocalizationManager.Localize("ARCHIVE_DIALOG_CLOSE").c_str()))
          {
            s_Dialogs.erase(s_SelectedDialog);
            s_SelectedDialog.reset();
          }

          if (!isSelectedDialog || inProgress)
            ImGui::EndDisabled();

          ImGui::Separator();

          if (!isSelectedDialog || inProgress)
            ImGui::BeginDisabled();

          if (ImGui::MenuItem(g_LocalizationManager.Localize("ARCHIVE_DIALOG_RELOAD").c_str()))
          {
            assert(s_SelectedDialog);
            s_SelectedDialog->Load(s_SelectedDialog->GetPath());
          }

          if (!isSelectedDialog || inProgress)
            ImGui::EndDisabled();

          if (!isSelectedDialog || inProgress || !isSavingAllowed)
            ImGui::BeginDisabled();

          if (ImGui::MenuItem(g_LocalizationManager.Localize("ARCHIVE_DIALOG_SAVE").c_str()))
          {
            assert(s_SelectedDialog);
            s_SelectedDialog->Save(s_SelectedDialog->GetPath(), true);
          }

          if (ImGui::MenuItem(g_LocalizationManager.Localize("ARCHIVE_DIALOG_SAVE_INTO").c_str()))
          {
            assert(s_SelectedDialog);

            const auto [archivePath, archiveType] = SaveFileDialog(MakeFileDialogFilter(s_SelectedDialog->GetSaveFilter()));

            s_SelectedDialog->Save(archivePath, true);
          }

          if (!isSelectedDialog || inProgress || !isSavingAllowed)
            ImGui::EndDisabled();

          ImGui::Separator();

          if (!isSelectedDialog || inProgress || !isExportAllowed)
            ImGui::BeginDisabled();

          if (ImGui::MenuItem(g_LocalizationManager.Localize("ARCHIVE_DIALOG_EXPORT_TO").c_str()))
          {
            assert(s_SelectedDialog);
            s_SelectedDialog->Export(BrowseDirectoryDialog());
          }

          if (!isSelectedDialog || inProgress || !isExportAllowed)
            ImGui::EndDisabled();

          if (!isSelectedDialog || inProgress || !isImportAllowed)
            ImGui::BeginDisabled();

          if (ImGui::MenuItem(g_LocalizationManager.Localize("ARCHIVE_DIALOG_IMPORT_FROM").c_str()))
          {
            assert(s_SelectedDialog);
            s_SelectedDialog->Import(BrowseDirectoryDialog());
          }

          if (!isSelectedDialog || inProgress || !isImportAllowed)
            ImGui::EndDisabled();

          ImGui::Separator();

          ImGui::MenuItem(g_LocalizationManager.Localize("SETTINGS_DIALOG_TITLE").c_str(), nullptr, &Options::Get().opened);

          ImGui::Separator();

          if (ImGui::MenuItem(g_LocalizationManager.Localize("ARCHIVE_DIALOG_EXIT").c_str()))
            stop();

          ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
      }

      for (auto hitmanDialogIt = s_Dialogs.begin(); hitmanDialogIt != s_Dialogs.end(); ++hitmanDialogIt)
      {
        auto& hitmanDialog = *hitmanDialogIt;
        ImGui::SetNextWindowDockID(dockID, ImGuiCond_Once);
        switch (hitmanDialog->DrawDialog())
        {
          case -1: {
            if (s_SelectedDialog == hitmanDialog)
              s_SelectedDialog.reset();

            hitmanDialogIt = s_Dialogs.erase(hitmanDialogIt);
            break;
          }
          case 0: {
            break;
          }
          case 1: {
            s_SelectedDialog = hitmanDialog;
            break;
          }
        }
        if (hitmanDialogIt == s_Dialogs.end())
          break;
      }

      Options::Get().DrawDialog();
    }

    // Rendering
    ImGui::Render();

    SDL_SetRenderDrawColor(m_window->get_native_renderer(), 100, 100, 100, 255);
    SDL_RenderClear(m_window->get_native_renderer());
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
    SDL_RenderPresent(m_window->get_native_renderer());
  }

  return m_exit_status;
}

void App::Application::stop()
{
  for (auto& hitmanDialog : s_Dialogs)
  {
    if (hitmanDialog->IsInProgress())
      return;
  }

  m_running = false;

  for (auto& hitmanDialog : s_Dialogs)
  {
    switch (hitmanDialog->UnsavedChangesPopup())
    {
      case 1: {
        hitmanDialog->Save(hitmanDialog->GetPath(), false);
        break;
      }
      case 0: {
        break;
      }
      default:
      case -1: {
        m_running = true;
        break;
      }
    }

    if (m_running)
      break;
  }
}

void Application::on_event(const SDL_WindowEvent& event) {
  switch (event.event) {
    case SDL_WINDOWEVENT_CLOSE:
      return on_close();
    case SDL_WINDOWEVENT_MINIMIZED:
      return on_minimize();
    case SDL_WINDOWEVENT_RESTORED:
      return on_shown();
  }
}

void Application::on_minimize() {
  m_minimized = true;
}

void Application::on_shown() {
  m_minimized = false;
}

void Application::on_close() {
  stop();
}

}  // namespace App
