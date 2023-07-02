# Glacier 1 Audio Tool

## Supported

### Hitman: Codename 47
Format | Open | Save | Export | Import
:---|:---:|:---:|:---:|:---:
\*.IDX + \*.BIN | ✔ | ✔ | ✔ | ✔
Repeat.WAV + PackRepeat.WAV | ✘ | ✘ | ✘ | ✘


### Hitman: Silent Assassin
Format | Open | Save | Export | Import
:---|:---:|:---:|:---:|:---:
\*.WAV + \*.WHD + streams.wav | ✔ | ✔ | ✔ | ✔
\*.SND (in scene's \*.ZIP) | ✘ | ✘ | ✘ | ✘

### Hitman: Contracts
Format | Open | Save | Export | Import
:---|:---:|:---:|:---:|:---:
\*.WAV + \*.WHD + streams.wav | ✔ | ✔ | ✔ | ✔
\*.SND (in scene's \*.ZIP) | ✘ | ✘ | ✘ | ✘

### Hitman: Blood Money
Format | Open | Save | Export | Import
:---|:---:|:---:|:---:|:---:
\*.WAV + \*.WHD + \*.STR | ✔ | ✘ | ✔ | ✘
\*.SND (in scene's \*.ZIP) | ✘ | ✘ | ✘ | ✘

## Features
 * Supports multiple Glacier 1 games (see list of supported formats and games above).
   * Import and save functionality currently only encompasses replacements, not new additions.
 * Automatically converts data to desired format on import.
     * Offers nearly perfect reimport of data with default settings (imperfections are there only due to decompression and recompression).
     * Ability to import in highest quality PCM S16 (best quality format game supports out of the box, this is not default and enabling this is experimental!).
     * Automatically fixes number of channels and sample rate (both are enabled by default, disabling these options is experimental!).
     * Tool can try to import data as-is for fastest imports (not enabled by default, disabling this option is experimental!).
 * Ability to show modified entries in archives thanks to original records caching.
     * Automatically ignore unchanged original files from original archives when trying to import.
     * Records caching feature can be disabled, same as the option which ignores unchanged files on import. Beware that disabling records caching also causes unchanged files to get imported.
     * Currently pre-packaged with __Steam__ data and default __English__ language.
       * When you have different version, just remove `data/records` folder and let it regenerate records on first archive reload.
 * Currently 2 supported languages - __English__ and __Czech__
     * New localizations can be added by copying `data/localization/English.toml`, translating the entries and placing the copied file back into `data/localization`
     * For example, you could create `Deutsch.toml` which would contain translations to __German__ language and place it in the mentioned folder.
     * New option would then appear in the __Options__ tab under __Language__.
     * If newly added language is missing glyphs in fonts used by the tool, you may add additional font defining them into `data/fonts` folder (note that fonts are currently being read in alphabetical order, first font should always be the base font!).
     
## Used "Open-Source Licensed" Tools
 * [PVS-Studio](https://pvs-studio.com/en/pvs-studio/?utm_source=website&utm_medium=github&utm_campaign=open_source) - static analyzer for C, C++, C#, and Java code.
 * [ReSharper C++](https://www.jetbrains.com/community/opensource/#support) - provides multiple features for increasing productivity when working with C++ code.
 * [SmartGit](https://www.syntevo.com/register-non-commercial/) - cross-platform Git GUI client.
