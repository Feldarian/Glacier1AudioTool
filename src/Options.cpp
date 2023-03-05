//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2

#include "Precompiled.hpp"

#include "Options.hpp"

void CommonSettings::Load(const toml::table &aInputRoot)
{
  ResetToDefaults();

  const auto& commonTable = aInputRoot["common"];

  disableWarnings = commonTable["disable_warnings"].value_or(disableWarnings);
  convertToGameFormat = commonTable["convert_to_game_format"].value_or(convertToGameFormat);
  checkOriginality = commonTable["check_originality"].value_or(checkOriginality);
  importOriginalFiles = commonTable["import_original_files"].value_or(importOriginalFiles);
  fixChannels = commonTable["fix_channels"].value_or(fixChannels);
  fixSampleRate = commonTable["fix_sample_rate"].value_or(fixSampleRate);
  transcodeToOriginalFormat = commonTable["transcode_to_original_format"].value_or(transcodeToOriginalFormat);
  directImport = commonTable["direct_import"].value_or(directImport);

  LocalizationManager.SetLanguage(commonTable["language"].value_or(LocalizationManager.GetLanguage()));
}

void CommonSettings::Save(toml::table &aOutputRoot) const
{
  toml::table commonTable;
  
  commonTable.emplace("disable_warnings", disableWarnings);
  commonTable.emplace("convert_to_game_format", convertToGameFormat);
  commonTable.emplace("check_originality", checkOriginality);
  commonTable.emplace("import_original_files", importOriginalFiles);
  commonTable.emplace("fix_channels", fixChannels);
  commonTable.emplace("fix_sample_rate", fixSampleRate);
  commonTable.emplace("transcode_to_original_format", transcodeToOriginalFormat);
  commonTable.emplace("direct_import", directImport);

  commonTable.emplace("language", LocalizationManager.GetLanguage());

  aOutputRoot.emplace("common", std::move(commonTable));
}

void CommonSettings::ResetToDefaults()
{
  *this = {};
}

void CommonSettings::DrawDialog()
{
  if (!ImGui::TreeNodeEx(LocalizationManager.Localize("SETTINGS_DIALOG_COMMON_GROUP").c_str(), ImGuiTreeNodeFlags_DefaultOpen))
    return;
  
  if (ImGui::BeginCombo(LocalizationManager.Localize("SETTINGS_DIALOG_LANGUAGE").c_str(), LocalizationManager.GetLanguage().c_str()))
  {
    const auto availableLanguages = LocalizationManager.GetAvailableLanguages();
    for (const auto& language : availableLanguages)
    {
      bool selected = UTFCaseInsensitiveCompare(LocalizationManager.GetLanguage(), language) == 0;
      if (ImGui::Selectable(language.c_str(), &selected))
        LocalizationManager.SetLanguage(language);
    }
    ImGui::EndCombo();
  }

  ImGui::Checkbox(LocalizationManager.Localize("SETTINGS_DIALOG_DISABLE_WARNINGS").c_str(), &disableWarnings);

  ImGui::Checkbox(LocalizationManager.Localize("SETTINGS_DIALOG_CHECK_DATA_ORIGINALITY").c_str(), &checkOriginality);

  if (!checkOriginality && !importOriginalFiles)
    importOriginalFiles = true;

  if (!checkOriginality)
    ImGui::BeginDisabled();

  ImGui::TreePush(&checkOriginality);

  ImGui::Checkbox(LocalizationManager.Localize("SETTINGS_DIALOG_IMPORT_ORIGINAL_FILES").c_str(),
                  &importOriginalFiles);

  ImGui::TreePop();

  if (!checkOriginality)
    ImGui::EndDisabled();

  ImGui::Checkbox(LocalizationManager.Localize("SETTINGS_DIALOG_DIRECT_IMPORT").c_str(),
                  &directImport);

  if (directImport)
    ImGui::BeginDisabled();

  ImGui::TreePush(&directImport);

  convertToGameFormat &= !directImport;
  ImGui::Checkbox(LocalizationManager.Localize("SETTINGS_DIALOG_CONVERT_TO_GAME_FORMAT").c_str(),
                  &convertToGameFormat);

  if (!convertToGameFormat)
    ImGui::BeginDisabled();

  ImGui::TreePush(&convertToGameFormat);
    
  fixChannels &= convertToGameFormat;
  ImGui::Checkbox(LocalizationManager.Localize("SETTINGS_DIALOG_FIX_CHANNELS").c_str(), &fixChannels);

  fixSampleRate &= convertToGameFormat;
  ImGui::Checkbox(LocalizationManager.Localize("SETTINGS_DIALOG_FIX_SAMPLE_RATE").c_str(), &fixSampleRate);
  
  transcodeToOriginalFormat &= convertToGameFormat;
  ImGui::Checkbox(LocalizationManager.Localize("SETTINGS_DIALOG_TRANSCODE_TO_ORIGINAL_FORMAT").c_str(), &transcodeToOriginalFormat);

  ImGui::TreePop();

  if (!convertToGameFormat)
    ImGui::EndDisabled();

  ImGui::TreePop();

  if (directImport)
    ImGui::EndDisabled();

  ImGui::TreePop();
}

void Options::Load()
{
  std::filesystem::path optionsPath = GetProgramPath();
  if (optionsPath.empty())
    return;

  optionsPath /= L"config.toml";

  auto config = toml::parse_file(optionsPath.native());
  if (config.failed())
    return;

  common.Load(config);
}

void Options::Save() const
{
  std::filesystem::path optionsPath = GetProgramPath();
  if (optionsPath.empty())
    return;

  optionsPath /= L"config.toml";

  toml::table config;
  
  common.Save(config);

  std::ofstream(optionsPath, std::ios::trunc) << config;
}

void Options::ResetToDefaults()
{
  common.ResetToDefaults();

  Save();
}

void Options::DrawDialog()
{
  if (!ImGui::BeginTabItem(LocalizationManager.Localize("SETTINGS_DIALOG_TITLE").c_str()))
    return;

  common.DrawDialog();

  ImGui::EndTabItem();
}
