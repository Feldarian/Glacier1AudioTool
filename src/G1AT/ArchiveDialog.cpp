//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#include <Precompiled.hpp>

#include "ArchiveDialog.hpp"

#include "Utils.hpp"

bool ArchiveDirectory::Clear(const bool retVal)
{
  directories.clear();
  files.clear();

  return retVal;
}

ArchiveDirectory& ArchiveDirectory::GetDirectory(const StringView8CI &searchPath, OrderedSet<String8CI>& archivePaths)
{
  auto pathStems = GetPathStems(searchPath);
  return GetDirectory(pathStems, searchPath, archivePaths);
}

ArchiveDirectory& ArchiveDirectory::GetDirectory(std::vector<StringView8CI> &pathStems, StringView8CI searchPath, OrderedSet<String8CI>& archivePaths)
{
  assert(!pathStems.empty());

  if (pathStems.size() > 1)
  {
    const auto directoryIt = directories.find(pathStems.back());
    if (directoryIt != directories.cend())
    {
      pathStems.pop_back();
      return directoryIt->second.GetDirectory(pathStems, searchPath, archivePaths);
    }

    auto newPathStems = GetPathStems(*archivePaths.emplace(searchPath).first);
    while (newPathStems.size() > pathStems.size())
      newPathStems.pop_back();

    std::swap(pathStems, newPathStems);

    auto& directory = directories[pathStems.back()];
    pathStems.pop_back();
    return directory.GetDirectory(pathStems, searchPath, archivePaths);
  }

  const auto directoryIt = directories.find(pathStems.front());
  if (directoryIt != directories.cend())
    return directoryIt->second;

  searchPath = *archivePaths.emplace(searchPath).first;
  const auto newPathStems = GetPathStems(searchPath);
  auto& directory = directories[newPathStems.front()];
  directory.path = searchPath;
  return directory;
}

ArchiveFile& ArchiveDirectory::GetFile(const StringView8CI &searchPath, OrderedSet<String8CI>& archivePaths)
{
  auto pathStems = GetPathStems(searchPath);
  return GetFile(pathStems, searchPath, archivePaths);
}

ArchiveFile& ArchiveDirectory::GetFile(std::vector<StringView8CI>& pathStems, StringView8CI searchPath, OrderedSet<String8CI>& archivePaths)
{
  assert(!pathStems.empty());

  if (pathStems.size() > 1)
  {
    const auto directoryIt = directories.find(pathStems.back());
    if (directoryIt != directories.cend())
    {
      pathStems.pop_back();
      return directoryIt->second.GetFile(pathStems, searchPath, archivePaths);
    }

    auto newPathStems = GetPathStems(*archivePaths.emplace(searchPath).first);
    while (newPathStems.size() > pathStems.size())
      newPathStems.pop_back();

    std::swap(pathStems, newPathStems);

    auto& directory = directories[pathStems.back()];
    pathStems.pop_back();
    return directory.GetFile(pathStems, searchPath, archivePaths);
  }

  const auto fileIt = files.find(pathStems.front());
  if (fileIt != files.cend())
    return fileIt->second;

  searchPath = *archivePaths.emplace(searchPath).first;
  const auto newPathStems = GetPathStems(searchPath);
  auto& file = files[newPathStems.front()];
  file.path = searchPath;
  return file;
}

bool ArchiveDirectory::IsDirty() const
{
  for (const auto &file : files | ranges::views::values)
  {
    if (file.dirty)
      return true;
  }

  for (const auto &directory : directories | ranges::views::values)
  {
    if (directory.IsDirty())
      return true;
  }

  return false;
}

bool ArchiveDirectory::IsOriginal() const
{
  for (const auto &file : files | ranges::views::values)
  {
    if (!file.original)
      return false;
  }

  for (const auto &directory : directories | ranges::views::values)
  {
    if (!directory.IsOriginal())
      return false;
  }

  return true;
}

void ArchiveDirectory::CleanDirty()
{
  for (auto &directory : directories | ranges::views::values)
    directory.CleanDirty();

  for (auto &file : files | ranges::views::values)
    file.dirty = false;
}

void ArchiveDirectory::CleanOriginal()
{
  for (auto &directory : directories | ranges::views::values)
    directory.CleanOriginal();

  for (auto &file : files | ranges::views::values)
    file.original = true;
}

void ArchiveDirectory::DrawTree(const StringView8CI &thisPath) const
{
  if (!thisPath.empty() && !ImGui::TreeNode(String8(thisPath).c_str()))
    return;

  for (const auto& [directoryPath, directory] : directories)
  {
    if (directory.IsDirty())
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1.0f, 1.0f, 0.0f, 1.0f});
    else if (!directory.IsOriginal())
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.0f, 1.0f, 0.0f, 1.0f});

    directory.DrawTree(directoryPath);

    if (directory.IsDirty() || !directory.IsOriginal())
      ImGui::PopStyleColor();
  }

  for (const auto& [filePath, file] : files)
  {
    if (file.dirty)
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1.0f, 1.0f, 0.0f, 1.0f});
    else if (!file.original)
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.0f, 1.0f, 0.0f, 1.0f});

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    ImGui::Selectable(String8(filePath).c_str());

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);

    if (file.dirty || !file.original)
      ImGui::PopStyleColor();
  }

  if (!thisPath.empty())
    ImGui::TreePop();
}

