//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//
// TODO - access to path member is not thread safe!
//

#include "Precompiled.hpp"

#include "ArchiveDialog.hpp"

bool ArchiveDirectory::Clear(const bool retVal)
{
  directories.clear();
  files.clear();

  return retVal;
}

ArchiveDirectory& ArchiveDirectory::GetDirectory(const StringView8CI path, OrderedSet<String8CI>& archivePaths)
{
  auto pathStems = GetPathStems(path);
  return GetDirectory(pathStems, path, archivePaths);
}

ArchiveDirectory& ArchiveDirectory::GetDirectory(std::vector<StringView8CI> &pathStems, StringView8CI path, OrderedSet<String8CI>& archivePaths)
{
  assert(!pathStems.empty());

  if (pathStems.size() > 1)
  {
    const auto directoryIt = directories.find(pathStems.back());
    if (directoryIt != directories.cend())
    {
      pathStems.pop_back();
      return directoryIt->second.GetDirectory(pathStems, path, archivePaths);
    }

    auto newPathStems = GetPathStems(*archivePaths.emplace(path).first);
    while (newPathStems.size() > pathStems.size())
      newPathStems.pop_back();

    std::swap(pathStems, newPathStems);

    auto& directory = directories[pathStems.back()];
    pathStems.pop_back();
    return directory.GetDirectory(pathStems, path, archivePaths);
  }

  const auto directoryIt = directories.find(pathStems.front());
  if (directoryIt != directories.cend())
    return directoryIt->second;

  path = *archivePaths.emplace(path).first;
  const auto newPathStems = GetPathStems(path);
  auto& directory = directories[newPathStems.front()];
  directory.path = path;
  return directory;
}

ArchiveFile& ArchiveDirectory::GetFile(const StringView8CI path, OrderedSet<String8CI>& archivePaths)
{
  auto pathStems = GetPathStems(path);
  return GetFile(pathStems, path, archivePaths);
}

ArchiveFile& ArchiveDirectory::GetFile(std::vector<StringView8CI>& pathStems, StringView8CI path, OrderedSet<String8CI>& archivePaths)
{
  assert(!pathStems.empty());

  if (pathStems.size() > 1)
  {
    const auto directoryIt = directories.find(pathStems.back());
    if (directoryIt != directories.cend())
    {
      pathStems.pop_back();
      return directoryIt->second.GetFile(pathStems, path, archivePaths);
    }

    auto newPathStems = GetPathStems(*archivePaths.emplace(path).first);
    while (newPathStems.size() > pathStems.size())
      newPathStems.pop_back();

    std::swap(pathStems, newPathStems);

    auto& directory = directories[pathStems.back()];
    pathStems.pop_back();
    return directory.GetFile(pathStems, path, archivePaths);
  }

  const auto fileIt = files.find(pathStems.front());
  if (fileIt != files.cend())
    return fileIt->second;

  path = *archivePaths.emplace(path).first;
  const auto newPathStems = GetPathStems(path);
  auto& file = files[newPathStems.front()];
  file.path = path;
  return file;
}

bool ArchiveDirectory::IsDirty() const
{
  for (const auto &file : files | std::views::values)
  {
    if (file.dirty)
      return true;
  }

  for (const auto &directory : directories | std::views::values)
  {
    if (directory.IsDirty())
      return true;
  }

  return false;
}

bool ArchiveDirectory::IsOriginal() const
{
  for (const auto &file : files | std::views::values)
  {
    if (!file.original)
      return false;
  }

  for (const auto &directory : directories | std::views::values)
  {
    if (!directory.IsOriginal())
      return false;
  }

  return true;
}

void ArchiveDirectory::CleanDirty()
{
  for (auto &directory : directories | std::views::values)
    directory.CleanDirty();

  for (auto &file : files | std::views::values)
    file.dirty = false;
}

void ArchiveDirectory::CleanOriginal()
{
  for (auto &directory : directories | std::views::values)
    directory.CleanOriginal();

  for (auto &file : files | std::views::values)
    file.original = true;
}

void ArchiveDirectory::DrawTree(const StringView8CI thisPath) const
{
  if (!thisPath.empty() && !ImGui::TreeNode(String8(thisPath).c_str()))
    return;

  const auto checkOriginality = Options::Get().common.checkOriginality;

  for (const auto& [directoryPath, directory] : directories)
  {
    if (directory.IsDirty())
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1.0f, 1.0f, 0.0f, 1.0f});
    else if (checkOriginality && !directory.IsOriginal())
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.0f, 1.0f, 0.0f, 1.0f});

    directory.DrawTree(directoryPath);

    if (directory.IsDirty() || (checkOriginality && !directory.IsOriginal()))
      ImGui::PopStyleColor();
  }

  for (const auto& [filePath, file] : files)
  {
    if (file.dirty)
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1.0f, 1.0f, 0.0f, 1.0f});
    else if (checkOriginality && !file.original)
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.0f, 1.0f, 0.0f, 1.0f});

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    ImGui::Selectable(String8(filePath).c_str());

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);

    if (file.dirty || (checkOriginality && !file.original))
      ImGui::PopStyleColor();
  }

  if (!thisPath.empty())
    ImGui::TreePop();
}

