//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2

#include <Precompiled.hpp>

#include "Options.hpp"

#include "Utils.hpp"

void CommonSettings::Load(const toml::table &aInputRoot)
{
  ResetToDefaults();

  const auto& commonTable = aInputRoot["common"];

  disableWarnings = commonTable["disable_warnings"].value_or(disableWarnings);
  importSameFiles = commonTable["import_original_files"].value_or(importSameFiles);
  fixChannels = commonTable["fix_channels"].value_or(fixChannels);
  fixSampleRate = commonTable["fix_sample_rate"].value_or(fixSampleRate);
  transcodeToOriginalFormat = commonTable["transcode_to_original_format"].value_or(transcodeToOriginalFormat);
  directImport = commonTable["direct_import"].value_or(directImport);
  transcodeToPlayableFormat = commonTable["transcode_to_playable_format"].value_or(transcodeToPlayableFormat);
  transcodeOGGToPCM = commonTable["transcode_ogg_to_pcm"].value_or(transcodeOGGToPCM);

  g_LocalizationManager.SetLanguage(commonTable["language"].value_or(g_LocalizationManager.GetLanguage().native()));
}

void CommonSettings::Save(toml::table &aOutputRoot) const
{
  toml::table commonTable;

  commonTable.emplace("disable_warnings", disableWarnings);
  commonTable.emplace("import_original_files", importSameFiles);
  commonTable.emplace("fix_channels", fixChannels);
  commonTable.emplace("fix_sample_rate", fixSampleRate);
  commonTable.emplace("transcode_to_original_format", transcodeToOriginalFormat);
  commonTable.emplace("direct_import", directImport);
  commonTable.emplace("transcode_to_playable_format", transcodeToPlayableFormat);
  commonTable.emplace("transcode_ogg_to_pcm", transcodeOGGToPCM);

  commonTable.emplace("language", g_LocalizationManager.GetLanguage().native());

  aOutputRoot.emplace("common", std::move(commonTable));
}

void CommonSettings::ResetToDefaults()
{
  *this = {};
}

void CommonSettings::DrawDialog()
{
  if (!ImGui::BeginTabItem(g_LocalizationManager.Localize("SETTINGS_DIALOG_COMMON_GROUP").c_str()))
    return;

  if (ImGui::BeginCombo(g_LocalizationManager.Localize("SETTINGS_DIALOG_LANGUAGE").c_str(), g_LocalizationManager.GetLanguage().c_str()))
  {
    const auto availableLanguages = g_LocalizationManager.GetAvailableLanguages();
    for (const auto& language : availableLanguages)
    {
      bool selected = g_LocalizationManager.GetLanguage() == language;
      if (ImGui::Selectable(language.c_str(), &selected))
        g_LocalizationManager.SetLanguage(language);
    }
    ImGui::EndCombo();
  }

  ImGui::Checkbox(g_LocalizationManager.Localize("SETTINGS_DIALOG_DISABLE_WARNINGS").c_str(), &disableWarnings);

  ImGui::Checkbox(g_LocalizationManager.Localize("SETTINGS_DIALOG_IMPORT_ORIGINAL_FILES").c_str(), &importSameFiles);

  ImGui::Checkbox(g_LocalizationManager.Localize("SETTINGS_DIALOG_DIRECT_IMPORT").c_str(),
                  &directImport);

  if (directImport)
    ImGui::BeginDisabled();

  ImGui::TreePush(&directImport);

  fixChannels &= !directImport;
  ImGui::Checkbox(g_LocalizationManager.Localize("SETTINGS_DIALOG_FIX_CHANNELS").c_str(), &fixChannels);

  fixSampleRate &= !directImport;
  ImGui::Checkbox(g_LocalizationManager.Localize("SETTINGS_DIALOG_FIX_SAMPLE_RATE").c_str(), &fixSampleRate);

  transcodeToOriginalFormat &= !directImport;
  ImGui::Checkbox(g_LocalizationManager.Localize("SETTINGS_DIALOG_TRANSCODE_TO_ORIGINAL_FORMAT").c_str(), &transcodeToOriginalFormat);

  ImGui::TreePop();

  if (directImport)
    ImGui::EndDisabled();

  ImGui::Checkbox(g_LocalizationManager.Localize("SETTINGS_DIALOG_TRANSCODE_TO_PLAYABLE_FORMAT").c_str(), &transcodeToPlayableFormat);

  if (!transcodeToPlayableFormat)
    ImGui::BeginDisabled();

  ImGui::TreePush(&transcodeToPlayableFormat);

  transcodeOGGToPCM &= transcodeToPlayableFormat;
  ImGui::Checkbox(g_LocalizationManager.Localize("SETTINGS_DIALOG_TRANSCODE_OGG_TO_PCM").c_str(), &transcodeOGGToPCM);

  if (!transcodeToPlayableFormat)
    ImGui::EndDisabled();

  ImGui::TreePop();

  ImGui::EndTabItem();
}

void Hitman4Settings::Load(const toml::table &aInputRoot)
{
  ResetToDefaults();

  const auto& hitman4Table = aInputRoot["hitman4"];

  exportWithLIPData = hitman4Table["export_with_lip_data"].value_or(exportWithLIPData);
}

void Hitman4Settings::Save(toml::table &aOutputRoot) const
{
  toml::table hitman4Table;

  hitman4Table.emplace("export_with_lip_data", exportWithLIPData);

  aOutputRoot.emplace("hitman4", std::move(hitman4Table));
}

void Hitman4Settings::ResetToDefaults()
{
  *this = {};
}

void Hitman4Settings::DrawDialog()
{
  if (!ImGui::BeginTabItem(g_LocalizationManager.Localize("SETTINGS_DIALOG_HITMAN4_GROUP").c_str()))
    return;

  ImGui::Checkbox(g_LocalizationManager.Localize("SETTINGS_DIALOG_HITMAN4_EXPORT_WITH_LIP_DATA").c_str(), &exportWithLIPData);

  ImGui::EndTabItem();
}

void Options::Load()
{
  auto optionsPath = GetProgramPath().path();
  if (optionsPath.empty())
    return;

  optionsPath /= L"config.toml";

  auto config = toml::parse_file(optionsPath.native());
  if (config.failed())
    return;

  common.Load(config);
  hitman4.Load(config);
}

void Options::Save() const
{
  auto optionsPath = GetProgramPath().path();
  if (optionsPath.empty())
    return;

  optionsPath /= L"config.toml";

  toml::table config;

  common.Save(config);
  hitman4.Save(config);

  std::ofstream(optionsPath, std::ios::trunc) << config;
}

void Options::ResetToDefaults()
{
  common.ResetToDefaults();
  hitman4.ResetToDefaults();

  Save();
}

void Options::DrawDialog()
{
  if (!opened)
    return;

  if (!ImGui::Begin(g_LocalizationManager.Localize("SETTINGS_DIALOG_TITLE").c_str(), &opened))
    return;

  const auto beforeUpdate = *this;

  if (ImGui::BeginTabBar("##SettingCategoryTabs"))
  {
    common.DrawDialog();
    hitman4.DrawDialog();

    ImGui::EndTabBar();
  }

  if (beforeUpdate != *this)
    Save();

  ImGui::End();
}
