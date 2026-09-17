#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/RefCounted.h>

class WMemoryStreamReader;
class WMemoryStreamWriter;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// Abstract interface for memory stream storage providing pluggable backend implementations.
///
/// MemoryStreamStorageInterface defines the contract for storage backends used by memory streams.
/// Different implementations can optimize for specific use cases such as small temporary buffers,
/// large datasets, or zero-copy operations with existing containers.
class W_FOUNDATION_DLL WMemoryStreamStorageInterface
{
public:
  WMemoryStreamStorageInterface();
  virtual ~WMemoryStreamStorageInterface();

  /// Returns the number of bytes that are currently stored. Asserts that the stored amount is less than 4GB.
  WUInt32 GetStorageSize32() const
  {
    W_ASSERT_ALWAYS(GetStorageSize64() <= WMath::MaxValue<WUInt32>(), "The memory stream storage object has grown beyond 4GB. The code using it has to be adapted to support this.");
    return (WUInt32)GetStorageSize64();
  }

  /// Returns the number of bytes that are currently stored.
  virtual WUInt64 GetStorageSize64() const = 0; // [tested]

  /// Clears the entire storage. All readers and writers must be reset to start from the beginning again.
  virtual void Clear() = 0;

  /// Deallocates any allocated memory that's not needed to hold the currently stored data.
  virtual void Compact() = 0;

  /// Returns the amount of bytes that are currently allocated on the heap.
  virtual WUInt64 GetHeapMemoryUsage() const = 0;

  /// Copies all data from the given stream into the storage.
  void ReadAll(WStreamReader& inout_stream, WUInt64 uiMaxBytes = WMath::MaxValue<WUInt64>());

  /// Reserves N bytes of storage.
  virtual void Reserve(WUInt64 uiBytes) = 0;

  /// Writes the entire content of the storage to the provided stream.
  virtual WResult CopyToStream(WStreamWriter& inout_stream) const = 0;

  /// Returns a read-only WArrayPtr that represents a contiguous area in memory which starts at the given first byte.
  ///
  /// This piece of memory can be read/copied/modified in one operation (memcpy etc).
  /// The next byte after this slice may be located somewhere entirely different in memory.
  /// Call GetContiguousMemoryRange() again with the next byte after this range, to get access to the next memory area.
  ///
  /// Chunks may differ in size.
  virtual WArrayPtr<const WUInt8> GetContiguousMemoryRange(WUInt64 uiStartByte) const = 0;

  /// Non-const overload of GetContiguousMemoryRange().
  virtual WArrayPtr<WUInt8> GetContiguousMemoryRange(WUInt64 uiStartByte) = 0;

private:
  virtual void SetInternalSize(WUInt64 uiSize) = 0;

  friend class WMemoryStreamReader;
  friend class WMemoryStreamWriter;
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// Templated implementation of WMemoryStreamStorageInterface that adapts most standard W containers to the interface.
///
/// Note that WMemoryStreamContainerStorage assumes contiguous storage, so using an WDeque for storage will not work.
template <typename CONTAINER>
class WMemoryStreamContainerStorage : public WMemoryStreamStorageInterface
{
public:
  /// Creates the storage object for a memory stream. Use \a uiInitialCapacity to reserve some memory up front.
  WMemoryStreamContainerStorage(WUInt32 uiInitialCapacity = 0, WAllocator* pAllocator = WFoundation::GetDefaultAllocator())
    : m_Storage(pAllocator)
  {
    m_Storage.Reserve(uiInitialCapacity);
  }

  virtual WUInt64 GetStorageSize64() const override { return m_Storage.GetCount(); }
  virtual void Clear() override { m_Storage.Clear(); }
  virtual void Compact() override { m_Storage.Compact(); }
  virtual WUInt64 GetHeapMemoryUsage() const override { return m_Storage.GetHeapMemoryUsage(); }

  virtual void Reserve(WUInt64 uiBytes) override
  {
    W_ASSERT_DEV(uiBytes <= WMath::MaxValue<WUInt32>(), "WMemoryStreamContainerStorage only supports 32 bit addressable sizes.");
    m_Storage.Reserve(static_cast<WUInt32>(uiBytes));
  }

