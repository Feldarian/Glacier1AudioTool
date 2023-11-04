//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//
// TODO - GOG and Steam version have a hash conflict! Either they truly have streams file the exact same or I don't know... But it reports everything as changed!
//      - workaround for now - remove "h4_" prefixed files from "data/records" before opening GOG archive...
// TODO - try to interleave aliased data
//        * doubling sample rate seems to fix things, this may mean half of the frame is real sample and half is not
//          * this may be confirmed purely technically by Audacity or something for example, just try to playback single channel... make sure though that you use NATIVE EXPORT! (samplerates will mess it up...)
//        * it may also mean that there are truly 2 different audio files, they are just interlieved in some way
//

#include <Precompiled.hpp>

#include "Hitman4Dialog.hpp"

#include <Config/Config.hpp>

#include "Utils.hpp"

HitmanSoundRecord Hitman4WHDRecordScene::ToHitmanSoundRecord() const
{
  assert(filePathLength > 0 && filePathLength < 0x004F4850);

  const auto resBitsPerSample = static_cast<uint16_t>(formatTag == 4096 || bitsPerSample == 0 ? 16 : bitsPerSample);
  const auto resChannels = static_cast<uint16_t>(channels);
  const auto resBlockAlign = static_cast<uint16_t>(formatTag == 4096 || blockAlign == 0 ? 2 * channels : blockAlign);
  const auto resFmtExtra = static_cast<uint16_t>(formatTag == 17 ? samplesPerBlock : 1);

  //assert(resBlockAlign == 2 * channels || formatTag == 17);
  //assert(resBlockAlign != 1024 || formatTag == 17);

  assert(pad30[0] == 0);
  assert(pad30[1] == 0);
  assert(pad30[2] == 0);
  assert(pad30[3] == 0);

  assert(dataInStreams == 0);

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

void Hitman4WHDRecordScene::FromHitmanSoundRecord(const HitmanSoundRecord &soundRecord)
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

HitmanSoundRecord Hitman4STRRecordHeader::ToHitmanSoundRecord() const
{
  return {
    0,
    static_cast<uint32_t>(samplesCount * sizeof(int16_t)),
    0, // must be filled externally, cannot deduce from this header alone...
    sampleRate,
    static_cast<uint16_t>(magic == 0x02 || magic == 0x11 ? 0x01 : (magic == 0x04 ? 0x1000 : (magic == 0x03 ? 0x11 : 0x00))),
    static_cast<uint16_t>(bitsPerSample ? bitsPerSample : 16),
    static_cast<uint16_t>(channels),
    static_cast<uint16_t>(magic == 0x03 && blockAlign ? blockAlign : channels * ((bitsPerSample ? bitsPerSample : 16) / 8)),
    static_cast<uint16_t>(samplesPerBlock ? samplesPerBlock : channels)
  };
}

void Hitman4STRRecordHeader::FromHitmanSoundRecord(const HitmanSoundRecord &soundRecord)
{
  // TODO - 0x03 should not be hardcoded in magic!
  magic = soundRecord.formatTag == 0x01 ? 0x02 : (soundRecord.formatTag == 0x1000 ? 0x04 : (soundRecord.formatTag == 0x11 ? 0x03 : 0x00));
  samplesCount = soundRecord.dataSizeUncompressed / sizeof(int16_t);
  channels = soundRecord.channels;
  sampleRate = soundRecord.sampleRate;
  bitsPerSample = soundRecord.formatTag == 0x1000 ? 0 : soundRecord.bitsPerSample;
  blockAlign = soundRecord.formatTag == 0x01 || soundRecord.formatTag == 0x11 ? soundRecord.blockAlign : 0;
  samplesPerBlock = soundRecord.formatTag == 0x11 ? soundRecord.samplesPerBlock : 0;
}

bool Hitman4STRFile::Clear(const bool retVal)
{
  header = {};
  path.clear();

  return retVal;
}

bool Hitman4STRFile::Load(Hitman4Dialog& archiveDialog, const std::vector<char> &wavData)
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
  wavHeaderTable.resize(header.entriesCount);
  stringTable.resize(header.entriesCount);

  wavHeaderTableBeginOffset = header.offsetToEntryTable;
  stringTableBeginOffset = header.offsetToEntryTable;

  OrderedSet<uint64_t> dataOffsets;
  for (uint32_t i = 0; i < header.entriesCount; ++i)
  {
    const auto& strRecord = recordTable[i];

    dataOffsets.insert(strRecord.dataOffset);

    if (wavData.size() - strRecord.headerOffset < strRecord.sizeOfHeader)
      return Clear(false);

    if (wavData.size() - strRecord.fileNameOffset < strRecord.fileNameLength + 1)
      return Clear(false);

    wavHeaderTableBeginOffset = std::min(wavHeaderTableBeginOffset, strRecord.headerOffset);
    stringTableBeginOffset = std::min(stringTableBeginOffset, strRecord.fileNameOffset);
  }

  if (dataOffsets.empty())
    return Clear(false);

  dataOffsets.insert(wavHeaderTableBeginOffset);

  for (uint32_t i = 0; i < header.entriesCount; ++i)
  {
    const auto& strRecord = recordTable[i];

    auto& strWAVHeader = wavHeaderTable[i];
    std::memcpy(&strWAVHeader, wavData.data() + strRecord.headerOffset, strRecord.sizeOfHeader);

    auto& strFilename = stringTable[i];
    strFilename.resize(strRecord.fileNameLength);
    std::memcpy(strFilename.data(), wavData.data() + strRecord.fileNameOffset, strRecord.fileNameLength);

    if (!fileNameToIndex.try_emplace(strFilename, i).second)
      return Clear(false);
  }

  struct StreamsAliasedRecord
  {
    uint32_t masterId = 0;
    uint32_t firstReferenceId = 0;
    uint32_t secondaryDataId = 0;
  };

  OrderedMap<uint64_t, StreamsAliasedRecord> aliasedDataMap;
  for (uint32_t i = 0; i < header.entriesCount; ++i)
  {
    auto& strRecord = recordTable[i];
    auto& strFilename = stringTable[i];

    if (!strFilename.path().extension().empty())
      continue;

    const auto& [aliasedDataIt, emplaced] = aliasedDataMap.try_emplace(strRecord.dataOffset, StreamsAliasedRecord{i});
    if (!emplaced)
      return Clear(false);

    strRecord.dataSize = 0;
  }

  for (uint32_t i = 0; i < header.entriesCount; ++i)
  {
    auto& strRecord = recordTable[i];
    auto& strFilename = stringTable[i];
    auto& strWAVHeader = wavHeaderTable[i];

    if (strFilename.path().extension().empty())
      continue;

    const auto aliasedDataIt = aliasedDataMap.find(strRecord.dataOffset);
    if (aliasedDataIt == aliasedDataMap.end())
      continue;

    auto& streamsAliasedRecord = aliasedDataIt->second;
    auto& strRecordMaster = recordTable[streamsAliasedRecord.masterId];

    if (strRecord.aliasedEntryOrderOrReference == 1)
    {
      assert(streamsAliasedRecord.firstReferenceId == 0);
      streamsAliasedRecord.firstReferenceId = i;
    }
    else if (strRecord.aliasedEntryOrderOrReference == 2)
    {
      assert(streamsAliasedRecord.secondaryDataId == 0);
      streamsAliasedRecord.secondaryDataId = i;
    }
    else
    {
      assert(false);
      return Clear(false);
    }

    strRecordMaster.hasLIP |= strRecord.hasLIP;
    strRecordMaster.dataSize += strWAVHeader.samplesCount * sizeof(int16_t);
  }

  recordMap.clear();

  std::vector<std::reference_wrapper<HitmanFile>> strFiles;
  for (uint32_t i = 0; i < header.entriesCount; ++i)
  {
    auto& strFilename = stringTable[i];
    auto& strLIPData = lipDataTable[i];

    auto strIndex = i;
    auto strRecord = recordTable[strIndex];
    const auto aliasedDataIt = aliasedDataMap.find(strRecord.dataOffset);
    if (aliasedDataIt != aliasedDataMap.end())
    {
      if (aliasedDataIt->second.masterId == i)
        continue;

      strIndex = aliasedDataIt->second.masterId;
      strRecord = recordTable[aliasedDataIt->second.masterId];
    }

    auto& strWAVHeader = wavHeaderTable[strIndex];

    auto strFilePath = strFilename.path();
    assert(!strFilePath.extension().empty());

    const auto strFilePathCI = String8CI("Streams\\") += strFilePath;

    auto soundRecord = strWAVHeader.ToHitmanSoundRecord();
    soundRecord.dataSize = static_cast<uint32_t>(strRecord.dataSize);
    auto& file = archiveDialog.GetFile(strFilePathCI);
    const auto [fileMapIt, fileMapEmplaced] = archiveDialog.fileMap.try_emplace(file.path, HitmanFile{file.path, soundRecord});
    if (!fileMapEmplaced)
      return Clear(false);

    auto& hitmanFile = fileMapIt->second;

    assert(dataOffsets.contains(strRecord.dataOffset));
    assert(++dataOffsets.find(strRecord.dataOffset) != dataOffsets.end());

    const auto wavDataEndOffset = *(++dataOffsets.find(strRecord.dataOffset));
    const auto wavDataSize = wavDataEndOffset - strRecord.dataOffset;

    assert(wavData.size() >= wavDataEndOffset);
    assert(wavDataSize % 0x0100 == 0);

    assert(strWAVHeader.magic != 0x02 || strRecord.sizeOfHeader == 24);
    assert(strWAVHeader.magic != 0x03 || strRecord.sizeOfHeader == 0x1c);
    assert(strWAVHeader.magic != 0x04 || strRecord.sizeOfHeader == 20);
    assert(strWAVHeader.magic != 0x11 || strRecord.sizeOfHeader == 0x18);
    assert(strRecord.id < header.entriesCount);

    hitmanFile.data.resize(strRecord.dataSize);
    ranges::fill(hitmanFile.data, 0);
    strFiles.emplace_back(hitmanFile);

    if (!strRecord.hasLIP)
    {
      std::memcpy(hitmanFile.data.data(), wavData.data() + strRecord.dataOffset, strRecord.dataSize);

      if (strIndex == i)
      {
        [[maybe_unused]] const auto [recordMapIt, recordMapEmplaced] = recordMap.try_emplace(strRecord.dataOffset, Hitman4WAVRecord{hitmanFile.data, static_cast<uint32_t>(strRecord.dataOffset)});
        if (!recordMapEmplaced)
          return Clear(false);

        continue;
      }

      auto& strSub1Record = recordTable[aliasedDataIt->second.firstReferenceId];
      auto& strSub1WAVHeader = wavHeaderTable[aliasedDataIt->second.firstReferenceId];
      size_t strSub1DataIndex = 0;
      auto& strSub2Record = recordTable[aliasedDataIt->second.secondaryDataId];
      auto& strSub2WAVHeader = wavHeaderTable[aliasedDataIt->second.secondaryDataId];
      size_t strSub2DataIndex = 0;

      assert(strSub1WAVHeader.sampleRate >= strSub2WAVHeader.sampleRate);
      assert(strSub1WAVHeader.channels >= strSub2WAVHeader.channels);
      const auto samplesBlockSecondaryDataOffset = (strSub1WAVHeader.sampleRate / strSub2WAVHeader.sampleRate) * (strSub1WAVHeader.channels / strSub2WAVHeader.channels) * sizeof(int16_t);
      const auto samplesBlockSecondaryDataSize = strSub2WAVHeader.channels * sizeof(int16_t);

      auto& strThisSubRecord = (i == aliasedDataIt->second.firstReferenceId) ? strSub1Record : strSub2Record;
      auto& strThisWAVHeader = (i == aliasedDataIt->second.firstReferenceId) ? strSub1WAVHeader : strSub2WAVHeader;
      auto& strThisDataIndex = (i == aliasedDataIt->second.firstReferenceId) ? strSub1DataIndex : strSub2DataIndex;
      auto& strThisSamplesBlockSize = (i == aliasedDataIt->second.firstReferenceId) ? samplesBlockSecondaryDataOffset : samplesBlockSecondaryDataSize;

      if (i == aliasedDataIt->second.secondaryDataId)
        strSub1DataIndex += samplesBlockSecondaryDataOffset;

      assert(strSub1Record.dataSize + strSub2Record.dataSize <= strRecord.dataSize);
      while (strThisDataIndex < strThisSubRecord.dataSize)
      {
        std::memmove(hitmanFile.data.data() + strThisDataIndex, hitmanFile.data.data() + strSub1DataIndex + strSub2DataIndex, std::min(strThisSubRecord.dataSize - strThisDataIndex, strThisSamplesBlockSize));
        strSub1DataIndex = std::min(strSub1DataIndex + samplesBlockSecondaryDataOffset, strSub1Record.dataSize);
        strSub2DataIndex = std::min(strSub2DataIndex + samplesBlockSecondaryDataSize, strSub2Record.dataSize);
      }

      hitmanFile.data.resize(strThisSubRecord.dataSize);
      hitmanFile.archiveRecord = strThisWAVHeader.ToHitmanSoundRecord();
      hitmanFile.archiveRecord.dataSize = static_cast<uint32_t>(strThisSubRecord.dataSize);

      [[maybe_unused]] const auto [recordMapIt, recordMapEmplaced] = recordMap.try_emplace(strRecord.dataOffset | (strThisSubRecord.aliasedEntryOrderOrReference & 0xFF), Hitman4WAVRecord{hitmanFile.data, static_cast<uint32_t>(strRecord.dataOffset)});
      if (!recordMapEmplaced)
        return Clear(false);

      continue;
    }

    assert(*reinterpret_cast<const uint32_t*>(wavData.data() + strRecord.dataOffset) == 0x2050494C);

    // TODO - LIP data should be copied only into entry which has it if strIndex != i
    const auto lipDataSize = (wavDataSize - strRecord.dataSize) & (~0xFFFull);
    strLIPData.resize(lipDataSize);
    ranges::fill(strLIPData, 0);

    if (lipDataSize <= 0x1000)
    {
      std::memcpy(strLIPData.data(), wavData.data() + strRecord.dataOffset, lipDataSize);
      std::memcpy(hitmanFile.data.data(), wavData.data() + strRecord.dataOffset + lipDataSize, strRecord.dataSize);

      if (strIndex == i)
      {
        [[maybe_unused]] const auto [recordMapIt, recordMapEmplaced] = recordMap.try_emplace(strRecord.dataOffset, Hitman4WAVRecord{hitmanFile.data, static_cast<uint32_t>(strRecord.dataOffset)});
        if (!recordMapEmplaced)
          return Clear(false);

        continue;
      }

      auto& strSub1Record = recordTable[aliasedDataIt->second.firstReferenceId];
      auto& strSub1WAVHeader = wavHeaderTable[aliasedDataIt->second.firstReferenceId];
      size_t strSub1DataIndex = 0;
      auto& strSub2Record = recordTable[aliasedDataIt->second.secondaryDataId];
      auto& strSub2WAVHeader = wavHeaderTable[aliasedDataIt->second.secondaryDataId];
      size_t strSub2DataIndex = 0;

      assert(strSub1WAVHeader.sampleRate >= strSub2WAVHeader.sampleRate);
      assert(strSub1WAVHeader.channels >= strSub2WAVHeader.channels);
      const auto samplesBlockSecondaryDataOffset = (strSub1WAVHeader.sampleRate / strSub2WAVHeader.sampleRate) * (strSub1WAVHeader.channels / strSub2WAVHeader.channels) * sizeof(int16_t);
      const auto samplesBlockSecondaryDataSize = strSub2WAVHeader.channels * sizeof(int16_t);

      auto& strThisSubRecord = (i == aliasedDataIt->second.firstReferenceId) ? strSub1Record : strSub2Record;
      auto& strThisWAVHeader = (i == aliasedDataIt->second.firstReferenceId) ? strSub1WAVHeader : strSub2WAVHeader;
      auto& strThisDataIndex = (i == aliasedDataIt->second.firstReferenceId) ? strSub1DataIndex : strSub2DataIndex;
      auto& strThisSamplesBlockSize = (i == aliasedDataIt->second.firstReferenceId) ? samplesBlockSecondaryDataOffset : samplesBlockSecondaryDataSize;

      if (i == aliasedDataIt->second.secondaryDataId)
        strSub1DataIndex += samplesBlockSecondaryDataOffset;

      assert(strSub1Record.dataSize + strSub2Record.dataSize <= strRecord.dataSize);
      while (strThisDataIndex < strThisSubRecord.dataSize)
      {
        std::memmove(hitmanFile.data.data() + strThisDataIndex, hitmanFile.data.data() + strSub1DataIndex + strSub2DataIndex, std::min(strThisSubRecord.dataSize - strThisDataIndex, strThisSamplesBlockSize));
        strSub1DataIndex = std::min(strSub1DataIndex + samplesBlockSecondaryDataOffset, strSub1Record.dataSize);
        strSub2DataIndex = std::min(strSub2DataIndex + samplesBlockSecondaryDataSize, strSub2Record.dataSize);
      }

      hitmanFile.data.resize(strThisSubRecord.dataSize);
      hitmanFile.archiveRecord = strThisWAVHeader.ToHitmanSoundRecord();
      hitmanFile.archiveRecord.dataSize = static_cast<uint32_t>(strThisSubRecord.dataSize);

      if (!strThisSubRecord.hasLIP)
        strLIPData.clear();

      [[maybe_unused]] const auto [recordMapIt, recordMapEmplaced] = recordMap.try_emplace(strRecord.dataOffset | (strThisSubRecord.aliasedEntryOrderOrReference & 0xFF), Hitman4WAVRecord{hitmanFile.data, static_cast<uint32_t>(strRecord.dataOffset)});
      if (!recordMapEmplaced)
        return Clear(false);

      continue;
    }

    uint64_t lipSegmentsCount = lipDataSize / 0x1000;
    assert(lipSegmentsCount * 0x1000 == lipDataSize);

    uint64_t lipSegmentSize = 0;
    for (uint64_t lipIndex = 0x1000; lipIndex < wavDataSize; lipIndex += 0x1000)
    {
      static constexpr std::array<char, 0x0400> nullBlock = {};
      if (std::memcmp(wavData.data() + strRecord.dataOffset + lipIndex + 0x0100 + (0x0F00 -nullBlock.size()), nullBlock.data(), nullBlock.size()) == 0)
      {
        lipSegmentSize = lipIndex;

        uint64_t foundSegments = 2;
        for (uint64_t lipSegmentOffset = lipSegmentSize * 2; lipSegmentOffset < wavDataSize; lipSegmentOffset += lipSegmentSize, ++foundSegments)
        {
          if (std::memcmp(wavData.data() + strRecord.dataOffset + lipSegmentOffset + 0x0100 + (0x0F00 - nullBlock.size()), nullBlock.data(), nullBlock.size()) != 0)
          {
            foundSegments = 0;
            break;
          }
        }

        if (foundSegments != lipSegmentsCount)
          lipSegmentSize = 0;

        if (lipSegmentSize)
          break;
      }
    }

    if (lipSegmentSize == 0)
      return Clear(false);

    auto lipBytesLeft = lipDataSize;
    auto wavBytesLeft = strRecord.dataSize;
    uint64_t lipDataWriteOffset = 0;
    uint64_t wavDataWriteOffset = 0;
    const uint64_t dataDividerOffset = 0x1000;
    assert(lipDataSize % dataDividerOffset == 0);
    for (uint64_t lipSegmentOffset = 0; lipSegmentOffset < wavDataSize; lipSegmentOffset += lipSegmentSize)
    {
      const auto lipCopySize = std::min(lipBytesLeft, dataDividerOffset);
      assert(lipCopySize == dataDividerOffset);
      std::memcpy(strLIPData.data() + lipDataWriteOffset, wavData.data() + strRecord.dataOffset + lipSegmentOffset, lipCopySize);
      lipDataWriteOffset += lipCopySize;
      lipBytesLeft -= lipCopySize;

      const auto wavCopySize = std::min(wavBytesLeft, lipSegmentSize - lipCopySize);
      std::memcpy(hitmanFile.data.data() + wavDataWriteOffset, wavData.data() + strRecord.dataOffset + lipSegmentOffset + lipCopySize, wavCopySize);
      wavDataWriteOffset += wavCopySize;
      wavBytesLeft -= wavCopySize;
    }

    if (strIndex == i)
    {
      [[maybe_unused]] const auto [recordMapIt, recordMapEmplaced] = recordMap.try_emplace(static_cast<uint32_t>(strRecord.dataOffset), Hitman4WAVRecord{hitmanFile.data, static_cast<uint32_t>(strRecord.dataOffset), strLIPData});
      if (!recordMapEmplaced)
        return Clear(false);

      continue;
    }

    auto& strSub1Record = recordTable[aliasedDataIt->second.firstReferenceId];
    auto& strSub1WAVHeader = wavHeaderTable[aliasedDataIt->second.firstReferenceId];
    size_t strSub1DataIndex = 0;
    auto& strSub2Record = recordTable[aliasedDataIt->second.secondaryDataId];
    auto& strSub2WAVHeader = wavHeaderTable[aliasedDataIt->second.secondaryDataId];
    size_t strSub2DataIndex = 0;

    assert(strSub1WAVHeader.sampleRate >= strSub2WAVHeader.sampleRate);
    assert(strSub1WAVHeader.channels >= strSub2WAVHeader.channels);
    const auto samplesBlockSecondaryDataOffset = (strSub1WAVHeader.sampleRate / strSub2WAVHeader.sampleRate) * (strSub1WAVHeader.channels / strSub2WAVHeader.channels) * sizeof(int16_t);
    const auto samplesBlockSecondaryDataSize = strSub2WAVHeader.channels * sizeof(int16_t);

    auto& strThisSubRecord = (i == aliasedDataIt->second.firstReferenceId) ? strSub1Record : strSub2Record;
    auto& strThisWAVHeader = (i == aliasedDataIt->second.firstReferenceId) ? strSub1WAVHeader : strSub2WAVHeader;
    auto& strThisDataIndex = (i == aliasedDataIt->second.firstReferenceId) ? strSub1DataIndex : strSub2DataIndex;
    auto& strThisSamplesBlockSize = (i == aliasedDataIt->second.firstReferenceId) ? samplesBlockSecondaryDataOffset : samplesBlockSecondaryDataSize;

    if (i == aliasedDataIt->second.secondaryDataId)
      strSub1DataIndex += samplesBlockSecondaryDataOffset;

    assert(strSub1Record.dataSize + strSub2Record.dataSize <= strRecord.dataSize);
    while (strThisDataIndex < strThisSubRecord.dataSize)
    {
      std::memmove(hitmanFile.data.data() + strThisDataIndex, hitmanFile.data.data() + strSub1DataIndex + strSub2DataIndex, std::min(strThisSubRecord.dataSize - strThisDataIndex, strThisSamplesBlockSize));
      strSub1DataIndex = std::min(strSub1DataIndex + samplesBlockSecondaryDataOffset, strSub1Record.dataSize);
      strSub2DataIndex = std::min(strSub2DataIndex + samplesBlockSecondaryDataSize, strSub2Record.dataSize);
    }

    hitmanFile.data.resize(strThisSubRecord.dataSize);
    hitmanFile.archiveRecord = strThisWAVHeader.ToHitmanSoundRecord();
    hitmanFile.archiveRecord.dataSize = static_cast<uint32_t>(strThisSubRecord.dataSize);

    if (!strSub1Record.hasLIP)
      strLIPData.clear();

    [[maybe_unused]] const auto [recordMapIt, recordMapEmplaced] = recordMap.try_emplace(strRecord.dataOffset | (strThisSubRecord.aliasedEntryOrderOrReference & 0xFF), Hitman4WAVRecord{hitmanFile.data, static_cast<uint32_t>(strRecord.dataOffset)});
    if (!recordMapEmplaced)
      return Clear(false);
  }

  std::atomic_bool importFailed = false;
  std::for_each(std::execution::par, strFiles.begin(), strFiles.end(), [this, &importFailed](auto& hitmanFileRef)
  {
    if (importFailed.load(std::memory_order_relaxed))
      return;

    auto& hitmanFile = hitmanFileRef.get();
    auto updatedSoundRecord = SoundDataSoundRecord(hitmanFile.archiveRecord, {hitmanFile.data.data(), hitmanFile.data.size()});
    importFailed.store(importFailed.load(std::memory_order_relaxed) || updatedSoundRecord.dataXXH3 == 0);
    hitmanFile.archiveRecord = std::move(updatedSoundRecord);
  });

  if (importFailed)
    return Clear(false);

  if (recordMap.empty())
    return Clear(false);

  return true;
}

