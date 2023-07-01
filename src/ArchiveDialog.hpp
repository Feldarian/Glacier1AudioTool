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
  StringView8CI path;
};

class ArchiveDirectory
{
public:
  virtual ~ArchiveDirectory() = default;

  virtual bool Clear(bool retVal = false);

  ArchiveDirectory& GetDirectory(const StringView8CI &searchPath, OrderedSet<String8CI>& archivePaths);
  ArchiveFile& GetFile(const StringView8CI &searchPath, OrderedSet<String8CI>& archivePaths);

  bool IsDirty() const;
  bool IsOriginal() const;

  void CleanDirty();
  void CleanOriginal();

  void DrawTree(const StringView8CI &thisPath = "") const;

private:
  ArchiveDirectory& GetDirectory(std::vector<StringView8CI>& pathStems, StringView8CI searchPath, OrderedSet<String8CI>& archivePaths);
  ArchiveFile& GetFile(std::vector<StringView8CI>& pathStems, StringView8CI searchPath, OrderedSet<String8CI>& archivePaths);

  OrderedMap<StringView8CI, ArchiveDirectory> directories;
  OrderedMap<StringView8CI, ArchiveFile> files;
  StringView8CI path;
};

class ArchiveDialog
{
public:
  virtual ~ArchiveDialog() = default;

  virtual bool Clear(bool retVal = false);

  virtual bool LoadImpl(const StringView8CI &loadPath, const Options &options) = 0;
  bool Load(const StringView8CI &loadPath);
  bool GetAndLoad(const StringView8CI &filters, const StringView8CI &defaultFilename);

  virtual bool ImportSingle(const StringView8CI &importFolderPath, const StringView8CI &importFilePath, const Options &options) = 0;
  bool Import(const StringView8CI &importFolderPath);
  bool GetAndImport();

  virtual bool ExportSingle(const StringView8CI &exportFolderPath, const StringView8CI &exportFilePath, const Options &options) const = 0;
  bool Export(const StringView8CI &exportFolderPath);
  bool GetAndExport();

  virtual bool SaveImpl(const StringView8CI &savePath, const Options &options) = 0;
  bool Save(const StringView8CI &savePath, bool async);
  bool GetAndSave(const StringView8CI &filters, const StringView8CI &defaultFilename);

  int32_t UnsavedChangesPopup() const;

  virtual void DrawDialog() = 0;

  StringView8CI GetPath() const;

  bool IsInProgress() const;

  ArchiveDirectory& GetDirectory(const StringView8CI &searchPath);
  ArchiveFile& GetFile(const StringView8CI &searchPath);
  const OrderedSet<String8CI>& GetPaths() const;

  bool IsDirty() const;
  bool IsOriginal() const;

  void CleanDirty();
  void CleanOriginal();

protected:
  void DrawBaseDialog(const StringView8CI &dialogName, const StringView8CI &filters, const StringView8CI &defaultFilename);

  String8CI path;

  std::recursive_mutex progressMessageMutex;
  String8 progressMessage;
  std::atomic_uint64_t progressNext = 0;
  std::atomic_uint64_t progressNextTotal = 0;
  std::future<void> progressTask;

private:
  ArchiveDirectory archiveRoot;
  OrderedSet<String8CI> archivePaths;
};
