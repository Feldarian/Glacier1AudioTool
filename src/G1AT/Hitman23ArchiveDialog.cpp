//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#include <Precompiled.hpp>

#include "Hitman23ArchiveDialog.hpp"

#include "Utils.hpp"

Glacier1AudioRecord Hitman23WHDRecord::ToHitmanSoundRecord() const
{
  return {
    0,
    dataSizeUncompressed,
    dataSize,
    sampleRate,
    static_cast<AudioRecordFormat>(formatTag),
    static_cast<uint16_t>(bitsPerSample),
    static_cast<uint16_t>(channels),
    static_cast<uint16_t>(blockAlign),
    static_cast<uint16_t>(samplesPerBlock)
  };
}

void Hitman23WHDRecord::FromHitmanSoundRecord(const Glacier1AudioRecord &soundRecord)
{
  formatTag = static_cast<uint16_t>(soundRecord.format);
  sampleRate = soundRecord.sampleRate;
  bitsPerSample = soundRecord.bitsPerSample;
  dataSizeUncompressed = soundRecord.dataSizeUncompressed;
  dataSize = soundRecord.dataSize;
  channels = soundRecord.channels;
  samplesCount = soundRecord.dataSizeUncompressed / sizeof(int16_t);
  blockAlign = soundRecord.blockAlign;
  samplesPerBlock = soundRecord.samplesPerBlock;
}

bool Hitman23WAVFile::Clear(const bool retVal)
{
  header = nullptr;
  recordMap.clear();
  extraData.clear();
  path.clear();

  return retVal;
}

bool Hitman23WAVFile::Load(const std::vector<char> &wavData, const OrderedMap<StringView8CI, Hitman23WHDRecord *> &whdRecordsMap,
                           OrderedMap<StringView8CI, Glacier1AudioFile>& fileMap, const bool isMissionWAV)
{
  if (wavData.empty())
    return Clear(false);

  struct WAVFileData
  {
    uint32_t size = 0;
    Glacier1AudioFile& file;
  };

  auto resampledOffset = static_cast<uint32_t>(wavData.size());
  size_t foundItems = 0;
  OrderedMap<uint32_t, uint32_t> resampledMap;
  OrderedMap<uint32_t, WAVFileData> offsetToWAVFileDataMap;
  for (auto& [whdRecordPath, whdRecord] : whdRecordsMap)
  {
    if ((whdRecord->dataInStreams == 0) != isMissionWAV)
      continue;

    auto offsetToWAVFileDataIt = offsetToWAVFileDataMap.find(whdRecord->dataOffset);
    if (offsetToWAVFileDataIt != offsetToWAVFileDataMap.end())
    {
      resampledMap[resampledOffset] = whdRecord->dataOffset;
      whdRecord->dataOffset = resampledOffset;
      resampledOffset += whdRecord->dataSize;
    }

    offsetToWAVFileDataMap.insert({whdRecord->dataOffset, {whdRecord->dataSize, fileMap.at(whdRecordPath)}});

    ++foundItems;
  }

  if (offsetToWAVFileDataMap.size() != foundItems)
    return Clear(false);

  recordMap.clear();

  if (offsetToWAVFileDataMap.empty())
  {
    auto& newData = extraData.emplace_back();
    newData.resize(wavData.size(), 0);
    newData.shrink_to_fit();

    std::memcpy(newData.data(), wavData.data(), wavData.size());
    recordMap.try_emplace(0, Hitman23WAVRecord{newData, 0});
    if (isMissionWAV && !recordMap.empty())
      header = reinterpret_cast<Hitman23WAVHeader *>(recordMap.at(0).data.data());
    else
      header = nullptr;

    return true;
  }

  uint32_t currOffset = 0;
  for (auto& [offset, wavFileData] : offsetToWAVFileDataMap)
  {
    if (currOffset < offset)
    {
      auto& newData = extraData.emplace_back();
      newData.resize(offset - currOffset, 0);
      newData.shrink_to_fit();

      std::memcpy(newData.data(), wavData.data() + currOffset, offset - currOffset);
      recordMap.try_emplace(currOffset, Hitman23WAVRecord{newData, currOffset});
    }

    auto& newData = wavFileData.file.data;
    newData.resize(wavFileData.size, 0);
    newData.shrink_to_fit();

    const auto trueOffset = offset >= wavData.size() ? resampledMap[offset] : offset;
    std::memcpy(newData.data(), wavData.data() + trueOffset, wavFileData.size);
    recordMap.try_emplace(offset, Hitman23WAVRecord{newData, offset});
    currOffset = offset + wavFileData.size;
  }

  if (currOffset < wavData.size())
  {
    auto& newData = extraData.emplace_back();
    newData.resize(wavData.size() - currOffset, 0);
    newData.shrink_to_fit();

    std::memcpy(newData.data(), wavData.data() + currOffset, wavData.size() - currOffset);
    recordMap.try_emplace(currOffset, Hitman23WAVRecord{newData, currOffset});
  }

  const auto wavFileDataView = offsetToWAVFileDataMap | ranges::views::values;

  std::atomic_bool importFailed = false;
  std::for_each(std::execution::par, wavFileDataView.begin(), wavFileDataView.end(), [this, &importFailed](auto& wavFileData)
  {
    if (importFailed.load(std::memory_order_relaxed))
      return;

    auto& glacier1AudioFile = wavFileData.file;
    glacier1AudioFile.archiveRecord = SoundDataSoundRecord(glacier1AudioFile.archiveRecord, glacier1AudioFile.data);
    importFailed.store(importFailed.load(std::memory_order_relaxed) || glacier1AudioFile.archiveRecord.dataXXH3 == 0);
  });

  if (importFailed)
    return Clear(false);

  if (isMissionWAV && !recordMap.empty())
    header = reinterpret_cast<Hitman23WAVHeader *>(recordMap.at(0).data.data());
  else
    header = nullptr;

  return true;
}

