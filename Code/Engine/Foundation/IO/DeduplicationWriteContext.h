
#pragma once

#include <Foundation/Containers/ArrayBase.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/SerializationContext.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>

class WStreamWriter;

/// Serialization Context that de-duplicates objects when writing to a stream. Duplicated objects are identified by their address and
/// only the first occurrence is written to the stream while all subsequence occurrences are just written as an index.
class W_FOUNDATION_DLL WDeduplicationWriteContext : public WSerializationContext<WDeduplicationWriteContext>
{
  W_DECLARE_SERIALIZATION_CONTEXT(WDeduplicationWriteContext);

public:
  WDeduplicationWriteContext();
  ~WDeduplicationWriteContext();

  /// Writes a single object to the stream. Can be either a reference or a pointer to the object.
  template <typename T>
  WResult WriteObject(WStreamWriter& inout_stream, const T& obj); // [tested]

  /// Writes a single object to the stream.
  template <typename T>
  WResult WriteObject(WStreamWriter& inout_stream, const WSharedPtr<T>& pObject); // [tested]

  /// Writes a single object to the stream.
  template <typename T>
  WResult WriteObject(WStreamWriter& inout_stream, const WUniquePtr<T>& pObject); // [tested]

  /// Writes an array of de-duplicated objects.
  template <typename ArrayType, typename ValueType>
  WResult WriteArray(WStreamWriter& inout_stream, const WArrayBase<ValueType, ArrayType>& array); // [tested]

  /// Writes a set of de-duplicated objects.
  template <typename KeyType, typename Comparer>
  WResult WriteSet(WStreamWriter& inout_stream, const WSetBase<KeyType, Comparer>& set); // [tested]

  enum class WriteMapMode
  {
    DedupKey,
    DedupValue,
    DedupBoth
  };

  /// Writes a map. Mode controls whether key or value or both should de-duplicated.
  template <typename KeyType, typename ValueType, typename Comparer>
  WResult WriteMap(WStreamWriter& inout_stream, const WMapBase<KeyType, ValueType, Comparer>& map, WriteMapMode mode); // [tested]

private:
  template <typename T>
  WResult WriteObjectInternal(WStreamWriter& stream, const T* pObject);

  WHashTable<const void*, WUInt32> m_Objects;
};

#include <Foundation/IO/Implementation/DeduplicationWriteContext_inl.h>
