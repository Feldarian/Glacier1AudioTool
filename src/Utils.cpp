//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#include "Precompiled.hpp"

#include "Utils.hpp"

std::vector<char> ReadWholeBinaryFile(const std::filesystem::path &acpPath)
{
  if (acpPath.empty() || !exists(acpPath))
    return {};

  const auto oldSync = std::ios_base::sync_with_stdio(false);

  std::ifstream file{acpPath, std::ios::binary};
  std::vector<char> result(file_size(acpPath), 0);
  file.read(result.data(), result.size());

  std::ios_base::sync_with_stdio(oldSync);

  return result;
}

std::string ReadWholeTextFile(const std::filesystem::path &acpPath)
{
  if (acpPath.empty() || !exists(acpPath))
    return {};

  const auto oldSync = std::ios_base::sync_with_stdio(false);

  std::ifstream file{acpPath, std::ios::binary};
  std::string result(file_size(acpPath), 0);
  file.read(result.data(), result.size());

  std::ios_base::sync_with_stdio(oldSync);

  return result;
}

float GetAlignedItemWidth(const int64_t acItemsCount)
{
  return (ImGui::GetContentRegionAvail().x - static_cast<float>(acItemsCount - 1) * ImGui::GetStyle().ItemSpacing.x) /
         static_cast<float>(acItemsCount);
}

std::wstring BrowseDirectoryDialog()
{
  BROWSEINFOW bi;
  ZeroMemory(&bi, sizeof(bi));
  bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

  const auto pidl = SHBrowseForFolderW(&bi);
  if (!pidl)
    return L"";

  // get the name of the folder and put it in path
  std::wstring path(UINT16_MAX, L'\0');
  SHGetPathFromIDListW(pidl, path.data());

  // free memory used
  IMalloc *imalloc = nullptr;
  if (SUCCEEDED(SHGetMalloc(&imalloc)))
  {
    imalloc->Free(pidl);
    imalloc->Release();
  }

  path.resize(wcslen(path.c_str()));
  return path;
}

std::wstring OpenFileDialog(std::wstring_view filters, std::wstring fileName)
{
  OPENFILENAMEW ofn;
  ZeroMemory(&ofn, sizeof(ofn));

  fileName.resize(UINT16_MAX, '\0');

  // Initialize OPENFILENAME
  ofn.lStructSize = sizeof(ofn);
  ofn.lpstrFile = fileName.data();
  ofn.nMaxFile = static_cast<uint32_t>(fileName.size());
  ofn.lpstrFilter = filters.data();
  ofn.nFilterIndex = 0;
  ofn.lpstrFileTitle = nullptr;
  ofn.nMaxFileTitle = 0;
  ofn.Flags = OFN_LONGNAMES | OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

  if (!GetOpenFileNameW(&ofn))
    return {};

  fileName.resize(wcslen(fileName.c_str()));

  if (fileName.rfind(L'.') != std::wstring::npos)
    return fileName;

  const wchar_t* defaultExt = filters.empty() ? nullptr : filters.data() + wcslen(filters.data()) + 1;
  if (defaultExt && defaultExt[0] && defaultExt[0] != L'.')
    defaultExt = wcschr(defaultExt, L'.');

  if (defaultExt && defaultExt[0] && defaultExt[1] != L'*')
  {
    std::filesystem::path fullPath = fileName + defaultExt;
    if (!exists(fullPath))
      fullPath.clear();

    return fullPath.native();
  }

  std::filesystem::path filePath = fileName;
  const auto allFilesInParent = GetAllFilesInDirectory(filePath.parent_path(), L"", false);
  if (allFilesInParent.empty())
    return fileName;

  for (const auto& fileInParent : allFilesInParent)
  {
    if (fileInParent.native().find(fileName) != std::wstring::npos)
      return fileInParent.native();
  }

  return fileName;
}

