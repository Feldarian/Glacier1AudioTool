//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//
// TODO - GOG and Steam version have a hash conflict! Either they truly have streams file the exact same or I don't know... But it reports everything as changed!
//      - workaround for now - remove "h4_" prefixed files from "data/records" before opening GOG archive...
//
// TODO - aliased entries look different in STR - do they look different in scene WAV? can they be present there?
//

#include <Precompiled.hpp>

#include "Hitman4Dialog.hpp"

#include "Utils.hpp"

#include <Config/Config.hpp>

HitmanSoundRecord Hitman4WHDRecord::Hitman4WHDRecordScene::ToHitmanSoundRecord() const
{
  assert(filePathLength > 0 && filePathLength < 0x004F4850);

  const auto resBitsPerSample = static_cast<uint16_t>(formatTag == 4096 ? 16 : bitsPerSample);
  const auto resChannels = static_cast<uint16_t>(channels);
  const auto resBlockAlign = static_cast<uint16_t>(formatTag == 4096 ? 2 * channels : blockAlign);
  const auto resFmtExtra = static_cast<uint16_t>(formatTag == 17 ? samplesPerBlock : 1);

  assert(resBlockAlign == 2 * channels || formatTag == 17);
  assert(resBlockAlign != 1024 || formatTag == 17);

  return {
    0,
    dataSizeUncompressed,
    dataSize,
    sampleRate,
    formatTag,
    resBitsPerSample,
    resChannels,
    resBlockAlign,
    resFmtExtra
  };
}

void Hitman4WHDRecord::Hitman4WHDRecordScene::FromHitmanSoundRecord(const HitmanSoundRecord &soundRecord)
{
  assert(formatTag != 4096 || formatTag == soundRecord.formatTag);

  formatTag = soundRecord.formatTag;
  sampleRate = soundRecord.sampleRate;
  bitsPerSample = formatTag == 4096 ? 0 : soundRecord.bitsPerSample;
  dataSizeUncompressed = soundRecord.dataSizeUncompressed;
  dataSize = soundRecord.dataSize;
  channels = soundRecord.channels;
  samplesCount = soundRecord.dataSizeUncompressed / sizeof(int16_t);
  blockAlign = formatTag == 4096 ? blockAlign : soundRecord.blockAlign;
  samplesPerBlock = formatTag == 4096 ? 0x004F3E93 : (formatTag == 1 ? 0xCDCDCDCD : soundRecord.samplesPerBlock);
}

HitmanSoundRecord Hitman4WHDRecord::Hitman4WHDRecordSceneAliased::ToHitmanSoundRecord() const
{
  assert(nullByte == 0);

  const auto resBitsPerSample = static_cast<uint16_t>(formatTag == 4096 ? 16 : bitsPerSample);
  const auto resChannels = static_cast<uint16_t>(channels);
  const auto resBlockAlign = static_cast<uint16_t>(formatTag == 4096 ? 2 * channels : blockAlign);
  const auto resFmtExtra = static_cast<uint16_t>(formatTag == 17 ? fmtExtra : 1);

  assert(resBlockAlign != 1024 || formatTag == 17);

  return {
    0,
    dataSizeUncompressed,
    dataSize,
    sampleRate,
    formatTag,
    resBitsPerSample,
    resChannels,
    resBlockAlign,
    resFmtExtra
  };
}

void Hitman4WHDRecord::Hitman4WHDRecordSceneAliased::FromHitmanSoundRecord(const HitmanSoundRecord &soundRecord)
{
  assert(formatTag != 4096 || formatTag == soundRecord.formatTag);

  formatTag = soundRecord.formatTag;
  sampleRate = soundRecord.sampleRate;
  bitsPerSample = formatTag == 4096 ? 0 : soundRecord.bitsPerSample;
  dataSizeUncompressed = soundRecord.dataSizeUncompressed;
  dataSize = soundRecord.dataSize;
  channels = soundRecord.channels;
  samplesCount = soundRecord.dataSizeUncompressed / sizeof(int16_t);
  blockAlign = formatTag == 4096 ? blockAlign : soundRecord.blockAlign;
  fmtExtra = formatTag == 4096 ? 0x004F3E93 : (formatTag == 1 ? 0xCDCDCDCD : soundRecord.samplesPerBlock);
}

