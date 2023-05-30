//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#pragma once

namespace UTF
{

class LocalizationInstance
{
public:
  inline static const String8 Empty;

  void Clear()
  {
    localizationMap.clear();
  }

  bool Load(const toml::table &localizationTable)
  {
    for (const auto &[key, value] : localizationTable)
    {
      if (!value.is_string())
        return false;
    }

    for (const auto &[key, value] : localizationTable)
    {
      assert(value.is_string());

      String8 keyUTF{ key.str() };
      String8 valueUTF{ **value.as_string() };

      GlyphRangesBuilder::Get().AddText(keyUTF);
      GlyphRangesBuilder::Get().AddText(valueUTF);

      localizationMap.insert_or_assign(std::move(keyUTF), std::move(valueUTF));
    }

    return true;
  }

  const String8 &Localize(const String8CI &key) const
  {
    const auto localizationIt = localizationMap.find(key);
    if (localizationIt == localizationMap.end())
      return Empty;

    return localizationIt->second;
  }

private:
  std::map<String8CI, String8> localizationMap;
};

class LocalizationManager : public Singleton<LocalizationManager>
{
public:
  bool LoadLocalization(StringView8CI localizationPathView)
  {
    const auto localizationPath = localizationPathView.path();
    if (!exists(localizationPath))
      return false;

    const auto localizationFile = toml::parse_file(localizationPathView.native());
    if (localizationFile.failed())
      return false;

    String8CI languageName(localizationPath.stem());
    const auto languageNameSeparatorPosition = languageName.native().rfind('_');
    if (languageNameSeparatorPosition != String8CI::npos)
      languageName = languageName.native().substr(languageNameSeparatorPosition + 1);

    const auto languageGroup = localizationFile["language"];
    if (languageGroup.is_table())
    {
      const auto &languageNameValue = languageGroup["name"];
      if (languageNameValue.is_string())
        languageName = **languageNameValue.as_string();
    }

    if (languageName.empty())
      return false;

    GlyphRangesBuilder::Get().AddText(languageName);

    const auto localizationTableValue = localizationFile["localization"];
    if (!localizationTableValue.is_table())
      return false;

    const auto localizationTable = *localizationTableValue.as_table();

    std::lock_guard lock(dataMutex);

    const auto [localizationInstanceIt, localizationInstanceEmplaced] = localizationInstancesMap.try_emplace(languageName, LocalizationInstance{});
    const auto localizationLoaded = localizationInstanceIt->second.Load(localizationTable);

    if (localizationInstanceEmplaced)
    {
      if (!localizationLoaded)
        localizationInstancesMap.erase(localizationInstanceIt);

      SetDefaultLanguage(defaultLocalizationLanguage);
      SetLanguage(localizationLanguage);
    }

    return localizationLoaded;
  }

  std::vector<String8CI> GetAvailableLanguages() const
  {
    std::shared_lock lock(dataMutex);

    const auto languagesRange = localizationInstancesMap | std::views::keys;
    return {languagesRange.begin(), languagesRange.end()};
  }

  bool SetDefaultLanguage(const String8CI &language)
  {
    if (language.empty())
      return false;

    std::lock_guard lock(dataMutex);

    const auto localizationInstanceIt = localizationInstancesMap.find(language);
    if (localizationInstanceIt == localizationInstancesMap.cend())
      return false;

    defaultLocalizationLanguage = localizationInstanceIt->first;
    defaultLocalizationInstance = &localizationInstanceIt->second;
    return true;
  }

  const String8CI &GetDefaultLanguage() const
  {
    std::shared_lock lock(dataMutex);

    return defaultLocalizationLanguage;
  }

  bool SetLanguage(const String8CI &language)
  {
    if (language.empty())
      return false;

    std::lock_guard lock(dataMutex);

    const auto localizationInstanceIt = localizationInstancesMap.find(language);
    if (localizationInstanceIt == localizationInstancesMap.cend())
      return false;

    localizationLanguage = localizationInstanceIt->first;
    localizationInstance = &localizationInstanceIt->second;
    return true;
  }

