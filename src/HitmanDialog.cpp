//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#include "Precompiled.hpp"

#include "HitmanDialog.hpp"

bool HitmanSoundRecord::operator==(const HitmanSoundRecord &other) const
{
  return dataXXH3 == other.dataXXH3
   && dataSizeUncompressed == other.dataSizeUncompressed
   && dataSize == other.dataSize
   && sampleRate == other.sampleRate
   && formatTag == other.formatTag
   && bitsPerSample == other.bitsPerSample
   && channels == other.channels
   && blockAlign == other.blockAlign
   && fmtExtra == other.fmtExtra;
}

bool HitmanFile::Import(char inputBytes[], size_t inputBytesCount, const Options& options)
{
  SF_VIRTUAL_DATA_FIXED virtSndData{inputBytes, static_cast<sf_count_t>(inputBytesCount)};
  SndfileHandle sndFile(g_VirtSndFileIOFixed, &virtSndData);
  if (!sndFile)
  {
    DisplayWarning(LocalizationManager.Localize("HITMAN_DIALOG_WARNING_IMPORT_LOADING_DATA"));
    return false;
  }

  auto frames = static_cast<uint32_t>(sndFile.frames());
  if (frames == 0)
  {
    DisplayWarning(LocalizationManager.Localize("HITMAN_DIALOG_WARNING_IMPORT_DECODING_DATA"));
    return false;
  }

  auto channels = static_cast<uint16_t>(sndFile.channels());
  uint32_t dataSizeUncompressed = frames * channels * sizeof int16_t;
  uint32_t sampleRate = sndFile.samplerate();

  thread_local static std::vector<char> sndFileData;
  sndFileData.resize(dataSizeUncompressed, 0);
  if (sndFile.readf(reinterpret_cast<int16_t *>(sndFileData.data()), frames) != frames)
  {
    DisplayWarning(LocalizationManager.Localize("HITMAN_DIALOG_WARNING_IMPORT_DECODING_DATA"));
    return false;
  }

  sf_close(sndFile.takeOwnership());

  if (options.common.checkOriginality)
    archiveRecord.dataXXH3 = XXH3_64bits(sndFileData.data(), dataSizeUncompressed);

  if (!options.common.importOriginalFiles && (originalRecord.dataXXH3 == archiveRecord.dataXXH3))
    return true;

  auto castedDataSize = sndFileData.size() / sizeof int16_t;
  auto* castedData = reinterpret_cast<int16_t *>(sndFileData.data());

  if (options.common.fixChannels)
  {
    if (originalRecord.channels < channels)
    {
      assert(channels == 2);
      assert(originalRecord.channels == 1);

      for (size_t i = 0; i < castedDataSize / 2; ++i)
        castedData[i] = static_cast<int16_t>((static_cast<int32_t>(castedData[i * 2]) +
                                                static_cast<int32_t>(castedData[i * 2 + 1])) / 2);

      channels = 1;
      dataSizeUncompressed /= 2;

      sndFileData.resize(dataSizeUncompressed, 0);
      castedDataSize = sndFileData.size() / sizeof int16_t;
      castedData = reinterpret_cast<int16_t *>(sndFileData.data());
    }
    else if (originalRecord.channels > channels)
    {
      assert(channels == 1);
      assert(originalRecord.channels == 2);

      channels = 2;
      dataSizeUncompressed *= 2;

      sndFileData.resize(dataSizeUncompressed, 0);
      castedDataSize = sndFileData.size() / sizeof int16_t;
      castedData = reinterpret_cast<int16_t *>(sndFileData.data());

      for (size_t i = castedDataSize / 2 - 1; i < castedDataSize / 2; --i)
      {
        castedData[i * 2] = castedData[i];
        castedData[i * 2 + 1] = castedData[i];
      }
    }
  }

  if (options.common.fixSampleRate)
  {
    if (originalRecord.sampleRate != sampleRate)
    {
      SRC_DATA convData;
      convData.src_ratio = static_cast<double>(originalRecord.sampleRate) / sampleRate;

      thread_local static std::vector<float> convertedInSndFileData;
      convertedInSndFileData.resize(castedDataSize, 0);
      src_short_to_float_array(castedData, convertedInSndFileData.data(),
                               static_cast<int32_t>(castedDataSize));
      convData.data_in = convertedInSndFileData.data();
      convData.input_frames = static_cast<int32_t>(castedDataSize / channels);

      // UINT16_MAX is just an extra buffer space in case calculation is wrong, but src_ratio * in_size should be
      // enough
      thread_local static std::vector<float> convertedOutSndFileData;
      convertedOutSndFileData.resize(static_cast<size_t>(convData.src_ratio * castedDataSize) + UINT16_MAX, 0);
      convData.data_out = convertedOutSndFileData.data();
      convData.output_frames = static_cast<int32_t>(convertedOutSndFileData.size() / channels);

      const auto retVal = src_simple(&convData, SRC_SINC_BEST_QUALITY, channels);
      if (retVal != 0 || convData.input_frames != convData.input_frames_used)
      {
        DisplayWarning(LocalizationManager.LocalizeFormat("HITMAN_DIALOG_WARNING_IMPORT_TRANCODING_DATA", src_strerror(retVal)));
        return false;
      }

      sndFileData.resize(convData.output_frames_gen * channels * sizeof int16_t, 0);
      castedDataSize = sndFileData.size() / sizeof int16_t;
      castedData = reinterpret_cast<int16_t *>(sndFileData.data());

      src_float_to_short_array(convertedOutSndFileData.data(), castedData,
                               static_cast<int32_t>(castedDataSize));
      sndFileData.shrink_to_fit();

      sampleRate = originalRecord.sampleRate;
      dataSizeUncompressed = static_cast<uint32_t>(sndFileData.size());
    }
  }

  thread_local static std::vector<char> nativeData;
  nativeData.clear();
  nativeData.reserve(dataSizeUncompressed);
  SF_VIRTUAL_DATA nativeFileData = {nativeData};
  SndfileHandle nativeFile;

  switch (originalRecord.formatTag)
  {
    case 17: {
      if (options.common.transcodeToOriginalFormat)
      {
        nativeFile = SndfileHandle(g_VirtSndFileIO, &nativeFileData, SFM_WRITE, SF_FORMAT_WAV | SF_FORMAT_IMA_ADPCM, channels, sampleRate);
        break;
      }
    }
    case 4096: {
      if (options.common.transcodeToOriginalFormat)
      {
        nativeFile = SndfileHandle(g_VirtSndFileIO, &nativeFileData, SFM_WRITE, SF_FORMAT_OGG | SF_FORMAT_VORBIS, channels, sampleRate);
        break;
      }
    }
    case 1: {
      nativeFile = SndfileHandle(g_VirtSndFileIO, &nativeFileData, SFM_WRITE, SF_FORMAT_WAV | SF_FORMAT_PCM_16, channels, sampleRate);
      break;
    }
    default: {
      DisplayError(LocalizationManager.Localize("HITMAN_DIALOG_ERROR_UNKNOWN_FORMAT_IN_ARCHIVE"));
      return false;
    }
  }

  if (!nativeFile)
  {
    DisplayWarning(LocalizationManager.Localize("HITMAN_DIALOG_WARNING_IMPORT_CREATING_NATIVE_DATA"));
    return false;
  }

  constexpr uint32_t maxFramesPerWrite = UINT16_MAX;
  size_t castedDataOffset = 0;
  while (frames != 0)
  {
    const uint32_t framesToWrite = std::min(maxFramesPerWrite, frames);
    if (nativeFile.writef(castedData + castedDataOffset, framesToWrite) != framesToWrite)
    {
      DisplayWarning(LocalizationManager.Localize("HITMAN_DIALOG_WARNING_IMPORT_ENCODING_NATIVE_DATA"));
      return false;
    }
    frames -= framesToWrite;
    castedDataOffset += framesToWrite * channels;
  }

  sf_close(nativeFile.takeOwnership());

  return ImportNative(nativeData, options, false);
}

