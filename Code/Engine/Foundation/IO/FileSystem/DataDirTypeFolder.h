#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>
#include <Foundation/IO/OSFile.h>

namespace WDataDirectory
{
  class FolderReader;
  class FolderWriter;

  /// A data directory type to handle access to ordinary files.
  ///
  /// Register the 'Factory' function at WFileSystem to allow it to mount local directories.
  class W_FOUNDATION_DLL FolderType : public WDataDirectoryType
  {
  public:
    ~FolderType();

    /// The factory that can be registered at WFileSystem to create data directories of this type.
    static WDataDirectoryType* Factory(WStringView sDataDirectory, WStringView sGroup, WStringView sRootName, WDataDirUsage usage);

    /// A 'redirection file' is an optional file located inside a data directory that lists which file access is redirected to which other
    /// file lookup. Each redirection is one line in the file (terminated by a \n). Each line consists of the 'key' string, a semicolon and
    /// a 'value' string. No unnecessary whitespace is allowed. When a file that matches 'key' is accessed through a mounted data directory,
    /// the file access will be replaced by 'value' (plus s_sRedirectionPrefix) 'key' may be anything (e.g. a GUID string), 'value' should
    /// be a valid relative path into the SAME data directory. The redirection file can be used to implement an asset lookup, where assets
    /// are identified by GUIDs and need to be mapped to the actual asset file.
    static WString s_sRedirectionFile;

    /// If a redirection file is used AND the redirection lookup was successful, s_sRedirectionPrefix is prepended to the redirected file
    /// access.
    static WString s_sRedirectionPrefix;

    /// When s_sRedirectionFile and s_sRedirectionPrefix are used to enable file redirection, this will reload those config files.
    virtual void ReloadExternalConfigs() override;

    virtual const WString128& GetRedirectedDataDirectoryPath() const override { return m_sRedirectedDataDirPath; }

  protected:
    // The implementations of the abstract functions.

    virtual WDataDirectoryReader* OpenFileToRead(WStringView sFile, WFileShareMode::Enum FileShareMode, bool bSpecificallyThisDataDir) override;

    virtual bool ResolveAssetRedirection(WStringView sPathOrAssetGuid, WStringBuilder& out_sRedirection) override;
    virtual WDataDirectoryWriter* OpenFileToWrite(WStringView sFile, WFileShareMode::Enum FileShareMode) override;
    virtual void RemoveDataDirectory() override;
    virtual void DeleteFile(WStringView sFile) override;
    virtual bool ExistsFile(WStringView sFile, bool bOneSpecificDataDir) override;
    virtual WResult GetFileStats(WStringView sFileOrFolder, bool bOneSpecificDataDir, WFileStats& out_Stats) override;
    virtual FolderReader* CreateFolderReader() const;
    virtual FolderWriter* CreateFolderWriter() const;

    /// Called by 'WDataDirectoryType_Folder::Factory'
    virtual WResult InternalInitializeDataDirectory(WStringView sDirectory) override;

    /// Marks the given reader/writer as reusable.
    virtual void OnReaderWriterClose(WDataDirectoryReaderWriterBase* pClosed) override;

    void LoadRedirectionFile();

    mutable WMutex m_ReaderWriterMutex; ///< Locks m_Readers / m_Writers as well as the m_bIsInUse flag of each reader / writer.
    WHybridArray<WDataDirectory::FolderReader*, 4> m_Readers;
    WHybridArray<WDataDirectory::FolderWriter*, 4> m_Writers;

    mutable WMutex m_RedirectionMutex;
    WMap<WString, WString> m_FileRedirection;
    WString128 m_sRedirectedDataDirPath;
  };


  /// Handles reading from ordinary files.
  class W_FOUNDATION_DLL FolderReader : public WDataDirectoryReader
  {
    W_DISALLOW_COPY_AND_ASSIGN(FolderReader);

  public:
    FolderReader(WInt32 iDataDirUserData)
      : WDataDirectoryReader(iDataDirUserData)
    {
      m_bIsInUse = false;
    }

    virtual WUInt64 Skip(WUInt64 uiBytes) override;
    virtual WUInt64 Read(void* pBuffer, WUInt64 uiBytes) override;
    virtual WUInt64 GetFileSize() const override;

  protected:
    virtual WResult InternalOpen(WFileShareMode::Enum FileShareMode) override;
    virtual void InternalClose() override;

    friend class FolderType;

    bool m_bIsInUse;
    WOSFile m_File;
  };

  /// Handles writing to ordinary files.
  class W_FOUNDATION_DLL FolderWriter : public WDataDirectoryWriter
  {
    W_DISALLOW_COPY_AND_ASSIGN(FolderWriter);

  public:
    FolderWriter(WInt32 iDataDirUserData = 0)
      : WDataDirectoryWriter(iDataDirUserData)
    {
      m_bIsInUse = false;
    }

    virtual WResult Write(const void* pBuffer, WUInt64 uiBytes) override;
    virtual WUInt64 GetFileSize() const override;

  protected:
    virtual WResult InternalOpen(WFileShareMode::Enum FileShareMode) override;
    virtual void InternalClose() override;

    friend class FolderType;

    bool m_bIsInUse;
    WOSFile m_File;
  };
} // namespace WDataDirectory