std::vector<std::wstring> OpenFileDialogMultiple(std::wstring_view filters, std::wstring fileName)
{
  OPENFILENAMEW ofn;
  ZeroMemory(&ofn, sizeof(ofn));

  fileName.resize(UINT16_MAX, '\0');

  // Initialize OPENFILENAME
  ofn.lStructSize = sizeof(ofn);
  ofn.lpstrFile = fileName.data();
  ofn.nMaxFile = static_cast<uint32_t>(fileName.size());
  ofn.lpstrFilter = filters.data();
  ofn.nFilterIndex = 0;
  ofn.lpstrFileTitle = nullptr;
  ofn.nMaxFileTitle = 0;
  ofn.Flags = OFN_LONGNAMES | OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT;

  if (!GetOpenFileNameW(&ofn))
    return {};

  const auto *openListPtr = fileName.data();
  std::filesystem::path openPath = openListPtr;
  if (openPath.has_extension())
    openPath = openPath.parent_path();

  openListPtr += openPath.native().size() + 1;

  std::vector<std::wstring> out;
  do
  {
    out.emplace_back((openPath / openListPtr).native());
    openListPtr += wcslen(openListPtr) + 1;
  } while (*openListPtr);

  if (out.empty())
    return out;

  const wchar_t* defaultExt = filters.empty() ? nullptr : filters.data() + wcslen(filters.data()) + 1;
  if (defaultExt && defaultExt[0] && defaultExt[0] != L'.')
    defaultExt = wcschr(defaultExt, L'.');

  for (auto outIt = out.begin(); outIt != out.end();)
  {
    auto& outFileName = *outIt;

    if (outFileName.rfind(L'.') != std::wstring::npos)
    {
      ++outIt;
      continue;
    }

    if (defaultExt && defaultExt[0] && defaultExt[1] != L'*')
    {
      outFileName += defaultExt;
      outIt = std::filesystem::exists(outFileName) ? outIt + 1 : out.erase(outIt);
      continue;
    }

    std::filesystem::path filePath = outFileName;
    const auto allFilesInParent = GetAllFilesInDirectory(filePath.parent_path(), L"", false);
    if (allFilesInParent.empty())
    {
      ++outIt;
      continue;
    }

    bool foundInParent = false;
    for (const auto& fileInParent : allFilesInParent)
    {
      if (fileInParent.native().find(fileName) != std::wstring::npos)
      {
        outFileName = fileInParent.native();
        foundInParent = true;
        break;
      }
    }
    outIt = foundInParent ? outIt + 1 : out.erase(outIt);
  }


  return out;
}

std::wstring SaveFileDialog(std::wstring_view filters, std::wstring fileName)
{
  OPENFILENAMEW ofn;
  ZeroMemory(&ofn, sizeof(ofn));

  fileName.resize(UINT16_MAX, '\0');

  // Initialize OPENFILENAME
  ofn.lStructSize = sizeof(ofn);
  ofn.lpstrFile = fileName.data();
  ofn.nMaxFile = static_cast<uint32_t>(fileName.size());
  ofn.lpstrFilter = filters.data();
  ofn.nFilterIndex = 0;
  ofn.lpstrFileTitle = nullptr;
  ofn.nMaxFileTitle = 0;
  ofn.Flags = OFN_LONGNAMES | OFN_EXPLORER | OFN_OVERWRITEPROMPT;

  if (!GetSaveFileNameW(&ofn))
    return {};

  fileName.resize(wcslen(fileName.c_str()));

  if (fileName.rfind(L'.') != std::wstring::npos)
    return fileName;

  const wchar_t* defaultExt = filters.empty() ? nullptr : filters.data() + wcslen(filters.data()) + 1;
  if (defaultExt && defaultExt[0] && defaultExt[0] != L'.')
    defaultExt = wcschr(defaultExt, L'.');

  if (defaultExt && defaultExt[0] && defaultExt[1] != L'*')
    return fileName + defaultExt;

  return fileName;
}

