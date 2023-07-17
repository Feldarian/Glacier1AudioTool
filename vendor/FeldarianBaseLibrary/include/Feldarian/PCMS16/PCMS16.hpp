//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#pragma once

#include <cstdint>
#include <span>
#include <vector>

#define PCMS16_CONVERSION_FLAG_RAW_OUTPUT                  0x01
#define PCMS16_CONVERSION_FLAG_ADPCM_DISABLE_NOISE_SHAPING 0x02

struct PCMS16_Header
{
  char riffId[4] = {'R', 'I', 'F', 'F'};
  uint32_t riffSize = 0x24;
  char waveId[4] = {'W', 'A', 'V', 'E'};
  char fmtId[4] = {'f', 'm', 't', ' '};
  uint32_t fmtSize = 0x10;
  uint16_t fmtFormat = 0x01;
  uint16_t fmtChannels = 0;
  uint32_t fmtSampleRate = 0;
  uint32_t fmtAvgBytesRate = 0;
  uint16_t fmtBlockAlign = 0;
  uint16_t fmtBitsPerSample = 0;
  char dataId[4] = {'d', 'a', 't', 'a'};
  uint32_t dataSize = 0;
};

struct ADPCM_Header
{
  char riffId[4] = {'R', 'I', 'F', 'F'};
  uint32_t riffSize = 0x34;
  char waveId[4] = {'W', 'A', 'V', 'E'};
  char fmtId[4] = {'f', 'm', 't', ' '};
  uint32_t fmtSize = 0x14;
  uint16_t fmtFormat = 0x11;
  uint16_t fmtChannels = 0;
  uint32_t fmtSampleRate = 0;
  uint32_t fmtAvgBytesRate = 0;
  uint16_t fmtBlockAlign = 0;
  uint16_t fmtBitsPerSample = 0;
  uint16_t fmtExtraSize = 0x02;
  uint16_t fmtExtraSamplesPerBlock = 0x3F9;
  char factId[4] = {'f', 'a', 'c', 't'};
  uint32_t factSize = 0x04;
  uint32_t factSamplesCount = 0;
  char dataId[4] = {'d', 'a', 't', 'a'};
  uint32_t dataSize = 0;
};

struct SoundRecord
{
  uint64_t dataXXH3 = 0;
  uint32_t dataSizeUncompressed = 0;
  uint32_t dataSize = 0;
  uint32_t sampleRate = 0;
  uint16_t formatTag = 0;
  uint16_t bitsPerSample = 0;
  uint16_t channels = 0;
  uint16_t blockAlign = 0;
  uint16_t samplesPerBlock = 0;

  auto operator<=>(const SoundRecord & other) const = default;
};

PCMS16_Header PCMS16Header(const SoundRecord& record);
PCMS16_Header PCMS16Header(const std::span<const int16_t>& in);

SoundRecord PCMS16SoundRecord(const PCMS16_Header& header, uint64_t xxh3Hash = 0);
SoundRecord PCMS16SoundRecord(const PCMS16_Header& header, const std::span<const int16_t>& in);
SoundRecord PCMS16SoundRecord(const std::span<const int16_t>& in);

std::span<const int16_t> PCMS16DataView(const PCMS16_Header& header, const std::span<const int16_t>& in);
std::span<const int16_t> PCMS16DataView(const std::span<const int16_t>& in);

ADPCM_Header ADPCMHeader(const SoundRecord& record, int blocksizePow2 = 0);
ADPCM_Header ADPCMHeader(const std::span<const char>& in);

SoundRecord ADPCMSoundRecord(const ADPCM_Header& header, uint64_t xxh3Hash = 0);
SoundRecord ADPCMSoundRecord(const ADPCM_Header& header, const std::span<const char>& in, bool pcms16XXH3Hash = true);
SoundRecord ADPCMSoundRecord(const std::span<const char>& in);

std::span<const char> ADPCMDataView(const ADPCM_Header& header, const std::span<const char>& in);
std::span<const char> ADPCMDataView(const std::span<const char>& in);

SoundRecord VorbisHeader(const std::span<const char>& in);

SoundRecord VorbisSoundRecord(const SoundRecord& header, uint64_t xxh3Hash = 0);
SoundRecord VorbisSoundRecord(const SoundRecord& header, const std::span<const char>& in, bool pcms16XXH3Hash = true);
SoundRecord VorbisSoundRecord(const std::span<const char>& in);