bool Hitman23WAVFile::Load(const StringView8CI &loadPath, const OrderedMap<StringView8CI, Hitman23WHDRecord *> &whdRecordsMap,
                           OrderedMap<StringView8CI, Glacier1AudioFile>& fileMap, const bool isMissionWAV)
{
  if (!Load(ReadWholeBinaryFile(loadPath), whdRecordsMap, fileMap, isMissionWAV))
    return false;

  path = loadPath;
  return true;
}

bool Hitman23WAVFile::Save(const StringView8CI &savePathView)
{
  const auto savePath = savePathView.path();
  create_directories(savePath.parent_path());

  const auto oldSync = std::ios_base::sync_with_stdio(false);

  std::ofstream wavData(savePath, std::ios::binary | std::ios::trunc);

  uint32_t offset = 0;
  for (auto &record : recordMap | ranges::views::values)
  {
    record.newOffset = offset;
    offset += static_cast<uint32_t>(record.data.size());
  }

  if (header != nullptr)
    header->fileSizeWithHeader = offset;

  for (auto &record : recordMap | ranges::views::values)
    wavData.write(record.data.data(), record.data.size());

  std::ios_base::sync_with_stdio(oldSync);

  path = savePath;

  return true;
}

bool Hitman23WHDFile::Clear(const bool retVal)
{
  header = nullptr;
  recordMap.clear();
  data.clear();
  path.clear();

  return retVal;
}

