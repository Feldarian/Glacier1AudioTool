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
class StringViewWrapper;

template <typename UTFCharType, bool CaseSensitive = true, typename UTFCharTypeTraits = std::char_traits<UTFCharType>, typename UTFAllocator = std::allocator<UTFCharType>>
requires IsUTFCharType<UTFCharType>
class StringWrapper
{
public:
  inline static constexpr auto npos{ std::basic_string<UTFCharType, UTFCharTypeTraits, UTFAllocator>::npos };

  constexpr StringWrapper() = default;

  template <typename UTFCharTypeInput, bool CaseSensitiveInput = CaseSensitive, typename UTFCharTypeTraitsInput = std::char_traits<UTFCharTypeInput>>
  requires IsUTFCharType<UTFCharTypeInput>
  constexpr StringWrapper(const StringViewWrapper<UTFCharTypeInput, CaseSensitiveInput, UTFCharTypeTraitsInput> other);

  template <typename UTFCharTypeInput, bool CaseSensitiveInput = CaseSensitive, typename UTFCharTypeTraitsInput = std::char_traits<UTFCharTypeInput>, typename UTFAllocatorInput = std::allocator<UTFCharTypeInput>>
  requires IsUTFCharType<UTFCharTypeInput>
  constexpr StringWrapper(const StringWrapper<UTFCharTypeInput, CaseSensitiveInput, UTFCharTypeTraitsInput, UTFAllocatorInput>& other)
    : StringWrapper(StringViewWrapper<UTFCharTypeInput, CaseSensitiveInput, UTFCharTypeTraitsInput>{ other })
  {
  }

  constexpr StringWrapper(const std::basic_string<UTFCharType, UTFCharTypeTraits, UTFAllocator> &other)
    : utfData{ other }
  {}

  constexpr StringWrapper(std::basic_string<UTFCharType, UTFCharTypeTraits, UTFAllocator> &&other) noexcept
    : utfData{ std::move(other) }
  {}

  template <typename ...Args>
  requires StringViewConstructible<Args...>
  constexpr StringWrapper(const Args&... args)
  {
    if constexpr (StringView8Constructible<Args...>)
      *this = StringView8(args...);
    else if constexpr (StringView16Constructible<Args...>)
      *this = StringView16(args...);
    else if constexpr (StringView32Constructible<Args...>)
      *this = StringView32(args...);
  }

  constexpr StringWrapper(const StringWrapper& other)
    : utfData{ other.utfData }
  {}

  constexpr StringWrapper(StringWrapper&& other) noexcept
    : utfData{ std::move(other.utfData) }
  {}

  template <typename UTFCharTypeInput, bool CaseSensitiveInput = CaseSensitive, typename UTFCharTypeTraitsInput = std::char_traits<UTFCharTypeInput>>
  requires IsUTFCharType<UTFCharTypeInput>
  constexpr StringWrapper& operator=(const StringViewWrapper<UTFCharTypeInput, CaseSensitiveInput, UTFCharTypeTraitsInput> other);

  template <typename Type>
  requires StringViewConstructible<Type>
  constexpr StringWrapper& operator=(const Type& other)
  {
    utfData.clear();

    return *this += other;
  }

  constexpr StringWrapper& operator=(const std::basic_string<UTFCharType, UTFCharTypeTraits, UTFAllocator> &other)
  {
    if (static_cast<const void*>(data()) != static_cast<const void*>(other.data()))
      utfData = other;

    return *this;
  }

  constexpr StringWrapper& operator=(std::basic_string<UTFCharType, UTFCharTypeTraits, UTFAllocator> &&other) noexcept
  {
    if (static_cast<const void*>(data()) != static_cast<const void*>(other.data()))
      utfData = std::move(other);

    return *this;
  }

  constexpr StringWrapper& operator=(const StringWrapper& other)
  {
    return *this = other.utfData;
  }

  constexpr StringWrapper& operator=(StringWrapper&& other) noexcept
  {
    return *this = std::move(other.utfData);
  }

  [[nodiscard]] constexpr UTFCharType& operator[](const size_t index)
  {
    return utfData[index];
  }

  [[nodiscard]] constexpr UTFCharType operator[](const size_t index) const
  {
    return utfData[index];
  }

  template <typename UTFCharTypeInput, bool CaseSensitiveInput = CaseSensitive, typename UTFCharTypeTraitsInput = std::char_traits<UTFCharTypeInput>>
  requires IsSameUTFCharType<UTFCharType, UTFCharTypeInput>
  constexpr StringWrapper& operator+=(const StringViewWrapper<UTFCharTypeInput, CaseSensitiveInput, UTFCharTypeTraitsInput> other);

  template <typename UTFCharTypeInput, bool CaseSensitiveInput = CaseSensitive, typename UTFCharTypeTraitsInput = std::char_traits<UTFCharTypeInput>>
  requires IsUTF8CharType<UTFCharType> && IsUTF16CharType<UTFCharTypeInput>
  constexpr StringWrapper& operator+=(const StringViewWrapper<UTFCharTypeInput, CaseSensitiveInput, UTFCharTypeTraitsInput> other);

