# Hitman Audio Tool

## Features
 * Full support of import and export of the Hitman: Codename 47 *.idx/*.bin pairs existing entries
     * Support for adding new ones is not planned for now
 * Full supports of import and export of the Hitman 2: Silent Assassin and Hitman 3: Silent Assassin *.whd/*.wav pairs + streams.wav existing entries
     * Support for adding new ones is not planned for now
 * Automatically convert input data to exact game format 
     * Note that reimported files won't be 100% the same, but they will have exact same format (unless user tunes the options)
     * Tool can also import all audio in highest quality as PCM 16-bit signed WAVs
     * Conversions can be completely disabled, in which case tool tries to import data directly
 * Automatically ignore unchanged original files from original archives when trying to import 
     * This feature can be turned off in case someone wants to reimport original files also for whatever reason
 * Currently 2 supported languages - English and Czech
     * New localizations can be added by copying `data/localization/English.toml`, translating the entries and placing the copied file back into `data/localization`
     * For example, you could create Deutsch.toml which would contain translations to German language and place it in the mentioned folder
     * New option would then appear in the Options tab under Language
     * If newly added language is missing glyphs in fonts used by the tool, you may add additional font defining them into `data/fonts` folder (note that fonts are currently being read in alphabetical order) 
     
## Future
 * Definitely some more cleanup/refactor of the source code
     * Code is clean enough to be readable and usable I would say, but it lacks comments and there is still a lot of duplication all around
     * Some parts are also glued together from my previous projects, so there is inconsistent styling and some other stuff...
     * I would still consider the tool in its early stages, but it can do much more for old Hitman 1, 2 and 3 games than the old Hitman Audio Tool floating around the internet (not mentioning it not correctly interpreting parts of the data, making exports broken and import from these exports impossible)
 * I would like to expand this beyond older Hitman titles to all Glacier 1 games
 * Freedom Fighters should be technically already in working state also, just have to update few smaller things in the tool
 * Potential rename of the tool (considering it should support all Glacier 1 games....)
 * Version 1.0.0 tagged code contains Hitman: Blood Money WIP code which seems to work for exports
     * This code is currently locked under debug builds, so you have to build the tool yourself if you want to try this feature out for now
     * Will be made as part of regular release when I figure out enough to be able to reliably import the files back and save the archives

## Used "Open-Source Licensed" Tools
 * [PVS-Studio](https://pvs-studio.com/en/pvs-studio/?utm_source=website&utm_medium=github&utm_campaign=open_source) - static analyzer for C, C++, C#, and Java code.
 * [ReSharper C++](https://www.jetbrains.com/community/opensource/#support) - provides multiple features for increasing productivity when working with C++ code.
 * [SmartGit](https://www.syntevo.com/register-non-commercial/) - cross-platform Git GUI client.
