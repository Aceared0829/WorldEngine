
#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/ArrayBase.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Containers/SmallArray.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Memory/EndianHelper.h>

using WTypeVersion = WUInt16;

template <WUInt16 Size, typename AllocatorWrapper>
struct WHybridString;

using WString = WHybridString<32, WDefaultAllocatorWrapper>;

/// Abstract base class for binary input streams providing unified reading interface.
///
/// StreamReader defines the fundamental interface for reading binary data from various sources
/// including files, memory buffers, network connections, and compressed streams. All read
/// operations are performed in a sequential manner with automatic endianness handling.
///
/// Core functionality:
/// - ReadBytes(): Raw binary data reading (pure virtual, must be implemented)
/// - Typed reading methods with endianness conversion (ReadWordValue, ReadDWordValue, etc.)
/// - High-level container reading (arrays, sets, maps, hash tables)
/// - String reading with automatic length handling
///
/// Implementation requirements:
/// - Derived classes must implement ReadBytes() as the primary read method
/// - All other methods are implemented in terms of ReadBytes()
/// - Should handle EOF conditions gracefully by returning actual bytes read
class W_FOUNDATION_DLL WStreamReader
{
  W_DISALLOW_COPY_AND_ASSIGN(WStreamReader);

public:
  /// Constructor
  WStreamReader();

  /// Virtual destructor to ensure correct cleanup
  virtual ~WStreamReader();

  /// Reads a raw number of bytes into the read buffer. This is the only method that must be implemented by derived classes.
  ///
  /// \param pReadBuffer Destination buffer for the read data
  /// \param uiBytesToRead Maximum number of bytes to read
  /// \return Actual number of bytes read (may be less than requested on EOF or error)
  virtual WUInt64 ReadBytes(void* pReadBuffer, WUInt64 uiBytesToRead) = 0; // [tested]

  /// Helper method to read a word value correctly (copes with potentially different endianess)
  template <typename T>
  WResult ReadWordValue(T* pWordValue); // [tested]

  /// Helper method to read a dword value correctly (copes with potentially different endianess)
  template <typename T>
  WResult ReadDWordValue(T* pDWordValue); // [tested]

  /// Helper method to read a qword value correctly (copes with potentially different endianess)
  template <typename T>
  WResult ReadQWordValue(T* pQWordValue); // [tested]

  /// Reads an array of elements from the stream
  template <typename ArrayType, typename ValueType>
  WResult ReadArray(WArrayBase<ValueType, ArrayType>& inout_array); // [tested]

  /// Reads a small array of elements from the stream
  template <typename ValueType, WUInt16 uiSize, typename AllocatorWrapper>
  WResult ReadArray(WSmallArray<ValueType, uiSize, AllocatorWrapper>& ref_array);

  /// Writes a C style fixed array
  template <typename ValueType, WUInt32 uiSize>
  WResult ReadArray(ValueType (&array)[uiSize]);

  /// Reads a set
  template <typename KeyType, typename Comparer>
  WResult ReadSet(WSetBase<KeyType, Comparer>& inout_set); // [tested]

  /// Reads a map
  template <typename KeyType, typename ValueType, typename Comparer>
  WResult ReadMap(WMapBase<KeyType, ValueType, Comparer>& inout_map); // [tested]

  /// Read a hash table (note that the entry order is not stable)
  template <typename KeyType, typename ValueType, typename Hasher>
  WResult ReadHashTable(WHashTableBase<KeyType, ValueType, Hasher>& inout_hashTable); // [tested]

  /// Reads a string into an WStringBuilder
  WResult ReadString(WStringBuilder& ref_sBuilder); // [tested]

  /// Reads a string into an WString
  WResult ReadString(WString& ref_sString);


  /// Helper method to skip a number of bytes (implementations of the stream reader may implement this more efficiently for example)
  virtual WUInt64 SkipBytes(WUInt64 uiBytesToSkip)
  {
    WUInt8 uiTempBuffer[1024];

    WUInt64 uiBytesSkipped = 0;

    while (uiBytesSkipped < uiBytesToSkip)
    {
      WUInt64 uiBytesToRead = WMath::Min<WUInt64>(uiBytesToSkip - uiBytesSkipped, 1024);

      WUInt64 uiBytesRead = ReadBytes(uiTempBuffer, uiBytesToRead);

      uiBytesSkipped += uiBytesRead;

      // Terminate early if the stream didn't read as many bytes as we requested (EOF for example)
      if (uiBytesRead < uiBytesToRead)
        break;
    }

    return uiBytesSkipped;
  }