bool Hitman23WHDFile::Load(Hitman23ArchiveDialog& archiveDialog, const StringView8CI &loadPathView)
{
  if (data.empty())
    data = ReadWholeBinaryFile(loadPathView);

  if (data.empty())
    return Clear(false);

  recordMap.clear();

  auto *whdPtr = data.data();
  header = reinterpret_cast<Hitman23WHDHeader *>(whdPtr);
  whdPtr += sizeof(Hitman23WHDHeader);

  while (*whdPtr)
  {
    whdPtr += std::strlen(whdPtr) + 1; // + 0-3 bytes for H3, so it is aligned on 4 bytes...
    auto *whdRecord = reinterpret_cast<Hitman23WHDRecord *>(whdPtr);
    if (whdRecord->type != 0x06)
    {
      whdPtr += 4 - (reinterpret_cast<uintptr_t>(whdPtr) % 4);
      whdRecord = reinterpret_cast<Hitman23WHDRecord *>(whdPtr);
    }
    whdPtr += sizeof(Hitman23WHDRecord);

    assert(whdRecord->type == 0x06);

    String8CI filePath(std::string_view(data.data() + whdRecord->filePathOffset));
    auto filePathNative = filePath.path();

    if (filePathNative.extension() != StringViewWCI(L".wav"))
      return Clear(false);

    if (whdRecord->dataInStreams == 0)
      filePath = relative(loadPathView.path(), archiveDialog.basePath.path()) / filePath.path();
    else if (!filePath.native().starts_with("Streams"))
      filePath = L"Streams" / filePathNative;

    auto& file = archiveDialog.GetFile(filePath);

    if (!recordMap.try_emplace(file.path, whdRecord).second)
      return Clear(false);

    archiveDialog.whdRecordsMap[file.path].emplace_back(whdRecord);

    [[maybe_unused]] const auto [fileMapIt, fileMapEmplaced] = archiveDialog.fileMap.try_emplace(file.path, Glacier1AudioFile{file.path, whdRecord->ToHitmanSoundRecord()});
    assert(whdRecord->dataInStreams || fileMapEmplaced);
  }

  path = loadPathView;

  return true;
}

bool Hitman23WHDFile::Save(const Hitman23WAVFile &streamsWAV, const Hitman23WAVFile &missionWAV, const StringView8CI &savePath)
{
  for (auto *whdRecord : recordMap | ranges::views::values)
  {
    const auto &wavRecordMap = whdRecord->dataInStreams == 0 ? missionWAV.recordMap : streamsWAV.recordMap;
    const auto wavRecordIt = wavRecordMap.find(whdRecord->dataOffset);
    assert(wavRecordIt != wavRecordMap.end());
    if (wavRecordIt != wavRecordMap.end())
      whdRecord->dataOffset = wavRecordIt->second.newOffset;
  }

  const auto oldSync = std::ios_base::sync_with_stdio(false);

  std::ofstream whdData(savePath.path(), std::ios::binary | std::ios::trunc);
  whdData.write(data.data(), data.size());
  whdData.close();

  std::ios_base::sync_with_stdio(oldSync);

  path = savePath;

  return true;
}

bool Hitman23ArchiveDialog::Clear(const bool retVal)
{
  whdFiles.clear();
  wavFiles.clear();
  streamsWAV.Clear();
  basePath.clear();
  fileMap.clear();
  whdRecordsMap.clear();

  return Glacier1ArchiveDialog::Clear(retVal);
}

bool Hitman23ArchiveDialog::ImportSingle(const StringView8CI &importFolderPathView, const StringView8CI &importFilePathView, const Options &options)
{
  auto filePath = relative(importFilePathView.path(), importFolderPathView.path());
  auto fileIt = fileMap.find(filePath);
  auto whdRecordsIt = whdRecordsMap.find(filePath);
  if (fileIt == fileMap.end() || whdRecordsIt == whdRecordsMap.end())
  {
    filePath.replace_extension(filePath.extension() == StringViewWCI(L".wav") ? L".ogg" : L".wav");
    fileIt = fileMap.find(filePath);
    whdRecordsIt = whdRecordsMap.find(filePath);
    if (fileIt == fileMap.end() || whdRecordsIt == whdRecordsMap.end())
    {
      DisplayWarning(g_LocalizationManager.LocalizeFormat("HITMAN_DIALOG_WARNING_MISSING_FILE", importFilePathView),
                     g_LocalizationManager.Localize("MESSAGEBOX_TITLE_WARNING"), false, options);
      return false;
    }
  }

  if (!ImportSingleHitmanFile(fileIt->second, importFilePathView, options))
    return false;

  for (auto* whdRecord : whdRecordsIt->second)
    whdRecord->FromHitmanSoundRecord(fileIt->second.archiveRecord);

  return true;
}