bool Hitman4STRFile::Load(Hitman4Dialog& archiveDialog, const StringView8CI &loadPath)
{
  if (!Load(archiveDialog, ReadWholeBinaryFile(loadPath)))
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

bool Hitman4WAVFile::Load(const std::vector<char> &wavData, const OrderedMap<StringView8CI, Hitman4WHDRecordScene *> &whdRecordsMap, OrderedMap<StringView8CI, HitmanFile>& fileMap)
{
  if (wavData.empty())
    return Clear(false);

  struct WAVFileData
  {
    Hitman4WHDRecordScene *record = nullptr;
    HitmanFile& file;
  };

  auto resampledOffset = static_cast<uint32_t>(wavData.size());
  size_t foundItems = 0;
  OrderedMap<uint32_t, uint32_t> resampledMap;
  OrderedMap<uint32_t, WAVFileData> offsetToWAVFileDataMap;
  for (auto& [whdRecordPath, whdRecord] : whdRecordsMap)
  {
    if (whdRecord->dataInStreams != 0)
    {
      assert(false);
      continue;
    }

    auto offsetToWAVFileDataIt = offsetToWAVFileDataMap.find(whdRecord->dataOffset);
    if (offsetToWAVFileDataIt != offsetToWAVFileDataMap.end())
    {
      resampledMap[resampledOffset] = whdRecord->dataOffset;
      whdRecord->dataOffset = resampledOffset;
      resampledOffset += whdRecord->dataSize;
    }

    offsetToWAVFileDataMap.insert({whdRecord->dataOffset, {whdRecord, fileMap.at(whdRecordPath)}});

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
    newData.resize(wavFileData.record->dataSize, 0);
    newData.shrink_to_fit();

    auto trueOffset = offset >= wavData.size() ? resampledMap[offset] : offset;

    std::memcpy(newData.data(), wavData.data() + trueOffset, wavFileData.record->dataSize);
    currOffset = offset + wavFileData.record->dataSize;
    recordMap.try_emplace(offset, Hitman4WAVRecord{newData, offset});
  }

  if (currOffset < wavData.size())
  {
    auto& newData = extraData.emplace_back();
    newData.resize(wavData.size() - currOffset, 0);
    newData.shrink_to_fit();

    std::memcpy(newData.data(), wavData.data() + currOffset, wavData.size() - currOffset);
    recordMap.try_emplace(currOffset, Hitman4WAVRecord{newData, currOffset});
  }

  const auto wavFileDataView = offsetToWAVFileDataMap | ranges::views::values;

  std::atomic_bool importFailed = false;
  std::for_each(std::execution::par, wavFileDataView.begin(), wavFileDataView.end(), [this, &importFailed](auto& wavFileData)
  {
    if (importFailed.load(std::memory_order_relaxed))
      return;

    auto& hitmanFile = wavFileData.file;
    hitmanFile.archiveRecord = SoundDataSoundRecord(hitmanFile.archiveRecord, hitmanFile.data);
    importFailed.store(importFailed.load(std::memory_order_relaxed) || hitmanFile.archiveRecord.dataXXH3 == 0);
  });

  if (importFailed)
    return Clear(false);

  header = reinterpret_cast<Hitman4WAVHeader *>(recordMap.at(0).data.data());

  return true;
}

bool Hitman4WAVFile::Load(const StringView8CI &loadPath, const OrderedMap<StringView8CI, Hitman4WHDRecordScene *> &whdRecordsMap, OrderedMap<StringView8CI, HitmanFile>& fileMap)
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
  for (auto &record : recordMap | ranges::views::values)
  {
    record.newOffset = offset;
    offset += static_cast<uint32_t>(record.data.size());
  }

  for (auto &record : recordMap | ranges::views::values)
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

  while (memcmp(whdPtr, terminateBytes.data(), terminateBytes.size() * sizeof(uint32_t)) != 0)
  {
    Hitman4WHDRecordScene *whdRecord = nullptr;

    do
    {
      whdPtr += std::strlen(whdPtr) + 1; // + 0-15 bytes, so it is aligned on 16 bytes...
      assert(static_cast<size_t>(whdPtr - data.data()) <= data.size());

      if (reinterpret_cast<uintptr_t>(whdPtr) % 16 != 0)
      {
        whdPtr += 16 - (reinterpret_cast<uintptr_t>(whdPtr) % 16);
        assert(static_cast<size_t>(whdPtr - data.data()) <= data.size());
      }

      while (memcmp(whdPtr, terminateBytes.data(), terminateBytes.size() * sizeof(uint32_t)) == 0)
      {
        whdPtr += 16;
        assert(static_cast<size_t>(whdPtr - data.data()) <= data.size());
      }

      whdRecord = reinterpret_cast<Hitman4WHDRecordScene *>(whdPtr);
    } while (whdRecord->filePathLength >= data.size() && whdRecord->filePathLength != 0x004F4850);

    assert(whdRecord);

    if (whdRecord->dataInStreams)
    {
      whdPtr += whdRecord->filePathLength ? sizeof(Hitman4WHDRecordStreams) : sizeof(Hitman4WHDRecordStreamsAliased);
      assert(static_cast<size_t>(whdPtr - data.data()) <= data.size());
      continue;
    }

    whdPtr += sizeof(Hitman4WHDRecordScene);
    assert(static_cast<size_t>(whdPtr - data.data()) <= data.size());

    String8CI filePath(std::string_view(data.data() + whdRecord->filePathOffset));
    auto filePathNative = filePath.path();

    if (filePathNative.extension() != StringViewWCI(L".wav"))
      return Clear(false);

    filePath = relative(loadPathView.path(), archiveDialog.basePath.path()) / filePath.path();

    auto& file = archiveDialog.GetFile(filePath);

    if (!recordMap.try_emplace(file.path, whdRecord).second)
      return Clear(false);

    archiveDialog.whdRecordsMap[file.path].emplace_back(whdRecord);
    [[maybe_unused]] const auto [fileMapIt, fileMapEmplaced] = archiveDialog.fileMap.try_emplace(file.path, HitmanFile{file.path, whdRecord->ToHitmanSoundRecord()});
    if (!fileMapEmplaced)
      return Clear(false);
  }

  path = loadPathView;

  return true;
}

