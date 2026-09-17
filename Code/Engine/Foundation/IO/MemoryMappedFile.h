#pragma once

#include <Foundation/Types/UniquePtr.h>

struct WMemoryMappedFileImpl;

/// Platform-abstracted interface for memory-mapped file I/O and shared memory access
///
/// Provides efficient file access by mapping files directly into virtual memory, eliminating
/// the need for explicit read/write operations. Particularly beneficial for large files where
/// random access patterns would cause excessive disk I/O with traditional file operations.
/// Also supports creating shared memory regions for inter-process communication.
///
/// Memory mapping allows the OS to handle paging and caching automatically, improving performance
/// for scenarios like asset loading, database files, or large data processing pipelines.
class W_FOUNDATION_DLL WMemoryMappedFile
{
  W_DISALLOW_COPY_AND_ASSIGN(WMemoryMappedFile);

public:
  WMemoryMappedFile();
  ~WMemoryMappedFile();

  enum class Mode
  {
    None,      ///< Currently no file is mapped
    ReadOnly,  ///< File is mapped for read-only access
    ReadWrite, ///< File is mapped for read/write access
  };

  /// The start point for interpreting byte offsets into the memory
  enum class OffsetBase
  {
    Start, ///< Byte offsets are relative to the start of the mapped memory
    End,   ///< Byte offsets are relative to the end of the mapped memory. Increasing positive values go towards the start
           ///< of the memory.
  };

#if W_ENABLED(W_SUPPORTS_MEMORY_MAPPED_FILE) || defined(W_DOCS)
  /// Attempts to open the given file and map it into memory
  ///
  /// \param szAbsolutePath must be an absolute path to the file that should be mapped.
  ///        The file also must exist and have a size larger than zero bytes.
  /// \param mode How to map the file into memory.
  WResult Open(WStringView sAbsolutePath, Mode mode);
#endif

#if W_ENABLED(W_SUPPORTS_SHARED_MEMORY) || defined(W_DOCS)
  /// Attempts to open or create the given shared memory block addressed by szSharedName
  ///
  /// \param szSharedName The name of the shared memory region.
  /// \param uiSize The size of the memory which should be mapped.
  /// \param mode How to map the file into memory.
  WResult OpenShared(WStringView sSharedName, WUInt64 uiSize, Mode mode);
#endif

  /// Removes the memory mapping. Outstanding modifications will be written back to disk at this point.
  void Close();

  /// Returns the mode with which the file was opened or None, if is currently not in use.
  Mode GetMode() const;

  /// Returns the size (in bytes) of the memory mapping. Zero if no file is mapped at the moment.
  WUInt64 GetFileSize() const;

  /// Returns a pointer for reading the mapped file. Asserts that the memory mapping was done successfully.
  const void* GetReadPointer(WUInt64 uiOffset = 0, OffsetBase base = OffsetBase::Start) const;

  /// Returns a pointer for writing the mapped file. Asserts that the memory mapping was successful and the mode was ReadWrite.
  void* GetWritePointer(WUInt64 uiOffset = 0, OffsetBase base = OffsetBase::Start);

private:
  WUniquePtr<WMemoryMappedFileImpl> m_pImpl;
};
