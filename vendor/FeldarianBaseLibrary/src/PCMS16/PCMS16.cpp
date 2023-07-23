//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//
// TODO - there are checks whether there is some data in header (dataSize != 0), but there are missing checks whether requested format matches! wrong header and data may be supplied!
// TODO - support more different WAV data directly
//

#include <Feldarian/PCMS16/PCMS16.hpp>

extern "C"
{

#if FBL_PCMS16_USE_ADPCMXQ
#include <adpcm-lib.h>
#endif

#include <samplerate.h>
#include <sndfile.h>
#include <xxhash.h>

}

#include <sndfile.hh>

#include <algorithm>
#include <cassert>
#include <memory>

namespace
{

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

SF_VIRTUAL_IO g_VirtSndFileIOFixed {
  .get_filelen = [](void *user_data) -> sf_count_t {
    const auto* virtData = static_cast<SF_VIRTUAL_DATA_FIXED *>(user_data);
    return virtData->dataSize;
  },
  .seek = [](const sf_count_t offset, const int whence, void *user_data) -> sf_count_t {
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
  .read = [](void *ptr, const sf_count_t count, void *user_data) -> sf_count_t {
    auto* virtData = static_cast<SF_VIRTUAL_DATA_FIXED *>(user_data);

    const auto bytesToRead = std::min(virtData->dataSize - virtData->offset, count);

    memcpy(ptr, virtData->data + virtData->offset, bytesToRead);
    virtData->offset += bytesToRead;

    return bytesToRead;
  },
  .write = [](const void *ptr, const sf_count_t count, void *user_data) -> sf_count_t {
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

SF_VIRTUAL_IO g_VirtSndFileIO {
  .get_filelen = [](void *user_data) -> sf_count_t {
    const auto* virtData = static_cast<SF_VIRTUAL_DATA *>(user_data);
    return static_cast<sf_count_t>(virtData->data.size());
  },
  .seek = [](const sf_count_t offset, const int whence, void *user_data) -> sf_count_t {
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
  .read = [](void *ptr, const sf_count_t count, void *user_data) -> sf_count_t {
    auto* virtData = static_cast<SF_VIRTUAL_DATA *>(user_data);

    const auto bytesToRead = std::min(static_cast<sf_count_t>(virtData->data.size()) - virtData->offset, count);

    memcpy(ptr, virtData->data.data() + virtData->offset, bytesToRead);
    virtData->offset += bytesToRead;

    return bytesToRead;
  },
  .write = [](const void *ptr, const sf_count_t count, void *user_data) -> sf_count_t {
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

void PCMS16WriteHeader(std::vector<int16_t> &out, int num_channels, uint32_t num_samples, uint32_t sample_rate)
{
  const auto total_data_bytes = static_cast<uint32_t>(num_samples * num_channels * sizeof(int16_t));

  PCMS16_Header riffHeader;
  riffHeader.riffSize += total_data_bytes;
  riffHeader.fmtChannels = static_cast<uint16_t>(num_channels);
  riffHeader.fmtSampleRate = sample_rate;
  riffHeader.fmtBlockAlign = static_cast<uint16_t>(num_channels * sizeof(int16_t));
  riffHeader.fmtAvgBytesRate = sample_rate * riffHeader.fmtBlockAlign;
  riffHeader.fmtBitsPerSample = 16;
  riffHeader.dataSize = total_data_bytes;

  const auto outOffsetInput = out.size();
  out.resize(outOffsetInput + sizeof(PCMS16_Header) / sizeof(int16_t));
  std::memcpy(out.data() + outOffsetInput, &riffHeader, sizeof(PCMS16_Header));
}

#if FBL_PCMS16_USE_ADPCMXQ
void ADPCMWriteHeader(std::vector<char> &out, uint16_t num_channels, uint32_t num_samples, uint32_t sample_rate, uint32_t samples_per_block)
{
  const auto block_size = (samples_per_block - 1) / (num_channels ^ 3) + (num_channels * 4);
  const auto num_blocks = num_samples / samples_per_block;
  const auto leftover_samples = num_samples % samples_per_block;
  const auto total_data_bytes = (num_blocks + (leftover_samples != 0)) * block_size;

  ADPCM_Header riffHeader;
  riffHeader.riffSize += total_data_bytes;
  riffHeader.fmtChannels = num_channels;
  riffHeader.fmtSampleRate = sample_rate;
  riffHeader.fmtAvgBytesRate = sample_rate * block_size / samples_per_block;
  riffHeader.fmtBlockAlign = block_size;
  riffHeader.fmtBitsPerSample = 4;
  riffHeader.fmtExtraSamplesPerBlock = samples_per_block;
  riffHeader.dataSize = total_data_bytes;

  const auto outOffsetInput = out.size();
  out.resize(outOffsetInput + sizeof(ADPCM_Header));
  std::memcpy(out.data() + outOffsetInput, &riffHeader, sizeof(ADPCM_Header));
}

bool ADPCMDecodeData(const std::span<const char>& in, std::vector<int16_t> &out, uint16_t num_channels, uint32_t num_samples, uint16_t block_size)
{
  const auto samples_per_block = static_cast<uint16_t>((block_size - num_channels * 4) * (num_channels ^ 3) + 1);
  const auto total_data_shorts = num_samples * num_channels;

  thread_local static std::vector<int16_t> pcmBlock;
  pcmBlock.resize(samples_per_block * num_channels);

  thread_local static std::vector<uint8_t> adpcmBlock;
  adpcmBlock.resize(block_size);

  const auto outOffsetInput = out.size();
  out.resize(outOffsetInput + total_data_shorts);

  size_t inOffset = 0;
  auto outOffset = outOffsetInput;
  while (num_samples)
  {
    auto this_block_adpcm_samples = samples_per_block;
    auto this_block_pcm_samples = samples_per_block;

    if (this_block_adpcm_samples > num_samples)
    {
      this_block_adpcm_samples = ((num_samples + 6) & ~7) + 1;
      block_size = (this_block_adpcm_samples - 1) / (num_channels ^ 3) + (num_channels * 4);
      this_block_pcm_samples = num_samples;
    }

    if (inOffset + block_size > in.size())
    {
      out.resize(outOffsetInput);
      return false;
    }

    std::ranges::fill(adpcmBlock, 0);
    std::memcpy(adpcmBlock.data(), in.data() + inOffset, block_size);

    if (outOffset + this_block_pcm_samples * num_channels > out.size())
    {
      out.resize(outOffsetInput);
      return false;
    }

    std::ranges::fill(pcmBlock, 0);

    if (adpcm_decode_block(pcmBlock.data(), adpcmBlock.data(), block_size, num_channels) != this_block_adpcm_samples)
    {
      out.resize(outOffsetInput);
      return false;
    }

    std::memcpy(out.data() + outOffset, pcmBlock.data(), this_block_pcm_samples * num_channels * sizeof(int16_t));

    inOffset += block_size;
    outOffset += samples_per_block * num_channels;

    num_samples -= this_block_pcm_samples;
  }

  return true;
}

bool ADPCMEncodeData(const std::span<const int16_t>& in, std::vector<char> &out, uint16_t num_channels, uint32_t num_samples, uint16_t samples_per_block, int lookahead, int noise_shaping)
{
  auto block_size = (samples_per_block - 1) / (num_channels ^ 3) + (num_channels * 4);
  const auto num_blocks = num_samples / samples_per_block;
  const auto  leftover_samples = num_samples % samples_per_block;
  auto total_data_bytes = (num_blocks + (leftover_samples != 0)) * block_size;

  const auto outOffsetInput = out.size();
  out.resize(outOffsetInput + total_data_bytes);

  thread_local static std::vector<int16_t> pcmBlock;
  pcmBlock.resize(samples_per_block * num_channels);

  thread_local static std::vector<uint8_t> adpcmBlock;
  adpcmBlock.resize(block_size);

  void *adpcm_cnxt = nullptr;

  size_t inOffset = 0;
  auto outOffset = outOffsetInput;
  while (num_samples) {
    auto this_block_adpcm_samples = samples_per_block;
    auto this_block_pcm_samples = samples_per_block;

    if (this_block_pcm_samples > num_samples)
    {
      this_block_adpcm_samples = ((num_samples + 6) & ~7) + 1;
      block_size = (this_block_adpcm_samples - 1) / (num_channels ^ 3) + (num_channels * 4);
      this_block_pcm_samples = num_samples;
    }

    if (inOffset + this_block_pcm_samples * num_channels > in.size())
    {
      out.resize(outOffsetInput);
      return false;
    }

    std::ranges::fill(pcmBlock, 0);
    std::memcpy(pcmBlock.data(), in.data() + inOffset, this_block_pcm_samples * num_channels * sizeof(int16_t));

    if (this_block_adpcm_samples > this_block_pcm_samples)
    {
      int16_t *dst = pcmBlock.data() + this_block_pcm_samples * num_channels, *src = dst - num_channels;
      int dups = (this_block_adpcm_samples - this_block_pcm_samples) * num_channels;

      while (dups--)
        *dst++ = *src++;
    }

    // if this is the first block, compute a decaying average (in reverse) so that we can let the
    // encoder know what kind of initial deltas to expect (helps initializing index)

    if (!adpcm_cnxt) {
      int32_t average_deltas [2];

      average_deltas [0] = average_deltas [1] = 0;

      for (int i = this_block_adpcm_samples * num_channels; i -= num_channels;) {
        average_deltas [0] -= average_deltas [0] >> 3;
        average_deltas [0] += abs (static_cast<int32_t>(pcmBlock[i]) - pcmBlock[i - num_channels]);

        if (num_channels == 2) {
          average_deltas [1] -= average_deltas [1] >> 3;
          average_deltas [1] += abs (static_cast<int32_t>(pcmBlock[i - 1]) - pcmBlock[i+1]);
        }
      }

      average_deltas [0] >>= 3;
      average_deltas [1] >>= 3;

      adpcm_cnxt = adpcm_create_context (num_channels, lookahead, noise_shaping, average_deltas);
    }

    if (outOffset + block_size > out.size())
    {
      out.resize(outOffsetInput);
      return false;
    }

    std::ranges::fill(adpcmBlock, 0);

    size_t num_bytes;
    adpcm_encode_block(adpcm_cnxt, adpcmBlock.data(), &num_bytes, pcmBlock.data(), this_block_adpcm_samples);

    if (num_bytes != block_size)
    {
      out.resize(outOffsetInput);
      return false;
    }

    std::memcpy(out.data() + outOffset, adpcmBlock.data(), block_size);

    inOffset += samples_per_block * num_channels;
    outOffset += block_size;

    num_samples -= this_block_pcm_samples;
  }

  if (adpcm_cnxt)
    adpcm_free_context (adpcm_cnxt);

  return true;
}

bool PCMS16FromADPCM_ADPCMXQ(const ADPCM_Header &header, const std::span<const char> &in, std::vector<int16_t> &out, int flags)
{
  if (!header.dataSize || in.size() < header.dataSize)
    return false;

  const uint32_t complete_blocks = header.dataSize / header.fmtBlockAlign;
  const int leftover_bytes = header.dataSize % header.fmtBlockAlign;
  int samples_last_block;

  auto num_samples = complete_blocks * header.fmtExtraSamplesPerBlock;

  if (leftover_bytes) {
    if (leftover_bytes % (header.fmtChannels * 4))
      return false;

    samples_last_block = (leftover_bytes - (header.fmtChannels * 4)) * (header.fmtChannels ^ 3) + 1;
    num_samples += samples_last_block;
  }
  else
    samples_last_block = header.fmtExtraSamplesPerBlock;

  if (const auto fact_samples = header.factSamplesCount)
    num_samples = fact_samples;

  const auto outIndexInput = out.size();

  if (!(flags & PCMS16_CONVERSION_FLAG_RAW_OUTPUT))
    PCMS16WriteHeader(out, header.fmtChannels, num_samples, header.fmtSampleRate);

  const auto result = ADPCMDecodeData(in, out, header.fmtChannels, num_samples, header.fmtBlockAlign);

  if (!result)
    out.resize(outIndexInput);

  return result;
}

bool PCMS16ToADPCM_ADPCMXQ(const PCMS16_Header &header, const std::span<const int16_t> &in, std::vector<char> &out, int flags, int blocksizePow2, int lookahead)
{
  if (!header.dataSize || in.size() < header.dataSize)
    return false;

  const auto num_samples = header.dataSize / header.fmtBlockAlign;

  uint16_t block_size;

  if (blocksizePow2 > 0 && blocksizePow2 < 16)
    block_size = static_cast<uint16_t>(1 << blocksizePow2);
  else
    block_size = static_cast<uint16_t>(0x100 * header.fmtChannels * (header.fmtSampleRate < 11000 ? 1 : header.fmtSampleRate / 11000));

  const auto samples_per_block = (block_size - header.fmtChannels * 4) * (header.fmtChannels ^ 3) + 1;

  const auto outIndexInput = out.size();

  if (!(flags & PCMS16_CONVERSION_FLAG_RAW_OUTPUT))
    ADPCMWriteHeader(out, header.fmtChannels, num_samples, header.fmtSampleRate, samples_per_block);

  const auto result = ADPCMEncodeData(in, out, header.fmtChannels, num_samples, samples_per_block, lookahead,
      !(flags & PCMS16_CONVERSION_FLAG_ADPCM_DISABLE_NOISE_SHAPING) ? (header.fmtSampleRate > 64000 ? NOISE_SHAPING_STATIC : NOISE_SHAPING_DYNAMIC) : NOISE_SHAPING_OFF);

  if (!result)
    out.resize(outIndexInput);

  return result;
}
#endif

bool PCMS16FromADPCM_LibSNDFile(const ADPCM_Header &header, const std::span<const char> &in, std::vector<int16_t> &out, int flags)
{
  if (!header.dataSize || in.size() < header.dataSize)
    return false;

  std::vector<char> headerAndIn(sizeof(header) + header.dataSize, '\0');
  std::memcpy(headerAndIn.data(), &header, sizeof(header));
  std::memcpy(headerAndIn.data() + sizeof(header), in.data(), in.size());

  SF_VIRTUAL_DATA_FIXED virtSndData{const_cast<char*>(headerAndIn.data()), static_cast<sf_count_t>(headerAndIn.size())};
  SndfileHandle sndFile(g_VirtSndFileIOFixed, &virtSndData);
  if (sndFile.error() != SF_ERR_NO_ERROR)
  {
    const auto* err = sndFile.strError();
    return false;
  }

  auto frames = static_cast<uint32_t>(sndFile.frames());
  if (frames < header.factSamplesCount / header.fmtChannels)
    return false;

  frames = header.factSamplesCount / header.fmtChannels;

  // format check already performed, we skip it here

  const auto outIndexInput = out.size();
  if (!(flags & PCMS16_CONVERSION_FLAG_RAW_OUTPUT))
    PCMS16WriteHeader(out, header.fmtChannels, header.factSamplesCount, header.fmtSampleRate);

  //const auto outIndex = out.size();
  out.resize(outIndexInput + header.factSamplesCount);

  size_t outOffset = outIndexInput;
  while (frames != 0)
  {
    static constexpr uint32_t maxFramesPerRead = UINT16_MAX;
    const uint32_t framesToRead = std::min(maxFramesPerRead, frames);
    if (sndFile.readf(out.data() + outOffset, framesToRead) != framesToRead)
    {
      sf_close(sndFile.takeOwnership());

      out.resize(outIndexInput);
      return false;
    }

    frames -= framesToRead;
    outOffset += framesToRead * header.fmtChannels;
  }

  return true;
}

bool PCMS16ToADPCM_LibSNDFile(const PCMS16_Header &header, const std::span<const int16_t> &in, std::vector<char> &out, int flags)
{
  if (!header.dataSize || in.size() < header.dataSize)
    return false;

  const auto outIndexInput = out.size();

  SF_VIRTUAL_DATA virtSndData{out};
  SndfileHandle sndFile(g_VirtSndFileIO, &virtSndData, SFM_WRITE, SF_FORMAT_WAV | SF_FORMAT_IMA_ADPCM, header.fmtChannels, header.fmtSampleRate);
  if (sndFile.error() != SF_ERR_NO_ERROR)
  {
    const auto* err = sndFile.strError();
    out.resize(outIndexInput);
    return false;
  }

  size_t inOffset = 0;
  auto frames = header.dataSize / header.fmtBlockAlign;
  while (frames != 0)
  {
    static constexpr uint32_t maxFramesPerWrite = UINT16_MAX;
    const uint32_t framesToWrite = std::min(maxFramesPerWrite, frames);
    if (sndFile.writef(in.data() + inOffset, framesToWrite) != framesToWrite)
    {
      sf_close(sndFile.takeOwnership());

      out.resize(outIndexInput);
      return false;
    }

    frames -= framesToWrite;
    inOffset += framesToWrite * header.fmtChannels;
  }

  if (flags & PCMS16_CONVERSION_FLAG_RAW_OUTPUT)
  {
    const auto adpcmHeader = ADPCMHeader(out);
    std::memmove(out.data(), ADPCMDataView(adpcmHeader, out).data(), adpcmHeader.dataSize);
    out.resize(adpcmHeader.dataSize);
  }

  return true;
}

std::vector<int16_t> PCMS16ChangeSampleRate(PCMS16_Header &header, const std::span<int16_t> &data, uint32_t newSampleRate)
{
  if (header.fmtSampleRate == newSampleRate)
  {
    assert(false);
    return {};
  }

  SRC_DATA convData;
  convData.src_ratio = static_cast<double>(newSampleRate) / header.fmtSampleRate;

  const auto samplesPerBlock = header.fmtBlockAlign / sizeof(int16_t);
  const auto numberOfBlocks = header.dataSize / header.fmtBlockAlign;
  const auto numberOfRealSamples = numberOfBlocks * header.fmtChannels;

  const auto newNumberOfBlocks = static_cast<size_t>(convData.src_ratio * numberOfBlocks);
  const auto newDataSize = newNumberOfBlocks * header.fmtBlockAlign;
  const auto newNumberOfRealSamples = newNumberOfBlocks * header.fmtChannels;

  std::vector<float> convDataCache(numberOfRealSamples + newNumberOfRealSamples);

  if (header.fmtBlockAlign == header.fmtChannels * sizeof(int16_t))
    src_short_to_float_array(data.data(), convDataCache.data(), static_cast<int32_t>(numberOfRealSamples));
  else
  {
    size_t convDataOffset = 0;
    for (size_t i = 0; i < numberOfBlocks; ++i)
    {
      src_short_to_float_array(data.data() + i * header.fmtBlockAlign / sizeof(int16_t), convDataCache.data() + convDataOffset, static_cast<int32_t>(samplesPerBlock));
      convDataOffset += samplesPerBlock;
    }
  }

  convData.data_in = convDataCache.data();
  convData.input_frames = static_cast<long>(numberOfBlocks);
  convData.data_out = convDataCache.data() + numberOfRealSamples;
  convData.output_frames = static_cast<long>(newNumberOfBlocks);

  const auto retVal = src_simple(&convData, SRC_SINC_BEST_QUALITY, header.fmtChannels);
  if (retVal != 0 || convData.input_frames != convData.input_frames_used || convData.output_frames != convData.output_frames_gen)
  {
    assert(false);
    return {};
  }

  header.fmtSampleRate = newSampleRate;
  header.fmtAvgBytesRate = newSampleRate * header.fmtBlockAlign;
  header.dataSize = static_cast<uint32_t>(newDataSize);
  header.riffSize = header.dataSize + 0x24;

  std::vector<int16_t> newSamples(newDataSize / sizeof(int16_t), 0);

  if (header.fmtBlockAlign == header.fmtChannels * sizeof(int16_t))
    src_float_to_short_array(convData.data_out, newSamples.data(), static_cast<int32_t>(newNumberOfRealSamples));
  else
  {
    size_t convDataOffset = 0;
    for (size_t i = 0; i < numberOfBlocks; ++i)
    {
      src_float_to_short_array(convData.data_out + convDataOffset, newSamples.data() + i * header.fmtBlockAlign / sizeof(int16_t), static_cast<int32_t>(newNumberOfRealSamples));
      convDataOffset += samplesPerBlock;
    }
  }

  return newSamples;
}

std::vector<int16_t> PCMS16ChangeChannelCount(PCMS16_Header &header, const std::span<int16_t> &data, uint16_t newChannelCount)
{
  if (header.fmtChannels == newChannelCount)
  {
    assert(false);
    return {};
  }

  const auto samplesPerBlock = header.fmtBlockAlign / sizeof(int16_t);
  const auto numberOfBlocks = header.dataSize / header.fmtBlockAlign;

  const auto newBlockAlign = header.fmtBlockAlign != header.fmtChannels * sizeof(int16_t) ? header.fmtBlockAlign : static_cast<uint16_t>(newChannelCount * sizeof(int16_t));
  const auto newSamplesPerBlock = newBlockAlign / sizeof(int16_t);
  const auto newDataSize = numberOfBlocks * newBlockAlign;

  assert(data.size() * sizeof(int16_t) == header.dataSize);

  std::vector<int16_t> newSamples(numberOfBlocks * newSamplesPerBlock, 0);

  if (header.fmtChannels < newChannelCount)
  {
    assert(header.fmtChannels == 1);
    assert(newChannelCount == 2);

    for (size_t i = 0; i < numberOfBlocks; ++i)
    {
      const auto blockBeginIndex = i * header.fmtBlockAlign / sizeof(int16_t);
      const auto newBlockBeginIndex = i * newBlockAlign / sizeof(int16_t);

      newSamples[newBlockBeginIndex] = data[blockBeginIndex];
      newSamples[newBlockBeginIndex + 1] = data[blockBeginIndex];
    }

    header.fmtChannels *= 2;
  }
  else // if (header.fmtChannels > newChannelCount)
  {
    assert(header.fmtChannels == 2);
    assert(newChannelCount == 1);

    for (size_t i = 0; i < numberOfBlocks; ++i)
    {
      const auto blockBeginIndex = i * header.fmtBlockAlign / sizeof(int16_t);
      const auto newBlockBeginIndex = i * newBlockAlign / sizeof(int16_t);

      newSamples[newBlockBeginIndex] = static_cast<int16_t>((static_cast<int32_t>(data[blockBeginIndex]) +
                                              static_cast<int32_t>(data[blockBeginIndex + 1])) / 2) ;
    }

    header.fmtChannels /= 2;
  }

  header.fmtAvgBytesRate = header.fmtSampleRate * newBlockAlign;
  header.fmtBlockAlign = newBlockAlign;
  header.dataSize = newDataSize;
  header.riffSize = newDataSize + 0x24;

  return newSamples;
}

}

PCMS16_Header PCMS16Header(const SoundRecord &record)
{
  if (record.formatTag == 0x01 && record.bitsPerSample == 16)
  {
    PCMS16_Header header;

    header.riffSize += record.dataSize;
    header.fmtChannels = record.channels;
    header.fmtSampleRate = record.sampleRate;
    header.fmtAvgBytesRate = record.sampleRate * record.blockAlign;
    header.fmtBlockAlign = record.blockAlign;
    header.fmtBitsPerSample = record.bitsPerSample;
    header.dataSize = record.dataSize;

    return header;
  }

  PCMS16_Header header;

  header.riffSize += record.dataSizeUncompressed;
  header.fmtChannels = record.channels;
  header.fmtSampleRate = record.sampleRate;
  header.fmtBlockAlign = record.channels * sizeof(int16_t);
  header.fmtAvgBytesRate = record.sampleRate * header.fmtBlockAlign;
  header.fmtBitsPerSample = 16;
  header.dataSize = record.dataSizeUncompressed;

  return header;
}

PCMS16_Header PCMS16Header(const std::span<const int16_t> &in)
{
  if (in.size() < sizeof(PCMS16_Header))
    return {};

  PCMS16_Header header;
  std::memcpy(&header, in.data(), sizeof(PCMS16_Header));

  if (std::memcmp(header.riffId, "RIFF", 4) != 0)
    return {};

  if (std::memcmp(header.waveId, "WAVE", 4) != 0)
    return {};

  if (std::memcmp(header.fmtId, "fmt ", 4) != 0)
    return {};

  if (header.fmtSize != 0x10)
    return {};

  if (header.fmtFormat != 0x01)
    return {};

  if (header.fmtChannels > 2)
    return {};

  if (header.fmtBitsPerSample != 16)
    return {};

  if (header.fmtBlockAlign % sizeof(int16_t))
    return {};

  if (std::memcmp(header.dataId, "data", 4) != 0)
    return {};

  if (header.dataSize % header.fmtBlockAlign)
    return {};

  if (!header.dataSize || in.size() < sizeof(PCMS16_Header) + header.dataSize)
    return {};

  return header;
}

SoundRecord PCMS16SoundRecord(const PCMS16_Header &header, const uint64_t xxh3Hash)
{
  return {
    xxh3Hash,
    header.dataSize,
    header.dataSize,
    header.fmtSampleRate,
    header.fmtFormat,
    header.fmtBitsPerSample,
    header.fmtChannels,
    header.fmtBlockAlign,
    header.fmtChannels
  };
}

SoundRecord PCMS16SoundRecord(const PCMS16_Header &header, const std::span<const int16_t> &in)
{
  if (!header.dataSize || in.size() < header.dataSize)
    return {};

  const auto dataXXH3Hash = XXH3_64bits(in.data(), header.dataSize);
  return PCMS16SoundRecord(header, dataXXH3Hash);
}

SoundRecord PCMS16SoundRecord(const std::span<const int16_t> &in)
{
  const auto header = PCMS16Header(in);
  return PCMS16SoundRecord(header, PCMS16DataView(header, in));
}

std::span<const int16_t> PCMS16DataView(const PCMS16_Header &header, const std::span<const int16_t> &in)
{
  if (!header.dataSize || in.size() < sizeof(PCMS16_Header) + header.dataSize)
    return {};

  return in.subspan(sizeof(PCMS16_Header) / sizeof(int16_t), header.dataSize);
}

std::span<const int16_t> PCMS16DataView(const std::span<const int16_t> &in)
{
  return PCMS16DataView(PCMS16Header(in), in);
}

ADPCM_Header ADPCMHeader(const SoundRecord &in, int blocksizePow2)
{
  const auto samplesCount = static_cast<uint32_t>(in.dataSizeUncompressed / (in.channels * sizeof(int16_t)));

  if (in.formatTag == 0x11 && in.bitsPerSample == 4)
  {
    ADPCM_Header header;

    header.riffSize += in.dataSize;
    header.fmtChannels = in.channels;
    header.fmtSampleRate = in.sampleRate;
    header.fmtAvgBytesRate = (in.sampleRate * in.blockAlign) / in.samplesPerBlock;
    header.fmtBlockAlign = in.blockAlign;
    header.fmtBitsPerSample = in.bitsPerSample;
    header.fmtExtraSamplesPerBlock = in.samplesPerBlock;
    header.factSamplesCount = samplesCount;
    header.dataSize = in.dataSize;

    return header;
  }

  uint16_t block_size;

  if (blocksizePow2 > 0 && blocksizePow2 < 16)
    block_size = static_cast<uint16_t>(1 << blocksizePow2);
  else
    block_size = static_cast<uint16_t>(0x100 * in.channels * (in.sampleRate < 11000 ? 1 : in.sampleRate / 11000));

  const auto samples_per_block = static_cast<uint16_t>((block_size - in.channels * 4) * (in.channels ^ 3) + 1);

  const auto num_blocks = samplesCount / samples_per_block;
  const auto leftover_samples = samplesCount % samples_per_block;
  const auto total_data_bytes = (num_blocks + (leftover_samples != 0)) * block_size;

  ADPCM_Header header;
  header.riffSize += total_data_bytes;
  header.fmtChannels = in.channels;
  header.fmtSampleRate = in.sampleRate;
  header.fmtAvgBytesRate = in.sampleRate * block_size / samples_per_block;
  header.fmtBlockAlign = block_size;
  header.fmtBitsPerSample = 4;
  header.fmtExtraSamplesPerBlock = samples_per_block;
  header.factSamplesCount = samplesCount;
  header.dataSize = total_data_bytes;

  return header;
}

ADPCM_Header ADPCMHeader(const std::span<const char> &in)
{
  if (in.size() < sizeof(ADPCM_Header))
    return {};

  ADPCM_Header header;
  std::memcpy(&header, in.data(), sizeof(ADPCM_Header));

  if (std::memcmp(header.riffId, "RIFF", 4) != 0)
    return {};

  if (std::memcmp(header.waveId, "WAVE", 4) != 0)
    return {};

  if (std::memcmp(header.fmtId, "fmt ", 4) != 0)
    return {};

  if (header.fmtSize != 0x14)
    return {};

  if (header.fmtFormat != 0x11)
    return {};

  if (header.fmtChannels > 2)
    return {};

  if (header.fmtBitsPerSample != 4)
    return {};

  if (header.fmtExtraSize != 0x02)
    return {};

  if (std::memcmp(header.factId, "fact", 4) != 0)
    return {};

  if (header.factSize != 0x04)
    return {};

  if (std::memcmp(header.dataId, "data", 4) != 0)
    return {};

  if (header.dataSize % header.fmtBlockAlign)
    return {};

  if (!header.dataSize || in.size() < sizeof(ADPCM_Header) + header.dataSize)
    return {};

  return header;
}

SoundRecord ADPCMSoundRecord(const ADPCM_Header &header, const uint64_t xxh3Hash)
{
  return {
    xxh3Hash,
    static_cast<uint32_t>(header.factSamplesCount * header.fmtChannels * sizeof(int16_t)),
    header.dataSize,
    header.fmtSampleRate,
    header.fmtFormat,
    header.fmtBitsPerSample,
    header.fmtChannels,
    header.fmtBlockAlign,
    header.fmtExtraSamplesPerBlock
  };
}

SoundRecord ADPCMSoundRecord(const ADPCM_Header &header, const std::span<const char> &in, bool pcms16XXH3Hash)
{
  if (!header.dataSize || in.size() < header.dataSize)
    return {};

  if (!pcms16XXH3Hash)
  {
    const auto dataXXH3Hash = XXH3_64bits(in.data(), header.dataSize);
    return ADPCMSoundRecord(header, dataXXH3Hash);
  }

  std::vector<int16_t> pcms16Decoded;
  if (!PCMS16FromADPCM(header, in, pcms16Decoded, PCMS16_CONVERSION_FLAG_RAW_OUTPUT))
    return {};

  const auto dataXXH3Hash = XXH3_64bits(pcms16Decoded.data(), pcms16Decoded.size() * sizeof(int16_t));
  return ADPCMSoundRecord(header, dataXXH3Hash);
}

SoundRecord ADPCMSoundRecord(const std::span<const char> &in)
{
  const auto header = ADPCMHeader(in);
  return ADPCMSoundRecord(header, ADPCMDataView(header, in));
}

std::span<const char> ADPCMDataView(const ADPCM_Header &header, const std::span<const char> &in)
{
  if (!header.dataSize || in.size() < sizeof(ADPCM_Header) + header.dataSize)
    return {};

  return in.subspan(sizeof(ADPCM_Header), header.dataSize);
}

std::span<const char> ADPCMDataView(const std::span<const char> &in)
{
  return ADPCMDataView(ADPCMHeader(in), in);
}

SoundRecord VorbisHeader(const std::span<const char> &in)
{
  if (in.size() < sizeof(uint32_t))
    return {};

  uint32_t vorbisHeader;
  std::memcpy(&vorbisHeader, in.data(), sizeof(uint32_t));

  if (std::memcmp(&vorbisHeader, "OggS", 4) != 0)
    return {};

  SF_VIRTUAL_DATA_FIXED virtSndData{const_cast<char*>(in.data()), static_cast<sf_count_t>(in.size())};
  SndfileHandle sndFile(g_VirtSndFileIOFixed, &virtSndData);
  if (sndFile.error() != SF_ERR_NO_ERROR)
  {
    const auto* err = sndFile.strError();
    return {};
  }

  const auto format = sndFile.format();
  if ((format & (SF_FORMAT_OGG | SF_FORMAT_VORBIS)) != (SF_FORMAT_OGG | SF_FORMAT_VORBIS))
    return {};

  const auto frames = static_cast<uint32_t>(sndFile.frames());
  if (frames == 0)
    return {};

  const auto channels = static_cast<uint16_t>(sndFile.channels());
  if (channels == 0 || channels > 2)
    return {};

  const auto sampleRate = static_cast<uint32_t>(sndFile.samplerate());
  if (sampleRate == 0)
    return {};

  return {
    0,
    static_cast<uint32_t>(frames * channels * sizeof(int16_t)),
    static_cast<uint32_t>(in.size()),
    sampleRate,
    0x1000,
    16,
    channels,
    static_cast<uint16_t>(channels * sizeof(int16_t)),
    channels
  };
}

SoundRecord VorbisSoundRecord(const SoundRecord &header, const uint64_t xxh3Hash)
{
  return {
    xxh3Hash,
    header.dataSizeUncompressed,
    header.dataSize,
    header.sampleRate,
    header.formatTag,
    header.bitsPerSample,
    header.channels,
    header.blockAlign,
    header.samplesPerBlock
  };
}

SoundRecord VorbisSoundRecord(const SoundRecord &header, const std::span<const char> &in, bool pcms16XXH3Hash)
{
  if (!header.dataSize || in.size() < header.dataSize)
    return {};

  if (!pcms16XXH3Hash)
  {
    const auto dataXXH3Hash = XXH3_64bits(in.data(), header.dataSize);
    return VorbisSoundRecord(header, dataXXH3Hash);
  }

  std::vector<int16_t> pcms16Decoded;
  if (!PCMS16FromVorbis(header, in, pcms16Decoded, PCMS16_CONVERSION_FLAG_RAW_OUTPUT))
    return {};

  const auto dataXXH3Hash = XXH3_64bits(pcms16Decoded.data(), pcms16Decoded.size() * sizeof(int16_t));
  return VorbisSoundRecord(header, dataXXH3Hash);
}

SoundRecord VorbisSoundRecord(const std::span<const char> &in)
{
  const auto header = VorbisHeader(in);
  return VorbisSoundRecord(header, VorbisDataView(header, in));
}

std::span<const char> VorbisDataView(const SoundRecord &header, const std::span<const char> &in)
{
  if (!header.dataSize || in.size() < header.dataSize)
    return {};

  return in.subspan(0, header.dataSize);
}

std::span<const char> VorbisDataView(const std::span<const char> &in)
{
  return VorbisDataView(VorbisHeader(in), in);
}

SoundRecord UnknownSoundDataHeader(const std::span<const char> &in)
{
  SF_VIRTUAL_DATA_FIXED virtSndData{const_cast<char*>(in.data()), static_cast<sf_count_t>(in.size())};
  SndfileHandle sndFile(g_VirtSndFileIOFixed, &virtSndData);
  if (sndFile.error() != SF_ERR_NO_ERROR)
  {
    const auto* err = sndFile.strError();
    return {};
  }

  const auto frames = static_cast<uint32_t>(sndFile.frames());
  if (frames == 0)
    return {};

  const auto channels = static_cast<uint16_t>(sndFile.channels());
  if (channels == 0 || channels > 2)
    return {};

  const auto sampleRate = static_cast<uint32_t>(sndFile.samplerate());
  if (sampleRate == 0)
    return {};

  return {
    0,
    static_cast<uint32_t>(frames * channels * sizeof(int16_t)),
    static_cast<uint32_t>(in.size()),
    sampleRate,
    0x8000,
    16,
    channels,
    static_cast<uint16_t>(channels * sizeof(int16_t)),
    channels
  };
}

SoundRecord UnknownSoundDataSoundRecord(const SoundRecord &header, const uint64_t xxh3Hash)
{
  return {
    xxh3Hash,
    header.dataSizeUncompressed,
    header.dataSize,
    header.sampleRate,
    header.formatTag,
    header.bitsPerSample,
    header.channels,
    header.blockAlign,
    header.samplesPerBlock
  };
}

SoundRecord UnknownSoundDataSoundRecord(const SoundRecord &header, const std::span<const char> &in, bool pcms16XXH3Hash)
{
  if (!header.dataSize || in.size() < header.dataSize)
    return {};

  if (!pcms16XXH3Hash)
  {
    const auto dataXXH3Hash = XXH3_64bits(in.data(), header.dataSize);
    return UnknownSoundDataSoundRecord(header, dataXXH3Hash);
  }

  std::vector<int16_t> pcms16Decoded;
  if (!PCMS16FromUnknownSoundData(header, in, pcms16Decoded, PCMS16_CONVERSION_FLAG_RAW_OUTPUT))
    return {};

  const auto dataXXH3Hash = XXH3_64bits(pcms16Decoded.data(), pcms16Decoded.size() * sizeof(int16_t));
  return UnknownSoundDataSoundRecord(header, dataXXH3Hash);
}

SoundRecord UnknownSoundDataSoundRecord(const std::span<const char> &in)
{
  const auto header = UnknownSoundDataHeader(in);
  return UnknownSoundDataSoundRecord(header, UnknownSoundDataDataView(header, in));
}

std::span<const char> UnknownSoundDataDataView(const SoundRecord &header, const std::span<const char> &in)
{
  if (!header.dataSize || in.size() < header.dataSize)
    return {};

  return in.subspan(0, header.dataSize);
}

std::span<const char> UnknownSoundDataDataView(const std::span<const char> &in)
{
  return UnknownSoundDataDataView(UnknownSoundDataHeader(in), in);
}

SoundRecord SoundDataHeader(const std::span<const char> &in)
{
  const auto pcms16Header = PCMS16Header(ToSpan<const int16_t>(in));
  if (pcms16Header.dataSize)
    return PCMS16SoundRecord(pcms16Header);

  const auto adpcmHeader = ADPCMHeader(in);
  if (adpcmHeader.dataSize)
    return ADPCMSoundRecord(adpcmHeader);

  const auto vorbisHeader = VorbisHeader(in);
  if (vorbisHeader.dataSize)
    return vorbisHeader;

  return UnknownSoundDataHeader(in);
}

SoundRecord SoundDataSoundRecord(const SoundRecord &header, const uint64_t xxh3Hash)
{
  return {
    xxh3Hash,
    header.dataSizeUncompressed,
    header.dataSize,
    header.sampleRate,
    header.formatTag,
    header.bitsPerSample,
    header.channels,
    header.blockAlign,
    header.samplesPerBlock
  };
}

SoundRecord SoundDataSoundRecord(const SoundRecord &header, const std::span<const char> &in, bool pcms16XXH3Hash)
{
  if (!header.dataSize || in.size() < header.dataSize)
    return {};

  if (!pcms16XXH3Hash || header.formatTag == 0x01)
  {
    const auto dataXXH3Hash = XXH3_64bits(in.data(), header.dataSize);
    return SoundDataSoundRecord(header, dataXXH3Hash);
  }

  std::vector<int16_t> pcms16Decoded;
  if (!PCMS16FromSoundData(header, in, pcms16Decoded, PCMS16_CONVERSION_FLAG_RAW_OUTPUT))
    return {};

  const auto dataXXH3Hash = XXH3_64bits(pcms16Decoded.data(), pcms16Decoded.size() * sizeof(int16_t));
  return SoundDataSoundRecord(header, dataXXH3Hash);
}

SoundRecord SoundDataSoundRecord(const std::span<const char> &in)
{
  const auto header = SoundDataHeader(in);
  return SoundDataSoundRecord(header, SoundDataDataView(header, in));
}

std::span<const char> SoundDataDataView(const SoundRecord &header, const std::span<const char> &in)
{
  if (!header.dataSize || in.size() < header.dataSize)
    return {};

  switch (header.formatTag)
  {
    case 0x01: {
      return ToSpan<const char>(PCMS16DataView(PCMS16Header(header), ToSpan<const int16_t>(in)));
    }
    case 0x11: {
      return ADPCMDataView(ADPCMHeader(header), in);
    }
    case 0x1000: {
      return VorbisDataView(header, in);
    }
    case 0x8000: {
      return UnknownSoundDataDataView(header, in);
    }
    default: {
      return {};
    }
  }
}

std::span<const char> SoundDataDataView(const std::span<const char> &in)
{
  return SoundDataDataView(SoundDataHeader(in), in);
}

int32_t PCMS16ChangeSampleRate(PCMS16_Header &header, std::vector<char> &data, uint32_t newSampleRate)
{
  if (header.fmtSampleRate == newSampleRate)
    return 0;

  auto result = PCMS16ChangeSampleRate(header, ToSpan<int16_t>(std::span<char>{data}), newSampleRate);
  if (result.empty())
    return -1;

  data.resize(result.size() * sizeof(int16_t));
  std::memcpy(data.data(), result.data(), result.size());
  return 1;
}

int32_t PCMS16ChangeSampleRate(PCMS16_Header &header, std::vector<int16_t> &data, uint32_t newSampleRate)
{
  if (header.fmtSampleRate == newSampleRate)
    return 0;

  auto result = PCMS16ChangeSampleRate(header, std::span<int16_t>{data}, newSampleRate);
  if (result.empty())
    return -1;

  data = std::move(result);
  return 1;
}

int32_t PCMS16ChangeChannelCount(PCMS16_Header &header, std::vector<char> &data, uint16_t newChannelCount)
{
  if (header.fmtChannels == newChannelCount)
    return 0;

  auto result = PCMS16ChangeChannelCount(header, ToSpan<int16_t>(std::span<char>{data}), newChannelCount);
  if (result.empty())
    return -1;

  data.resize(result.size() * sizeof(int16_t));
  std::memcpy(data.data(), result.data(), result.size());
  return 1;
}

int32_t PCMS16ChangeChannelCount(PCMS16_Header &header, std::vector<int16_t> &data, uint16_t newChannelCount)
{
  if (header.fmtChannels == newChannelCount)
    return 0;

  auto result = PCMS16ChangeChannelCount(header, std::span<int16_t>{data}, newChannelCount);
  if (result.empty())
    return -1;

  data = std::move(result);
  return 1;
}

bool PCMS16FromADPCM(const ADPCM_Header &header, const std::span<const char> &in, std::vector<int16_t> &out, int flags)
{
#if FBL_PCMS16_USE_ADPCMXQ
  if (PCMS16FromADPCM_ADPCMXQ(header, in, out, flags))
    return true;
#endif

  return PCMS16FromADPCM_LibSNDFile(header, in, out, flags);
}

bool PCMS16FromADPCM(const std::span<const char>& in, std::vector<int16_t> &out, int flags)
{
  const auto header = ADPCMHeader(in);
  return PCMS16FromADPCM(header, ADPCMDataView(in), out, flags);
}

bool PCMS16FromVorbis(const SoundRecord &header, const std::span<const char> &in, std::vector<int16_t> &out, int flags)
{
  if (!header.dataSize || in.size() < header.dataSize)
    return false;

  SF_VIRTUAL_DATA_FIXED virtSndData{const_cast<char*>(in.data()), static_cast<sf_count_t>(in.size())};
  SndfileHandle sndFile(g_VirtSndFileIOFixed, &virtSndData);
  if (sndFile.error() != SF_ERR_NO_ERROR)
  {
    const auto* err = sndFile.strError();
    return false;
  }

  assert(header.sampleRate == sndFile.samplerate());
  assert(header.channels == sndFile.channels());

  auto frames = static_cast<uint32_t>(sndFile.frames());
  if (frames != header.dataSizeUncompressed / header.blockAlign)
    return false;

  // format check already performed, we skip it here

  const auto outIndexInput = out.size();
  if (!(flags & PCMS16_CONVERSION_FLAG_RAW_OUTPUT))
    PCMS16WriteHeader(out, header.channels, header.sampleRate, header.sampleRate);

  const auto outIndex = out.size();
  out.resize(outIndex + header.dataSizeUncompressed / sizeof(int16_t));

  size_t outOffset = outIndex;
  while (frames != 0)
  {
    static constexpr uint32_t maxFramesPerRead = UINT16_MAX;
    const uint32_t framesToRead = std::min(maxFramesPerRead, frames);
    if (sndFile.readf(out.data() + outOffset, framesToRead) != framesToRead)
    {
      sf_close(sndFile.takeOwnership());

      out.resize(outIndexInput);
      return false;
    }

    frames -= framesToRead;
    outOffset += framesToRead * header.channels;
  }

  return true;
}

bool PCMS16FromVorbis(const std::span<const char> &in, std::vector<int16_t> &out, int flags)
{
  const auto header = VorbisHeader(in);
  return PCMS16FromVorbis(header, VorbisDataView(in), out, flags);
}

bool PCMS16FromUnknownSoundData(const SoundRecord &header, const std::span<const char> &in, std::vector<int16_t> &out, int flags)
{
  if (!header.dataSize || in.size() < header.dataSize)
    return false;

  SF_VIRTUAL_DATA_FIXED virtSndData{const_cast<char*>(in.data()), static_cast<sf_count_t>(in.size())};
  SndfileHandle sndFile(g_VirtSndFileIOFixed, &virtSndData);
  if (sndFile.error() != SF_ERR_NO_ERROR)
  {
    const auto* err = sndFile.strError();
    return false;
  }

  assert(header.sampleRate == sndFile.samplerate());
  assert(header.channels == sndFile.channels());

  auto frames = static_cast<uint32_t>(sndFile.frames());
  if (frames != header.dataSizeUncompressed / header.blockAlign)
    return false;

  // format check already performed, we skip it here

  const auto outIndexInput = out.size();
  if (!(flags & PCMS16_CONVERSION_FLAG_RAW_OUTPUT))
    PCMS16WriteHeader(out, header.channels, header.sampleRate, header.sampleRate);

  const auto outIndex = out.size();
  out.resize(outIndex + header.dataSizeUncompressed / sizeof(int16_t));

  size_t outOffset = outIndex;
  while (frames != 0)
  {
    static constexpr uint32_t maxFramesPerRead = UINT16_MAX;
    const uint32_t framesToRead = std::min(maxFramesPerRead, frames);
    if (sndFile.readf(out.data() + outOffset, framesToRead) != framesToRead)
    {
      sf_close(sndFile.takeOwnership());

      out.resize(outIndexInput);
      return false;
    }

    frames -= framesToRead;
    outOffset += framesToRead * header.channels;
  }

  return true;
}

bool PCMS16FromUnknownSoundData(const std::span<const char> &in, std::vector<int16_t> &out, int flags)
{
  const auto header = UnknownSoundDataSoundRecord(in);
  return PCMS16FromUnknownSoundData(header, UnknownSoundDataDataView(in), out, flags);
}

bool PCMS16FromSoundData(const SoundRecord &header, const std::span<const char> &in, std::vector<int16_t> &out, int flags)
{
  if (!header.dataSize || in.size() < header.dataSize)
    return false;

  switch (header.formatTag)
  {
    case 0x01: {
      if (!(flags & PCMS16_CONVERSION_FLAG_RAW_OUTPUT))
      {
        const auto pcms16Header = PCMS16Header(header);

        const auto outIndexInput = out.size();
        out.resize(outIndexInput + sizeof(PCMS16_Header) / sizeof(int16_t));
        std::memcpy(out.data() + outIndexInput, &pcms16Header, sizeof(PCMS16_Header));
      }

      const auto outIndexInput = out.size();
      out.resize(outIndexInput + header.dataSize / sizeof(int16_t));
      std::memcpy(out.data() + outIndexInput, in.data(), header.dataSize);

      return true;
    }
    case 0x11: {
      return PCMS16FromADPCM(ADPCMHeader(header), in, out, flags);
    }
    case 0x1000: {
      return PCMS16FromVorbis(header, in, out, flags);
    }
    case 0x8000: {
      return PCMS16FromUnknownSoundData(header, in, out, flags);
    }
    default: {
      return false;
    }
  }
}

bool PCMS16FromSoundData(const std::span<const char> &in, std::vector<int16_t> &out, int flags)
{
  const auto header = SoundDataSoundRecord(in);
  return PCMS16FromSoundData(header, SoundDataDataView(in), out, flags);
}

bool PCMS16ToADPCM(const PCMS16_Header &header, const std::span<const int16_t> &in, std::vector<char> &out, int flags, [[maybe_unused]] int blocksizePow2, [[maybe_unused]] int lookahead)
{
#if FBL_PCMS16_USE_ADPCMXQ
  if (PCMS16ToADPCM_ADPCMXQ(header, in, out, flags, blocksizePow2, lookahead))
    return true;
#endif

  return PCMS16ToADPCM_LibSNDFile(header, in, out, flags);
}

bool PCMS16ToADPCM(const std::span<const int16_t>& in, std::vector<char> &out, int flags, int blocksizePow2, int lookahead)
{
  const auto header = PCMS16Header(in);
  return PCMS16ToADPCM(header, PCMS16DataView(header, in), out, flags, blocksizePow2, lookahead);
}

bool PCMS16ToVorbis(const PCMS16_Header &header, const std::span<const int16_t> &in, std::vector<char> &out, int)
{
  if (!header.dataSize || in.size() < header.dataSize)
    return false;

  const auto outIndexInput = out.size();

  SF_VIRTUAL_DATA virtSndData{out};
  SndfileHandle sndFile(g_VirtSndFileIO, &virtSndData, SFM_WRITE, SF_FORMAT_OGG | SF_FORMAT_VORBIS, header.fmtChannels, header.fmtSampleRate);
  if (sndFile.error() != SF_ERR_NO_ERROR)
  {
    const auto* err = sndFile.strError();
    out.resize(outIndexInput);
    return false;
  }

  assert(header.fmtSampleRate == sndFile.samplerate());
  assert(header.fmtChannels == sndFile.channels());

  size_t inOffset = 0;
  auto frames = header.dataSize / header.fmtBlockAlign;
  while (frames != 0)
  {
    static constexpr uint32_t maxFramesPerWrite = UINT16_MAX;
    const uint32_t framesToWrite = std::min(maxFramesPerWrite, frames);
    if (sndFile.writef(in.data() + inOffset, framesToWrite) != framesToWrite)
    {
      sf_close(sndFile.takeOwnership());

      out.resize(outIndexInput);
      return false;
    }

    frames -= framesToWrite;
    inOffset += framesToWrite * header.fmtChannels;
  }

  return true;
}

bool PCMS16ToVorbis(const std::span<const int16_t> &in, std::vector<char> &out, int flags)
{
  const auto header = PCMS16Header(in);
  return PCMS16ToVorbis(header, PCMS16DataView(header, in), out, flags);
}