bool HitmanFile::Import(std::vector<char> &inputBytes, const Options& options)
{
  return Import(inputBytes.data(), inputBytes.size(), options);
}

bool HitmanFile::Import(const std::filesystem::path& importPath, const Options& options)
{
  auto importData = ReadWholeBinaryFile(importPath);
  if (importData.empty())
    return false;

  return Import(importData, options);
}

bool HitmanFile::ImportNative(char inputBytes[], size_t inputBytesCount, const Options& options, bool doHashing)
{
  SF_VIRTUAL_DATA_FIXED virtSndData{inputBytes, static_cast<sf_count_t>(inputBytesCount)};
  SndfileHandle sndFile(g_VirtSndFileIOFixed, &virtSndData);
  if (!sndFile)
  {
    DisplayWarning(LocalizationManager.Localize("HITMAN_DIALOG_WARNING_IMPORT_LOADING_DATA"));
    return false;
  }

  const auto frames = static_cast<uint32_t>(sndFile.frames());
  if (frames == 0)
  {
    DisplayWarning(LocalizationManager.Localize("HITMAN_DIALOG_WARNING_IMPORT_DECODING_DATA"));
    return false;
  }

  const auto format = sndFile.format();
  const auto channels = static_cast<uint16_t>(sndFile.channels());
  const auto dataSizeUncompressed = frames * channels * 2;
  const auto sampleRate = sndFile.samplerate();

  thread_local static std::vector<int16_t> sndFileData;
  sndFileData.resize(dataSizeUncompressed / 2, 0);
  if (sndFile.readf(sndFileData.data(), frames) != frames)
  {
    DisplayWarning(LocalizationManager.Localize("HITMAN_DIALOG_WARNING_IMPORT_DECODING_DATA"));
    return false;
  }

  sf_close(sndFile.takeOwnership());

  if (doHashing)
  {
    if (options.common.checkOriginality)
      archiveRecord.dataXXH3 = XXH3_64bits(sndFileData.data(), dataSizeUncompressed);

    if (!options.common.importOriginalFiles && (originalRecord.dataXXH3 == archiveRecord.dataXXH3))
      return true;
  }

  uint16_t inputFormat = UINT16_MAX;
  if ((format & SF_FORMAT_WAV) == SF_FORMAT_WAV)
    inputFormat = (format & SF_FORMAT_IMA_ADPCM) == SF_FORMAT_IMA_ADPCM ? 17 : 1;
  else if ((format & SF_FORMAT_OGG) == SF_FORMAT_OGG)
    inputFormat = (format & SF_FORMAT_VORBIS) == SF_FORMAT_VORBIS ? 4096 : UINT16_MAX;

  switch (inputFormat)
  {
    case 1: {
      const auto* riffHeader = reinterpret_cast<RIFFHeaderPCM*>(inputBytes);

      archiveRecord.dataSizeUncompressed = riffHeader->dataSize;
      archiveRecord.dataSize = riffHeader->dataSize;
      archiveRecord.sampleRate = riffHeader->fmtSampleRate;
      archiveRecord.formatTag = riffHeader->fmtFormat;
      archiveRecord.bitsPerSample = riffHeader->fmtBitsPerSample;
      archiveRecord.channels = riffHeader->fmtChannels;
      archiveRecord.blockAlign = riffHeader->fmtBlockAlign;
      archiveRecord.fmtExtra = 1;

      data.resize(riffHeader->dataSize);
      memcpy(data.data(), inputBytes + sizeof RIFFHeaderPCM, riffHeader->dataSize);

      break;
    }
    case 17: {
      const auto* riffHeader = reinterpret_cast<RIFFHeaderADPCM*>(inputBytes);

      archiveRecord.dataSizeUncompressed = riffHeader->factSamplesCount * sizeof int16_t;
      archiveRecord.dataSize = riffHeader->dataSize;
      archiveRecord.sampleRate = riffHeader->fmtSampleRate;
      archiveRecord.formatTag = riffHeader->fmtFormat;
      archiveRecord.bitsPerSample = riffHeader->fmtBitsPerSample;
      archiveRecord.channels = riffHeader->fmtChannels;
      archiveRecord.blockAlign = riffHeader->fmtBlockAlign;
      archiveRecord.fmtExtra = riffHeader->fmtExtra;

      data.resize(riffHeader->dataSize);
      memcpy(data.data(), inputBytes + sizeof RIFFHeaderADPCM, riffHeader->dataSize);

      break;
    }
    case 4096: {
      archiveRecord.dataSizeUncompressed = dataSizeUncompressed;
      archiveRecord.dataSize = static_cast<uint32_t>(inputBytesCount);
      archiveRecord.sampleRate = sampleRate;
      archiveRecord.formatTag = 4096;
      archiveRecord.bitsPerSample = 16;
      archiveRecord.channels = channels;
      archiveRecord.blockAlign = channels * sizeof int16_t;
      archiveRecord.fmtExtra = 1;

      data.resize(inputBytesCount);
      memcpy(data.data(), inputBytes, inputBytesCount);

      break;
    }
    default: {
      DisplayError(LocalizationManager.Localize("HITMAN_DIALOG_ERROR_UNKNOWN_FORMAT_IN_ARCHIVE"));
      return false;
    }
  }

  data.shrink_to_fit();
  return true;
}