  virtual WResult CopyToStream(WStreamWriter& inout_stream) const override
  {
    return inout_stream.WriteBytes(m_Storage.GetData(), m_Storage.GetCount());
  }

  virtual WArrayPtr<const WUInt8> GetContiguousMemoryRange(WUInt64 uiStartByte) const override
  {
    if (uiStartByte >= m_Storage.GetCount())
      return {};

    return WArrayPtr<const WUInt8>(m_Storage.GetData() + uiStartByte, m_Storage.GetCount() - static_cast<WUInt32>(uiStartByte));
  }

  virtual WArrayPtr<WUInt8> GetContiguousMemoryRange(WUInt64 uiStartByte) override
  {
    if (uiStartByte >= m_Storage.GetCount())
      return {};

    return WArrayPtr<WUInt8>(m_Storage.GetData() + uiStartByte, m_Storage.GetCount() - static_cast<WUInt32>(uiStartByte));
  }

  /// The data is guaranteed to be contiguous.
  const WUInt8* GetData() const { return m_Storage.GetData(); }

private:
  virtual void SetInternalSize(WUInt64 uiSize) override
  {
    W_ASSERT_DEV(uiSize <= WMath::MaxValue<WUInt32>(), "Storage that large is not supported.");
    m_Storage.SetCountUninitialized(static_cast<WUInt32>(uiSize));
  }

  CONTAINER m_Storage;
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


/// WContiguousMemoryStreamStorage holds internally an WHybridArray<WUInt8, 256>, to prevent allocations when only small temporary memory streams
/// are needed. That means it will have a memory overhead of that size.
/// Also it reallocates memory on demand, and the data is guaranteed to be contiguous. This may be desirable,
/// but can have a high performance overhead when data grows very large.
class W_FOUNDATION_DLL WContiguousMemoryStreamStorage : public WMemoryStreamContainerStorage<WHybridArray<WUInt8, 256>>
{
public:
  WContiguousMemoryStreamStorage(WUInt32 uiInitialCapacity = 0, WAllocator* pAllocator = WFoundation::GetDefaultAllocator())
    : WMemoryStreamContainerStorage<WHybridArray<WUInt8, 256>>(uiInitialCapacity, pAllocator)
  {
  }
};

/// The default implementation for memory stream storage.
///
/// This implementation of WMemoryStreamStorageInterface handles use cases both from very small to extremely large storage needs.
/// It starts out with some inplace memory that can accommodate small amounts of data.
/// To grow, additional chunks of data are allocated. No memory ever needs to be copied to grow the container.
/// However, that also means that the memory isn't stored in one contiguous array, therefore data has to be accessed piece-wise
/// through GetContiguousMemoryRange().
class W_FOUNDATION_DLL WDefaultMemoryStreamStorage final : public WMemoryStreamStorageInterface
{
public:
  WDefaultMemoryStreamStorage(WUInt32 uiInitialCapacity = 0, WAllocator* pAllocator = WFoundation::GetDefaultAllocator());
  ~WDefaultMemoryStreamStorage();

  virtual void Reserve(WUInt64 uiBytes) override;    // [tested]

  virtual WUInt64 GetStorageSize64() const override; // [tested]
  virtual void Clear() override;
  virtual void Compact() override;
  virtual WUInt64 GetHeapMemoryUsage() const override;
  virtual WResult CopyToStream(WStreamWriter& inout_stream) const override;
  virtual WArrayPtr<const WUInt8> GetContiguousMemoryRange(WUInt64 uiStartByte) const override; // [tested]
  virtual WArrayPtr<WUInt8> GetContiguousMemoryRange(WUInt64 uiStartByte) override;             // [tested]

private:
  virtual void SetInternalSize(WUInt64 uiSize) override;

  void AddChunk(WUInt32 uiMinimumSize);

  struct Chunk
  {
    WUInt64 m_uiStartOffset = 0;
    WArrayPtr<WUInt8> m_Bytes;
  };