bool ArchiveDialog::Clear(const bool retVal)
{
  path.clear();
  archiveRoot.Clear();

  return retVal;
}

bool ArchiveDialog::Load(StringView8CI loadPath)
{
  if (loadPath.empty())
    return false;

  GlyphRangesBuilder::Get().AddText(loadPath);

  progressMessage = LocalizationManager::Get().Localize("ARCHIVE_DIALOG_LOAD_PROGRESS_READING_ARCHIVE");
  progressNext = 0;
  progressNextTotal = 1;

  progressTask = std::async(std::launch::async, [loadPath = String8CI(loadPath), this] {
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

    if (LoadImpl(loadPath))
    {
      // TODO - this is causing a lot of synchronizations at the end of loading
      //        best would be to do this on the main thread
      for (const auto& archivePath : archivePaths)
        GlyphRangesBuilder::Get().AddText(archivePath);

      path = loadPath;
    }
    else
      Clear();

    std::unique_lock progressMessageLock(progressMessageMutex);
    progressMessage.clear();
  });

  return true;
}

bool ArchiveDialog::GetAndLoad(const StringView8CI filters, const StringView8CI defaultFilename)
{
  return Load(OpenFileDialog(filters, defaultFilename));
}

bool ArchiveDialog::Import(const StringView8CI importFolderPath)
{
  if (importFolderPath.empty())
    return false;

  progressMessage = LocalizationManager::Get().Localize("ARCHIVE_DIALOG_IMPORT_PROGRESS_IMPORTING_DATA");
  progressNext = 0;
  progressNextTotal = 1;

  progressTask = std::async(std::launch::async, [importFolderPath = String8CI(importFolderPath), this] {
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

    for (const auto &importFilePath : allImportFiles)
    {
      {
        std::unique_lock progressMessageLock(progressMessageMutex);
        progressMessage = LocalizationManager::Get().LocalizeFormat("ARCHIVE_DIALOG_IMPORT_PROGRESS_IMPORTING_FILE", String8(relative(importFilePath.path(), importFolderPath.path())));
        ++progressNext;
      }

      ImportSingle(importFolderPath, importFilePath);
    }

    std::unique_lock progressMessageLock(progressMessageMutex);
    progressMessage.clear();
  });

  return true;
}

bool ArchiveDialog::GetAndImport()
{
  return Import(BrowseDirectoryDialog());
}

bool ArchiveDialog::Export(const StringView8CI exportFolderPath)
{
  if (exportFolderPath.empty())
    return false;

  progressMessage = LocalizationManager::Get().Localize("ARCHIVE_DIALOG_EXPORT_PROGRESS_EXPORTING_DATA");
  progressNext = 0;
  progressNextTotal = 1;

  progressTask = std::async(std::launch::async, [exportFolderPath = String8CI(exportFolderPath), this] {
    if (archivePaths.empty())
    {
      std::unique_lock progressMessageLock(progressMessageMutex);
      progressMessage.clear();
      progressNext = 1;
      return;
    }

    progressNextTotal = archivePaths.size();
    progressNext = 0;

    for (const auto &exportFilePath : archivePaths)
    {
      {
        std::unique_lock progressMessageLock(progressMessageMutex);
        progressMessage = LocalizationManager::Get().LocalizeFormat("ARCHIVE_DIALOG_IMPORT_PROGRESS_EXPORTING_FILE", exportFilePath);
        ++progressNext;
      }

      ExportSingle(exportFolderPath, exportFilePath);
    }

    std::unique_lock progressMessageLock(progressMessageMutex);
    progressMessage.clear();
  });

  return true;
}

bool ArchiveDialog::GetAndExport()
{
  return Export(BrowseDirectoryDialog());
}