bool HitmanFile::ImportNative(std::vector<char> &inputBytes, const Options& options, bool doHashing)
{
  return ImportNative(inputBytes.data(), inputBytes.size(), options, doHashing);
}

bool HitmanFile::ImportNative(const std::filesystem::path& importPath, const Options& options, bool doHashing)
{
  auto importData = ReadWholeBinaryFile(importPath);
  if (importData.empty())
    return false;

  return ImportNative(importData, options, doHashing);
}

bool HitmanFile::Export(std::vector<char> &outputBytes) const
{
  outputBytes.clear();

  if (archiveRecord.formatTag == 1)
  {
    RIFFHeaderPCM riffHeader;
    riffHeader.riffSize += archiveRecord.dataSize;
    riffHeader.fmtFormat = archiveRecord.formatTag;
    riffHeader.fmtChannels = archiveRecord.channels;
    riffHeader.fmtSampleRate = archiveRecord.sampleRate;
    riffHeader.fmtAvgBytesRate = archiveRecord.sampleRate * archiveRecord.bitsPerSample * archiveRecord.channels / 8;
    riffHeader.fmtBlockAlign = archiveRecord.blockAlign;
    riffHeader.fmtBitsPerSample = archiveRecord.bitsPerSample;
    riffHeader.dataSize = archiveRecord.dataSize;

    outputBytes.resize(sizeof riffHeader);
    memcpy(outputBytes.data(), &riffHeader, sizeof riffHeader);
  }
  else if (archiveRecord.formatTag == 17)
  {
    RIFFHeaderADPCM riffHeader;
    riffHeader.riffSize += archiveRecord.dataSize;
    riffHeader.fmtFormat = archiveRecord.formatTag;
    riffHeader.fmtChannels = archiveRecord.channels;
    riffHeader.fmtSampleRate = archiveRecord.sampleRate;
    riffHeader.fmtAvgBytesRate = archiveRecord.sampleRate * archiveRecord.blockAlign /
                                 ((archiveRecord.blockAlign - archiveRecord.channels * 4) * (archiveRecord.channels ^ 3) + 1);
    riffHeader.fmtBlockAlign = archiveRecord.blockAlign;
    riffHeader.fmtBitsPerSample = archiveRecord.bitsPerSample;
    riffHeader.fmtExtra = archiveRecord.fmtExtra;
    riffHeader.factSamplesCount = archiveRecord.dataSizeUncompressed / sizeof int16_t;
    riffHeader.dataSize = archiveRecord.dataSize;

    outputBytes.resize(sizeof riffHeader, 0);
    memcpy(outputBytes.data(), &riffHeader, sizeof riffHeader);
  }
  else if (archiveRecord.formatTag == 4096)
  {
    // NOTE: nothing to do, OGG files are completely contained in data
  }
  else
  {
    DisplayError(LocalizationManager.Localize("HITMAN_DIALOG_ERROR_UNKNOWN_FORMAT_IN_ARCHIVE"));
    return false;
  }

  const auto dataOffset = outputBytes.size();
  outputBytes.resize(outputBytes.size() + data.size(), 0);
  memcpy(outputBytes.data() + dataOffset, data.data(), data.size());

  return true;
}