std::vector<std::filesystem::path> GetAllFilesInDirectory(const std::filesystem::path &directory,
                                                          std::wstring_view extension, bool recursive)
{
  std::vector<std::filesystem::path> filesWithExtension;

  for (const auto &directoryEntry : std::filesystem::directory_iterator(directory))
  {
    if (directoryEntry.is_directory() && recursive)
    {
      const auto filesWithExtensionInThisDirectory =
          GetAllFilesInDirectory(directoryEntry.path(), extension, recursive);
      filesWithExtension.insert(filesWithExtension.end(), filesWithExtensionInThisDirectory.cbegin(),
                                filesWithExtensionInThisDirectory.cend());
    }

    if (directoryEntry.is_regular_file() &&
        (extension.empty() || _wcsicmp(directoryEntry.path().extension().c_str(), extension.data()) == 0))
      filesWithExtension.emplace_back(directoryEntry);
  }

  return filesWithExtension;
}

const std::wstring &GetProgramPath()
{
  static std::wstring outPath;
  if (!outPath.empty())
    return outPath;

  outPath.resize(UINT16_MAX, L'\0');
  const auto dwRetVal = GetModuleFileNameW(nullptr, outPath.data(), static_cast<uint32_t>(outPath.size()));
  if (dwRetVal >= outPath.size() || dwRetVal == 0)
  {
    outPath.clear();
    return outPath;
  }

  outPath.resize(wcslen(outPath.c_str()));

  const auto lastSeparatorPosition = outPath.find_last_of(L"\\/");
  if (lastSeparatorPosition == std::wstring::npos || lastSeparatorPosition == 0)
  {
    outPath.clear();
    return outPath;
  }

  outPath.resize(lastSeparatorPosition);
  return outPath;
}

int32_t DisplayError(const StringView8 message, StringView8 title, bool yesNo)
{
  if (title.empty())
    title = LocalizationManager::Get().Localize("MESSAGEBOX_ERROR_GENERIC_TITLE");

  return MessageBoxA(nullptr, message.data(), title.data(), MB_ICONERROR | (yesNo ? MB_YESNOCANCEL : MB_OK));
}

int32_t DisplayWarning(const StringView8 message, StringView8 title, bool yesNo)
{
  if (!yesNo && Options::Get().common.disableWarnings)
    return IDCLOSE;

  if (title.empty())
    title = LocalizationManager::Get().Localize("MESSAGEBOX_WARNING_GENERIC_TITLE");

  return MessageBoxA(nullptr, message.data(), title.data(), MB_ICONWARNING | (yesNo ? MB_YESNOCANCEL : MB_OK));
}

std::vector<std::wstring_view> GetPathStems(const std::filesystem::path& path)
{
  if (!path.has_parent_path())
    return {path.native()};

  std::wstring_view pathView = path.native();

  std::vector<std::wstring_view> pathStems;
  while (!pathView.empty())
  {
    auto pathDelim = pathView.rfind(L'/');
    if (pathDelim == std::wstring_view::npos)
      pathDelim = pathView.rfind(L'\\');

    if (pathDelim != std::wstring_view::npos)
    {
      pathStems.emplace_back(pathView.substr(pathDelim + 1));
      pathView = pathView.substr(0, pathDelim);
    }
    else
    {
      pathStems.emplace_back(pathView);
      pathView = L"";
    }
  }

  return pathStems;
}

std::filesystem::path ChangeExtension(const std::filesystem::path& path, std::wstring_view newExtension)
{
  const auto& pathNative = path.native();
  const auto pathNativeExtPos = pathNative.rfind(L'.');
  if (pathNativeExtPos == std::wstring::npos)
    return {pathNative + newExtension.data()};

  auto newPath = pathNative.substr(0, pathNativeExtPos);
  newPath += newExtension.data();
  return {newPath};
}