HitmanSoundRecord Hitman4WHDRecord::Hitman4WHDRecordStreams::ToHitmanSoundRecord() const
{
  assert(id == 0x004F4850);

  const auto resBitsPerSample = static_cast<uint16_t>(formatTag == 4096 ? 16 : bitsPerSample);
  const auto resChannels = static_cast<uint16_t>(channels);
  const auto resBlockAlign = static_cast<uint16_t>(formatTag == 4096 || formatTag == 1 ? 2 * channels : 1024);
  const auto resFmtExtra = static_cast<uint16_t>(formatTag == 17 ? fmtExtra : 1);

  assert(resBlockAlign == 2 * channels || formatTag == 17);
  assert(resBlockAlign != 1024 || formatTag == 17);

  return {
    0,
    dataSizeUncompressed,
    dataSize,
    unkC,
    formatTag,
    resBitsPerSample,
    resChannels,
    resBlockAlign,
    resFmtExtra
  };
}

void Hitman4WHDRecord::Hitman4WHDRecordStreams::FromHitmanSoundRecord(const HitmanSoundRecord &soundRecord)
{
  assert(formatTag == soundRecord.formatTag);

  formatTag = soundRecord.formatTag;
  unk2C = soundRecord.sampleRate;
  bitsPerSample = formatTag == 4096 ? 0 : soundRecord.bitsPerSample;
  dataSizeUncompressed = soundRecord.dataSizeUncompressed;
  dataSize = soundRecord.dataSize;
  channels = soundRecord.channels;
  samplesCount = soundRecord.dataSizeUncompressed / sizeof(int16_t);
  fmtExtra = formatTag == 4096 ? 0x004F3E93 : (formatTag == 1 ? 0xCDCDCDCD : soundRecord.samplesPerBlock);
}

HitmanSoundRecord Hitman4WHDRecord::ToHitmanSoundRecord() const
{
  assert((streams.id == 0x004F4850) == (streams.dataInStreams == 128));

  assert(streams.formatTag != 1 || streams.bitsPerSample == 16);
  assert(streams.formatTag != 17 || streams.bitsPerSample == 4);
  assert(streams.formatTag != 4096 || streams.bitsPerSample == 0);

  assert(scene.formatTag != 17 || scene.blockAlign == 1024);

  assert(streams.formatTag != 17 || streams.fmtExtra == 2041);
  assert(streams.formatTag != 4096 || streams.fmtExtra == 0x004F3E93);

  assert(streams.nullBytes[0] == 0);

  switch (streams.id)
  {
    case 0x004F4850:
      return streams.ToHitmanSoundRecord();
    case 0:
      return sceneAliased.ToHitmanSoundRecord();
    default:
      return scene.ToHitmanSoundRecord();
  }
}

void Hitman4WHDRecord::FromHitmanSoundRecord(const HitmanSoundRecord &soundRecord)
{
  switch (streams.id)
  {
    case 0x004F4850:
      return streams.FromHitmanSoundRecord(soundRecord);
    case 0:
      return sceneAliased.FromHitmanSoundRecord(soundRecord);
    default:
      return scene.FromHitmanSoundRecord(soundRecord);
  }
}

bool Hitman4STRFile::Clear(const bool retVal)
{
  header = {};
  path.clear();

  return retVal;
}