bool HitmanFile::Export(std::filesystem::path exportPath, bool fixExtension) const
{
  thread_local static std::vector<char> exportData;
  if (!Export(exportData))
    return false;

  if (fixExtension)
  {
    if (archiveRecord.formatTag == 1 || archiveRecord.formatTag == 17)
    {
      if (_wcsicmp(exportPath.extension().c_str(), L".wav") != 0)
        exportPath = ChangeExtension(exportPath, L".wav");
    }
    else if (archiveRecord.formatTag == 4096)
    {
      if (_wcsicmp(exportPath.extension().c_str(), L".ogg") != 0)
        exportPath = ChangeExtension(exportPath, L".ogg");
    }
  }

  create_directories(exportPath.parent_path());

  const auto oldSync = std::ios_base::sync_with_stdio(false);

  std::ofstream exportBin(exportPath, std::ios::binary | std::ios::trunc);
  exportBin.write(exportData.data(), static_cast<int64_t>(exportData.size()));

  std::ios_base::sync_with_stdio(oldSync);

  return true;
}

bool HitmanDialog::Clear(bool retVal)
{
  fileMap.clear();

  return ArchiveDialog::Clear(retVal);
}

bool HitmanDialog::GenerateOriginalData(const std::filesystem::path &originalDataPath)
{
  create_directories(originalDataPath.parent_path());

  const auto oldSync = std::ios_base::sync_with_stdio(false);

  std::ofstream genFile(originalDataPath, std::ios::binary | std::ios::trunc);
  uint64_t entriesCount = fileMap.size();
  genFile.write(reinterpret_cast<char *>(&entriesCount), sizeof entriesCount);

  thread_local static std::vector<char> exportBytes;
  for (auto &[filePath, file] : fileMap)
  {
    auto &archiveFile = archiveRoot.GetFile(filePath);
    ZeroMemory(&file.originalRecord, sizeof file.originalRecord);

    file.Export(exportBytes);

    SF_VIRTUAL_DATA_FIXED virtData{exportBytes.data(), static_cast<sf_count_t>(exportBytes.size())};
    SndfileHandle sndFile(g_VirtSndFileIOFixed, &virtData);
    if (!sndFile)
    {
      assert(false);
      return false;
    }

    const auto frames = static_cast<uint32_t>(sndFile.frames());
    const auto channels = static_cast<uint16_t>(sndFile.channels());
    const auto dataUncompressedSize = frames * channels * sizeof int16_t;

    std::vector<int16_t> sndFileData(dataUncompressedSize / sizeof int16_t, 0);
    if (sndFile.readf(sndFileData.data(), frames) != frames)
    {
      assert(false);
      return false;
    }

    sf_close(sndFile.takeOwnership());

    file.archiveRecord.dataXXH3 = XXH3_64bits(sndFileData.data(), dataUncompressedSize);
    file.originalRecord = file.archiveRecord;

    archiveFile.original = true;

    genFile.write(reinterpret_cast<char *>(&file.originalRecord), sizeof file.originalRecord);
  }

  std::ios_base::sync_with_stdio(oldSync);

  return true;
}

