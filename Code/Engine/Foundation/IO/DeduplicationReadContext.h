
#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/SerializationContext.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>

class WStreamReader;

/// Serialization Context that reads de-duplicated objects from a stream and restores the pointers.
class W_FOUNDATION_DLL WDeduplicationReadContext : public WSerializationContext<WDeduplicationReadContext>
{
  W_DECLARE_SERIALIZATION_CONTEXT(WDeduplicationReadContext);

public:
  WDeduplicationReadContext();
  ~WDeduplicationReadContext();

  /// Reads a single object inplace.
  template <typename T>
  WResult ReadObjectInplace(WStreamReader& inout_stream, T& ref_obj); // [tested]

  /// Reads a single object and sets the pointer to it. The given allocator is used to create the object if it doesn't exist yet.
  template <typename T>
  WResult ReadObject(WStreamReader& inout_stream, T*& ref_pObject,
    WAllocator* pAllocator = WFoundation::GetDefaultAllocator()); // [tested]

  /// Reads a single object and sets the shared pointer to it. The given allocator is used to create the object if it doesn't exist
  /// yet.
  template <typename T>
  WResult ReadObject(WStreamReader& inout_stream, WSharedPtr<T>& ref_pObject,
    WAllocator* pAllocator = WFoundation::GetDefaultAllocator()); // [tested]

  /// Reads a single object and sets the unique pointer to it. The given allocator is used to create the object if it doesn't exist
  /// yet.
  template <typename T>
  WResult ReadObject(WStreamReader& inout_stream, WUniquePtr<T>& ref_pObject,
    WAllocator* pAllocator = WFoundation::GetDefaultAllocator()); // [tested]

  /// Reads an array of de-duplicated objects.
  template <typename ArrayType, typename ValueType>
  WResult ReadArray(WStreamReader& inout_stream, WArrayBase<ValueType, ArrayType>& ref_array,
    WAllocator* pAllocator = WFoundation::GetDefaultAllocator()); // [tested]

  /// Reads a set of de-duplicated objects.
  template <typename KeyType, typename Comparer>
  WResult ReadSet(WStreamReader& inout_stream, WSetBase<KeyType, Comparer>& ref_set,
    WAllocator* pAllocator = WFoundation::GetDefaultAllocator()); // [tested]

  enum class ReadMapMode
  {
    DedupKey,
    DedupValue,
    DedupBoth
  };

  /// Reads a map. Mode controls whether key or value or both should de-duplicated.
  template <typename KeyType, typename ValueType, typename Comparer>
  WResult ReadMap(WStreamReader& inout_stream, WMapBase<KeyType, ValueType, Comparer>& ref_map, ReadMapMode mode,
    WAllocator* pKeyAllocator = WFoundation::GetDefaultAllocator(),
    WAllocator* pValueAllocator = WFoundation::GetDefaultAllocator()); // [tested]

private:
  template <typename T>
  WResult ReadObject(WStreamReader& stream, T& obj, WAllocator* pAllocator); // [tested]

  WDynamicArray<void*> m_Objects;
};

#include <Foundation/IO/Implementation/DeduplicationReadContext_inl.h>