bool Hitman4STRFile::Load(Hitman4Dialog& archiveDialog, const std::vector<char> &wavData, const OrderedMap<StringView8CI, Hitman4WHDRecord *> &whdRecordsMap)
{
  Clear();

  if (wavData.empty())
    return Clear(false);

  if (wavData.size() < sizeof(Hitman4STRHeader))
    return Clear(false);

  std::memcpy(&header, wavData.data(), sizeof(Hitman4STRHeader));

  if (wavData.size() - header.offsetToEntryTable < header.entriesCount * sizeof(Hitman4STRRecord))
    return Clear(false);

  recordTable.resize(header.entriesCount);
  std::memcpy(recordTable.data(), wavData.data() + header.offsetToEntryTable, header.entriesCount * sizeof(Hitman4STRRecord));

  wavDataTable.resize(header.entriesCount);
  lipDataTable.resize(header.entriesCount);
  wavHeaderTableData.resize(header.entriesCount);
  stringTable.resize(header.entriesCount);

  wavHeaderTableBeginOffset = header.offsetToEntryTable;
  stringTableBeginOffset = header.offsetToEntryTable;

  OrderedMap<uint64_t, std::vector<uint32_t>> wavDataOffsets;

  //OrderedSet<uint32_t> missingWHDRecord;

  for (uint32_t i = 0; i < header.entriesCount; ++i)
  {
    const auto& strRecord = recordTable[i];

    const auto [offsetsIt, ignored] = wavDataOffsets.try_emplace(strRecord.dataOffset, std::vector<uint32_t>{});
    offsetsIt->second.emplace_back(i);

    if (wavData.size() - strRecord.headerOffset < strRecord.sizeOfHeader)
      return Clear(false);

    if (wavData.size() - strRecord.fileNameOffset < strRecord.fileNameLength + 1)
      return Clear(false);

    wavHeaderTableBeginOffset = std::min(wavHeaderTableBeginOffset, strRecord.headerOffset);
    stringTableBeginOffset = std::min(stringTableBeginOffset, strRecord.fileNameOffset);
  }

  if (wavDataOffsets.empty())
    return Clear(false);

  const auto [offsetsEndIt, emplaced] = wavDataOffsets.try_emplace(wavHeaderTableBeginOffset, std::vector<uint32_t>{});
  if (!emplaced)
    return Clear(false);

  offsetsEndIt->second.emplace_back(header.entriesCount);

  for (uint32_t i = 0; i < header.entriesCount; ++i)
  {
    const auto& strRecord = recordTable[i];

    auto& strWAVHeader = wavHeaderTableData[i];
    strWAVHeader.resize(strRecord.sizeOfHeader);
    std::memcpy(strWAVHeader.data(), wavData.data() + strRecord.headerOffset, strRecord.sizeOfHeader);

    auto& strFilename = stringTable[i];
    strFilename.resize(strRecord.fileNameLength);
    std::memcpy(strFilename.data(), wavData.data() + strRecord.fileNameOffset, strRecord.fileNameLength);

    if (!fileNameToIndex.try_emplace(strFilename, i).second)
      return Clear(false);

    if (!whdRecordsMap.contains(String8CI("Streams\\") += strFilename))
    {
      // TODO - add entrie into map when they are not present and they point to "valid" data
      //const auto& archiveFile = archiveDialog.GetFile(String8CI("Streams\\") += strFilename);
      //[[maybe_unused]] const auto [fileMapIt, fileMapEmplaced] = archiveDialog.fileMap.try_emplace(archiveFile.path, HitmanFile{archiveFile.path, whdRecord->ToHitmanSoundRecord()});
      //assert(fileMapEmplaced);
      //
      //if (!missingWHDRecord.insert(i).second)
      //  return Clear(false);

      assert(wavDataOffsets.find(strRecord.dataOffset) != wavDataOffsets.end());
      assert(++wavDataOffsets.find(strRecord.dataOffset) != wavDataOffsets.end());

      auto& wavDataSet = wavDataOffsets.find(strRecord.dataOffset)->second;
      const auto wavDataEndOffset = (++wavDataOffsets.find(strRecord.dataOffset))->first;
      const auto wavDataSize = wavDataEndOffset - strRecord.dataOffset;

      assert(wavData.size() >= wavDataEndOffset);
      assert(wavDataSize % 0x0100 == 0);

      auto& strWAVData = wavDataTable[i];

      const auto hasLIP = std::memcmp(wavData.data() + strRecord.dataOffset, "LIP ", sizeof(uint32_t)) == 0;

      auto strWAVDataOffset = wavDataSet.size() > 1 || hasLIP ? wavDataSize : 0;
      if (strWAVDataOffset != 0)
      {
        if (wavDataSet.size() > 1)
        {
          for (auto wavDataSetItemIt = wavDataSet.begin(); wavDataSetItemIt != wavDataSet.end(); ++wavDataSetItemIt)
          {
            if (*wavDataSetItemIt == i)
            {
              wavDataSet.erase(wavDataSetItemIt);
              break;
            }
          }
        }

        for (const auto recordIndex : wavDataSet | std::views::reverse)
        {
          const auto& setRecord = recordTable[recordIndex];
          assert(strWAVDataOffset >= setRecord.dataSize);
          strWAVDataOffset -= setRecord.dataSize;
          strWAVDataOffset &= ~0xFFull;
        }
      }

      strWAVData.resize(strRecord.dataSize);
      std::memcpy(strWAVData.data(), wavData.data() + strRecord.dataOffset + strWAVDataOffset, strRecord.dataSize);

      OptionalReference<std::vector<char>> lipDataOptRef;
      if (hasLIP)
      {
        strWAVDataOffset = wavDataSize;
        for (const auto recordIndex : wavDataSet | std::views::reverse)
        {
          const auto& setRecord = recordTable[recordIndex];
          assert(strWAVDataOffset >= setRecord.dataSize);
          strWAVDataOffset -= setRecord.dataSize;
          strWAVDataOffset &= ~0xFFull;
        }

        assert(std::memcmp(wavData.data() + strRecord.dataOffset, "LIP ", sizeof(uint32_t)) == 0);

        auto& lipData = lipDataTable[i];
        lipData.resize(strWAVDataOffset);
        std::memcpy(lipData.data(), wavData.data() + strRecord.dataOffset, strWAVDataOffset);
        lipDataOptRef = lipData;
      }

      recordMap.try_emplace(static_cast<uint32_t>(strRecord.dataOffset), Hitman4WAVRecord{strWAVData, static_cast<uint32_t>(strRecord.dataOffset), lipDataOptRef});
    }
  }

  if (wavDataOffsets.empty())
    return Clear(false);

  recordMap.clear();

  for (auto& [whdRecordPath, whdRecord] : whdRecordsMap)
  {
    if (whdRecord->streams.dataInStreams == 0)
      continue;

    const auto fileIt = archiveDialog.fileMap.find(whdRecordPath);
    if (fileIt == archiveDialog.fileMap.end())
      return Clear(false);

    auto& file = fileIt->second;

    const auto strRecordIndexIt = fileNameToIndex.find(whdRecordPath.native().substr(8));
    if (strRecordIndexIt == fileNameToIndex.end())
      return Clear(false);

    auto& strRecord = recordTable[strRecordIndexIt->second];

    assert(wavDataOffsets.contains(strRecord.dataOffset));
    assert(++wavDataOffsets.find(strRecord.dataOffset) != wavDataOffsets.end());

    const auto& wavDataSet = wavDataOffsets.find(strRecord.dataOffset)->second;
    const auto wavDataEndOffset = (++wavDataOffsets.find(strRecord.dataOffset))->first;
    const auto wavDataSize = wavDataEndOffset - strRecord.dataOffset;

    assert(wavData.size() >= wavDataEndOffset);
    assert(wavDataSize % 0x0100 == 0);

    auto& strWAVHeader = wavHeaderTableData[strRecordIndexIt->second];
    const auto* strWAVHeaderCasted = reinterpret_cast<const Hitman4STRRecordHeader1*>(strWAVHeader.data());
    assert(strWAVHeaderCasted->unk4 == whdRecord->streams.dataSize || strRecord.dataSize == whdRecord->streams.dataSize || strRecord.unk24 == whdRecord->streams.dataSize);

    const auto hasLIP = std::memcmp(wavData.data() + strRecord.dataOffset, "LIP ", sizeof(uint32_t)) == 0;

    auto strWAVDataOffset = whdRecord->streams.id == 0 || hasLIP ? wavDataSize : 0;
    if (strWAVDataOffset != 0)
    {
      assert(hasLIP || wavDataSet.size() == 2);
      for (const auto recordIndex : wavDataSet | std::views::reverse)
      {
        const auto& setRecord = recordTable[recordIndex];
        assert(strWAVDataOffset >= setRecord.dataSize);
        strWAVDataOffset -= setRecord.dataSize;
        strWAVDataOffset &= ~0xFFull;

        if (recordIndex == strRecordIndexIt->second)
          break;
      }
    }

    assert(hasLIP || strWAVDataOffset == (strWAVDataOffset & (~0xFFull)));

    file.data.resize(whdRecord->streams.dataSize);
    std::memcpy(file.data.data(), wavData.data() + strRecord.dataOffset + strWAVDataOffset, whdRecord->streams.dataSize);

    OptionalReference<std::vector<char>> lipDataOptRef;
    if (hasLIP)
    {
      strWAVDataOffset = wavDataSize;
      for (const auto recordIndex : wavDataSet | std::views::reverse)
      {
        const auto& setRecord = recordTable[recordIndex];
        assert(strWAVDataOffset >= setRecord.dataSize);
        strWAVDataOffset -= setRecord.dataSize;
        strWAVDataOffset &= ~0xFFull;
      }

      assert(std::memcmp(wavData.data() + strRecord.dataOffset, "LIP ", sizeof(uint32_t)) == 0);

      auto& lipData = lipDataTable[strRecordIndexIt->second];
      lipData.resize(strWAVDataOffset);
      std::memcpy(lipData.data(), wavData.data() + strRecord.dataOffset, strWAVDataOffset);
      lipDataOptRef = lipData;
    }

    assert((whdRecord->streams.dataInStreams & 0xFF00) == 0 || ((whdRecord->streams.dataInStreams & 0xFF00) == 0x8200 || (whdRecord->streams.dataInStreams & 0xFF00) == 0x0100 && whdRecord->streams.formatTag == 0x01));

    recordMap.try_emplace(whdRecord->streams.dataOffset, Hitman4WAVRecord{file.data, whdRecord->streams.dataOffset, lipDataOptRef});

    const auto updatedArchiveRecord = SoundDataSoundRecord(file.archiveRecord, {file.data.data(), whdRecord->streams.dataSize});

    if (updatedArchiveRecord.dataXXH3 == 0)
    {
      assert(hasLIP);
      return Clear(false);
    }

    file.archiveRecord = updatedArchiveRecord;
  }

  if (recordMap.empty())
    return Clear(false);

  return true;
}

