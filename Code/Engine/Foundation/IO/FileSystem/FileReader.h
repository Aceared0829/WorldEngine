#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/IO/FileSystem/Implementation/FileReaderWriterBase.h>
#include <Foundation/IO/Stream.h>

/// The default class to use to read data from a file, implements the WStreamReader interface.
///
/// This file reader buffers reads up to a certain amount of bytes (configurable).
/// It closes the file automatically once it goes out of scope.
class W_FOUNDATION_DLL WFileReader : public WFileReaderBase
{
  W_DISALLOW_COPY_AND_ASSIGN(WFileReader);

public:
  /// Constructor, does nothing.
  WFileReader()

    = default;

  /// Destructor, closes the file, if it is still open (RAII).
  ~WFileReader() { Close(); }

  /// Opens the given file for reading. Returns W_SUCCESS if the file could be opened. A cache is created to speed up small reads.
  ///
  /// You should typically not disable bAllowFileEvents, unless you need to prevent recursive file events,
  /// which is only the case, if you are doing file accesses from within a File Event Handler.
  WResult Open(WStringView sFile, WUInt32 uiCacheSize = 1024 * 64, WFileShareMode::Enum fileShareMode = WFileShareMode::Default, bool bAllowFileEvents = true);

  /// Closes the file, if it is open.
  void Close();

  /// Attempts to read the given number of bytes into the buffer. Returns the actual number of bytes read.
  virtual WUInt64 ReadBytes(void* pReadBuffer, WUInt64 uiBytesToRead) override;

  /// Helper method to skip a number of bytes. Returns the actual number of bytes skipped.
  virtual WUInt64 SkipBytes(WUInt64 uiBytesToSkip) override;
  /// Whether the end of the file was reached during reading.
  ///
  /// \note This is not 100% accurate, it does not guarantee that if it returns false, that the next read will return any data.
  bool IsEOF() const { return m_bEOF; }

private:
  WUInt64 m_uiBytesCached = 0;
  WUInt64 m_uiCacheReadPosition = 0;
  WDynamicArray<WUInt8> m_Cache;
  bool m_bEOF = true;
};