  W_ALWAYS_INLINE WTypeVersion ReadVersion();
  W_ALWAYS_INLINE WTypeVersion ReadVersion(WTypeVersion expectedMaxVersion);
};

/// Abstract base class for binary output streams providing unified writing interface.
///
/// StreamWriter defines the fundamental interface for writing binary data to various destinations
/// including files, memory buffers, network connections, and compressed streams. All write
/// operations are performed sequentially with automatic endianness handling.
///
/// Core functionality:
/// - WriteBytes(): Raw binary data writing (pure virtual, must be implemented)
/// - Typed writing methods with endianness conversion (WriteWordValue, WriteDWordValue, etc.)
/// - High-level container writing (arrays, sets, maps, hash tables)
/// - String writing with automatic length prefixing
/// - Optional flush support for ensuring data persistence
///
/// Implementation requirements:
/// - Derived classes must implement WriteBytes() as the primary write method
/// - All other methods are implemented in terms of WriteBytes()
/// - Flush() can be overridden for buffered implementations
/// - Should handle write errors gracefully by returning appropriate WResult
class W_FOUNDATION_DLL WStreamWriter
{
  W_DISALLOW_COPY_AND_ASSIGN(WStreamWriter);

public:
  /// Constructor
  WStreamWriter();

  /// Virtual destructor to ensure correct cleanup
  virtual ~WStreamWriter();

  /// Writes a raw number of bytes from the buffer. This is the only method that must be implemented by derived classes.
  ///
  /// \param pWriteBuffer Source buffer containing data to write
  /// \param uiBytesToWrite Number of bytes to write from the buffer
  /// \return W_SUCCESS if all bytes were written successfully, W_FAILURE otherwise
  virtual WResult WriteBytes(const void* pWriteBuffer, WUInt64 uiBytesToWrite) = 0; // [tested]

  /// Flushes buffered data to the underlying storage, ensuring data persistence.
  ///
  /// Default implementation is a no-op. Derived classes with internal buffering should
  /// override this method to force writing of buffered data to the actual destination.
  virtual WResult Flush() // [tested]
  {
    return W_SUCCESS;
  }

  /// Helper method to write a word value correctly (copes with potentially different endianess)
  template <typename T>
  WResult WriteWordValue(const T* pWordValue); // [tested]

  /// Helper method to write a dword value correctly (copes with potentially different endianess)
  template <typename T>
  WResult WriteDWordValue(const T* pDWordValue); // [tested]

  /// Helper method to write a qword value correctly (copes with potentially different endianess)
  template <typename T>
  WResult WriteQWordValue(const T* pQWordValue); // [tested]

  /// Writes a type version to the stream
  W_ALWAYS_INLINE void WriteVersion(WTypeVersion version);

  /// Writes an array of elements to the stream
  template <typename ArrayType, typename ValueType>
  WResult WriteArray(const WArrayBase<ValueType, ArrayType>& array); // [tested]

  /// Writes a small array of elements to the stream
  template <typename ValueType, WUInt16 uiSize>
  WResult WriteArray(const WSmallArrayBase<ValueType, uiSize>& array);

  /// Writes a C style fixed array
  template <typename ValueType, WUInt32 uiSize>
  WResult WriteArray(const ValueType (&array)[uiSize]);

  /// Writes a set
  template <typename KeyType, typename Comparer>
  WResult WriteSet(const WSetBase<KeyType, Comparer>& set); // [tested]

  /// Writes a map
  template <typename KeyType, typename ValueType, typename Comparer>
  WResult WriteMap(const WMapBase<KeyType, ValueType, Comparer>& map); // [tested]

  /// Writes a hash table (note that the entry order might change on read)
  template <typename KeyType, typename ValueType, typename Hasher>
  WResult WriteHashTable(const WHashTableBase<KeyType, ValueType, Hasher>& hashTable); // [tested]

  /// Writes a string
  WResult WriteString(const WStringView sStringView); // [tested]
};

// Contains the helper methods of both interfaces
#include <Foundation/IO/Implementation/Stream_inl.h>

// Standard operators for overloads of common data types
#include <Foundation/IO/Implementation/StreamOperations_inl.h>

#include <Foundation/IO/Implementation/StreamOperationsMath_inl.h>

#include <Foundation/IO/Implementation/StreamOperationsOther_inl.h>