bool Hitman4STRFile::Load(Hitman4Dialog& archiveDialog, const StringView8CI &loadPath, const OrderedMap<StringView8CI, Hitman4WHDRecord *> &whdRecordsMap)
{
  if (!Load(archiveDialog, ReadWholeBinaryFile(loadPath), whdRecordsMap))
    return false;

  path = loadPath;
  return true;
}

bool Hitman4STRFile::Save(const StringView8CI &savePath)
{
  assert(false);
  return false;
}


bool Hitman4WAVFile::Clear(const bool retVal)
{
  header = nullptr;
  recordMap.clear();
  extraData.clear();
  path.clear();

  return retVal;
}

bool Hitman4WAVFile::Load(const std::vector<char> &wavData, const OrderedMap<StringView8CI, Hitman4WHDRecord *> &whdRecordsMap, OrderedMap<StringView8CI, HitmanFile>& fileMap)
{
  Clear();

  if (wavData.empty())
    return Clear(false);

  struct WAVFileData
  {
    Hitman4WHDRecord *record = nullptr;
    HitmanFile& file;
  };

  auto resampledOffset = static_cast<uint32_t>(wavData.size());
  size_t foundItems = 0;
  OrderedMap<uint32_t, uint32_t> resampledMap;
  OrderedMap<uint32_t, WAVFileData> offsetToWAVFileDataMap;
  for (auto& [whdRecordPath, whdRecord] : whdRecordsMap)
  {
    if (whdRecord->streams.dataInStreams != 0)
      continue;

    auto offsetToWAVFileDataIt = offsetToWAVFileDataMap.find(whdRecord->streams.dataOffset);
    if (offsetToWAVFileDataIt != offsetToWAVFileDataMap.end())
    {
      resampledMap[resampledOffset] = whdRecord->streams.dataOffset;
      whdRecord->streams.dataOffset = resampledOffset;
      resampledOffset += whdRecord->streams.dataSize;
    }

    offsetToWAVFileDataMap.insert({whdRecord->streams.dataOffset, {whdRecord, fileMap.at(whdRecordPath)}});

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
    recordMap.try_emplace(0, Hitman4WAVRecord{newData, 0});
    if (!recordMap.empty())
      header = reinterpret_cast<Hitman4WAVHeader *>(recordMap.at(0).data.data());
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
      recordMap.try_emplace(currOffset, Hitman4WAVRecord{newData, currOffset, std::nullopt});
    }
    assert(currOffset <= offset);

    auto& newData = wavFileData.file.data;
    newData.resize(wavFileData.record->streams.dataSize, 0);
    newData.shrink_to_fit();

    auto trueOffset = offset >= wavData.size() ? resampledMap[offset] : offset;
    if (std::memcmp(wavData.data() + trueOffset, "LIP ", sizeof(uint32_t)) == 0)
      return false;

    std::memcpy(newData.data(), wavData.data() + trueOffset, wavFileData.record->streams.dataSize);
    currOffset = offset + wavFileData.record->streams.dataSize;
    recordMap.try_emplace(offset, Hitman4WAVRecord{newData, offset});

    wavFileData.file.archiveRecord = SoundDataSoundRecord(wavFileData.file.archiveRecord, wavFileData.file.data);

    if (wavFileData.file.archiveRecord.dataXXH3 == 0)
      return Clear(false);
  }

  if (currOffset < wavData.size())
  {
    auto& newData = extraData.emplace_back();
    newData.resize(wavData.size() - currOffset, 0);
    newData.shrink_to_fit();

    std::memcpy(newData.data(), wavData.data() + currOffset, wavData.size() - currOffset);
    recordMap.try_emplace(currOffset, Hitman4WAVRecord{newData, currOffset});
  }

  header = reinterpret_cast<Hitman4WAVHeader *>(recordMap.at(0).data.data());

  return true;
}