  WHybridArray<Chunk, 16> m_Chunks;

  WUInt64 m_uiCapacity = 0;
  WUInt64 m_uiInternalSize = 0;
  WUInt8 m_InplaceMemory[512]; // used for the very first bytes, might cover small memory streams without an allocation
  mutable WUInt32 m_uiLastChunkAccessed = 0;
  mutable WUInt64 m_uiLastByteAccessed = 0;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// Wrapper around an existing container to implement WMemoryStreamStorageInterface
template <typename CONTAINER>
class WMemoryStreamContainerWrapperStorage : public WMemoryStreamStorageInterface
{
public:
  WMemoryStreamContainerWrapperStorage(CONTAINER* pContainer) { m_pStorage = pContainer; }

  virtual WUInt64 GetStorageSize64() const override { return m_pStorage->GetCount(); }
  virtual void Clear() override
  {
    if constexpr (!std::is_const<CONTAINER>::value)
    {
      m_pStorage->Clear();
    }
  }
  virtual void Compact() override
  {
    if constexpr (!std::is_const<CONTAINER>::value)
    {
      m_pStorage->Compact();
    }
  }

  virtual WUInt64 GetHeapMemoryUsage() const override { return m_pStorage->GetHeapMemoryUsage(); }

  virtual void Reserve(WUInt64 uiBytes) override
  {
    if constexpr (!std::is_const<CONTAINER>::value)
    {
      W_ASSERT_DEV(uiBytes <= WMath::MaxValue<WUInt32>(), "WMemoryStreamContainerWrapperStorage only supports 32 bit addressable sizes.");
      m_pStorage->Reserve(static_cast<WUInt32>(uiBytes));
    }
  }

  virtual WResult CopyToStream(WStreamWriter& inout_stream) const override
  {
    return inout_stream.WriteBytes(m_pStorage->GetData(), m_pStorage->GetCount());
  }

  virtual WArrayPtr<const WUInt8> GetContiguousMemoryRange(WUInt64 uiStartByte) const override
  {
    if (uiStartByte >= m_pStorage->GetCount())
      return {};

    return WArrayPtr<const WUInt8>(m_pStorage->GetData() + uiStartByte, m_pStorage->GetCount() - static_cast<WUInt32>(uiStartByte));
  }

  virtual WArrayPtr<WUInt8> GetContiguousMemoryRange(WUInt64 uiStartByte) override
  {
    if constexpr (!std::is_const<CONTAINER>::value)
    {
      if (uiStartByte >= m_pStorage->GetCount())
        return {};

      return WArrayPtr<WUInt8>(m_pStorage->GetData() + uiStartByte, m_pStorage->GetCount() - static_cast<WUInt32>(uiStartByte));
    }
    else
    {
      return {};
    }
  }

private:
  virtual void SetInternalSize(WUInt64 uiSize) override
  {
    if constexpr (!std::is_const<CONTAINER>::value)
    {
      W_ASSERT_DEV(uiSize <= WMath::MaxValue<WUInt32>(), "WMemoryStreamContainerWrapperStorage only supports up to 4GB sizes.");
      m_pStorage->SetCountUninitialized(static_cast<WUInt32>(uiSize));
    }
  }

  CONTAINER* m_pStorage;
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// A reader which can access a memory stream.
///
/// MemoryStreamReader provides sequential reading access to data stored in any MemoryStreamStorageInterface
/// implementation. It maintains an internal read position and supports both forward reading and seeking.
///
/// Thread safety:
/// - NOT thread-safe - each thread requires its own reader instance
/// - Multiple readers can safely share the same storage concurrently
/// - Concurrent reading and writing to the same storage is unsafe
class W_FOUNDATION_DLL WMemoryStreamReader : public WStreamReader
{
public:
  /// Pass the memory storage object from which to read from.
  /// Pass nullptr if you are going to set the storage stream later via SetStorage().
  WMemoryStreamReader(const WMemoryStreamStorageInterface* pStreamStorage = nullptr);

