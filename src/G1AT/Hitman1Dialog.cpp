//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#include <Precompiled.hpp>

#include "Hitman1Dialog.hpp"

#include "Utils.hpp"

bool Hitman1Dialog::Clear(const bool retVal)
{
  indexToKey.clear();
  lastModifiedDatesMap.clear();

  return HitmanDialog::Clear(retVal);
}

bool Hitman1Dialog::LoadImpl(const StringView8CI &loadPathView, const Options &options)
{
  const auto loadPath = loadPathView.path();
  Clear();

  auto archiveBinFilePath = loadPathView.path();
  archiveBinFilePath.replace_extension(L".bin");
  if (!exists(archiveBinFilePath))
  {
    DisplayError(g_LocalizationManager.Localize("HITMAN_1_DIALOG_ERROR_MISSING_BIN"));
    return false;
  }

  const auto oldSync = std::ios_base::sync_with_stdio(false);

  const auto archiveBin = ReadWholeBinaryFile(String8(archiveBinFilePath));
  auto archiveIdx = ReadWholeTextFile(loadPathView).native();
  auto archiveIdxScan = scn::make_result(archiveIdx);
  size_t archiveBinOffset = 0;

  while (archiveIdxScan && archiveBinOffset < archiveBin.size())
  {
    std::string_view entryPath;
    std::string_view month;
    auto day = 0ull;
    std::string_view time;
    auto dataSize = 0ull;
    archiveIdxScan = scn::scan(archiveIdxScan.range(), "-rw-rw-r--   1 zope {} {} {} {} {}", dataSize, month, day, time, entryPath);
    archiveIdxScan = scn::ignore_until(archiveIdxScan.range(), '-');

    if (!archiveIdxScan)
    {
      if (archiveIdxScan.error() == scn::error::code::end_of_range)
        break;

      return Clear(false);
    }

    auto& file = GetFile(entryPath);

    auto [fileMapIt, inserted] = fileMap.try_emplace(file.path, HitmanFile{file.path});
    if (!inserted)
      return Clear(false);

    if (!lastModifiedDatesMap.try_emplace(file.path, std::format("{} {:2d} {}", month, day, time)).second)
      return Clear(false);

    indexToKey.emplace_back(file.path);

    std::vector<char> fileData(dataSize, 0);
    std::memcpy(fileData.data(), archiveBin.data() + archiveBinOffset, dataSize);
    if (!ImportSingleHitmanFile(fileMapIt->second, fileData, false, options))
      return Clear(false);

    archiveBinOffset += dataSize;
  }

  if (archiveBinOffset < archiveBin.size())
    return Clear(false);

  std::ios_base::sync_with_stdio(oldSync);

  auto dataPath = GetProgramPath().path();
  if (dataPath.empty())
    return Clear(false);

  dataPath /= L"data";
  dataPath /= L"records";
  dataPath /= L"h1_";

  originalDataPathPrefix = dataPath;
  originalDataID = XXH3_64bits(archiveBin.data(), archiveBin.size());
  originalDataParentID = 0;

  if (!LoadOriginalData(options))
    return Clear(false);

  return true;
}

bool Hitman1Dialog::ImportSingle(const StringView8CI &importFolderPathView, const StringView8CI &importFilePathView, const Options &options)
{
  auto filePath = relative(importFilePathView.path(), importFolderPathView.path());
  auto fileIt = fileMap.find(filePath);
  if (fileIt == fileMap.end())
  {
    filePath.replace_extension(filePath.extension() == StringViewWCI(L".wav") ? L".ogg" : L".wav");
    fileIt = fileMap.find(filePath);
    if (fileIt == fileMap.end())
    {
      DisplayWarning(g_LocalizationManager.LocalizeFormat("HITMAN_DIALOG_WARNING_MISSING_FILE", importFilePathView),
                     g_LocalizationManager.Localize("MESSAGEBOX_WARNING_GENERIC_TITLE"), false, options);
      return false;
    }
  }

  if (!ImportSingleHitmanFile(fileIt->second, importFilePathView, options))
    return false;

  return true;
}

bool Hitman1Dialog::SaveImpl(const StringView8CI &savePathView, const Options &options)
{
  const auto archiveIdxFilePath = savePathView.path();
  auto archiveBinFilePath = archiveIdxFilePath;
  archiveBinFilePath.replace_extension(L".bin");

  const auto oldSync = std::ios_base::sync_with_stdio(false);

  std::ofstream archiveIdx(archiveIdxFilePath, std::ios::trunc);
  std::ofstream archiveBin(archiveBinFilePath, std::ios::binary | std::ios::trunc);

  std::vector<char> exportBytes;
  for (const auto &filePath : indexToKey)
  {
    auto fileIt = fileMap.find(filePath);
    if (fileIt == fileMap.end())
      return false;

    auto lastModifiedDateIt = lastModifiedDatesMap.find(filePath);
    if (lastModifiedDateIt == lastModifiedDatesMap.end())
      return false;

    fileIt->second.ExportNative(exportBytes, options);

    archiveIdx << std::format("-rw-rw-r--   1 zope {:12d} {} {}\n", exportBytes.size(), lastModifiedDateIt->second,
                              filePath);
    archiveBin.write(exportBytes.data(), static_cast<int64_t>(exportBytes.size()));

    GetFile(filePath).dirty = false;
  }

  archiveBin.close();
  archiveIdx.close();

  std::ios_base::sync_with_stdio(oldSync);

  originalDataParentID = originalDataID;

  const auto archiveBinData = ReadWholeBinaryFile(String8CI(archiveBinFilePath));
  originalDataID = XXH3_64bits(archiveBinData.data(), archiveBinData.size());

  if (!LoadOriginalData(options))
    return Clear(false);

  return true;
}

int32_t Hitman1Dialog::DrawDialog()
{
  return DrawHitmanDialog();
}

const std::vector<std::pair<String8, String8CI>>& Hitman1Dialog::GetOpenFilter()
{
  static std::vector<std::pair<String8, String8CI>> filters;
  if (!filters.empty())
    return filters;

  filters.emplace_back("FILE_DIALOG_FILTER_HITMAN1_SPEECH", "*.idx");

  return filters;
}

const std::vector<std::pair<String8, String8CI>>& Hitman1Dialog::GetSaveFilter() const
{
  static std::vector<std::pair<String8, String8CI>> filters;
  if (!filters.empty())
    return filters;

  filters.emplace_back("FILE_DIALOG_FILTER_HITMAN1_SPEECH", "*.idx");

  return filters;
}
