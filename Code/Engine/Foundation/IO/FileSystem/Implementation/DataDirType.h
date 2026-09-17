#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/FileEnums.h>
#include <Foundation/Strings/String.h>

class WDataDirectoryReaderWriterBase;
class WDataDirectoryReader;
class WDataDirectoryWriter;
struct WFileStats;
class WDataDirectoryType;

/// Describes in which mode a data directory is mounted.
enum class WDataDirUsage
{
  ReadOnly,
  AllowWrites,
};

struct WDataDirectoryInfo
{
  WDataDirUsage m_Usage;

  WString m_sRootName;
  WString m_sGroup;
  WDataDirectoryType* m_pDataDirType = nullptr;
};

/// The base class for all data directory types.
///
/// There are different data directory types, such as a simple folder, a ZIP file or some kind of library
/// (e.g. image files from procedural data). Even a HTTP server that actually transmits files over a network
/// can provided by implementing it as a data directory type.
/// Data directories are added through WFileSystem, which uses factories to decide which WDataDirectoryType
/// to use for handling which data directory.
class W_FOUNDATION_DLL WDataDirectoryType
{
  W_DISALLOW_COPY_AND_ASSIGN(WDataDirectoryType);

public:
  WDataDirectoryType() = default;
  virtual ~WDataDirectoryType() = default;

  /// Returns the absolute path to the data directory.
  const WString128& GetDataDirectoryPath() const { return m_sDataDirectoryPath; }

  /// By default this is the same as GetDataDirectoryPath(), but derived implementations may use a different location where they
  /// actually get the files from.
  virtual const WString128& GetRedirectedDataDirectoryPath() const { return GetDataDirectoryPath(); }

  /// Some data directory types may use external configuration files (e.g. asset lookup tables)
  ///        that may get updated, while the directory is mounted. This function allows each directory type to implement
  ///        reloading and reapplying of configurations, without dismounting and remounting the data directory.
  virtual void ReloadExternalConfigs() {};

protected:
  friend class WFileSystem;

  /// Tries to setup the data directory. Can fail, if the type is incorrect (e.g. a ZIP file data directory type cannot handle a
  /// simple folder and vice versa)
  WResult InitializeDataDirectory(WStringView sDataDirPath);

  /// Must be implemented to create a WDataDirectoryReader for accessing the given file. Returns nullptr if the file could not be
  /// opened.
  ///
  /// \param szFile is given as a path relative to the data directory's path.
  /// So unless the data directory path is empty, this will never be an absolute path.
  /// If a rooted path was given, the root name is also removed and only the relative part is passed along.
  /// \param bSpecificallyThisDataDir This is true when the original path specified to open the file through exactly this data directory,
  /// by using a rooted path.
  /// If an absolute path is used, which incidentally matches the prefix of this data directory, bSpecificallyThisDataDir is NOT set to
  /// true, as there might be other data directories that also match.
  virtual WDataDirectoryReader* OpenFileToRead(WStringView sFile, WFileShareMode::Enum FileShareMode, bool bSpecificallyThisDataDir) = 0;

  /// Must be implemented to create a WDataDirectoryWriter for accessing the given file. Returns nullptr if the file could not be
  /// opened.
  ///
  /// If it always returns nullptr (default) the data directory is read-only (at least through this type).
  virtual WDataDirectoryWriter* OpenFileToWrite(WStringView sFile, WFileShareMode::Enum FileShareMode)
  {
    W_IGNORE_UNUSED(sFile);
    W_IGNORE_UNUSED(FileShareMode);
    return nullptr;
  }

  /// This function is called by the filesystem when a data directory is removed.
  ///
  /// It should delete itself using the proper allocator.
  virtual void RemoveDataDirectory() = 0;

  /// If a Data Directory Type supports it, this function will remove the given file from it.
  virtual void DeleteFile(WStringView sFile) { W_IGNORE_UNUSED(sFile); }

  /// This function checks whether the given file exists in this data directory.
  ///
  /// The default implementation simply calls WOSFile::ExistsFile
  /// An optimized implementation might look this information up in some hash-map.
  virtual bool ExistsFile(WStringView sFile, bool bOneSpecificDataDir);

  /// Upon success returns the WFileStats for a file in this data directory.
  virtual WResult GetFileStats(WStringView sFileOrFolder, bool bOneSpecificDataDir, WFileStats& out_Stats) = 0;

  /// If this data directory knows how to redirect the given path, it should do so and return true.
  /// Called by WFileSystem::ResolveAssetRedirection
  virtual bool ResolveAssetRedirection(WStringView sPathOrAssetGuid, WStringBuilder& out_sRedirection)
  {
    out_sRedirection = sPathOrAssetGuid;
    return false;
  }

protected:
  friend class WDataDirectoryReaderWriterBase;