  ~WMemoryStreamReader();

  /// Sets the storage object upon which to operate. Resets the read position to zero.
  /// Pass nullptr if you want to detach from any previous storage stream, for example to ensure its reference count gets properly reduced.
  void SetStorage(const WMemoryStreamStorageInterface* pStreamStorage)
  {
    m_pStreamStorage = pStreamStorage;
    m_uiReadPosition = 0;
  }

  /// Reads either uiBytesToRead or the amount of remaining bytes in the stream into pReadBuffer.
  ///
  /// It is valid to pass nullptr for pReadBuffer, in this case the memory stream position is only advanced by the given number of bytes.
  virtual WUInt64 ReadBytes(void* pReadBuffer, WUInt64 uiBytesToRead) override; // [tested]

  /// Skips bytes in the stream (e.g. for skipping objects which can't be serialized due to missing information etc.)
  virtual WUInt64 SkipBytes(WUInt64 uiBytesToSkip) override; // [tested]

  /// Sets the read position to be used
  void SetReadPosition(WUInt64 uiReadPosition); // [tested]

  /// Returns the current read position
  WUInt64 GetReadPosition() const { return m_uiReadPosition; }

  /// Returns the total available bytes in the memory stream
  WUInt32 GetByteCount32() const; // [tested]
  WUInt64 GetByteCount64() const; // [tested]

  /// Allows to set a string as the source of information in the memory stream for debug purposes.
  void SetDebugSourceInformation(WStringView sDebugSourceInformation);

private:
  const WMemoryStreamStorageInterface* m_pStreamStorage = nullptr;

  WString m_sDebugSourceInformation;

  WUInt64 m_uiReadPosition = 0;
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// A writer which can access a memory stream
///
/// MemoryStreamWriter provides sequential writing access to any MemoryStreamStorageInterface
/// implementation. It automatically grows the underlying storage as needed and maintains
/// an internal write position that can be positioned anywhere within the stream.
///
/// Thread safety:
/// - NOT thread-safe - requires exclusive access
/// - Cannot safely share writer instances between threads
/// - Concurrent writing and reading to the same storage is unsafe
class W_FOUNDATION_DLL WMemoryStreamWriter : public WStreamWriter
{
public:
  /// Pass the memory storage object to which to write to.
  WMemoryStreamWriter(WMemoryStreamStorageInterface* pStreamStorage = nullptr);

  ~WMemoryStreamWriter();

  /// Sets the storage object upon which to operate. Resets the write position to the end of the storage stream.
  /// Pass nullptr if you want to detach from any previous storage stream, for example to ensure its reference count gets properly reduced.
  void SetStorage(WMemoryStreamStorageInterface* pStreamStorage)
  {
    m_pStreamStorage = pStreamStorage;
    m_uiWritePosition = 0;
    if (m_pStreamStorage)
      m_uiWritePosition = m_pStreamStorage->GetStorageSize64();
  }

  /// Copies uiBytesToWrite from pWriteBuffer into the memory stream.
  ///
  /// pWriteBuffer must be a valid buffer and must hold that much data.
  virtual WResult WriteBytes(const void* pWriteBuffer, WUInt64 uiBytesToWrite) override; // [tested]

  /// Sets the write position to be used
  void SetWritePosition(WUInt64 uiWritePosition); // [tested]

  /// Returns the current write position
  WUInt64 GetWritePosition() const { return m_uiWritePosition; }

  /// Returns the total stored bytes in the memory stream
  WUInt32 GetByteCount32() const; // [tested]
  WUInt64 GetByteCount64() const; // [tested]

private:
  WMemoryStreamStorageInterface* m_pStreamStorage = nullptr;

  WUInt64 m_uiWritePosition = 0;
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// Maps a raw chunk of memory to the WStreamReader interface.
///
/// RawMemoryStreamReader provides direct read access to a pre-existing memory buffer without
/// requiring any storage interface or memory management. It's optimized for scenarios where
/// you have a fixed chunk of memory (array, buffer, etc.) and need stream interface access.
class W_FOUNDATION_DLL WRawMemoryStreamReader : public WStreamReader
{
public:
  WRawMemoryStreamReader();