bool Hitman4WAVFile::Load(const StringView8CI &loadPath, const OrderedMap<StringView8CI, Hitman4WHDRecord *> &whdRecordsMap, OrderedMap<StringView8CI, HitmanFile>& fileMap)
{
  if (!Load(ReadWholeBinaryFile(loadPath), whdRecordsMap, fileMap))
    return false;

  path = loadPath;
  return true;
}

bool Hitman4WAVFile::Save(const StringView8CI &savePathView)
{
  const auto savePath = savePathView.path();
  create_directories(savePath.parent_path());

  const auto oldSync = std::ios_base::sync_with_stdio(false);

  std::ofstream wavData(savePath, std::ios::binary | std::ios::trunc);

  uint32_t offset = 0;
  for (auto &record : recordMap | std::views::values)
  {
    record.newOffset = offset;
    offset += static_cast<uint32_t>(record.data.size());
  }

  for (auto &record : recordMap | std::views::values)
    wavData.write(record.data.data(), record.data.size());

  std::ios_base::sync_with_stdio(oldSync);

  path = savePath;

  return true;
}

bool Hitman4WHDFile::Clear(const bool retVal)
{
  header = nullptr;
  recordMap.clear();
  data.clear();
  path.clear();

  return retVal;
}

bool Hitman4WHDFile::Load(Hitman4Dialog& archiveDialog, const StringView8CI &loadPathView)
{
  if (data.empty())
    data = ReadWholeBinaryFile(loadPathView);

  if (data.empty())
    return Clear(false);

  recordMap.clear();

  auto *whdPtr = data.data();
  header = reinterpret_cast<Hitman4WHDHeader *>(whdPtr);
  whdPtr += sizeof(Hitman4WHDHeader);

  while (*whdPtr)
  {
    whdPtr += std::strlen(whdPtr) + 1; // + 0-15 bytes, so it is aligned on 16 bytes...
    if (reinterpret_cast<uintptr_t>(whdPtr) % 16 != 0)
      whdPtr += 16 - (reinterpret_cast<uintptr_t>(whdPtr) % 16);
    while (memcmp(whdPtr, terminateBytes.data(), terminateBytes.size() * sizeof(uint32_t)) == 0)
      whdPtr += 16;

    uint32_t iterCount = 0;
    do {
      auto *whdRecord = reinterpret_cast<Hitman4WHDRecord *>(whdPtr);
      whdPtr += sizeof(Hitman4WHDRecord);

      bool nullBytesCheckPassed = true;
      for (const auto nullByte : whdRecord->scene.nullBytes)
        nullBytesCheckPassed &= nullByte == 0;
      if (!nullBytesCheckPassed && whdRecord->streams.id != 0x004F4850)
      {
        if (whdRecord->sceneAliased.nullByte != 0)
        {
          whdPtr = reinterpret_cast<char *>(whdRecord);
          break;
        }
        whdPtr -= 3 * sizeof(uint32_t);
      }

      String8CI filePath(std::string_view(data.data() + whdRecord->scene.filePathOffset));
      auto filePathNative = filePath.path();

      if (filePathNative.extension() != StringViewWCI(L".wav"))
        return Clear(false);

      if (whdRecord->streams.dataInStreams == 0)
        filePath = relative(loadPathView.path(), archiveDialog.basePath.path()) / filePath.path();
      else if (!filePath.native().starts_with("Streams"))
        filePath = L"Streams" / filePathNative;

      auto& file = archiveDialog.GetFile(filePath);

      if (!recordMap.try_emplace(file.path, whdRecord).second)
        return Clear(false);

      archiveDialog.whdRecordsMap[file.path].emplace_back(whdRecord);
      [[maybe_unused]] const auto [fileMapIt, fileMapEmplaced] = archiveDialog.fileMap.try_emplace(file.path, HitmanFile{file.path, whdRecord->ToHitmanSoundRecord()});
      assert(whdRecord->streams.dataInStreams || fileMapEmplaced);

      ++iterCount;
    } while (!reinterpret_cast<uint32_t *>(whdPtr)[0] && reinterpret_cast<uint32_t *>(whdPtr)[1] && std::distance(whdPtr, &data.back() + 1) != 16);

    if (iterCount > 1)
      whdPtr += 2 * sizeof(uint32_t);
  }

  path = loadPathView;

  return true;
}

