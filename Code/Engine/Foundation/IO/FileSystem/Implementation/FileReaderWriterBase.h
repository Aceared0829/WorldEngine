#pragma once

#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>
#include <Foundation/IO/Stream.h>

/// The base class for all file readers.
/// Provides access to WFileSystem::GetFileReader, which is necessary to get access to the streams that
/// WDataDirectoryType's provide.
/// Derive from this class if you want to implement different policies on how to read files.
/// E.g. the default reader (WFileReader) implements a buffered read policy (using an internal cache).
class W_FOUNDATION_DLL WFileReaderBase : public WStreamReader
{
  W_DISALLOW_COPY_AND_ASSIGN(WFileReaderBase);

public:
  WFileReaderBase() { m_pDataDirReader = nullptr; }

  /// Returns the absolute path with which the file was opened (including the prefix of the data directory).
  WString128 GetFilePathAbsolute() const
  {
    WStringBuilder sAbs = m_pDataDirReader->GetDataDirectory()->GetRedirectedDataDirectoryPath();
    sAbs.AppendPath(m_pDataDirReader->GetFilePath().GetView());
    return sAbs;
  }

  /// Returns the relative path of the file within its data directory (excluding the prefix of the data directory).
  WString128 GetFilePathRelative() const { return m_pDataDirReader->GetFilePath(); }

  /// Returns the WDataDirectoryType over which this file has been opened.
  WDataDirectoryType* GetDataDirectory() const { return m_pDataDirReader->GetDataDirectory(); }

  /// Returns true, if the file is currently open.
  bool IsOpen() const { return m_pDataDirReader != nullptr; }

  /// Returns the current total size of the file.
  WUInt64 GetFileSize() const { return m_pDataDirReader->GetFileSize(); }

protected:
  WDataDirectoryReader* GetFileReader(WStringView sFile, WFileShareMode::Enum FileShareMode, bool bAllowFileEvents)
  {
    return WFileSystem::GetFileReader(sFile, FileShareMode, bAllowFileEvents);
  }

  WDataDirectoryReader* m_pDataDirReader;
};


/// The base class for all file writers.
/// Provides access to WFileSystem::GetFileWriter, which is necessary to get access to the streams that
/// WDataDirectoryType's provide.
/// Derive from this class if you want to implement different policies on how to write files.
/// E.g. the default writer (WFileWriter) implements a buffered write policy (using an internal cache).
class W_FOUNDATION_DLL WFileWriterBase : public WStreamWriter
{
  W_DISALLOW_COPY_AND_ASSIGN(WFileWriterBase);

public:
  WFileWriterBase() { m_pDataDirWriter = nullptr; }

  /// Returns the absolute path with which the file was opened (including the prefix of the data directory).
  WString128 GetFilePathAbsolute() const
  {
    WStringBuilder sAbs = m_pDataDirWriter->GetDataDirectory()->GetRedirectedDataDirectoryPath();
    sAbs.AppendPath(m_pDataDirWriter->GetFilePath().GetView());
    return sAbs;
  }

  /// Returns the relative path of the file within its data directory (excluding the prefix of the data directory).
  WString128 GetFilePathRelative() const { return m_pDataDirWriter->GetFilePath(); }

  /// Returns the WDataDirectoryType over which this file has been opened.
  WDataDirectoryType* GetDataDirectory() const { return m_pDataDirWriter->GetDataDirectory(); }

  /// Returns true, if the file is currently open.
  bool IsOpen() const { return m_pDataDirWriter != nullptr; }

  /// Returns the current total size of the file.
  WUInt64 GetFileSize() const { return m_pDataDirWriter->GetFileSize(); } // [tested]

protected:
  WDataDirectoryWriter* GetFileWriter(WStringView sFile, WFileShareMode::Enum FileShareMode, bool bAllowFileEvents)
  {
    return WFileSystem::GetFileWriter(sFile, FileShareMode, bAllowFileEvents);
  }

  WDataDirectoryWriter* m_pDataDirWriter;
};
