//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#include <Precompiled.hpp>

#include "Hitman1ArchiveDialog.hpp"

#include "Utils.hpp"

bool Hitman1ArchiveDialog::Clear(const bool retVal)
{
  indexToKey.clear();
  lastModifiedDatesMap.clear();

  return Glacier1ArchiveDialog::Clear(retVal);
}

bool Hitman1ArchiveDialog::LoadImpl(const StringView8CI &loadPathView, const Options &options)
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

  struct Hitman1Record
  {
    Glacier1AudioFile& file;
    std::span<const char> data;
  };

  std::vector<Hitman1Record> records;
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

    auto [fileMapIt, inserted] = fileMap.try_emplace(file.path, Glacier1AudioFile{file.path});
    if (!inserted)
      return Clear(false);

    if (!lastModifiedDatesMap.try_emplace(file.path, Format("{} {:2d} {}", month, day, time)).second)
      return Clear(false);

    indexToKey.emplace_back(file.path);
    records.emplace_back(fileMapIt->second, std::span<const char>{archiveBin.data() + archiveBinOffset, dataSize});

    archiveBinOffset += dataSize;
  }

  if (archiveBinOffset < archiveBin.size())
    return Clear(false);

  std::atomic_bool importFailed = false;
  std::for_each(std::execution::par, records.begin(), records.end(), [this, &importFailed, options](const auto& record)
  {
    if (importFailed.load(std::memory_order_relaxed))
      return;

    importFailed.store(importFailed.load(std::memory_order_relaxed) || !ImportSingleHitmanFile(record.file, record.data, false, options));
  });

  if (importFailed)
    return Clear(false);

  std::ios_base::sync_with_stdio(oldSync);

  auto dataPath = GetUserPath().path();
  if (dataPath.empty())
    return Clear(false);

  dataPath /= L"records";
  dataPath /= L"h1_";

  originalDataPathPrefix = dataPath;
  originalDataID = XXH3_64bits(archiveBin.data(), archiveBin.size());
  originalDataParentID = 0;

  if (!LoadOriginalData(options))
    return GenerateOriginalData(options);

  return true;
}

bool Hitman1ArchiveDialog::ImportSingle(const StringView8CI &importFolderPathView, const StringView8CI &importFilePathView, const Options &options)
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
                     g_LocalizationManager.Localize("MESSAGEBOX_TITLE_WARNING"), false, options);
      return false;
    }
  }

  if (!ImportSingleHitmanFile(fileIt->second, importFilePathView, options))
    return false;

  return true;
}

bool Hitman1ArchiveDialog::SaveImpl(const StringView8CI &savePathView, const Options &options)
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

    archiveIdx << Format("-rw-rw-r--   1 zope {:12d} {} {}\n", exportBytes.size(), lastModifiedDateIt->second,
                              filePath).native();
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

int32_t Hitman1ArchiveDialog::DrawDialog()
{
  return DrawGlacier1ArchiveDialog();
}

const std::vector<std::pair<StringView8CI, StringView8>>& Hitman1ArchiveDialog::GetOpenFilter()
{
  static std::vector<std::pair<StringView8CI, StringView8>> filters;
  if (!filters.empty())
    return filters;

  filters.emplace_back("*.idx", "FILE_DIALOG_FILTER_HITMAN1_SPEECH");

  return filters;
}

const std::vector<std::pair<StringView8CI, StringView8>>& Hitman1ArchiveDialog::GetSaveFilter() const
{
  static std::vector<std::pair<StringView8CI, StringView8>> filters;
  if (!filters.empty())
    return filters;

  filters.emplace_back("*.idx", "FILE_DIALOG_FILTER_HITMAN1_SPEECH");

  return filters;
}