bool ArchiveDialog::Save(const StringView8CI savePath, bool async)
{
  if (savePath.empty())
    return false;

  GlyphRangesBuilder::Get().AddText(savePath);

  progressMessage = LocalizationManager::Get().Localize("ARCHIVE_DIALOG_SAVE_PROGRESS_SAVING_ARCHIVE");
  progressNext = 0;
  progressNextTotal = 1;

  progressTask = std::async(std::launch::async, [savePath = String8CI(savePath), this] {
    if (archivePaths.empty())
    {
      std::unique_lock progressMessageLock(progressMessageMutex);
      progressMessage.clear();
      progressNext = 1;
      return;
    }

    if (SaveImpl(savePath))
      path = savePath;

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

bool ArchiveDialog::GetAndSave(const StringView8CI filters, const StringView8CI defaultFilename)
{
  return Save(SaveFileDialog(filters, defaultFilename), true);
}

int32_t ArchiveDialog::UnsavedChangesPopup() const
{
  if (!archiveRoot.IsDirty())
    return 0;

  const auto msgBoxResult = DisplayWarning(LocalizationManager::Get().Localize("ARCHIVE_DIALOG_UNSAVED_CHANGES_MESSAGE"),
      LocalizationManager::Get().Localize("ARCHIVE_DIALOG_UNSAVED_CHANGES_TITLE"), true);

  switch (msgBoxResult)
  {
    case IDYES: {
      return 1;
    }
    default:
    case IDCLOSE:
    case IDNO: {
      return 0;
    }
    case IDCANCEL: {
      return -1;
    }
  }
}

StringView8CI ArchiveDialog::GetPath() const
{
  return path;
}

bool ArchiveDialog::IsInProgress() const
{
  return progressTask.valid();
}

ArchiveDirectory& ArchiveDialog::GetDirectory(StringView8CI path)
{
  return archiveRoot.GetDirectory(path, archivePaths);
}

ArchiveFile& ArchiveDialog::GetFile(StringView8CI path)
{
  return archiveRoot.GetFile(path, archivePaths);
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

void ArchiveDialog::DrawBaseDialog(const StringView8CI dialogName, const StringView8CI filters, const StringView8CI defaultFilename)
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

  auto wasProgressActive = progressActive;

  const auto archivePath = progressActive ? "" : path;

  if (!ImGui::BeginTabItem(dialogName.c_str()))
    return;

  const auto itemWidth = GetAlignedItemWidth(5);

  if (progressActive)
    ImGui::BeginDisabled();

  if (ImGui::Button(LocalizationManager::Get().Localize("ARCHIVE_DIALOG_OPEN").c_str(), ImVec2(itemWidth, 0)))
    GetAndLoad(filters, defaultFilename);

  progressActive = IsInProgress();
  if (!wasProgressActive && (progressActive || archivePath.empty()))
  {
    wasProgressActive |= progressActive;
    ImGui::BeginDisabled();
  }

  ImGui::SameLine();

  if (ImGui::Button(LocalizationManager::Get().Localize("ARCHIVE_DIALOG_SAVE").c_str(), ImVec2(itemWidth, 0)))
    Save(archivePath, true);

  progressActive = IsInProgress();
  if (!archivePath.empty() && !wasProgressActive && progressActive)
  {
    wasProgressActive |= progressActive;
    ImGui::BeginDisabled();
  }

  ImGui::SameLine();

  if (ImGui::Button(LocalizationManager::Get().Localize("ARCHIVE_DIALOG_SAVE_INTO").c_str(), ImVec2(itemWidth, 0)))
    GetAndSave(filters, defaultFilename);

  progressActive = IsInProgress();
  if (!archivePath.empty() && !wasProgressActive && progressActive)
  {
    wasProgressActive |= progressActive;
    ImGui::BeginDisabled();
  }

  ImGui::SameLine();

  if (ImGui::Button(LocalizationManager::Get().Localize("ARCHIVE_DIALOG_EXPORT_TO").c_str(), ImVec2(itemWidth, 0)))
    GetAndExport();

  progressActive = IsInProgress();
  if (!archivePath.empty() && !wasProgressActive && progressActive)
  {
    wasProgressActive |= progressActive;
    ImGui::BeginDisabled();
  }

  ImGui::SameLine();

  if (ImGui::Button(LocalizationManager::Get().Localize("ARCHIVE_DIALOG_IMPORT_FROM").c_str(), ImVec2(itemWidth, 0)))
    GetAndImport();

  progressActive = IsInProgress();
  if (!archivePath.empty() && !wasProgressActive && progressActive)
  {
    wasProgressActive |= progressActive;
    ImGui::BeginDisabled();
  }

  if (wasProgressActive)
  {
    ImGui::EndDisabled();

    std::unique_lock progressMessageLock(progressMessageMutex);

    auto progressTotal = progressNextTotal.load();
    if (progressTotal > 1)
    {
      auto progressCurrent = progressNext.load();

      const auto& progressSummary = LocalizationManager::Get().LocalizeFormat("ARCHIVE_DIALOG_PROGRESS_SUMMARY", progressCurrent, progressTotal);
      if (progressMessage.empty())
        ImGui::TextUnformatted(progressSummary.c_str());
      else
        ImGui::TextUnformatted(std::format("{}\n{}", progressSummary, progressMessage).c_str());
    }
    else if (!progressMessage.empty())
      ImGui::TextUnformatted(progressMessage.c_str());
  }
  else
  {
    if (archivePath.empty())
    {
      ImGui::EndDisabled();
      ImGui::TextUnformatted(LocalizationManager::Get().Localize("ARCHIVE_DIALOG_NO_ARCHIVE").c_str());
    }
    else
      ImGui::TextUnformatted(LocalizationManager::Get().LocalizeFormat("ARCHIVE_DIALOG_LOADED_ARCHIVE", archivePath).c_str());

    ImGui::Separator();

    ImGui::BeginChild("##FileList", {}, false, ImGuiWindowFlags_HorizontalScrollbar);

    archiveRoot.DrawTree();

    ImGui::EndChild();
  }

  ImGui::EndTabItem();
}
