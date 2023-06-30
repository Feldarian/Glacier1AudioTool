//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#pragma once

namespace UTF
{

template <typename UTFCharType, bool CaseSensitive, typename UTFCharTypeTraits>
requires IsUTFCharType<UTFCharType>
template <typename UTFCharTypeInput, bool CaseSensitiveInput, typename UTFCharTypeTraitsInput, typename UTFAllocatorInput>
requires IsSameUTFCharType<UTFCharType, UTFCharTypeInput>
inline constexpr StringViewWrapper<UTFCharType, CaseSensitive, UTFCharTypeTraits>::StringViewWrapper(const StringWrapper<UTFCharTypeInput, CaseSensitiveInput, UTFCharTypeTraitsInput, UTFAllocatorInput>& other)
  : StringViewWrapper{ std::basic_string_view<UTFCharTypeInput>(other.native()), true }
{}

template <typename UTFCharType, bool CaseSensitive, typename UTFCharTypeTraits>
requires IsUTFCharType<UTFCharType>
[[nodiscard]] inline constexpr std::filesystem::path StringViewWrapper<UTFCharType, CaseSensitive, UTFCharTypeTraits>::path() const
{
  if constexpr (IsSameUTFCharType<UTFCharType, wchar_t>)
    return std::wstring{ reinterpret_cast<const wchar_t *>(data()), size() };
  else
  {
    // TODO - clean this up a bit, ugly we can't use default template values...
    return StringWrapper<wchar_t>(utfData).path();
  }
}

}