bool Hitman23ArchiveDialog::LoadImpl(const StringView8CI &loadPathView, const Options &options)
{
  Clear();

  const auto rootPath = loadPathView.path().parent_path();
  const auto scenesPath = rootPath / L"Scenes";
  if (!exists(scenesPath))
  {
    DisplayError(g_LocalizationManager.Localize("HITMAN_23_DIALOG_ERROR_MISSING_SCENES"));
    return false;
  }

  const auto allWHDFiles = GetAllFilesInDirectory(String8CI(scenesPath), ".whd", true);

  whdFiles.reserve(allWHDFiles.size());
  wavFiles.reserve(allWHDFiles.size());

  basePath = rootPath;

  OrderedMap<StringView8CI, Hitman23WHDRecord *> allWHDRecords;
  for (const auto &whdPath : allWHDFiles)
  {
    auto &whdFile = whdFiles.emplace_back();
    if (!whdFile.Load(*this, whdPath))
      return Clear(false);

    for (const auto& [filePath, whdRecord] : whdFile.recordMap)
    {
      [[maybe_unused]] const auto res = allWHDRecords.try_emplace(filePath, whdRecord);

      // TODO - do we want to handle this in some way? duplicates pointing to streams should not be an issue unless offsets are different...
      assert(res.second || (res.first->second->dataInStreams && res.first->second->dataInStreams == whdRecord->dataInStreams && res.first->second->dataOffset == whdRecord->dataOffset));
    }

    if (!wavFiles.emplace_back().Load(String8CI(whdFile.path.path().replace_extension(L".wav")), whdFile.recordMap, fileMap, true))
      return Clear(false);
  }

  const auto streamsWAVData = ReadWholeBinaryFile(loadPathView);
  if (!streamsWAV.Load(streamsWAVData, allWHDRecords, fileMap, false))
    return Clear(false);

  auto dataPath = GetUserPath().path();
  if (dataPath.empty())
    return Clear(false);

  dataPath /= L"records";
  dataPath /= L"h23_";

  originalDataPathPrefix = dataPath;
  originalDataID = XXH3_64bits(streamsWAVData.data(), streamsWAVData.size());
  originalDataParentID = 0;

  if (!LoadOriginalData(options))
    return GenerateOriginalData(options);

  return true;
}

bool Hitman23ArchiveDialog::SaveImpl(const StringView8CI &savePathView, const Options &options)
{
  const auto newBasePath = savePathView.path().parent_path();

  streamsWAV.Save(savePathView);

  for (size_t i = 0; i < whdFiles.size(); ++i)
  {
    wavFiles[i].Save(String8CI(newBasePath / relative(wavFiles[i].path.path(), basePath.path())));
    whdFiles[i].Save(streamsWAV, wavFiles[i], String8CI(newBasePath / relative(whdFiles[i].path.path(), basePath.path())));
  }

  basePath = newBasePath;

  CleanDirty();

  originalDataParentID = originalDataID;

  const auto streamsWAVData = ReadWholeBinaryFile(savePathView);
  originalDataID = XXH3_64bits(streamsWAVData.data(), streamsWAVData.size());

  if (!LoadOriginalData(options))
    return Clear(false);

  return true;
}

int32_t Hitman23ArchiveDialog::DrawDialog()
{
  return DrawGlacier1ArchiveDialog();
}

const std::vector<std::pair<StringView8CI, StringView8>>& Hitman23ArchiveDialog::GetOpenFilter()
{
  static std::vector<std::pair<StringView8CI, StringView8>> filters;
  if (!filters.empty())
    return filters;

  filters.emplace_back("streams.wav", "FILE_DIALOG_FILTER_HITMAN23_STREAMS");

  return filters;
}

const std::vector<std::pair<StringView8CI, StringView8>>& Hitman23ArchiveDialog::GetSaveFilter() const
{
  static std::vector<std::pair<StringView8CI, StringView8>> filters;
  if (!filters.empty())
    return filters;

  filters.emplace_back("streams.wav", "FILE_DIALOG_FILTER_HITMAN23_STREAMS");

  return filters;
}
