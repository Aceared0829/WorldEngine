#pragma once

#include <FileservePlugin/Client/FileserveClient.h>
#include <Foundation/Communication/RemoteInterface.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Types/UniquePtr.h>

namespace WDataDirectory
{
  class FileserveDataDirectoryReader : public FolderReader
  {
  public:
    FileserveDataDirectoryReader(WInt32 iDataDirUserData);

  protected:
    virtual WResult InternalOpen(WFileShareMode::Enum FileShareMode) override;
  };

  class FileserveDataDirectoryWriter : public FolderWriter
  {
  protected:
    virtual void InternalClose() override;
  };

  /// A data directory type to handle access to files that are served from a network host.
  class W_FILESERVEPLUGIN_DLL FileserveType : public FolderType
  {
  public:
    /// The factory that can be registered at WFileSystem to create data directories of this type.
    static WDataDirectoryType* Factory(WStringView sDataDirectory, WStringView sGroup, WStringView sRootName, WDataDirUsage usage);

    /// [internal] Makes sure the redirection config files are up to date and then reloads them.
    virtual void ReloadExternalConfigs() override;

    /// [internal] Called by FileserveDataDirectoryWriter when it is finished to upload the written file to the server
    void FinishedWriting(FolderWriter* pWriter);

  protected:
    virtual WDataDirectoryReader* OpenFileToRead(WStringView sFile, WFileShareMode::Enum FileShareMode, bool bSpecificallyThisDataDir) override;
    virtual WDataDirectoryWriter* OpenFileToWrite(WStringView sFile, WFileShareMode::Enum FileShareMode) override;
    virtual WResult InternalInitializeDataDirectory(WStringView sDirectory) override;
    virtual void RemoveDataDirectory() override;
    virtual void DeleteFile(WStringView sFile) override;
    virtual bool ExistsFile(WStringView sFile, bool bOneSpecificDataDir) override;
    /// Limitation: Fileserve does not handle folders, only files. If someone stats a folder, this will fail.
    virtual WResult GetFileStats(WStringView sFileOrFolder, bool bOneSpecificDataDir, WFileStats& out_Stats) override;
    virtual FolderReader* CreateFolderReader() const override;
    virtual FolderWriter* CreateFolderWriter() const override;

    WUInt16 m_uiDataDirID = 0xffff;
    WString128 m_sFileserveCacheMetaFolder;
  };
} // namespace WDataDirectory