bool Hitman4WHDFile::Save(const Hitman4STRFile &streamsWAV, const Hitman4WAVFile &missionWAV, const StringView8CI &savePathView)
{
  for (auto *whdRecord : recordMap | std::views::values)
  {
    //const auto &wavRecordMap = whdRecord->streams.dataInStreams == 0 ? missionWAV.recordMap : streamsWAV.recordMap;
    assert(whdRecord->streams.dataInStreams == 0);
    const auto &wavRecordMap = missionWAV.recordMap;
    const auto wavRecordIt = wavRecordMap.find(whdRecord->streams.dataOffset);
    assert(wavRecordIt != wavRecordMap.end());
    if (wavRecordIt != wavRecordMap.end())
      whdRecord->streams.dataOffset = wavRecordIt->second.newOffset;
  }

  const auto oldSync = std::ios_base::sync_with_stdio(false);

  std::ofstream whdData(savePathView.path(), std::ios::binary | std::ios::trunc);
  whdData.write(data.data(), data.size());
  whdData.close();

  std::ios_base::sync_with_stdio(oldSync);

  path = savePathView;

  return true;
}

bool Hitman4Dialog::Clear(const bool retVal)
{
  whdFiles.clear();
  wavFiles.clear();
  streamsWAV.Clear();
  basePath.clear();
  fileMap.clear();
  whdRecordsMap.clear();

  return HitmanDialog::Clear(retVal);
}

