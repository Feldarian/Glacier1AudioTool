//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#include <Precompiled.hpp>

#include "HitmanDialog.hpp"

#include "Utils.hpp"

// TODO - use spans like in PCMS16 which are resized to max. required capacity before use and optimize a bit calling of this that way
//        or even better, try to upgrade to std::ranges or views
bool HitmanFile::Import(SoundRecord soundRecord, const std::span<const char> &soundDataView, const Options &options)
{
  if (!soundRecord.dataSize || soundDataView.size() < soundRecord.dataSize)
    return false;

  if (options.common.directImport)
    return ImportNative(soundRecord, soundDataView, true, options);

  std::vector<int16_t> pcms16Decoded;
  if (!PCMS16FromSoundData(soundRecord, soundDataView, pcms16Decoded, PCMS16_CONVERSION_FLAG_RAW_OUTPUT))
    return false;

  soundRecord.dataXXH3 = XXH3_64bits(pcms16Decoded.data(), pcms16Decoded.size() * sizeof(int16_t));

  if (!options.common.importSameFiles && (archiveRecord.dataXXH3 == soundRecord.dataXXH3))
    return true;

  bool fixedData = false;
  auto fixedHeader = PCMS16Header(soundRecord);

  if (options.common.fixChannels)
  {
    const auto result = PCMS16ChangeChannelCount(fixedHeader, pcms16Decoded, originalRecord.channels);
    if (result < 0)
      return false;

    fixedData |= result > 0;
  }

  if (options.common.fixSampleRate)
  {
    const auto result = PCMS16ChangeSampleRate(fixedHeader, pcms16Decoded, originalRecord.sampleRate);
    if (result < 0)
      return false;

    fixedData |= result > 0;
  }

  if (!options.common.transcodeToOriginalFormat || originalRecord.formatTag == 0x01)
  {
    archiveRecord = PCMS16SoundRecord(fixedHeader, pcms16Decoded);
    archiveRecord.samplesPerBlock = 1;

    data.resize(archiveRecord.dataSize);
    std::memcpy(data.data(), pcms16Decoded.data(), archiveRecord.dataSize);

    return true;
  }

  if (fixedData || soundRecord.formatTag != originalRecord.formatTag)
  {
    switch (originalRecord.formatTag)
    {
      case 0x11: {
        std::vector<char> adpcmData(archiveRecord.dataSize);
        if (!PCMS16ToADPCM(fixedHeader, pcms16Decoded, adpcmData, PCMS16_CONVERSION_FLAG_RAW_OUTPUT))
          return false;

        archiveRecord = ADPCMSoundRecord(ADPCMHeader(PCMS16SoundRecord(fixedHeader)), adpcmData);

        data.resize(archiveRecord.dataSize);
        std::memcpy(data.data(), adpcmData.data(), archiveRecord.dataSize);

        return true;
      }
      case 0x1000: {
        std::vector<char> vorbisData;
        if (!PCMS16ToVorbis(fixedHeader, pcms16Decoded, vorbisData))
          return false;

        archiveRecord = VorbisSoundRecord(VorbisHeader(vorbisData), vorbisData);

        data.resize(archiveRecord.dataSize);
        std::memcpy(data.data(), vorbisData.data(), archiveRecord.dataSize);

        return true;
      }
      default: {
        // Never should get here!
        assert(false);
        return false;
      }
    }
  }

  return ImportNative(soundRecord, soundDataView, {}, options);
}

bool HitmanFile::Import(const std::span<const char> &in, const Options &options)
{
  if (options.common.directImport)
    return ImportNative(in, true, options);

  const auto soundRecord = SoundDataHeader(in);
  return Import(soundRecord, SoundDataDataView(soundRecord, in), options);
}

bool HitmanFile::Import(const StringView8CI &importPath, const Options& options)
{
  auto importData = ReadWholeBinaryFile(importPath);
  if (importData.empty())
    return false;

  return Import(importData, options);
}

bool HitmanFile::ImportNative(const SoundRecord &soundRecord, const std::span<const char> &soundDataView, const std::span<const int16_t> &pcms16DataView, const Options &options)
{
  if (!soundRecord.dataSize || soundDataView.size() < soundRecord.dataSize)
    return false;

  if (!options.common.importSameFiles && (archiveRecord.dataXXH3 == soundRecord.dataXXH3))
    return true;

  switch (soundRecord.formatTag)
  {
    case 0x01:
    case 0x11:
    case 0x1000: {
      archiveRecord = soundRecord;

      if (archiveRecord.formatTag != 0x11)
        archiveRecord.samplesPerBlock = 1;

      data.resize(archiveRecord.dataSize);
      std::memcpy(data.data(), soundDataView.data(), archiveRecord.dataSize);

      return true;
    }
    case 0x8000: {
      if (pcms16DataView.empty())
        return false;

      archiveRecord = PCMS16SoundRecord(PCMS16Header(soundRecord), soundRecord.dataXXH3);
      archiveRecord.samplesPerBlock = 1;

      const auto newDataSize = pcms16DataView.size() * sizeof(int16_t);
      data.resize(newDataSize);
      std::memcpy(data.data(), pcms16DataView.data(), newDataSize);

      return true;
    }
    default: {
      return false;
    }
  }
}

