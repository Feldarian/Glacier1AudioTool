//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#pragma once

extern "C"
{
#include "Precompiled.h"
}

#include <imgui.h>
#include <scn/scn.h>
#include <sndfile.hh>
#include <toml++/toml.h>

#include <array>
#include <bitset>
#include <compare>
#include <filesystem>
#include <fstream>
#include <future>
#include <map>
#include <set>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

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

inline SF_VIRTUAL_IO g_VirtSndFileIO {
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

template <typename KeyType, typename ValueType, typename ComparatorLessType = std::less<>, typename AllocatorType = std::allocator<std::pair<const KeyType, ValueType>>>
using OrderedMap = std::map<KeyType, ValueType, ComparatorLessType, AllocatorType>;

template <typename ValueType, typename ComparatorLessType = std::less<>, typename AllocatorType = std::allocator<ValueType>>
using OrderedSet = std::set<ValueType, ComparatorLessType, AllocatorType>;

template <typename ValueType>
using OptionalReference = std::optional<std::reference_wrapper<ValueType>>;

#include "Singleton.hpp"
#include "UTF/UTF.hpp"

using namespace UTF;

#include "Options.hpp"
#include "Utils.hpp"