  const String8CI &GetLanguage() const
  {
    std::shared_lock lock(dataMutex);

    return localizationLanguage.empty() ? GetDefaultLanguage() : localizationLanguage;
  }

  template <typename UTFStorageType, typename UTFCharType, bool CaseSensitive>
  requires IsUTF8CharType<UTFCharType> && !CaseSensitive
  const String8 &Localize(const StringWrapper<UTFStorageType, UTFCharType, CaseSensitive> &key) const
  {
    std::shared_lock lock(dataMutex);

    const auto &localized = localizationInstance ? localizationInstance->Localize(key) : LocalizationInstance::Empty;
    if (!localized.empty())
      return localized;

    return defaultLocalizationInstance ? defaultLocalizationInstance->Localize(key) : LocalizationInstance::Empty;
  }

  template <typename UTFStorageType, typename UTFCharType, bool CaseSensitive>
  requires !IsUTF8CharType<UTFCharType>
  const String8 &Localize(const StringWrapper<UTFStorageType, UTFCharType, CaseSensitive> &key) const
  {
    return Localize(String8CI{ key });
  }

  template <typename UTFCharTypeInput>
  requires IsUTFCharType<UTFCharTypeInput>
  const String8 &Localize(const std::basic_string_view<UTFCharTypeInput> key) const
  {
    return Localize(String8CI{ key });
  }

  template <typename UTFCharTypeInput>
  requires IsUTFCharType<UTFCharTypeInput>
  const String8 &Localize(const std::basic_string<UTFCharTypeInput> &key) const
  {
    return Localize(String8CI{ key });
  }

  const String8 &Localize(std::basic_string<char> &&key) const
  {
    return Localize(String8CI{ std::move(key) });
  }

  template <typename UTFCharTypeInput, size_t UTFInputSize>
  requires IsUTFCharType<UTFCharTypeInput>
  const String8 &Localize(const UTFCharTypeInput (&key)[UTFInputSize]) const
  {
    return Localize(String8CI{ key });
  }

  template <typename UTFCharTypeInput>
  requires IsUTFCharType<UTFCharTypeInput>
  const String8 &Localize(const UTFCharTypeInput *key, const size_t length) const
  {
    return Localize(String8CI{ key, length });
  }

  const String8 &Localize(String8 &&key) const
  {
    return Localize(String8CI(std::move(key)));
  }

  template <typename UTFStorageType, typename UTFCharType, bool CaseSensitive, typename... FormatArgs>
  requires IsUTFCharType<UTFCharType> && IsAnyOfTypes<UTFStorageType, std::basic_string<UTFCharType>, std::basic_string_view<UTFCharType>>
  String8 &LocalizeFormatTo(String8& buffer, const StringWrapper<UTFStorageType, UTFCharType, CaseSensitive> &key, FormatArgs &&...args) const
  {
    if constexpr (std::same_as<UTFStorageType, std::basic_string<UTFCharType>> && IsUTF8CharType<UTFCharType> && !CaseSensitive)
    {
      std::shared_lock lock(dataMutex);

      buffer.clear();

      const auto &localizedFormat = Localize(key);
      if (localizedFormat.empty())
        return buffer;

      try
      {
        std::vformat_to(std::back_inserter(buffer.native()), localizedFormat.native(), std::make_format_args(std::forward<FormatArgs>(args)...));
      }
      catch ([[maybe_unused]] std::format_error error)
      {
        buffer.clear();
      }

      return buffer;
    }
    else
      return LocalizeFormatTo(buffer, String8CI{ key }, std::forward<FormatArgs>(args)...);
  }

  template <typename UTFCharTypeInput, typename... FormatArgs>
  requires IsUTFCharType<UTFCharTypeInput>
  String8 &LocalizeFormatTo(String8& buffer, const std::basic_string_view<UTFCharTypeInput> key, FormatArgs &&...args) const
  {
    return LocalizeFormatTo(buffer, String8CI{ key }, std::forward<FormatArgs>(args)...);
  }

