//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#pragma once

namespace UTF
{

template <typename TypeInput, typename... TypesToCompareWith>
concept IsAnyOfTypes = (std::same_as<TypeInput, TypesToCompareWith> || ...);

template <typename UTFCharType>
concept IsUTF8CharType = IsAnyOfTypes<UTFCharType, char, char8_t, int8_t, uint8_t>;

static_assert(std::same_as<char16_t, UChar>, "ICU UChar typedef is not char16_t!");

#ifdef _WIN32

template <typename UTFCharType>
concept IsUTF16CharType = IsAnyOfTypes<UTFCharType, wchar_t, char16_t, int16_t, uint16_t, UChar>;

template <typename UTFCharType>
concept IsUTF32CharType = IsAnyOfTypes<UTFCharType, char32_t, int32_t, uint32_t, UChar32>;

#else // _WIN32

static_assert(std::same_as<wchar_t, UChar32>, "STD wchar_t had unexpected size!");

template <typename UTFCharType>
concept IsUTF16CharType = IsAnyOfTypes<UTFCharType, char16_t, int16_t, uint16_t, UChar>;

template <typename UTFCharType>
concept IsUTF32CharType = IsAnyOfTypes<UTFCharType, wchar_t, char32_t, int32_t, uint32_t, UChar32>;

#endif // _WIN32

template <typename UTFCharType>
concept IsUTFNativeCharType = IsAnyOfTypes<UTFCharType, char, wchar_t>;

template <typename UTFCharType>
concept IsUTFCharType = IsUTF8CharType<UTFCharType> || IsUTF16CharType<UTFCharType> || IsUTF32CharType<UTFCharType> || IsUTFNativeCharType<UTFCharType>;

template <typename UTFCharTypeLeft, typename UTFCharTypeRight>
concept IsSameUTFCharType = IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
	&& IsUTF8CharType<UTFCharTypeLeft> == IsUTF8CharType<UTFCharTypeRight>
	&& IsUTF16CharType<UTFCharTypeLeft> == IsUTF16CharType<UTFCharTypeRight>
	&& IsUTF32CharType<UTFCharTypeLeft> == IsUTF32CharType<UTFCharTypeRight>;

static constexpr auto CodepointInvalid{ 0u };
static constexpr auto CodepointMax{ 0x10FFFFu };

}
