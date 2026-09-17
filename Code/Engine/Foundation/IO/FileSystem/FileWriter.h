#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/IO/FileSystem/Implementation/FileReaderWriterBase.h>
#include <Foundation/IO/Stream.h>

/// The default class to use to write data to a file, implements the WStreamWriter interface.
///
/// This file writer buffers writes up to a certain amount of bytes (configurable).
/// It closes the file automatically once it goes out of scope.
class W_FOUNDATION_DLL WFileWriter : public WFileWriterBase
{
  W_DISALLOW_COPY_AND_ASSIGN(WFileWriter);

public:
  /// Constructor, does nothing.
  WFileWriter() = default;

  /// Destructor, closes the file, if it is still open (RAII).
  ~WFileWriter() { Close(); }

  /// Opens the given file for writing. Returns W_SUCCESS if the file could be opened. A cache is created to speed up small writes.
  ///
  /// You should typically not disable bAllowFileEvents, unless you need to prevent recursive file events,
  /// which is only the case, if you are doing file accesses from within a File Event Handler.
  WResult Open(WStringView sFile, WUInt32 uiCacheSize = 1024 * 1024, WFileShareMode::Enum fileShareMode = WFileShareMode::Default,
    bool bAllowFileEvents = true);

  /// Closes the file, if it is open.
  void Close();

  /// Writes the given number of bytes to the file. Returns W_SUCCESS if all bytes were successfully written.
  ///
  /// As this class buffers writes with an internal cache, W_SUCCESS does NOT mean that the data is actually written to disk.
  virtual WResult WriteBytes(const void* pWriteBuffer, WUInt64 uiBytesToWrite) override;

  /// Will write anything that's currently in the write-cache to disk. Will decrease performance if used excessively.
  ///
  /// \note Flush only guarantees that the data is sent through the OS file functions. It does not guarantee that the OS
  /// actually wrote the data on the disk, it might still use buffer itself and thus an application that crashes might
  /// still see data loss even when 'Flush' had been called.
  virtual WResult Flush() override;

private:
  WUInt64 m_uiCacheWritePosition;
  WDynamicArray<WUInt8> m_Cache;
};