std::span<const char> VorbisDataView(const SoundRecord& header, const std::span<const char>& in);
std::span<const char> VorbisDataView(const std::span<const char>& in);

SoundRecord UnknownSoundDataHeader(const std::span<const char>& in);

SoundRecord UnknownSoundDataSoundRecord(const SoundRecord& header, uint64_t xxh3Hash = 0);
SoundRecord UnknownSoundDataSoundRecord(const SoundRecord& header, const std::span<const char>& in, bool pcms16XXH3Hash = true);
SoundRecord UnknownSoundDataSoundRecord(const std::span<const char>& in);

std::span<const char> UnknownSoundDataDataView(const SoundRecord& header, const std::span<const char>& in);
std::span<const char> UnknownSoundDataDataView(const std::span<const char>& in);

SoundRecord SoundDataHeader(const std::span<const char>& in);

SoundRecord SoundDataSoundRecord(const SoundRecord& header, uint64_t xxh3Hash = 0);
SoundRecord SoundDataSoundRecord(const SoundRecord& header, const std::span<const char>& in, bool pcms16XXH3Hash = true);
SoundRecord SoundDataSoundRecord(const std::span<const char>& in);

std::span<const char> SoundDataDataView(const SoundRecord& header, const std::span<const char>& in);
std::span<const char> SoundDataDataView(const std::span<const char>& in);

int32_t PCMS16ChangeSampleRate(PCMS16_Header& header, std::vector<char>& data, uint32_t newSampleRate);
int32_t PCMS16ChangeSampleRate(PCMS16_Header& header, std::vector<int16_t>& data, uint32_t newSampleRate);

int32_t PCMS16ChangeChannelCount(PCMS16_Header& header, std::vector<char>& data, uint16_t newChannelCount);
int32_t PCMS16ChangeChannelCount(PCMS16_Header& header, std::vector<int16_t>& data, uint16_t newChannelCount);

bool PCMS16FromADPCM(const ADPCM_Header &header, const std::span<const char>& in, std::vector<int16_t> &out, int flags = 0);
bool PCMS16FromADPCM(const std::span<const char>& in, std::vector<int16_t> &out, int flags = 0);

bool PCMS16FromVorbis(const SoundRecord &header, const std::span<const char>& in, std::vector<int16_t> &out, int flags = 0);
bool PCMS16FromVorbis(const std::span<const char>& in, std::vector<int16_t> &out, int flags = 0);

bool PCMS16FromUnknownSoundData(const SoundRecord &header, const std::span<const char>& in, std::vector<int16_t> &out, int flags = 0);
bool PCMS16FromUnknownSoundData(const std::span<const char>& in, std::vector<int16_t> &out, int flags = 0);

bool PCMS16FromSoundData(const SoundRecord &header, const std::span<const char>& in, std::vector<int16_t> &out, int flags = 0);
bool PCMS16FromSoundData(const std::span<const char>& in, std::vector<int16_t> &out, int flags = 0);

bool PCMS16ToADPCM(const PCMS16_Header& header, const std::span<const int16_t>& in, std::vector<char> &out, int flags = 0, int blocksizePow2 = 0, int lookahead = 3);
bool PCMS16ToADPCM(const std::span<const int16_t>& in, std::vector<char> &out, int flags = 0, int blocksizePow2 = 0, int lookahead = 3);

bool PCMS16ToVorbis(const PCMS16_Header& header, const std::span<const int16_t>& in, std::vector<char> &out, int flags = 0);
bool PCMS16ToVorbis(const std::span<const int16_t>& in, std::vector<char> &out, int flags = 0);

template <typename OutputSpanType, typename InputSpanType>
requires std::is_pod_v<InputSpanType> && std::is_pod_v<OutputSpanType> && (std::is_const_v<OutputSpanType> || !std::is_const_v<InputSpanType>)
std::span<OutputSpanType> ToSpan(const std::span<InputSpanType>& in)
{
  const auto byteSize = (in.size() * sizeof(InputSpanType));
  if (byteSize % sizeof(OutputSpanType))
    return {};

  return {reinterpret_cast<OutputSpanType*>(in.data()), byteSize / sizeof(OutputSpanType)};
}