bool HitmanDialog::LoadOriginalData(const std::filesystem::path& originalDataPath)
{
  if (!exists(originalDataPath))
    return GenerateOriginalData(originalDataPath);

  const auto oldSync = std::ios_base::sync_with_stdio(false);

  std::ifstream genFile(originalDataPath, std::ios::binary);
  uint64_t entriesCount = 0;
  genFile.read(reinterpret_cast<char *>(&entriesCount), sizeof entriesCount);

  thread_local static std::vector<char> exportBytes;
  for (auto &[filePath, file] : fileMap)
  {
    auto &archiveFile = archiveRoot.GetFile(filePath);
    auto &originalRecord = file.originalRecord;
    genFile.read(reinterpret_cast<char *>(&originalRecord), sizeof originalRecord);

    file.archiveRecord.dataXXH3 = file.originalRecord.dataXXH3;
    archiveFile.original = file.originalRecord == file.archiveRecord;

    // skip hash check if format is of type we cannot import
    if (archiveFile.original == false || (file.archiveRecord.formatTag != 1 && file.archiveRecord.formatTag != 17))
      continue;

    file.Export(exportBytes);

    SF_VIRTUAL_DATA_FIXED virtData{exportBytes.data(), static_cast<sf_count_t>(exportBytes.size())};
    SndfileHandle sndFile(g_VirtSndFileIOFixed, &virtData);
    if (!sndFile)
    {
      assert(false);
      return false;
    }

    const auto frames = static_cast<uint32_t>(sndFile.frames());
    const auto channels = static_cast<uint16_t>(sndFile.channels());
    const auto dataUncompressedSize = frames * channels * sizeof int16_t;

    std::vector<int16_t> sndFileData(dataUncompressedSize / sizeof int16_t, 0);
    if (sndFile.readf(sndFileData.data(), frames) != frames)
    {
      assert(false);
      return false;
    }

    sf_close(sndFile.takeOwnership());

    file.archiveRecord.dataXXH3 = XXH3_64bits(sndFileData.data(), dataUncompressedSize);
    archiveFile.original &= originalRecord.dataXXH3 == file.archiveRecord.dataXXH3;
  }

  std::ios_base::sync_with_stdio(oldSync);

  return true;
}