bool Hitman4Dialog::ExportSingle(const StringView8CI &exportFolderPath, const StringView8CI &exportFilePath,
    const Options &options) const
{
  if (!HitmanDialog::ExportSingle(exportFolderPath, exportFilePath, options))
    return false;

  if (!options.hitman4.exportWithLIPData)
    return true;

  const auto streamsFileNameIt = std::ranges::find_if(streamsWAV.stringTable, [fileName = exportFilePath.native().substr(8)](const auto& elem){ return fileName == elem; });
  if (streamsFileNameIt == streamsWAV.stringTable.end())
    return true;

  const auto streamsIndex = std::distance(streamsWAV.stringTable.begin(), streamsFileNameIt);

  const auto& lipData = streamsWAV.lipDataTable[streamsIndex];
  if (lipData.empty())
    return true;

  const auto exportPath = (exportFolderPath.path() / exportFilePath.path()).replace_extension(L".LIP");

  create_directories(exportPath.parent_path());

  const auto oldSync = std::ios_base::sync_with_stdio(false);

  std::ofstream exportBin(exportPath, std::ios::binary | std::ios::trunc);
  exportBin.write(lipData.data(), static_cast<int64_t>(lipData.size()));

  std::ios_base::sync_with_stdio(oldSync);

  return true;
}

