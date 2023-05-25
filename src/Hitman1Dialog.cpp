 //
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#include "Precompiled.hpp"

#include "Hitman1Dialog.hpp"

bool Hitman1Dialog::Clear(const bool retVal)
{
  indexToKey.clear();
  lastModifiedDatesMap.clear();

  return HitmanDialog::Clear(retVal);
}

bool Hitman1Dialog::LoadImpl(StringView8CI loadPathView)
{
  auto loadPath = loadPathView.path();
  Clear();

  const auto archiveBinFilePath = ChangeExtension(loadPathView, ".bin");
  if (!exists(archiveBinFilePath.path()))
  {
    DisplayError(LocalizationManager::Get().Localize("HITMAN_1_DIALOG_ERROR_MISSING_BIN"));
    return false;
  }

  const auto oldSync = std::ios_base::sync_with_stdio(false);

  std::ifstream archiveIdx(loadPath);

  std::string archiveIdxLine;
  while (std::getline(archiveIdx, archiveIdxLine))
  {
    std::string month, day, time;
    int64_t dataSize = 0;
    std::string entryPath;
    scn::scan(archiveIdxLine, "-rw-rw-r--   1 zope {} {} {} {} {}", dataSize, month, day, time, entryPath);

    if (!archivePaths.emplace(entryPath).second)
      return Clear(false);
  }

  auto archiveBin = ReadWholeBinaryFile(archiveBinFilePath);
  size_t archiveBinOffset = 0;

  archiveIdx.clear();
  archiveIdx.seekg(0);
  while (std::getline(archiveIdx, archiveIdxLine))
  {
    std::string month, day, time;
    int64_t dataSize = 0;
    std::string entryPath;
    scn::scan(archiveIdxLine, "-rw-rw-r--   1 zope {} {} {} {} {}", dataSize, month, day, time, entryPath);

    auto archivePathIt = archivePaths.find(entryPath);
    if (archivePathIt == archivePaths.end())
      return Clear(false);

    auto [fileMapIt, inserted] = fileMap.try_emplace(archivePathIt->native(), HitmanFile{});
    if (!inserted)
      return Clear(false);

    if (!lastModifiedDatesMap.try_emplace(archivePathIt->native(), std::format("{} {} {}", month, day, time)).second)
      return Clear(false);

    indexToKey.emplace_back(archivePathIt->native());

    std::vector<char> fileData(dataSize, 0);
    std::memcpy(fileData.data(), archiveBin.data() + archiveBinOffset, dataSize);
    if (!ImportSingleHitmanFile(fileMapIt->second, *archivePathIt, fileData, false))
      return Clear(false);

    archiveBinOffset += dataSize;
  }

  std::ios_base::sync_with_stdio(oldSync);

  for (const auto& archivePath : archivePaths)
    archiveRoot.GetFile(archivePath);

  if (!options.common.checkOriginality)
    return true;

  auto dataPath = GetProgramPath().path();
  if (dataPath.empty())
    return Clear(false);

  dataPath /= L"data";
  dataPath /= L"records";
  dataPath /= L"h1";

  originalDataPath = dataPath;

  if (!LoadOriginalData())
    return Clear(false);

  return true;
}

bool Hitman1Dialog::ImportSingle(const StringView8CI importFolderPathView, StringView8CI importFilePathView)
{
  const auto importFolderPath = importFolderPathView.path();
  const auto importFilePath = importFilePathView.path();

  auto filePath = String8CI(relative(importFilePath, importFolderPath));
  auto fileIt = fileMap.find(filePath);
  if (fileIt == fileMap.end())
  {
    const StringView8CI nextExtension(filePath.path().extension() == StringView8CI(".wav") ? ".ogg" : ".wav");
    filePath = ChangeExtension(filePath, nextExtension);
    fileIt = fileMap.find(filePath);
    if (fileIt == fileMap.end())
    {
      DisplayWarning(LocalizationManager::Get().LocalizeFormat("HITMAN_DIALOG_WARNING_MISSING_FILE", importFilePathView));
      return false;
    }
  }

  if (!ImportSingleHitmanFile(fileIt->second, filePath, importFilePathView))
    return false;

  return true;
}

bool Hitman1Dialog::SaveImpl(StringView8CI savePathView)
{
  auto savePath = savePathView.path();
  const auto archiveBinFilePath = savePath.parent_path() / (savePath.stem().native() + L".bin");

  const auto oldSync = std::ios_base::sync_with_stdio(false);

  std::ofstream archiveIdx(savePath, std::ios::trunc);
  std::ofstream archiveBin(archiveBinFilePath, std::ios::binary | std::ios::trunc);

  thread_local static std::vector<char> exportBytes;
  for (const auto filePath : indexToKey)
  {
    auto archivePathIt = archivePaths.find(filePath);
    if (archivePathIt == archivePaths.end())
      return Clear(false);

    auto fileIt = fileMap.find(filePath);
    if (fileIt == fileMap.end())
      return false;

    auto lastModifiedDateIt = lastModifiedDatesMap.find(filePath);
    if (lastModifiedDateIt == lastModifiedDatesMap.end())
      return false;

    fileIt->second.Export(exportBytes);

    archiveIdx << std::format("-rw-rw-r--   1 zope {:12d} {} {}\n", exportBytes.size(),
                                          lastModifiedDateIt->second, *archivePathIt);
    archiveBin.write(exportBytes.data(), static_cast<int64_t>(exportBytes.size()));

    archiveRoot.GetFile(*archivePathIt).dirty = false;
  }

  std::ios_base::sync_with_stdio(oldSync);

  return true;
}

void Hitman1Dialog::DrawDialog()
{
  DrawHitmanDialog("Codename 47", "Hitman 1 Speech (*.idx)\0*.idx\0", "English.idx");
}
