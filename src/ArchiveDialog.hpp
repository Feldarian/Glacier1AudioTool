//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#pragma once

struct ArchiveFile
{
  bool dirty = false;
  bool original = true;
};

struct ArchiveDirectory
{
  bool Clear(bool retVal = false);

  ArchiveDirectory& GetDirectory(std::vector<std::wstring_view>& pathStems);
  ArchiveDirectory& GetDirectory(const std::filesystem::path& path);

  ArchiveFile& GetFile(std::vector<std::wstring_view>& pathStems);
  ArchiveFile& GetFile(const std::filesystem::path& path);

  bool IsDirty() const;
  bool IsOriginal() const;

  void CleanDirty();
  void CleanOriginal();

  void DrawTree(std::wstring_view thisPath = L"") const;

  UTFViewToTypeMapCI<wchar_t, ArchiveDirectory> directories;
  UTFViewToTypeMapCI<wchar_t, ArchiveFile> files;
};

class ArchiveDialog
{
public:
  virtual ~ArchiveDialog() = default;

  virtual bool Clear(bool retVal = false);

  virtual bool LoadImpl(const std::filesystem::path &loadPath) = 0;
  bool Load(const std::filesystem::path &loadPath);
  bool GetAndLoad(std::wstring_view filters, std::wstring_view defaultFilename);

  virtual bool ImportSingle(const std::filesystem::path &importFolderPath, const std::filesystem::path &importFilePath) = 0;
  bool Import(const std::filesystem::path &importFolderPath);
  bool GetAndImport();

  virtual bool ExportSingle(const std::filesystem::path &exportFolderPath, const std::filesystem::path &exportFilePath) const = 0;
  bool Export(const std::filesystem::path &exportFolderPath);
  bool GetAndExport();

  virtual bool SaveImpl(const std::filesystem::path &savePath) = 0;
  bool Save(const std::filesystem::path &savePath, bool async);
  bool GetAndSave(std::wstring_view filters, std::wstring_view defaultFilename);

  int32_t UnsavedChangesPopup() const;

  virtual void DrawDialog() = 0;

  const std::filesystem::path& GetPath() const;

  bool IsInProgress() const;

protected:
  void DrawBaseDialog(std::wstring_view dialogName, std::wstring_view filters, std::wstring_view defaultFilename);

  std::filesystem::path path;
  PathSetCI archivePaths;
  ArchiveDirectory archiveRoot;

  std::recursive_mutex progressMessageMutex;
  std::string progressMessage;
  std::atomic_uint64_t progressNext = 0;
  std::atomic_uint64_t progressNextTotal = 0;
  std::atomic_bool progressNextActive = false;
};