bool Hitman4Dialog::ImportSingle(const StringView8CI &importFolderPath, const StringView8CI &importFilePath, const Options &options)
{
  auto filePath = relative(importFilePath.path(), importFolderPath.path());
  auto fileIt = fileMap.find(filePath);
  auto whdRecordsIt = whdRecordsMap.find(filePath);
  if (fileIt == fileMap.end() || whdRecordsIt == whdRecordsMap.end())
  {
    filePath.replace_extension(filePath.extension() == StringViewWCI(L".wav") ? L".ogg" : L".wav");
    fileIt = fileMap.find(filePath);
    whdRecordsIt = whdRecordsMap.find(filePath);
    if (fileIt == fileMap.end() || whdRecordsIt == whdRecordsMap.end())
    {
      DisplayWarning(g_LocalizationManager.LocalizeFormat("HITMAN_DIALOG_WARNING_MISSING_FILE", importFilePath),
                     g_LocalizationManager.Localize("MESSAGEBOX_WARNING_GENERIC_TITLE"), false, options);
      return false;
    }
  }

  if (!ImportSingleHitmanFile(fileIt->second, importFilePath, options))
    return false;

  for (auto* whdRecord : whdRecordsIt->second)
    whdRecord->FromHitmanSoundRecord(fileIt->second.archiveRecord);

  return true;
}

bool Hitman4Dialog::LoadImpl(const StringView8CI &loadPath, const Options &options)
{
  Clear();

  const auto rootPath = loadPath.path().parent_path();
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

  OrderedMap<StringView8CI, Hitman4WHDRecord *> allWHDRecords;
  for (const auto &whdPath : allWHDFiles)
  {
    auto &whdFile = whdFiles.emplace_back();
    if (!whdFile.Load(*this, whdPath))
      return Clear(false);

    for (const auto& [filePath, whdRecord] : whdFile.recordMap)
    {
      [[maybe_unused]] const auto res = allWHDRecords.try_emplace(filePath, whdRecord);

      // TODO - do we want to handle this in some way? duplicates pointing to streams should not be an issue unless offsets are different...
      assert(res.second || (res.first->second->scene.dataInStreams && res.first->second->scene.dataInStreams == whdRecord->scene.dataInStreams && res.first->second->scene.dataOffset == whdRecord->scene.dataOffset));
    }

    if (!wavFiles.emplace_back().Load(String8CI(whdFile.path.path().replace_extension(L".wav")), whdFile.recordMap, fileMap))
      return Clear(false);
  }

  const auto streamsWAVData = ReadWholeBinaryFile(loadPath);
  if (!streamsWAV.Load(*this, streamsWAVData, allWHDRecords))
    return Clear(false);

  auto dataPath = GetProgramPath().path();
  if (dataPath.empty())
    return Clear(false);

  dataPath /= L"data";
  dataPath /= L"records";
  dataPath /= L"h4_";

  originalDataPathPrefix = dataPath;
  originalDataID = XXH3_64bits(streamsWAVData.data(), streamsWAVData.size());
  originalDataParentID = 0;

  if (!LoadOriginalData(options))
    return Clear(false);

  return true;
}

bool Hitman4Dialog::SaveImpl(const StringView8CI &savePathView, const Options &options)
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

int32_t Hitman4Dialog::DrawDialog()
{
  return DrawHitmanDialog();
}

bool Hitman4Dialog::IsSaveAllowed() const
{
  return G1AT_DEBUG_BUILD == 1;
}

bool Hitman4Dialog::IsExportAllowed() const
{
  return true;
}

bool Hitman4Dialog::IsImportAllowed() const
{
  return G1AT_DEBUG_BUILD == 1;
}

const std::vector<std::pair<String8, String8CI>>& Hitman4Dialog::GetOpenFilter()
{
  static std::vector<std::pair<String8, String8CI>> filters;
  if (!filters.empty())
    return filters;

  filters.emplace_back("FILE_DIALOG_FILTER_HITMAN4_STREAMS", "pc_*.str");

  return filters;
}

const std::vector<std::pair<String8, String8CI>>& Hitman4Dialog::GetSaveFilter() const
{
  static std::vector<std::pair<String8, String8CI>> filters;
  if (!filters.empty())
    return filters;

  filters.emplace_back("FILE_DIALOG_FILTER_HITMAN4_STREAMS", "pc_*.str");

  return filters;
}