bool ArchiveDialog::Clear(const bool retVal)
{
  path.clear();
  archiveRoot.Clear();
  archivePaths.clear();

  return retVal;
}

bool ArchiveDialog::Load(const StringView8CI &loadPath)
{
  if (loadPath.empty())
    return false;

  g_GlyphRangesBuilder.AddText(loadPath);

  progressMessage = g_LocalizationManager.Localize("ARCHIVE_DIALOG_LOAD_PROGRESS_READING_ARCHIVE");
  progressNext = 0;
  progressNextTotal = 1;
  nextPath = loadPath;

  progressTask = std::async(std::launch::async, [this, options = Options::Get()] {
    switch (UnsavedChangesPopup())
    {
      case 1: {
        Save(path, false);
        break;
      }
      case 0: {
        break;
      }
      default:
      case -1: {
        std::unique_lock progressMessageLock(progressMessageMutex);
        progressMessage.clear();
        return;
      }
    }

    if (LoadImpl(nextPath, options))
    {
      // TODO - this is causing a lot of synchronizations at the end of loading
      //        best would be to do this on the main thread
      for (const auto& archivePath : archivePaths)
        g_GlyphRangesBuilder.AddText(archivePath);

      path = nextPath;
    }
    else
      Clear();

    std::unique_lock progressMessageLock(progressMessageMutex);
    progressMessage.clear();
  });

  return true;
}

bool ArchiveDialog::Import(const StringView8CI &importFolderPath)
{
  if (importFolderPath.empty())
    return false;

  progressMessage = g_LocalizationManager.Localize("ARCHIVE_DIALOG_IMPORT_PROGRESS_IMPORTING_DATA");
  progressNext = 0;
  progressNextTotal = 1;

  progressTask = std::async(std::launch::async, [this, importFolderPath = String8CI(importFolderPath), options = Options::Get()] {
    const auto allImportFiles = GetAllFilesInDirectory(importFolderPath, "", true);

    if (allImportFiles.empty())
    {
      std::unique_lock progressMessageLock(progressMessageMutex);
      progressMessage.clear();
      progressNext = 1;
      return;
    }

    progressNextTotal = allImportFiles.size();
    progressNext = 0;

    std::for_each(std::execution::par, allImportFiles.begin(), allImportFiles.end(), [this, importFolderPath, options](const auto& importFilePath)
    {
      {
        std::unique_lock progressMessageLock(progressMessageMutex);
        progressMessage = g_LocalizationManager.LocalizeFormat("ARCHIVE_DIALOG_IMPORT_PROGRESS_IMPORTING_FILE", String8(relative(importFilePath.path(), importFolderPath.path())));
        ++progressNext;
      }

      ImportSingle(importFolderPath, importFilePath, options);
    });

    std::unique_lock progressMessageLock(progressMessageMutex);
    progressMessage.clear();
  });

  return true;
}

bool ArchiveDialog::Export(const StringView8CI &exportFolderPathView)
{
  if (exportFolderPathView.empty())
    return false;

  progressMessage = g_LocalizationManager.Localize("ARCHIVE_DIALOG_EXPORT_PROGRESS_EXPORTING_DATA");
  progressNext = 0;
  progressNextTotal = 1;

  progressTask = std::async(std::launch::async, [this, exportFolderPath = String8CI(exportFolderPathView), options = Options::Get()] {
    if (archivePaths.empty())
    {
      std::unique_lock progressMessageLock(progressMessageMutex);
      progressMessage.clear();
      progressNext = 1;
      return;
    }

    progressNextTotal = archivePaths.size();
    progressNext = 0;

    std::for_each(std::execution::par, archivePaths.begin(), archivePaths.end(), [this, exportFolderPath, options](const auto& exportFilePath)
    {
      {
        std::unique_lock progressMessageLock(progressMessageMutex);
        progressMessage = g_LocalizationManager.LocalizeFormat("ARCHIVE_DIALOG_IMPORT_PROGRESS_EXPORTING_FILE", exportFilePath);
        ++progressNext;
      }

      ExportSingle(exportFolderPath, exportFilePath, options);
    });

    std::unique_lock progressMessageLock(progressMessageMutex);
    progressMessage.clear();
  });

  return true;
}

