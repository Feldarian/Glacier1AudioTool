//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#pragma once

#include "Options.hpp"

std::vector<char> ReadWholeBinaryFile(const StringView8CI &acpPath);

String8 ReadWholeTextFile(const StringView8CI &acpPath);

float GetAlignedItemWidth(int64_t acItemsCount);

String8CI BrowseDirectoryDialog();

std::pair<String8CI, uint32_t> OpenFileDialog(const StringView8CI &filters, const StringView8CI &defaultFileName = "");

std::pair<String8CI, uint32_t> SaveFileDialog(const StringView8CI &filters, const StringView8CI &defaultFileName = "");

std::vector<String8CI> GetAllFilesInDirectory(const StringView8CI &directory, const StringView8CI &extension, bool recursive);

StringView8CI GetProgramPath();

StringView8CI GetUserPath();

int32_t DisplayMessage(const StringView8 &message, const StringView8 &title = g_LocalizationManager.Localize("MESSAGEBOX_TITLE_INFORMATION"), bool yesNo = false, const uint32_t messageFlags = SDL_MESSAGEBOX_INFORMATION);

int32_t DisplayInformation(const StringView8 &message, const StringView8 &title = g_LocalizationManager.Localize("MESSAGEBOX_TITLE_INFORMATION"), bool yesNo = false, const Options &options = Options::Get());

int32_t DisplayWarning(const StringView8 &message, const StringView8 &title = g_LocalizationManager.Localize("MESSAGEBOX_TITLE_WARNING"), bool yesNo = false, const Options &options = Options::Get());

int32_t DisplayError(const StringView8 &message, const StringView8 &title = g_LocalizationManager.Localize("MESSAGEBOX_TITLE_ERROR"), bool yesNo = false);

std::vector<StringView8CI> GetPathStems(StringView8CI pathView);

String8CI MakeFileDialogFilter(const std::vector<std::pair<String8, String8CI>>& filters);
