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

  ArchiveDirectory& GetDirectory(std::vector<StringView8CI>& pathStems);
  ArchiveDirectory& GetDirectory(StringView8CI path);

  ArchiveFile& GetFile(std::vector<StringView8CI>& pathStems);
  ArchiveFile& GetFile(StringView8CI path);

  bool IsDirty() const;
  bool IsOriginal() const;

  void CleanDirty();
  void CleanOriginal();

  void DrawTree(StringView8CI thisPath = "") const;

  std::map<StringView8CI, ArchiveDirectory> directories;
  std::map<StringView8CI, ArchiveFile> files;
};

class ArchiveDialog
{
public:
  virtual ~ArchiveDialog() = default;

  virtual bool Clear(bool retVal = false);

  virtual bool LoadImpl(StringView8CI loadPath) = 0;
  bool Load(StringView8CI loadPath);
  bool GetAndLoad(StringView8CI filters, StringView8CI defaultFilename);

  virtual bool ImportSingle(StringView8CI importFolderPath, StringView8CI importFilePath) = 0;
  bool Import(StringView8CI importFolderPath);
  bool GetAndImport();

  virtual bool ExportSingle(StringView8CI exportFolderPath, StringView8CI exportFilePath) const = 0;
  bool Export(StringView8CI exportFolderPath);
  bool GetAndExport();

  virtual bool SaveImpl(StringView8CI savePath) = 0;
  bool Save(StringView8CI savePath, bool async);
  bool GetAndSave(StringView8CI filters, StringView8CI defaultFilename);

  int32_t UnsavedChangesPopup() const;

  virtual void DrawDialog() = 0;

  StringView8CI GetPath() const;

  bool IsInProgress() const;

protected:
  void DrawBaseDialog(StringView8CI dialogName, StringView8CI filters, StringView8CI defaultFilename);

  String8CI path;
  std::set<String8CI> archivePaths;
  ArchiveDirectory archiveRoot;

  std::recursive_mutex progressMessageMutex;
  String8 progressMessage;
  std::atomic_uint64_t progressNext = 0;
  std::atomic_uint64_t progressNextTotal = 0;
  std::atomic_bool progressNextActive = false;
};
