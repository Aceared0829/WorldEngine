#pragma once

#include <Foundation/IO/Archive/ArchiveReader.h>
#include <Foundation/IO/CompressedStreamZlib.h>
#include <Foundation/IO/CompressedStreamZstd.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Time/Timestamp.h>

class WArchiveEntry;

namespace WDataDirectory
{
  class ArchiveReaderUncompressed;
  class ArchiveReaderZstd;
  class ArchiveReaderZip;

  class W_FOUNDATION_DLL ArchiveType : public WDataDirectoryType
  {
  public:
    ArchiveType();
    ~ArchiveType();

    static WDataDirectoryType* Factory(WStringView sDataDirectory, WStringView sGroup, WStringView sRootName, WDataDirUsage usage);

    virtual const WString128& GetRedirectedDataDirectoryPath() const override { return m_sRedirectedDataDirPath; }

  protected:
    virtual WDataDirectoryReader* OpenFileToRead(WStringView sFile, WFileShareMode::Enum FileShareMode, bool bSpecificallyThisDataDir) override;

    virtual void RemoveDataDirectory() override;

    virtual bool ExistsFile(WStringView sFile, bool bOneSpecificDataDir) override;

    virtual WResult GetFileStats(WStringView sFileOrFolder, bool bOneSpecificDataDir, WFileStats& out_Stats) override;

    virtual WResult InternalInitializeDataDirectory(WStringView sDirectory) override;

    virtual void OnReaderWriterClose(WDataDirectoryReaderWriterBase* pClosed) override;

    WString128 m_sRedirectedDataDirPath;
    WString32 m_sArchiveSubFolder;
    WTimestamp m_LastModificationTime;
    WArchiveReader m_ArchiveReader;

    WMutex m_ReaderMutex;
    WHybridArray<WUniquePtr<ArchiveReaderUncompressed>, 4> m_ReadersUncompressed;
    WHybridArray<ArchiveReaderUncompressed*, 4> m_FreeReadersUncompressed;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
    WHybridArray<WUniquePtr<ArchiveReaderZstd>, 4> m_ReadersZstd;
    WHybridArray<ArchiveReaderZstd*, 4> m_FreeReadersZstd;
#endif
#ifdef BUILDSYSTEM_ENABLE_ZLIB_SUPPORT
    WHybridArray<WUniquePtr<ArchiveReaderZip>, 4> m_ReadersZip;
    WHybridArray<ArchiveReaderZip*, 4> m_FreeReadersZip;
#endif
  };

  class W_FOUNDATION_DLL ArchiveReaderCommon : public WDataDirectoryReader
  {
    W_DISALLOW_COPY_AND_ASSIGN(ArchiveReaderCommon);

  public:
    ArchiveReaderCommon(WInt32 iDataDirUserData);

    virtual WUInt64 GetFileSize() const override;

  protected:
    friend class ArchiveType;

    WUInt64 m_uiUncompressedSize = 0;
    WUInt64 m_uiCompressedSize = 0;
    WRawMemoryStreamReader m_MemStreamReader;
  };

  class W_FOUNDATION_DLL ArchiveReaderUncompressed : public ArchiveReaderCommon
  {
    W_DISALLOW_COPY_AND_ASSIGN(ArchiveReaderUncompressed);

  public:
    ArchiveReaderUncompressed(WInt32 iDataDirUserData);

    virtual WUInt64 Skip(WUInt64 uiBytes) override;
    virtual WUInt64 Read(void* pBuffer, WUInt64 uiBytes) override;

  protected:
    virtual WResult InternalOpen(WFileShareMode::Enum FileShareMode) override;
    virtual void InternalClose() override;
  };

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  class W_FOUNDATION_DLL ArchiveReaderZstd : public ArchiveReaderCommon
  {
    W_DISALLOW_COPY_AND_ASSIGN(ArchiveReaderZstd);

  public:
    ArchiveReaderZstd(WInt32 iDataDirUserData);

    virtual WUInt64 Read(void* pBuffer, WUInt64 uiBytes) override;

  protected:
    virtual WResult InternalOpen(WFileShareMode::Enum FileShareMode) override;
    virtual void InternalClose() override;

    WCompressedStreamReaderZstd m_CompressedStreamReader;
  };
#endif

#ifdef BUILDSYSTEM_ENABLE_ZLIB_SUPPORT
  /// Allows reading of zip / apk containers.
  /// Needed to allow Android to read data from the apk.
  class W_FOUNDATION_DLL ArchiveReaderZip : public ArchiveReaderUncompressed
  {
    W_DISALLOW_COPY_AND_ASSIGN(ArchiveReaderZip);

  public:
    ArchiveReaderZip(WInt32 iDataDirUserData);
    ~ArchiveReaderZip();

    virtual WUInt64 Read(void* pBuffer, WUInt64 uiBytes) override;

  protected:
    virtual WResult InternalOpen(WFileShareMode::Enum FileShareMode) override;

    friend class ArchiveType;

    WCompressedStreamReaderZip m_CompressedStreamReader;
  };
#endif
} // namespace WDataDirectory
