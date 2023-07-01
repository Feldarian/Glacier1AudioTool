//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#pragma once

namespace UTF
{

template <typename UTFCharType, bool CaseSensitive, typename UTFCharTypeTraits, typename UTFAllocator>
requires IsUTFCharType<UTFCharType>
template <typename UTFCharTypeInput, bool CaseSensitiveInput, typename UTFCharTypeTraitsInput>
requires IsUTFCharType<UTFCharTypeInput>
StringWrapper<UTFCharType, CaseSensitive, UTFCharTypeTraits, UTFAllocator>::StringWrapper(const StringViewWrapper<UTFCharTypeInput, CaseSensitiveInput, UTFCharTypeTraitsInput> other)
{
  *this += other;
}

template <typename UTFCharType, bool CaseSensitive, typename UTFCharTypeTraits, typename UTFAllocator>
requires IsUTFCharType<UTFCharType>
template <typename UTFCharTypeInput, bool CaseSensitiveInput, typename UTFCharTypeTraitsInput>
requires IsUTFCharType<UTFCharTypeInput>
StringWrapper<UTFCharType, CaseSensitive, UTFCharTypeTraits, UTFAllocator>& StringWrapper<UTFCharType, CaseSensitive, UTFCharTypeTraits, UTFAllocator>::operator=(const StringViewWrapper<UTFCharTypeInput, CaseSensitiveInput, UTFCharTypeTraitsInput> other)
{
  if ((static_cast<const void *>(data()) == static_cast<const void *>(other.data())) && (size() == other.size()))
    return *this;

  utfData.clear();

  return *this += other;
}

template <typename UTFCharType, bool CaseSensitive, typename UTFCharTypeTraits, typename UTFAllocator>
requires IsUTFCharType<UTFCharType>
template <typename UTFCharTypeInput, bool CaseSensitiveInput, typename UTFCharTypeTraitsInput>
requires IsSameUTFCharType<UTFCharType, UTFCharTypeInput>
StringWrapper<UTFCharType, CaseSensitive, UTFCharTypeTraits, UTFAllocator>& StringWrapper<UTFCharType, CaseSensitive, UTFCharTypeTraits, UTFAllocator>::operator+=(const StringViewWrapper<UTFCharTypeInput, CaseSensitiveInput, UTFCharTypeTraitsInput> other)
{
  if (!other.empty())
    utfData.append(reinterpret_cast<const UTFCharType *>(other.data()), other.size());

  return *this;
}

template <typename UTFCharType, bool CaseSensitive, typename UTFCharTypeTraits, typename UTFAllocator>
requires IsUTFCharType<UTFCharType>
template <typename UTFCharTypeInput, bool CaseSensitiveInput, typename UTFCharTypeTraitsInput>
requires IsUTF8CharType<UTFCharType> && IsUTF16CharType<UTFCharTypeInput>
StringWrapper<UTFCharType, CaseSensitive, UTFCharTypeTraits, UTFAllocator>& StringWrapper<UTFCharType, CaseSensitive, UTFCharTypeTraits, UTFAllocator>::operator+=(const StringViewWrapper<UTFCharTypeInput, CaseSensitiveInput, UTFCharTypeTraitsInput> other)
{
  if (other.empty())
    return *this;

  UErrorCode errorCode = U_ZERO_ERROR;

  const auto oldSize = size();
  resize(oldSize + other.size() * 2);

  int32_t length = 0;

  u_strToUTF8(reinterpret_cast<char *>(data() + oldSize), size() - oldSize, &length, reinterpret_cast<const UChar *>(other.data()), other.size(), &errorCode);

  if (errorCode > U_ZERO_ERROR)
    length = 0;

  resize(oldSize + length);

  return *this;
}

template <typename UTFCharType, bool CaseSensitive, typename UTFCharTypeTraits, typename UTFAllocator>
requires IsUTFCharType<UTFCharType>
template <typename UTFCharTypeInput, bool CaseSensitiveInput, typename UTFCharTypeTraitsInput>
requires IsUTF8CharType<UTFCharType> && IsUTF32CharType<UTFCharTypeInput>
StringWrapper<UTFCharType, CaseSensitive, UTFCharTypeTraits, UTFAllocator>& StringWrapper<UTFCharType, CaseSensitive, UTFCharTypeTraits, UTFAllocator>::operator+=(const StringViewWrapper<UTFCharTypeInput, CaseSensitiveInput, UTFCharTypeTraitsInput> other)
{
  if (other.empty())
    return *this;

  const auto oldSize = size();
  resize(oldSize + other.size() * 4);

  size_t offset = 0;
  int32_t length = 0;

  auto *reinterpretedData = reinterpret_cast<uint8_t *>(data() + oldSize);
  const auto reinterpretedDataSize = size() - oldSize;
  for (const auto otherCodePoint : other)
  {
    bool wasError = false;
    const auto codePoint = static_cast<uint32_t>(otherCodePoint);
    U8_APPEND(reinterpretedData, offset, reinterpretedDataSize, codePoint, wasError);
    if (wasError)
    {
      length = 0;
      break;
    }

    length = offset;
  }

  resize(oldSize + length);

  return *this;
}

template <typename UTFCharType, bool CaseSensitive, typename UTFCharTypeTraits, typename UTFAllocator>
requires IsUTFCharType<UTFCharType>
template <typename UTFCharTypeInput, bool CaseSensitiveInput, typename UTFCharTypeTraitsInput>
requires IsUTF16CharType<UTFCharType> && IsUTF8CharType<UTFCharTypeInput>
StringWrapper<UTFCharType, CaseSensitive, UTFCharTypeTraits, UTFAllocator>& StringWrapper<UTFCharType, CaseSensitive, UTFCharTypeTraits, UTFAllocator>::operator+=(const StringViewWrapper<UTFCharTypeInput, CaseSensitiveInput, UTFCharTypeTraitsInput> other)
{
  if (other.empty())
    return *this;

  UErrorCode errorCode = U_ZERO_ERROR;

  const auto oldSize = size();
  resize(oldSize + other.size());

  int32_t length = 0;

  u_strFromUTF8(reinterpret_cast<UChar *>(data() + oldSize), size() - oldSize, &length, reinterpret_cast<const char *>(other.data()), other.size(), &errorCode);

  if (errorCode > U_ZERO_ERROR)
    length = 0;

  resize(oldSize + length);

  return *this;
}

template <typename UTFCharType, bool CaseSensitive, typename UTFCharTypeTraits, typename UTFAllocator>
requires IsUTFCharType<UTFCharType>
template <typename UTFCharTypeInput, bool CaseSensitiveInput, typename UTFCharTypeTraitsInput>
requires IsUTF16CharType<UTFCharType> && IsUTF32CharType<UTFCharTypeInput>
StringWrapper<UTFCharType, CaseSensitive, UTFCharTypeTraits, UTFAllocator>& StringWrapper<UTFCharType, CaseSensitive, UTFCharTypeTraits, UTFAllocator>::operator+=(const StringViewWrapper<UTFCharTypeInput, CaseSensitiveInput, UTFCharTypeTraitsInput> other)
{
  if (other.empty())
    return *this;

  UErrorCode errorCode = U_ZERO_ERROR;

  const auto oldSize = size();
  resize(oldSize + other.size() * 2);

  int32_t length = 0;

  u_strFromUTF32(reinterpret_cast<UChar *>(data() + oldSize), size() - oldSize, &length, reinterpret_cast<const UChar32 *>(other.data()), other.size(), &errorCode);

  if (errorCode > U_ZERO_ERROR)
    length = 0;

  resize(oldSize + length);

  return *this;
}

template <typename UTFCharType, bool CaseSensitive, typename UTFCharTypeTraits, typename UTFAllocator>
requires IsUTFCharType<UTFCharType>
template <typename UTFCharTypeInput, bool CaseSensitiveInput, typename UTFCharTypeTraitsInput>
requires IsUTF32CharType<UTFCharType> && IsUTF8CharType<UTFCharTypeInput>
StringWrapper<UTFCharType, CaseSensitive, UTFCharTypeTraits, UTFAllocator>& StringWrapper<UTFCharType, CaseSensitive, UTFCharTypeTraits, UTFAllocator>::operator+=(const StringViewWrapper<UTFCharTypeInput, CaseSensitiveInput, UTFCharTypeTraitsInput> other)
{
  if (other.empty())
    return *this;

  const auto oldSize = size();
  resize(oldSize + other.size());

  size_t offset = 0;
  int32_t length = 0;

  auto *reinterpretedData = reinterpret_cast<UChar32 *>(data() + oldSize);
  const auto *inputData = other.data();
  const auto inputDataSize = other.size();
  for (size_t inputOffset = 0; inputOffset < inputDataSize;)
  {
    auto &outputChar = reinterpretedData[offset++];
    U8_NEXT(inputData, inputOffset, inputDataSize, outputChar);
  }
  length = offset;

  resize(oldSize + length);

  return *this;
}

template <typename UTFCharType, bool CaseSensitive, typename UTFCharTypeTraits, typename UTFAllocator>
requires IsUTFCharType<UTFCharType>
template <typename UTFCharTypeInput, bool CaseSensitiveInput, typename UTFCharTypeTraitsInput>
requires IsUTF32CharType<UTFCharType> && IsUTF16CharType<UTFCharTypeInput>
StringWrapper<UTFCharType, CaseSensitive, UTFCharTypeTraits, UTFAllocator>& StringWrapper<UTFCharType, CaseSensitive, UTFCharTypeTraits, UTFAllocator>::operator+=(const StringViewWrapper<UTFCharTypeInput, CaseSensitiveInput, UTFCharTypeTraitsInput> other)
{
  if (other.empty())
    return *this;

  const auto oldSize = size();
  resize(oldSize + other.size());

  size_t offset = 0;
  int32_t length = 0;

  auto *reinterpretedData = reinterpret_cast<UChar32 *>(data() + oldSize);
  const auto *inputData = other.data();
  const auto inputDataSize = other.size();
  for (size_t inputOffset = 0; inputOffset < inputDataSize;)
  {
    auto &outputChar = reinterpretedData[offset++];
    U16_NEXT(inputData, inputOffset, inputDataSize, outputChar);
  }
  length = offset;

  resize(oldSize + length);

  return *this;
}

}
