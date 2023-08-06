//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#include <Precompiled.hpp>

#include "Options.hpp"
#include "Utils.hpp"

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
  BROWSEINFOA bi;
  ZeroMemory(&bi, sizeof(bi));
  bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

  const auto pidl = SHBrowseForFolderA(&bi);
  if (!pidl)
    return "";

  String8CI path;
  path.resize(UINT16_MAX, L'\0');
  SHGetPathFromIDListA(pidl, path.data());

  IMalloc *imalloc = nullptr;
  if (SUCCEEDED(SHGetMalloc(&imalloc)))
  {
    imalloc->Free(pidl);
    imalloc->Release();
  }

  path.resize(strlen(path.c_str()));
  return path;
}

std::pair<String8CI, uint32_t> OpenFileDialog(const StringView8CI &filters, const StringView8CI &defaultFileName)
{
  OPENFILENAMEA ofn;
  ZeroMemory(&ofn, sizeof(ofn));

  String8CI fileName{ defaultFileName };
  fileName.resize(UINT16_MAX, '\0');

  // Initialize OPENFILENAME
  ofn.lStructSize = sizeof(ofn);
  ofn.lpstrFile = fileName.data();
  ofn.nMaxFile = static_cast<uint32_t>(fileName.size());
  ofn.lpstrFilter = filters.c_str();
  ofn.nFilterIndex = 0;
  ofn.lpstrFileTitle = nullptr;
  ofn.nMaxFileTitle = 0;
  ofn.Flags = OFN_LONGNAMES | OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

  if (!GetOpenFileNameA(&ofn))
    return {{}, 0};

  fileName.resize(strlen(fileName.c_str()));

  if (fileName.native().rfind('.') == String8CI::npos)
    return {{}, 0};

  return {fileName, ofn.nFilterIndex};
}

std::pair<String8CI, uint32_t> SaveFileDialog(const StringView8CI &filters, const StringView8CI &defaultFileName)
{
  OPENFILENAMEA ofn;
  ZeroMemory(&ofn, sizeof(ofn));

  String8CI fileName{ defaultFileName };
  fileName.resize(UINT16_MAX, '\0');

  // Initialize OPENFILENAME
  ofn.lStructSize = sizeof(ofn);
  ofn.lpstrFile = fileName.data();
  ofn.nMaxFile = static_cast<uint32_t>(fileName.size());
  ofn.lpstrFilter = filters.c_str();
  ofn.nFilterIndex = 0;
  ofn.lpstrFileTitle = nullptr;
  ofn.nMaxFileTitle = 0;
  ofn.Flags = OFN_LONGNAMES | OFN_EXPLORER | OFN_OVERWRITEPROMPT;

  if (!GetSaveFileNameA(&ofn))
    return {};

  fileName.resize(strlen(fileName.c_str()));

  if (fileName.native().rfind('.') == String8CI::npos)
    return {{}, 0};

  return {fileName, ofn.nFilterIndex};
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

  programPath.resize(UINT16_MAX, L'\0');

  const auto requiredLength = GetModuleFileNameA(nullptr, programPath.data(), static_cast<uint32_t>(programPath.size()));
  if (requiredLength >= programPath.size() || requiredLength == 0)
    return "";

  programPath.resize(requiredLength);

  const auto lastSeparatorPosition = programPath.native().find_last_of("\\/");
  if (lastSeparatorPosition == String8CI::npos || lastSeparatorPosition == 0)
    return "";

  programPath.resize(lastSeparatorPosition);
  return programPath;
}

int32_t DisplayError(const StringView8 &message, const StringView8 &title, const bool yesNo)
{
  return MessageBoxA(nullptr, message.c_str(), title.c_str(), MB_ICONERROR | (yesNo ? MB_YESNOCANCEL : MB_OK));
}

int32_t DisplayWarning(const StringView8 &message, const StringView8 &title, const bool yesNo, const Options &options)
{
  if (!yesNo && options.common.disableWarnings)
    return IDCLOSE;

  return MessageBoxA(nullptr, message.c_str(), title.c_str(), MB_ICONWARNING | (yesNo ? MB_YESNOCANCEL : MB_OK));
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

String8CI MakeFileDialogFilter(const std::vector<std::pair<String8, String8CI>> &filters)
{
  if (filters.empty())
    return {};

  String8CI allFiltersFilter;
  String8CI allFiltersDisplay;
  for (const auto& filter : filters | ranges::views::values)
  {
    if (!allFiltersFilter.empty())
      allFiltersFilter += ";";

    allFiltersFilter += filter;

    if (!allFiltersDisplay.empty())
      allFiltersDisplay += "/";

    allFiltersDisplay += filter;
  }

  String8CI result;

  if (filters.size() > 1)
    result = Format("{0} ({1})?{2}?", g_LocalizationManager.Localize("FILE_DIALOG_FILTER_ALL_SUPPORTED"), allFiltersDisplay, allFiltersFilter);

  for (const auto& [identifier, filter] : filters)
    result += Format("{0} ({1})?{1}?", g_LocalizationManager.Localize(identifier), filter);

  ranges::replace(result, '?', '\0');

  return result;
}
