//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//
// TODO - optimize conversions, as right now, we force keys to StringView8CI when we can now use generic <=> comparator
//      - look in code for other examples like this (there are quite a few), we often do unnecessary UTF conversions
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

      String8CI keyUTF{ key.str() };
      String8 valueUTF{ **value.as_string() };

      GlyphRangesBuilder::Get().AddText(keyUTF);
      GlyphRangesBuilder::Get().AddText(valueUTF);

      localizationMap.insert_or_assign(std::move(keyUTF), std::move(valueUTF));
    }

    return true;
  }

  const String8 &Localize(const StringView8CI key) const
  {
    const auto localizationIt = localizationMap.find(key);
    if (localizationIt == localizationMap.end())
      return Empty;

    return localizationIt->second;
  }

private:
  OrderedMap<String8CI, String8> localizationMap;
};

class LocalizationManager : public Singleton<LocalizationManager>
{
public:
  bool LoadLocalization(const StringView8CI localizationPathView)
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

  bool SetDefaultLanguage(const StringView8CI language)
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

  bool SetLanguage(const StringView8CI language)
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

  template <typename UTFCharType, bool CaseSensitive = false, typename UTFCharTypeTraits = std::char_traits<UTFCharType>>
  requires IsUTF8CharType<UTFCharType>
  const String8 &Localize(const StringViewWrapper<UTFCharType, CaseSensitive, UTFCharTypeTraits> &key) const
  {
    std::shared_lock lock(dataMutex);

    const auto &localized = localizationInstance ? localizationInstance->Localize(key) : LocalizationInstance::Empty;
    if (!localized.empty())
      return localized;

    return defaultLocalizationInstance ? defaultLocalizationInstance->Localize(key) : LocalizationInstance::Empty;
  }

  template <typename UTFCharType, bool CaseSensitive = false, typename UTFCharTypeTraits = std::char_traits<UTFCharType>>
  requires (!IsUTF8CharType<UTFCharType>)
  const String8 &Localize(const StringViewWrapper<UTFCharType, CaseSensitive, UTFCharTypeTraits> &key) const
  {
    return Localize(String8CI{ key });
  }

  template <typename Type>
  requires StringViewConstructible<Type>
  const String8 &Localize(const Type &key) const
  {
    if constexpr (StringView8Constructible<Type>)
      return Localize(StringView8CI(key));
    else if constexpr (StringView16Constructible<Type>)
      return Localize(StringView16CI(key));
    else if constexpr (StringView32Constructible<Type>)
      return Localize(StringView32CI(key));
  }

  template <typename Type, typename... FormatArgs>
  requires StringViewConstructible<Type>
  String8 &LocalizeFormatTo(String8& buffer, const Type &key, FormatArgs &&...args) const
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

  template <typename Type, typename... FormatArgs>
  requires StringViewConstructible<Type>
  String8 LocalizeFormat(const Type &key, FormatArgs &&...args) const
  {
    String8 result;
    LocalizeFormatTo(result, key, std::forward<FormatArgs>(args)...);
    return std::move(result);
  }

private:
  OrderedMap<String8CI, LocalizationInstance, std::less<>> localizationInstancesMap;
  String8CI defaultLocalizationLanguage;
  LocalizationInstance *defaultLocalizationInstance = nullptr;
  String8CI localizationLanguage;
  LocalizationInstance *localizationInstance = nullptr;

  mutable std::shared_mutex dataMutex;
};

}
