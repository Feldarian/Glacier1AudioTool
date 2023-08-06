//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#include <Precompiled.hpp>

#include "Utils.hpp"

#include <Config/Config.hpp>

#include "Options.hpp"

#include <commdlg.h>
#include <shlobj.h>

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
    return 0;

  return DisplayMessage(message, title, yesNo, SDL_MESSAGEBOX_INFORMATION);
}

int32_t DisplayWarning(const StringView8 &message, const StringView8 &title, bool yesNo, const Options &options)
{
  if (!yesNo && options.common.disableWarnings)
    return 0;

  return DisplayMessage(message, title, yesNo, SDL_MESSAGEBOX_WARNING);
}

int32_t DisplayError(const StringView8 &message, const StringView8 &title, bool yesNo)
{
  return DisplayMessage(message, title, yesNo, SDL_MESSAGEBOX_ERROR);
}

int32_t DisplayMessage(const StringView8 &message, const StringView8 &title, bool yesNo, const uint32_t messageFlags)
{
  static std::array yesNoButtonDatas{SDL_MessageBoxButtonData{0, 2, g_LocalizationManager.Localize("MESSAGEBOX_BUTTON_YES").c_str()},
    SDL_MessageBoxButtonData{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, g_LocalizationManager.Localize("MESSAGEBOX_BUTTON_NO").c_str()},
    SDL_MessageBoxButtonData{SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, g_LocalizationManager.Localize("MESSAGEBOX_BUTTON_CANCEL").c_str()}};
  static SDL_MessageBoxButtonData okButtonData{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT | SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, g_LocalizationManager.Localize("MESSAGEBOX_BUTTON_OK").c_str()};
  static SDL_MessageBoxColorScheme colorScheme{
    {
      SDL_MessageBoxColor{static_cast<uint8_t>(0.94f * 0.06f * 255), static_cast<uint8_t>(0.94f * 0.06f * 255), static_cast<uint8_t>(0.94f * 0.06f * 255)},
      SDL_MessageBoxColor{static_cast<uint8_t>(1.00f * 1.00f * 255), static_cast<uint8_t>(1.00f * 1.00f * 255), static_cast<uint8_t>(1.00f * 1.00f * 255)},
      SDL_MessageBoxColor{static_cast<uint8_t>(0.50f * 0.43f * 255), static_cast<uint8_t>(0.50f * 0.43f * 255), static_cast<uint8_t>(0.50f * 0.50f * 255)},
      SDL_MessageBoxColor{static_cast<uint8_t>(0.40f * 0.26f * 255), static_cast<uint8_t>(0.40f * 0.59f * 255), static_cast<uint8_t>(0.40f * 0.98f * 255)},
      SDL_MessageBoxColor{static_cast<uint8_t>(1.00f * 0.06f * 255), static_cast<uint8_t>(1.00f * 0.53f * 255), static_cast<uint8_t>(1.00f * 0.98f * 255)}
    }};

  int32_t outButton = 0;
  SDL_MessageBoxData data{messageFlags, nullptr, title.c_str(), message.c_str(), yesNo ? 3 : 1, yesNo ? yesNoButtonDatas.data() : &okButtonData, &colorScheme};
  if (const auto err = SDL_ShowMessageBox(&data, &outButton))
  {
    [[maybe_unused]] const String8 errorMessage = SDL_GetError();
    SDL_ClearError();
  }
  return outButton;
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
