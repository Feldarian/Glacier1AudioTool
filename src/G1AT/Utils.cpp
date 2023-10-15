//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#include <Precompiled.hpp>

#include "Utils.hpp"

#include <Config/Config.hpp>

#include "Options.hpp"

std::vector<char> ReadWholeBinaryFile(const StringView8CI &acpPath)
{
  const auto path = acpPath.path();
  if (path.empty() || !exists(path))
    return {};

  const auto oldSync = std::ios_base::sync_with_stdio(false);

  std::ifstream file{path, std::ios::binary};
  std::vector<char> result(file_size(path), 0);
  file.read(result.data(), result.size());

  std::ios_base::sync_with_stdio(oldSync);

  return result;
}

String8 ReadWholeTextFile(const StringView8CI &acpPath)
{
  const auto path = acpPath.path();
  if (path.empty() || !exists(path))
    return {};

  const auto oldSync = std::ios_base::sync_with_stdio(false);

  String8 result;
  result.resize(file_size(path), 0);

  std::ifstream file{path, std::ios::binary};
  file.read(result.data(), result.size());

  std::ios_base::sync_with_stdio(oldSync);

  return result;
}

float GetAlignedItemWidth(const int64_t acItemsCount)
{
  return (ImGui::GetContentRegionAvail().x - static_cast<float>(acItemsCount - 1) * ImGui::GetStyle().ItemSpacing.x) /
         static_cast<float>(acItemsCount);
}

String8CI BrowseDirectoryDialog()
{
  if (const auto* directoryPath = tinyfd_selectFolderDialog(nullptr, nullptr))
    return {directoryPath, std::strlen(directoryPath)};

  return {};
}

String8CI OpenFileDialog(const std::vector<std::pair<StringView8CI, StringView8>> &filters, const StringView8CI &defaultFileName)
{
  std::vector<const char*> filtersTransformed;
  filtersTransformed.reserve(filters.size());
  for (const auto &filter : filters | ranges::views::keys)
    filtersTransformed.emplace_back(filter.c_str());

  if (const auto* filePath = tinyfd_openFileDialog(nullptr, defaultFileName.c_str(), static_cast<int32_t>(filtersTransformed.size()), filtersTransformed.data(), g_LocalizationManager.Localize("FILE_DIALOG_FILTER_ALL_SUPPORTED").c_str(), 0))
    return {filePath, std::strlen(filePath)};

  return {};
}

String8CI SaveFileDialog(const std::vector<std::pair<StringView8CI, StringView8>> &filters, const StringView8CI &defaultFileName)
{
  std::vector<const char*> filtersTransformed;
  filtersTransformed.reserve(filters.size());
  for (const auto &filter : filters | ranges::views::keys)
    filtersTransformed.emplace_back(filter.c_str());

  if (const auto* filePath = tinyfd_saveFileDialog(nullptr, defaultFileName.c_str(), static_cast<int32_t>(filtersTransformed.size()), filtersTransformed.data(), g_LocalizationManager.Localize("FILE_DIALOG_FILTER_ALL_SUPPORTED").c_str()))
    return {filePath, std::strlen(filePath)};

  return {};
}

std::vector<String8CI> GetAllFilesInDirectory(const StringView8CI &directory, const StringView8CI &extension, bool recursive)
{
  std::vector<String8CI> filesWithExtension;

  for (const auto &directoryEntry : std::filesystem::directory_iterator(directory.path()))
  {
    if (directoryEntry.is_directory() && recursive)
    {
      const auto filesWithExtensionInThisDirectory =
          GetAllFilesInDirectory(String8CI(directoryEntry.path()), extension, recursive);
      filesWithExtension.insert(filesWithExtension.end(), filesWithExtensionInThisDirectory.cbegin(),
                                filesWithExtensionInThisDirectory.cend());
    }

    if (directoryEntry.is_regular_file() &&
        (extension.empty() || extension == directoryEntry.path().extension()))
      filesWithExtension.emplace_back(directoryEntry);
  }

  return filesWithExtension;
}

StringView8CI GetProgramPath()
{
  static String8CI programPath;
  if (!programPath.empty())
    return programPath;

  auto* sdlBasePath = SDL_GetBasePath();
  programPath = sdlBasePath;
  SDL_free(sdlBasePath);

  return programPath;
}

StringView8CI GetUserPath()
{
  static String8CI userPath;
  if (!userPath.empty())
    return userPath;

  auto* sdlUserPath = SDL_GetPrefPath(G1AT_COMPANY_NAMESPACE, G1AT_NAME);
  userPath = sdlUserPath;
  SDL_free(sdlUserPath);

  return userPath;
}

int32_t DisplayInformation(const StringView8 &message, const StringView8 &title, bool yesNo, const Options &options)
{
  if (!yesNo && options.common.disableWarnings)
    return -1;

  const auto result = tinyfd_messageBox(title.c_str(), message.c_str(), yesNo ? "yesnocancel" : "ok", "info", yesNo ? 0 : 1);
  return result == 0 ? -1 : (result == 2 ? 0 : 1);
}

int32_t DisplayWarning(const StringView8 &message, const StringView8 &title, bool yesNo, const Options &options)
{
  if (!yesNo && options.common.disableWarnings)
    return -1;

  const auto result = tinyfd_messageBox(title.c_str(), message.c_str(), yesNo ? "yesnocancel" : "ok", "warning", yesNo ? 0 : 1);
  return result == 0 ? -1 : (result == 2 ? 0 : 1);
}

int32_t DisplayError(const StringView8 &message, const StringView8 &title, bool yesNo)
{
  const auto result = tinyfd_messageBox(title.c_str(), message.c_str(), yesNo ? "yesnocancel" : "ok", "error", yesNo ? 0 : 1);
  return result == 0 ? -1 : (result == 2 ? 0 : 1);
}

std::vector<StringView8CI> GetPathStems(StringView8CI pathView)
{
  if (!pathView.path().has_parent_path())
    return {pathView};


  std::vector<StringView8CI> pathStems;
  while (!pathView.empty())
  {
    auto pathDelim = pathView.native().rfind('/');
    if (pathDelim == StringView8CI::npos)
      pathDelim = pathView.native().rfind('\\');

    if (pathDelim != StringView8CI::npos)
    {
      pathStems.emplace_back(pathView.native().substr(pathDelim + 1));
      pathView = pathView.native().substr(0, pathDelim);
    }
    else
    {
      pathStems.emplace_back(pathView);
      pathView = "";
    }
  }

  return pathStems;
}