  /// Initialize the raw memory reader with the chunk of memory that is the data storage.
  WRawMemoryStreamReader(const void* pData, WUInt64 uiDataSize); // [tested]

  /// Initialize the raw memory reader with the chunk of memory from a standard W container.
  /// \note The container must store the data in a contiguous array.
  template <typename CONTAINER>
  WRawMemoryStreamReader(const CONTAINER& container) // [tested]
  {
    Reset(container);
  }

  ~WRawMemoryStreamReader();

  void Reset(const void* pData, WUInt64 uiDataSize); // [tested]

  template <typename CONTAINER>
  void Reset(const CONTAINER& container)              // [tested]
  {
    Reset(static_cast<const WUInt8*>(container.GetData()), container.GetCount());
  }

  /// Reads either uiBytesToRead or the amount of remaining bytes in the stream into pReadBuffer.
  ///
  /// It is valid to pass nullptr for pReadBuffer, in this case the memory stream position is only advanced by the given number of bytes.
  virtual WUInt64 ReadBytes(void* pReadBuffer, WUInt64 uiBytesToRead) override; // [tested]

  /// Skips bytes in the stream (e.g. for skipping objects which can't be serialized due to missing information etc.)
  virtual WUInt64 SkipBytes(WUInt64 uiBytesToSkip) override; // [tested]

  /// Sets the read position to be used
  void SetReadPosition(WUInt64 uiReadPosition); // [tested]

  /// Returns the current read position in the raw memory block
  WUInt64 GetReadPosition() const { return m_uiReadPosition; }

  /// Returns the total available bytes in the memory stream
  WUInt64 GetByteCount() const; // [tested]

  /// Allows to set a string as the source of information in the memory stream for debug purposes.
  void SetDebugSourceInformation(WStringView sDebugSourceInformation);

private:
  const WUInt8* m_pRawMemory = nullptr;

  WUInt64 m_uiChunkSize = 0;
  WUInt64 m_uiReadPosition = 0;

  WString m_sDebugSourceInformation;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


/// Maps a raw chunk of memory to the WStreamReader interface.
class W_FOUNDATION_DLL WRawMemoryStreamWriter : public WStreamWriter
{
public:
  WRawMemoryStreamWriter(); // [tested]

  /// Initialize the raw memory reader with the chunk of memory that is the data storage.
  WRawMemoryStreamWriter(void* pData, WUInt64 uiDataSize); // [tested]

  /// Initialize the raw memory reader with the chunk of memory from a standard W container.
  /// \note The container must store the data in a contiguous array.
  template <typename CONTAINER>
  WRawMemoryStreamWriter(CONTAINER& ref_container) // [tested]
  {
    Reset(ref_container);
  }

  ~WRawMemoryStreamWriter();                   // [tested]

  void Reset(void* pData, WUInt64 uiDataSize); // [tested]

  template <typename CONTAINER>
  void Reset(CONTAINER& ref_container)          // [tested]
  {
    Reset(static_cast<WUInt8*>(ref_container.GetData()), ref_container.GetCount());
  }

  /// Returns the total available bytes in the memory stream
  WUInt64 GetStorageSize() const; // [tested]

  /// Returns the number of bytes written to the storage
  WUInt64 GetNumWrittenBytes() const; // [tested]

  /// Allows to set a string as the source of information in the memory stream for debug purposes.
  void SetDebugSourceInformation(WStringView sDebugSourceInformation);

  /// Copies uiBytesToWrite from pWriteBuffer into the memory stream.
  ///
  /// pWriteBuffer must be a valid buffer and must hold that much data.
  virtual WResult WriteBytes(const void* pWriteBuffer, WUInt64 uiBytesToWrite) override; // [tested]

private:
  WUInt8* m_pRawMemory = nullptr;

  WUInt64 m_uiChunkSize = 0;
  WUInt64 m_uiWritePosition = 0;

  WString m_sDebugSourceInformation;
};