bool HitmanDialog::ImportSingleHitmanFile(HitmanFile &hitmanFile, const std::filesystem::path &hitmanFilePath, std::vector<char> &data, bool doConversion)
{
  if (doConversion)
    hitmanFile.Import(data, options);
  else
    hitmanFile.ImportNative(data, options, true);

  auto &archiveFile = archiveRoot.GetFile(hitmanFilePath);

  if (options.common.checkOriginality && hitmanFile.originalRecord.dataXXH3 != 0)
    archiveFile.original = hitmanFile.originalRecord == hitmanFile.archiveRecord;

  archiveFile.dirty = archiveFile.dirty || !archiveFile.original;

  return true;
}

bool HitmanDialog::ImportSingleHitmanFile(HitmanFile &hitmanFile, const std::filesystem::path &hitmanFilePath, const std::filesystem::path &importFilePath)
{
  auto inputData = ReadWholeBinaryFile(importFilePath);
  return ImportSingleHitmanFile(hitmanFile, hitmanFilePath, inputData, !Options::Get().common.directImport);
}

bool HitmanDialog::ExportSingle(const std::filesystem::path &exportFolderPath,
    const std::filesystem::path &exportFilePath) const
{
  const auto fileMapIt = fileMap.find(exportFilePath.native());
  if (fileMapIt == fileMap.cend())
    return false;

  return fileMapIt->second.Export(exportFolderPath / exportFilePath, true);
}

void HitmanDialog::DrawHitmanDialog(const std::filesystem::path &originalDataPath, std::wstring_view dialogName,
    std::wstring_view filters, std::wstring_view defaultFilename)
{
  if (!progressNextActive.load())
  {
    if (!recheckOriginality && options.common.checkOriginality)
    {
      if (!originalDataPath.empty() && !fileMap.empty())
      {
        progressMessage = LocalizationManager.Localize("HITMAN_DIALOG_LOADING_ORIGINAL_RECORDS");
        progressNext = 0;
        progressNextTotal = 1;
        progressNextActive = true;

        std::thread([originalDataPath, this] {
          LoadOriginalData(originalDataPath);

          std::unique_lock progressMessageLock(progressMessageMutex);
          progressMessage.clear();
          progressNext = 1;
          progressNextActive = false;
        }).detach();
      }
    }

    recheckOriginality = options.common.checkOriginality;
    options = Options::Get();
  }

  DrawBaseDialog(dialogName, filters, defaultFilename);
}