bool Hitman4WHDFile::Save(const Hitman4STRFile &streamsWAV, const Hitman4WAVFile &missionWAV, const StringView8CI &savePathView)
{
  return false;
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

  const auto streamsFileNameIt = ranges::find_if(streamsWAV.stringTable, [fileName = exportFilePath.native().substr(8)](const auto& elem){ return fileName == elem; });
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
                     g_LocalizationManager.Localize("MESSAGEBOX_TITLE_WARNING"), false, options);
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

  const auto streamsWAVData = ReadWholeBinaryFile(loadPath);
  if (!streamsWAV.Load(*this, streamsWAVData))
    return Clear(false);

  for (const auto &whdPath : allWHDFiles)
  {
    auto &whdFile = whdFiles.emplace_back();
    if (!whdFile.Load(*this, whdPath))
      return Clear(false);

    if (!wavFiles.emplace_back().Load(String8CI(whdFile.path.path().replace_extension(L".wav")), whdFile.recordMap, fileMap))
      return Clear(false);
  }

  auto dataPath = GetUserPath().path();
  if (dataPath.empty())
    return Clear(false);

  dataPath /= L"records";
  dataPath /= L"h4_";

  originalDataPathPrefix = dataPath;
  originalDataID = XXH3_64bits(streamsWAVData.data(), streamsWAVData.size());
  originalDataParentID = 0;

  if (!LoadOriginalData(options))
    return GenerateOriginalData(options);

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

const std::vector<std::pair<StringView8CI, StringView8>>& Hitman4Dialog::GetOpenFilter()
{
  static std::vector<std::pair<StringView8CI, StringView8>> filters;
  if (!filters.empty())
    return filters;

  filters.emplace_back("pc_*.str", "FILE_DIALOG_FILTER_HITMAN4_STREAMS");

  return filters;
}

const std::vector<std::pair<StringView8CI, StringView8>>& Hitman4Dialog::GetSaveFilter() const
{
  static std::vector<std::pair<StringView8CI, StringView8>> filters;
  if (!filters.empty())
    return filters;

  filters.emplace_back("pc_*.str", "FILE_DIALOG_FILTER_HITMAN4_STREAMS");

  return filters;
}