bool ArchiveDialog::Save(const StringView8CI &savePath, bool async)
{
  if (savePath.empty())
    return false;

  g_GlyphRangesBuilder.AddText(savePath);

  progressMessage = g_LocalizationManager.Localize("ARCHIVE_DIALOG_SAVE_PROGRESS_SAVING_ARCHIVE");
  progressNext = 0;
  progressNextTotal = 1;

  nextPath = savePath;

  progressTask = std::async(std::launch::async, [this, options = Options::Get()] {
    if (archivePaths.empty())
    {
      std::unique_lock progressMessageLock(progressMessageMutex);
      progressMessage.clear();
      progressNext = 1;
      return;
    }

    if (SaveImpl(nextPath, options))
      path = nextPath;

    std::unique_lock progressMessageLock(progressMessageMutex);
    progressMessage.clear();
  });

  if (!async)
  {
    progressTask.wait();
    progressTask.get();
  }

  return true;
}

int32_t ArchiveDialog::UnsavedChangesPopup() const
{
  if (!archiveRoot.IsDirty())
    return 0;

  return DisplayWarning(g_LocalizationManager.Localize("ARCHIVE_DIALOG_UNSAVED_CHANGES_MESSAGE"),
                                           g_LocalizationManager.Localize("ARCHIVE_DIALOG_UNSAVED_CHANGES_TITLE"), true);
}

StringView8CI ArchiveDialog::GetPath() const
{
  return IsInProgress() ? nextPath : path;
}

bool ArchiveDialog::IsAllowed() const
{
  return IsSaveAllowed() || IsExportAllowed() || IsImportAllowed();
}

bool ArchiveDialog::IsInProgress() const
{
  return progressTask.valid();
}

ArchiveDirectory& ArchiveDialog::GetDirectory(const StringView8CI &searchPath)
{
  return archiveRoot.GetDirectory(searchPath, archivePaths);
}

ArchiveFile& ArchiveDialog::GetFile(const StringView8CI &searchPath)
{
  return archiveRoot.GetFile(searchPath, archivePaths);
}

const OrderedSet<String8CI> & ArchiveDialog::GetPaths() const
{
  return archivePaths;
}

bool ArchiveDialog::IsDirty() const
{
  return archiveRoot.IsDirty();
}

bool ArchiveDialog::IsOriginal() const
{
  return archiveRoot.IsOriginal();
}

void ArchiveDialog::CleanDirty()
{
  archiveRoot.CleanDirty();
}

void ArchiveDialog::CleanOriginal()
{
  archiveRoot.CleanOriginal();
}

int32_t ArchiveDialog::DrawBaseDialog()
{
  auto progressActive = IsInProgress();
  if (progressActive)
  {
    using namespace std::chrono_literals;

    if (progressTask.wait_for(1ms) == std::future_status::ready)
    {
      progressTask.get();
      progressActive = false;
    }
  }

  const auto archivePath = progressActive ? "" : path;
  const auto displayPath = archivePath.empty() ? g_LocalizationManager.Localize("ARCHIVE_DIALOG_PROCESSING") : StringView8(archivePath);
  bool* openedPtr = progressActive ? nullptr : &opened;

  const auto shouldExit = [&]
    {
      if (!opened)
      {
        if (progressActive)
        {
          opened = true;
          return false;
        }

        switch (UnsavedChangesPopup())
        {
          case 1: {
            Save(GetPath(), false);
            return true;
          }
          case 0: {
            return true;
          }
          default:
          case -1: {
            opened = true;
            return false;
          }
        }
      }

      return false;
    };

  if (shouldExit())
    return -1;

  if (!ImGui::Begin(Format("{}##{}", displayPath, (void*)this).c_str(), openedPtr))
  {
    ImGui::End();

    return shouldExit() ? -1 : 0;
  }

  if (progressActive)
  {
    std::unique_lock progressMessageLock(progressMessageMutex);

    auto progressTotal = progressNextTotal.load();
    if (progressTotal > 1)
    {
      auto progressCurrent = progressNext.load();

      const auto& progressSummary = g_LocalizationManager.LocalizeFormat("ARCHIVE_DIALOG_PROGRESS_SUMMARY", progressCurrent, progressTotal);
      if (progressMessage.empty())
        ImGui::TextUnformatted(progressSummary.c_str());
      else
        ImGui::TextUnformatted(Format("{}\n{}", progressSummary, progressMessage).c_str());
    }
    else if (!progressMessage.empty())
      ImGui::TextUnformatted(progressMessage.c_str());
  }
  else if (!archivePath.empty())
  {
    if (ImGui::BeginChild("##FileList", {}, false, ImGuiWindowFlags_HorizontalScrollbar))
      archiveRoot.DrawTree();

    ImGui::EndChild();
  }
  else
    ImGui::TextUnformatted(g_LocalizationManager.Localize("ARCHIVE_DIALOG_NO_ARCHIVE").c_str());

  const auto isFocused = ImGui::IsWindowFocused();

  ImGui::End();

  return isFocused ? 1 : 0;
}