bool HitmanFile::ImportNative(SoundRecord soundRecord, const std::span<const char>& soundDataView, const bool allowConversions, const Options& options)
{
  if (!soundRecord.dataSize || soundDataView.size() < soundRecord.dataSize)
    return false;

  if (soundRecord.formatTag == 0x01)
  {
    soundRecord.dataXXH3 = XXH3_64bits(soundDataView.data(), soundRecord.dataSize);
    return ImportNative(soundRecord, soundDataView, std::span<const int16_t>{}, options);
  }

  std::vector<int16_t> pcms16Decoded;
  if (!PCMS16FromSoundData(soundRecord, soundDataView, pcms16Decoded, PCMS16_CONVERSION_FLAG_RAW_OUTPUT))
    return false;

  soundRecord.dataXXH3 = XXH3_64bits(pcms16Decoded.data(), pcms16Decoded.size() * sizeof(int16_t));
  std::span<const int16_t> pcms16Span;
  if (allowConversions)
    pcms16Span = pcms16Decoded;

  return ImportNative(soundRecord, soundDataView, pcms16Span, options);
}

bool HitmanFile::ImportNative(const std::span<const char>& in, const bool allowConversions, const Options& options)
{
  const auto soundRecord = SoundDataHeader(in);
  return ImportNative(soundRecord, SoundDataDataView(soundRecord, in), allowConversions, options);
}

bool HitmanFile::ImportNative(const StringView8CI &importPath, const bool allowConversions, const Options& options)
{
  auto importData = ReadWholeBinaryFile(importPath);
  if (importData.empty())
    return false;

  return ImportNative(importData, allowConversions, options);
}

bool HitmanFile::ExportNative(std::vector<char> &outputBytes, const Options&) const
{
  const auto outIndexInput = outputBytes.size();

  if (archiveRecord.formatTag == 0x01)
  {
    const auto header = PCMS16Header(archiveRecord);
    outputBytes.resize(outIndexInput + sizeof(PCMS16_Header));
    std::memcpy(outputBytes.data() + outIndexInput, &header, sizeof(PCMS16_Header));
  }
  else if (archiveRecord.formatTag == 0x11)
  {
    const auto header = ADPCMHeader(archiveRecord);
    outputBytes.resize(outIndexInput + sizeof(ADPCM_Header));
    std::memcpy(outputBytes.data() + outIndexInput, &header, sizeof(ADPCM_Header));
  }
  else if (archiveRecord.formatTag == 0x1000)
  {
    // NOTE: nothing to do, OGG files are completely contained in data
  }
  else
  {
    DisplayError(g_LocalizationManager.Localize("HITMAN_DIALOG_ERROR_UNKNOWN_FORMAT_IN_ARCHIVE"));
    return false;
  }

  const auto dataOffset = outputBytes.size();
  outputBytes.resize(dataOffset + data.size(), 0);
  std::memcpy(outputBytes.data() + dataOffset, data.data(), data.size());

  return true;
}