  template <typename UTFCharTypeInput, typename... FormatArgs>
  requires IsUTFCharType<UTFCharTypeInput>
  String8 &LocalizeFormatTo(String8& buffer, const std::basic_string<UTFCharTypeInput> &key, FormatArgs &&...args) const
  {
    return LocalizeFormatTo(buffer, String8CI{ key }, std::forward<FormatArgs>(args)...);
  }

  template <typename... FormatArgs>
  String8 &LocalizeFormatTo(String8& buffer, std::basic_string<char> &&key, FormatArgs &&...args) const
  {
    return LocalizeFormatTo(buffer, String8CI{ std::move(key) }, std::forward<FormatArgs>(args)...);
  }

  template <typename UTFCharTypeInput, size_t UTFInputSize, typename... FormatArgs>
  requires IsUTFCharType<UTFCharTypeInput>
  String8 &LocalizeFormatTo(String8& buffer, const UTFCharTypeInput (&key)[UTFInputSize], FormatArgs &&...args) const
  {
    return LocalizeFormatTo(buffer, String8CI{ key }, std::forward<FormatArgs>(args)...);
  }

  template <typename... FormatArgs>
  String8 &LocalizeFormatTo(String8& buffer, String8 &&key, FormatArgs &&...args) const
  {
    return LocalizeFormatTo(buffer, String8CI{ std::move(key) }, std::forward<FormatArgs>(args)...);
  }

  template <typename UTFStorageType, typename UTFCharType, bool CaseSensitive, typename... FormatArgs>
  requires IsUTFCharType<UTFCharType> && IsAnyOfTypes<UTFStorageType, std::basic_string<UTFCharType>, std::basic_string_view<UTFCharType>>
  const String8 &LocalizeFormat(const StringWrapper<UTFStorageType, UTFCharType, CaseSensitive> &key, FormatArgs &&...args) const
  {
    thread_local static String8 result;
    return LocalizeFormatTo(result, key, std::forward<FormatArgs>(args)...);
  }

  template <typename UTFCharTypeInput, typename... FormatArgs>
  requires IsUTFCharType<UTFCharTypeInput>
  const String8 &LocalizeFormat(const std::basic_string_view<UTFCharTypeInput> key, FormatArgs &&...args) const
  {
    return LocalizeFormat(String8CI{ key }, std::forward<FormatArgs>(args)...);
  }

  template <typename UTFCharTypeInput, typename... FormatArgs>
  requires IsUTFCharType<UTFCharTypeInput>
  const String8 &LocalizeFormat(const std::basic_string<UTFCharTypeInput> &key, FormatArgs &&...args) const
  {
    return LocalizeFormat(String8CI{ key }, std::forward<FormatArgs>(args)...);
  }

  template <typename... FormatArgs>
  const String8 &LocalizeFormat(std::basic_string<char> &&key, FormatArgs &&...args) const
  {
    return LocalizeFormat(String8CI{ std::move(key) }, std::forward<FormatArgs>(args)...);
  }

  template <typename UTFCharTypeInput, size_t UTFInputSize, typename... FormatArgs>
  requires IsUTFCharType<UTFCharTypeInput>
  const String8 &LocalizeFormat(const UTFCharTypeInput (&key)[UTFInputSize], FormatArgs &&...args) const
  {
    return LocalizeFormat(String8CI{ key }, std::forward<FormatArgs>(args)...);
  }

  template <typename... FormatArgs>
  const String8 &LocalizeFormat(String8 &&key, FormatArgs &&...args) const
  {
    return LocalizeFormat(String8CI{ std::move(key) }, std::forward<FormatArgs>(args)...);
  }

private:
  std::map<String8CI, LocalizationInstance> localizationInstancesMap;
  String8CI defaultLocalizationLanguage;
  LocalizationInstance *defaultLocalizationInstance = nullptr;
  String8CI localizationLanguage;
  LocalizationInstance *localizationInstance = nullptr;

  mutable std::shared_mutex dataMutex;
};

}