  /// This is automatically called whenever a WDataDirectoryReaderWriterBase that was opened by this type is being closed.
  ///
  /// It allows the WDataDirectoryType to return the reader/writer to a pool of reusable objects, or to destroy it
  /// using the proper allocator.
  virtual void OnReaderWriterClose(WDataDirectoryReaderWriterBase* pClosed) { W_IGNORE_UNUSED(pClosed); }

  /// This function should only be used by a Factory (which should be a static function in the respective WDataDirectoryType).
  ///
  /// It is used to initialize the data directory. If this WDataDirectoryType cannot handle the given type,
  /// it must return W_FAILURE and the Factory needs to clean it up properly.
  virtual WResult InternalInitializeDataDirectory(WStringView sDirectory) = 0;

  /// Derived classes can use 'GetDataDirectoryPath' to access this data.
  WString128 m_sDataDirectoryPath;
};



/// This is the base class for all data directory readers/writers.
///
/// Different data directory types (ZIP file, simple folder, etc.) use different reader/writer types.
class W_FOUNDATION_DLL WDataDirectoryReaderWriterBase
{
  W_DISALLOW_COPY_AND_ASSIGN(WDataDirectoryReaderWriterBase);

public:
  /// The derived class should pass along whether it is a reader or writer.
  WDataDirectoryReaderWriterBase(WInt32 iDataDirUserData, bool bIsReader);

  virtual ~WDataDirectoryReaderWriterBase() = default;

  /// Used by WDataDirectoryType's to try to open the given file. They need to pass along their own pointer.
  WResult Open(WStringView sFile, WDataDirectoryType* pOwnerDataDirectory, WFileShareMode::Enum fileShareMode);

  /// Closes this data stream.
  void Close();

  /// Returns the relative path of this file within the owner data directory.
  const WString128& GetFilePath() const;

  /// Returns the pointer to the data directory, which created this reader/writer.
  WDataDirectoryType* GetDataDirectory() const;

  /// Returns true if this is a reader stream, false if it is a writer stream.
  bool IsReader() const { return m_bIsReader; }

  /// Returns the current total size of the file.
  virtual WUInt64 GetFileSize() const = 0;

  WInt32 GetDataDirUserData() const { return m_iDataDirUserData; }

protected:
  /// This function must be implemented by the derived class.
  virtual WResult InternalOpen(WFileShareMode::Enum FileShareMode) = 0;

  /// This function must be implemented by the derived class.
  virtual void InternalClose() = 0;

  bool m_bIsReader;
  WInt32 m_iDataDirUserData = 0;
  WDataDirectoryType* m_pDataDirType;
  WString128 m_sFilePath;
};

/// A base class for readers that handle reading from a (virtual) file inside a data directory.
///
/// Different data directory types (ZIP file, simple folder, etc.) use different reader/writer types.
class W_FOUNDATION_DLL WDataDirectoryReader : public WDataDirectoryReaderWriterBase
{
  W_DISALLOW_COPY_AND_ASSIGN(WDataDirectoryReader);

public:
  WDataDirectoryReader(WInt32 iDataDirUserData)
    : WDataDirectoryReaderWriterBase(iDataDirUserData, true)
  {
  }

  virtual WUInt64 Read(void* pBuffer, WUInt64 uiBytes) = 0;

  /// Helper method to skip a number of bytes (implementations of the directory reader may implement this more efficiently for example)
  virtual WUInt64 Skip(WUInt64 uiBytes)
  {
    WUInt8 uiTempBuffer[1024];

    WUInt64 uiBytesSkipped = 0;

    while (uiBytesSkipped < uiBytes)
    {
      WUInt64 uiBytesToRead = WMath::Min<WUInt64>(uiBytes - uiBytesSkipped, 1024);

      WUInt64 uiBytesRead = Read(uiTempBuffer, uiBytesToRead);

      uiBytesSkipped += uiBytesRead;

      // Terminate early if the stream didn't read as many bytes as we requested (EOF for example)
      if (uiBytesRead < uiBytesToRead)
        break;
    }

    return uiBytesSkipped;
  }
};

/// A base class for writers that handle writing to a (virtual) file inside a data directory.
///
/// Different data directory types (ZIP file, simple folder, etc.) use different reader/writer types.
class W_FOUNDATION_DLL WDataDirectoryWriter : public WDataDirectoryReaderWriterBase
{
  W_DISALLOW_COPY_AND_ASSIGN(WDataDirectoryWriter);

public:
  WDataDirectoryWriter(WInt32 iDataDirUserData)
    : WDataDirectoryReaderWriterBase(iDataDirUserData, false)
  {
  }

  virtual WResult Write(const void* pBuffer, WUInt64 uiBytes) = 0;
};


#include <Foundation/IO/FileSystem/Implementation/DataDirType_inl.h>
