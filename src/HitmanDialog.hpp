//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#pragma once

#include "ArchiveDialog.hpp"

struct SF_VIRTUAL_DATA_FIXED
{
  char* data = nullptr;
  sf_count_t dataSize = 0;
  sf_count_t offset = 0;
};

struct SF_VIRTUAL_DATA
{
  std::vector<char> &data;
  sf_count_t offset = 0;
};

inline SF_VIRTUAL_IO g_VirtSndFileIOFixed {
  .get_filelen = [](void *user_data) -> sf_count_t {
    const auto* virtData = static_cast<SF_VIRTUAL_DATA_FIXED *>(user_data);
    return virtData->dataSize;
  },
  .seek = [](sf_count_t offset, int whence, void *user_data) -> sf_count_t {
    auto* virtData = static_cast<SF_VIRTUAL_DATA_FIXED *>(user_data);

    switch (whence)
    {
      case SEEK_SET:
        virtData->offset = offset;
        break;

      case SEEK_CUR:
        virtData->offset += offset;
        break;

      case SEEK_END:
        virtData->offset = virtData->dataSize + offset;
        break;

      default:
        break;
    }

    assert(virtData->offset <= virtData->dataSize);
    return virtData->offset;
  },
  .read = [](void *ptr, sf_count_t count, void *user_data) -> sf_count_t {
    auto* virtData = static_cast<SF_VIRTUAL_DATA_FIXED *>(user_data);

    const auto bytesToRead = std::min(virtData->dataSize - virtData->offset, count);

    memcpy(ptr, virtData->data + virtData->offset, bytesToRead);
    virtData->offset += bytesToRead;

    return bytesToRead;
  },
  .write = [](const void *ptr, sf_count_t count, void *user_data) -> sf_count_t {
    auto* virtData = static_cast<SF_VIRTUAL_DATA_FIXED *>(user_data);

    const auto bytesToWrite = std::min(virtData->dataSize - virtData->offset, count);

    memcpy(virtData->data + virtData->offset, ptr, bytesToWrite);
    virtData->offset += bytesToWrite;

    return bytesToWrite;
  },
  .tell = [](void *user_data) -> sf_count_t {
    const auto* virtData = static_cast<SF_VIRTUAL_DATA_FIXED *>(user_data);
    return virtData->offset;
  },
};

inline SF_VIRTUAL_IO g_VirtSndFileIO {
  .get_filelen = [](void *user_data) -> sf_count_t {
    const auto* virtData = static_cast<SF_VIRTUAL_DATA *>(user_data);
    return static_cast<sf_count_t>(virtData->data.size());
  },
  .seek = [](sf_count_t offset, int whence, void *user_data) -> sf_count_t {
    auto* virtData = static_cast<SF_VIRTUAL_DATA *>(user_data);

    switch (whence)
    {
      case SEEK_SET:
        virtData->offset = offset;
        break;

      case SEEK_CUR:
        virtData->offset += offset;
        break;

      case SEEK_END:
        virtData->offset = virtData->data.size() + offset;
        break;

      default:
        break;
    }

    assert(static_cast<size_t>(virtData->offset) <= virtData->data.size());
    return virtData->offset;
  },
  .read = [](void *ptr, sf_count_t count, void *user_data) -> sf_count_t {
    auto* virtData = static_cast<SF_VIRTUAL_DATA *>(user_data);

    const auto bytesToRead = std::min(static_cast<sf_count_t>(virtData->data.size()) - virtData->offset, count);

    memcpy(ptr, virtData->data.data() + virtData->offset, bytesToRead);
    virtData->offset += bytesToRead;

    return bytesToRead;
  },
  .write = [](const void *ptr, sf_count_t count, void *user_data) -> sf_count_t {
    auto* virtData = static_cast<SF_VIRTUAL_DATA *>(user_data);

    if (static_cast<sf_count_t>(virtData->data.size()) - virtData->offset < count)
      virtData->data.resize(virtData->data.size() + count, 0);

    memcpy(virtData->data.data() + virtData->offset, ptr, count);
    virtData->offset += count;

    return count;
  },
  .tell = [](void *user_data) -> sf_count_t {
    const auto* virtData = static_cast<SF_VIRTUAL_DATA *>(user_data);
    return virtData->offset;
  },
};

struct HitmanSoundRecord
{
  uint64_t dataXXH3 = 0;
  uint32_t dataSizeUncompressed = 0;
  uint32_t dataSize = 0;
  uint32_t sampleRate = 0;
  uint16_t formatTag = 0;
  uint16_t bitsPerSample = 0;
  uint16_t channels = 0;
  uint16_t blockAlign = 0;
  uint16_t fmtExtra = 0;

  bool operator==(const HitmanSoundRecord& other) const;
};

struct HitmanFile
{
  bool Import(char inputBytes[], size_t inputBytesCount, const Options& options);
  bool Import(std::vector<char> &inputBytes, const Options& options);
  bool Import(const std::filesystem::path& importPath, const Options& options);

  bool ImportNative(char inputBytes[], size_t inputBytesCount, const Options& options, bool doHashing);
  bool ImportNative(std::vector<char> &inputBytes, const Options& options, bool doHashing);
  bool ImportNative(const std::filesystem::path& importPath, const Options& options, bool doHashing);

  bool Export(std::vector<char> &outputBytes) const;
  bool Export(std::filesystem::path exportPath, bool fixExtension) const;

  HitmanSoundRecord originalRecord;
  HitmanSoundRecord archiveRecord;
  std::vector<char> data;
};

class HitmanDialog : public ArchiveDialog
{
public:
  bool Clear(bool retVal = false) override;

  bool GenerateOriginalData(const std::filesystem::path& originalDataPath);

  bool LoadOriginalData(const std::filesystem::path& originalDataPath);

  bool ImportSingleHitmanFile(HitmanFile &hitmanFile, const std::filesystem::path &hitmanFilePath, std::vector<char> &data, bool doConversion);

  bool ImportSingleHitmanFile(HitmanFile &hitmanFile, const std::filesystem::path &hitmanFilePath, const std::filesystem::path &importFilePath);

  bool ExportSingle(const std::filesystem::path &exportFolderPath, const std::filesystem::path &exportFilePath) const override;

  void DrawHitmanDialog(const std::filesystem::path& originalDataPath, std::wstring_view dialogName, std::wstring_view filters, std::wstring_view defaultFilename);

  UTFViewToTypeMapCI<wchar_t, HitmanFile> fileMap;
  bool recheckOriginality = Options::Get().common.checkOriginality;
  Options options = Options::Get();
};