  template <typename UTFCharTypeInput, bool CaseSensitiveInput = CaseSensitive, typename UTFCharTypeTraitsInput = std::char_traits<UTFCharTypeInput>>
  requires IsUTF8CharType<UTFCharType> && IsUTF32CharType<UTFCharTypeInput>
  constexpr StringWrapper& operator+=(const StringViewWrapper<UTFCharTypeInput, CaseSensitiveInput, UTFCharTypeTraitsInput> other);

  template <typename UTFCharTypeInput, bool CaseSensitiveInput = CaseSensitive, typename UTFCharTypeTraitsInput = std::char_traits<UTFCharTypeInput>>
  requires IsUTF16CharType<UTFCharType> && IsUTF8CharType<UTFCharTypeInput>
  constexpr StringWrapper& operator+=(const StringViewWrapper<UTFCharTypeInput, CaseSensitiveInput, UTFCharTypeTraitsInput> other);

  template <typename UTFCharTypeInput, bool CaseSensitiveInput = CaseSensitive, typename UTFCharTypeTraitsInput = std::char_traits<UTFCharTypeInput>>
  requires IsUTF16CharType<UTFCharType> && IsUTF32CharType<UTFCharTypeInput>
  constexpr StringWrapper& operator+=(const StringViewWrapper<UTFCharTypeInput, CaseSensitiveInput, UTFCharTypeTraitsInput> other);

  template <typename UTFCharTypeInput, bool CaseSensitiveInput = CaseSensitive, typename UTFCharTypeTraitsInput = std::char_traits<UTFCharTypeInput>>
  requires IsUTF32CharType<UTFCharType> && IsUTF8CharType<UTFCharTypeInput>
  constexpr StringWrapper& operator+=(const StringViewWrapper<UTFCharTypeInput, CaseSensitiveInput, UTFCharTypeTraitsInput> other);

  template <typename UTFCharTypeInput, bool CaseSensitiveInput = CaseSensitive, typename UTFCharTypeTraitsInput = std::char_traits<UTFCharTypeInput>>
  requires IsUTF32CharType<UTFCharType> && IsUTF16CharType<UTFCharTypeInput>
  constexpr StringWrapper& operator+=(const StringViewWrapper<UTFCharTypeInput, CaseSensitiveInput, UTFCharTypeTraitsInput> other);

  template <typename Type>
  requires StringViewConstructible<Type>
  constexpr StringWrapper& operator+=(const Type& other)
  {
    if constexpr (StringView8Constructible<Type>)
      return *this += StringView8(other);
    else if constexpr (StringView16Constructible<Type>)
      return *this += StringView16(other);
    else if constexpr (StringView32Constructible<Type>)
      return *this += StringView32(other);
  }

  [[nodiscard]] constexpr auto begin()
  {
    return utfData.begin();
  }

  [[nodiscard]] constexpr auto begin() const
  {
    return utfData.cbegin();
  }

  [[nodiscard]] constexpr auto cbegin() const
  {
    return utfData.cbegin();
  }

  [[nodiscard]] constexpr auto end()
  {
    return utfData.end();
  }

  [[nodiscard]] constexpr auto end() const
  {
    return utfData.cend();
  }

  [[nodiscard]] constexpr auto cend() const
  {
    return utfData.cend();
  }

  [[nodiscard]] constexpr UTFCharType* data()
  {
    return utfData.data();
  }

  [[nodiscard]] constexpr const UTFCharType* data() const
  {
    return utfData.data();
  }

  [[nodiscard]] constexpr const UTFCharType* c_str() const
  {
    return utfData.c_str();
  }

  [[nodiscard]] constexpr UTFCharType& front()
  {
    return utfData.front();
  }

  [[nodiscard]] constexpr const UTFCharType& front() const
  {
    return utfData.front();
  }

  [[nodiscard]] constexpr UTFCharType& back()
  {
    return utfData.back();
  }

  [[nodiscard]] constexpr const UTFCharType& back() const
  {
    return utfData.back();
  }

  [[nodiscard]] constexpr bool empty() const
  {
    return utfData.empty();
  }

  [[nodiscard]] constexpr size_t size() const
  {
    return utfData.size();
  }

  [[nodiscard]] constexpr size_t length() const
  {
    return utfData.length();
  }

  [[nodiscard]] constexpr auto& native()
  {
    return utfData;
  }

  [[nodiscard]] constexpr const auto& native() const
  {
    return utfData;
  }

  [[nodiscard]] constexpr std::filesystem::path path() const
  {
    if constexpr (IsSameUTFCharType<wchar_t, UTFCharType>)
      return std::wstring{ reinterpret_cast<const wchar_t *>(data()), size() };
    else
      return StringWrapper<wchar_t, true>(utfData).path();
  }

  void clear()
  {
    utfData.clear();
  }

  void resize(const size_t newSize, const UTFCharType defaultChar = static_cast<UTFCharType>(0))
  {
    utfData.resize(newSize, defaultChar);
  }

  [[nodiscard]] constexpr bool IsNullTerminated() const
  {
    return true;
  }

private:
  std::basic_string<UTFCharType> utfData;
};

using String8 = StringWrapper<char, true>;
using String16 = StringWrapper<UChar, true>;
using String32 = StringWrapper<UChar32, true>;

using StringW = StringWrapper<wchar_t, true>;

using String8CI = StringWrapper<char, false>;
using String16CI = StringWrapper<UChar, false>;
using String32CI = StringWrapper<UChar32, false>;

using StringWCI = StringWrapper<wchar_t, false>;

}