bool HitmanFile::Export(std::vector<char> &outputBytes, const Options& options) const
{
  if (!options.common.transcodeToPlayableFormat)
    return ExportNative(outputBytes, options);

  assert(archiveRecord.channels >= 1);
  assert(archiveRecord.channels <= 2);
  if (archiveRecord.channels < 1 || archiveRecord.channels > 2)
  {
    assert(false);
    return false;
  }

  assert(archiveRecord.sampleRate >= 8000);
  assert(archiveRecord.sampleRate <= 48000);
  if (archiveRecord.sampleRate < 8000 || archiveRecord.sampleRate > 48000)
  {
    assert(false);
    return false;
  }

  OrderedSet<uint32_t> normalSampleRates{0, 11025, 22050, 44100, 48000};
  auto normalSampleRateIt = std::ranges::lower_bound(normalSampleRates, archiveRecord.sampleRate);
  if (normalSampleRateIt == normalSampleRates.end())
  {
    assert(false);
    return false;
  }
  if (++normalSampleRateIt == normalSampleRates.end())
  {
    assert(false);
    return false;
  }

  const auto normalSampleRate = *normalSampleRateIt;
  if (archiveRecord.sampleRate == normalSampleRate&& archiveRecord.blockAlign == archiveRecord.channels * (archiveRecord.bitsPerSample / 8))
  {
    if (archiveRecord.formatTag == 0x01)
      return ExportNative(outputBytes, options);

    if (!options.common.transcodeOGGToPCM && archiveRecord.formatTag == 0x1000)
      return ExportNative(outputBytes, options);
  }

  auto pcms16Header = PCMS16Header(archiveRecord);
  std::vector<int16_t> pcms16Decoded;
  if (!PCMS16FromSoundData(archiveRecord, data, pcms16Decoded, PCMS16_CONVERSION_FLAG_RAW_OUTPUT))
  {
    assert(false);
    return false;
  }

  if (PCMS16ChangeSampleRate(pcms16Header, pcms16Decoded, normalSampleRate) < 0)
  {
    assert(false);
    return false;
  }

  const auto outIndexInput = outputBytes.size();
  outputBytes.resize(outIndexInput + sizeof(PCMS16_Header) + pcms16Decoded.size() * sizeof(int16_t));
  std::memcpy(outputBytes.data() + outIndexInput, &pcms16Header, sizeof(PCMS16_Header));
  std::memcpy(outputBytes.data() + outIndexInput + sizeof(PCMS16_Header), pcms16Decoded.data(), pcms16Decoded.size() * sizeof(int16_t));

  return true;
}

bool HitmanDialog::Clear(const bool retVal)
{
  opened = false;
  fileMap.clear();

  return ArchiveDialog::Clear(retVal);
}

bool HitmanDialog::IsSaveAllowed() const
{
  return true;
}

bool HitmanDialog::IsExportAllowed() const
{
  return true;
}

bool HitmanDialog::IsImportAllowed() const
{
  return true;
}

bool HitmanDialog::GenerateOriginalData(const Options &options)
{
  auto dataPathString = originalDataPathPrefix;
  dataPathString += std::format("{:16X}", originalDataID);
  const auto dataPath = dataPathString.path();
  create_directories(dataPath.parent_path());

  const auto oldSync = std::ios_base::sync_with_stdio(false);

  std::ofstream genFile(dataPath, std::ios::binary | std::ios::trunc);

  genFile.write(reinterpret_cast<const char *>(&originalDataParentID), sizeof(uint64_t));

  if (originalDataParentID)
  {
    std::ios_base::sync_with_stdio(oldSync);

    const auto thisDataID = originalDataID;
    originalDataID = originalDataParentID;
    const auto loadResult = LoadOriginalData(options);
    originalDataID = thisDataID;

    return loadResult;
  }

  const uint64_t entriesCount = fileMap.size();
  genFile.write(reinterpret_cast<const char *>(&entriesCount), sizeof(uint64_t));

  for (auto &[filePath, file] : fileMap)
  {
    assert(file.archiveRecord.dataXXH3 != 0);
    file.originalRecord = file.archiveRecord;

    auto &archiveFile = GetFile(filePath);
    archiveFile.original = true;

    genFile.write(reinterpret_cast<const char *>(&file.originalRecord), sizeof(HitmanSoundRecord));
  }

  std::ios_base::sync_with_stdio(oldSync);

  return true;
}

bool HitmanDialog::LoadOriginalData(const Options &options)
{
  auto dataPathString = originalDataPathPrefix;
  dataPathString += std::format("{:16X}", originalDataID);
  const auto dataPath = dataPathString.path();
  if (!exists(dataPath))
  {
    if (originalDataID != originalDataParentID)
      return GenerateOriginalData(options);

    DisplayError(g_LocalizationManager.Localize("HITMAN_DIALOG_ERROR_CORRUPTED_ORIGINAL_RECORDS_CACHE"));
    ReloadOriginalData(true, options);
    return true;
  }

  const auto oldSync = std::ios_base::sync_with_stdio(false);

  std::ifstream genFile(dataPath, std::ios::binary);

  uint64_t parentID = 0;
  genFile.read(reinterpret_cast<char *>(&parentID), sizeof(uint64_t));

  if (parentID != 0)
  {
    std::ios_base::sync_with_stdio(oldSync);

    originalDataParentID = parentID;

    const auto thisDataID = originalDataID;
    originalDataID = originalDataParentID;
    const auto loadResult = LoadOriginalData(options);
    originalDataID = thisDataID;

    return loadResult;
  }

  uint64_t entriesCount = 0;
  genFile.read(reinterpret_cast<char *>(&entriesCount), sizeof(uint64_t));

  if (entriesCount != fileMap.size())
    return false;

  for (auto &[filePath, file] : fileMap)
  {
    genFile.read(reinterpret_cast<char *>(&file.originalRecord), sizeof(HitmanSoundRecord));

    assert(file.originalRecord.dataXXH3 != 0);
    assert(file.archiveRecord.dataXXH3 != 0);

    auto &archiveFile = GetFile(filePath);
    archiveFile.original = file.originalRecord == file.archiveRecord;
  }

  std::ios_base::sync_with_stdio(oldSync);

  return true;
}

bool HitmanDialog::ImportSingleHitmanFile(HitmanFile &hitmanFile, const std::span<const char> &data, const bool allowConversions, const Options &options)
{
  if (allowConversions)
  {
    if (!hitmanFile.Import(data, options))
      return false;
  }
  else
  {
    if (!hitmanFile.ImportNative(data, false, options))
      return false;
  }

  auto &archiveFile = GetFile(hitmanFile.path);

  if (hitmanFile.originalRecord.dataXXH3 == 0)
    hitmanFile.originalRecord = hitmanFile.archiveRecord;

  archiveFile.original = hitmanFile.originalRecord == hitmanFile.archiveRecord;
  archiveFile.dirty = archiveFile.dirty || !archiveFile.original;

  return true;
}

bool HitmanDialog::ImportSingleHitmanFile(HitmanFile &hitmanFile, const StringView8CI &importFilePath, const Options &options)
{
  auto inputData = ReadWholeBinaryFile(importFilePath);
  return ImportSingleHitmanFile(hitmanFile, inputData, !options.common.directImport, options);
}

bool HitmanDialog::ExportSingleHitmanFile(const HitmanFile &hitmanFile, std::vector<char> &data, bool doConversion, const Options &options) const
{
  if (doConversion)
    return hitmanFile.Export(data, options);

  return hitmanFile.ExportNative(data, options);
}

bool HitmanDialog::ExportSingleHitmanFile(const HitmanFile &hitmanFile, const StringView8CI &exportFolderPath, const Options &options) const
{
  std::vector<char> outputData;
  if (!ExportSingleHitmanFile(hitmanFile, outputData, options.common.transcodeToPlayableFormat, options))
    return false;

  if (outputData.size() < 4)
    return false;

  auto exportPath = exportFolderPath.path() / hitmanFile.path.path();
  const auto magic = *reinterpret_cast<const uint32_t*>(outputData.data());
  switch (magic)
  {
    case 'FFIR': {
      exportPath.replace_extension(L".wav");
      break;
    }
    case 'SggO': {
      exportPath.replace_extension(L".ogg");
      break;
    }
    default: {
      return false;
    }
  }

  create_directories(exportPath.parent_path());

  const auto oldSync = std::ios_base::sync_with_stdio(false);

  std::ofstream exportBin(exportPath, std::ios::binary | std::ios::trunc);
  exportBin.write(outputData.data(), static_cast<int64_t>(outputData.size()));

  std::ios_base::sync_with_stdio(oldSync);

  return true;
}

bool HitmanDialog::ExportSingle(const StringView8CI &exportFolderPath, const StringView8CI &exportFilePath, const Options &options) const
{
  const auto fileMapIt = fileMap.find(exportFilePath);
  if (fileMapIt == fileMap.cend())
    return false;

  return ExportSingleHitmanFile(fileMapIt->second, exportFolderPath, options);
}

int32_t HitmanDialog::ReloadOriginalData(const bool reset, const Options &options)
{
  needsOriginalDataReload = true;
  needsOriginalDataReset |= reset;

  if (IsInProgress())
    return 2;

  if (originalDataID == 0)
  {
    needsOriginalDataReload = false;
    return needsOriginalDataReset ? 2 : 0;
  }

  if (needsOriginalDataReset)
  {
    auto dataPathString = originalDataPathPrefix;
    dataPathString += std::format("{:16X}", originalDataID);

    [[maybe_unused]] std::error_code errorCode;
    std::filesystem::remove(dataPathString.path(), errorCode);

    needsOriginalDataReset = false;
  }

  needsOriginalDataReload &= fileMap.empty();

  if (!needsOriginalDataReload)
    return 1;

  progressMessage = g_LocalizationManager.Localize("HITMAN_DIALOG_LOADING_ORIGINAL_RECORDS");
  progressNext = 0;
  progressNextTotal = 1;

  progressTask = std::async(std::launch::async, [this, options = Options::Get()] {
    LoadOriginalData(options);

    std::unique_lock progressMessageLock(progressMessageMutex);
    progressMessage.clear();
    progressNext = 1;
  });

  needsOriginalDataReload = false;

  return 1;
}

int32_t HitmanDialog::DrawHitmanDialog()
{
  if (needsOriginalDataReload || needsOriginalDataReset)
    ReloadOriginalData();

  return DrawBaseDialog();
}
