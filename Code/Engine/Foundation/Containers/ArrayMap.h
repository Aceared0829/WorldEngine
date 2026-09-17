#pragma once

#include <Foundation/Containers/DynamicArray.h>

/// An associative container, similar to WMap, but all data is stored in a sorted contiguous array, which makes frequent lookups more
/// efficient.
///
/// Prefer this container over WMap when you modify the container less often than you look things up (which is in most cases), and when
/// you do not need to store iterators to elements and require them to stay valid when the container is modified.
///
/// WArrayMapBase also allows to store multiple values under the same key (like a multi-map).
template <typename KEY, typename VALUE>
class WArrayMapBase
{
  /// \todo Custom comparer

public:
  struct Pair
  {
    KEY key;
    VALUE value;

    W_DETECT_TYPE_CLASS(KEY, VALUE);

    W_ALWAYS_INLINE bool operator<(const Pair& rhs) const { return key < rhs.key; }

    W_ALWAYS_INLINE bool operator==(const Pair& rhs) const { return key == rhs.key; }
  };

  /// Constructor.
  explicit WArrayMapBase(WAllocator* pAllocator); // [tested]

  /// Copy-Constructor.
  WArrayMapBase(const WArrayMapBase& rhs, WAllocator* pAllocator); // [tested]

  /// Copy assignment operator.
  void operator=(const WArrayMapBase& rhs); // [tested]

  /// Returns the number of elements stored in the map.
  WUInt32 GetCount() const; // [tested]

  /// True if the map contains no elements.
  bool IsEmpty() const; // [tested]

  /// Purges all elements from the map.
  void Clear(); // [tested]

  /// Always inserts a new value under the given key. Duplicates are allowed.
  /// Returns the index of the newly added element.
  template <typename CompatibleKeyType, typename CompatibleValueType>
  WUInt32 Insert(CompatibleKeyType&& key, CompatibleValueType&& value); // [tested]

  /// Ensures the internal data structure is sorted. This is done automatically every time a lookup needs to be made.
  void Sort() const; // [tested]

  /// Returns an index to one element with the given key. If the key is inserted multiple times, there is no guarantee which one is returned.
  /// Returns WInvalidIndex when no such element exists.
  template <typename CompatibleKeyType>
  WUInt32 Find(const CompatibleKeyType& key) const; // [tested]

  /// Returns the index to the first element with a key equal or larger than the given key.
  /// Returns WInvalidIndex when no such element exists.
  /// If there are multiple keys with the same value, the one at the smallest index is returned.
  template <typename CompatibleKeyType>
  WUInt32 LowerBound(const CompatibleKeyType& key) const; // [tested]

  /// Returns the index to the first element with a key that is LARGER than the given key.
  /// Returns WInvalidIndex when no such element exists.
  /// If there are multiple keys with the same value, the one at the smallest index is returned.
  template <typename CompatibleKeyType>
  WUInt32 UpperBound(const CompatibleKeyType& key) const; // [tested]

  /// Returns the key that is stored at the given index.
  const KEY& GetKey(WUInt32 uiIndex) const; // [tested]

  /// Returns the value that is stored at the given index.
  const VALUE& GetValue(WUInt32 uiIndex) const; // [tested]

  /// Returns the value that is stored at the given index.
  VALUE& GetValue(WUInt32 uiIndex); // [tested]

  /// Returns a reference to the map data array.
  WDynamicArray<Pair>& GetData();

  /// Returns a constant reference to the map data array.
  const WDynamicArray<Pair>& GetData() const;

  /// Returns the value stored at the given key. If none exists, one is created. \a bExisted indicates whether an element needed to be created.
  template <typename CompatibleKeyType>
  VALUE& FindOrAdd(const CompatibleKeyType& key, bool* out_pExisted = nullptr); // [tested]

  /// Same as FindOrAdd.
  template <typename CompatibleKeyType>
  VALUE& operator[](const CompatibleKeyType& key); // [tested]

  /// Returns the key/value pair at the given index.
  const Pair& GetPair(WUInt32 uiIndex) const; // [tested]

  /// Removes the element at the given index.
  ///
  /// If the map is sorted and bKeepSorted is true, the element will be removed such that the map stays sorted.
  /// This is only useful, if only a single (or very few) elements are removed before the next lookup. If multiple values
  /// are removed, or new values are going to be inserted, as well, \a bKeepSorted should be left to false.
  void RemoveAtAndCopy(WUInt32 uiIndex, bool bKeepSorted = false);

  /// Removes one element with the given key. Returns true, if one was found and removed. If the same key exists multiple times, you need to
  /// call this function multiple times to remove them all.
  ///
  /// If the map is sorted and bKeepSorted is true, the element will be removed such that the map stays sorted.
  /// This is only useful, if only a single (or very few) elements are removed before the next lookup. If multiple values
  /// are removed, or new values are going to be inserted, as well, \a bKeepSorted should be left to false.
  template <typename CompatibleKeyType>
  bool RemoveAndCopy(const CompatibleKeyType& key, bool bKeepSorted = false); // [tested]

  /// Returns whether an element with the given key exists.
  template <typename CompatibleKeyType>
  bool Contains(const CompatibleKeyType& key) const; // [tested]

  /// Returns whether an element with the given key and value already exists.
  template <typename CompatibleKeyType>
  bool Contains(const CompatibleKeyType& key, const VALUE& value) const; // [tested]

  /// Reserves enough memory to store \a size elements.
  void Reserve(WUInt32 uiSize); // [tested]

  /// Compacts the internal memory to not waste any space.
  void Compact(); // [tested]

  /// Compares the two containers for equality.
  bool operator==(const WArrayMapBase<KEY, VALUE>& rhs) const; // [tested]
  W_ADD_DEFAULT_OPERATOR_NOTEQUAL(const WArrayMapBase<KEY, VALUE>&);

  /// Returns the amount of bytes that are currently allocated on the heap.
  WUInt64 GetHeapMemoryUsage() const { return m_Data.GetHeapMemoryUsage(); } // [tested]

  using const_iterator = typename WDynamicArray<Pair>::const_iterator;
  using const_reverse_iterator = typename WDynamicArray<Pair>::const_reverse_iterator;
  using iterator = typename WDynamicArray<Pair>::iterator;
  using reverse_iterator = typename WDynamicArray<Pair>::reverse_iterator;

private:
  mutable bool m_bSorted;
  mutable WDynamicArray<Pair> m_Data;
};

/// See WArrayMapBase for details.
template <typename KEY, typename VALUE, typename AllocatorWrapper = WDefaultAllocatorWrapper>
class WArrayMap : public WArrayMapBase<KEY, VALUE>
{
public:
  W_DECLARE_MEM_RELOCATABLE_TYPE();

  WArrayMap();
  explicit WArrayMap(WAllocator* pAllocator);

  WArrayMap(const WArrayMap<KEY, VALUE, AllocatorWrapper>& rhs);
  WArrayMap(const WArrayMapBase<KEY, VALUE>& rhs);

  void operator=(const WArrayMap<KEY, VALUE, AllocatorWrapper>& rhs);
  void operator=(const WArrayMapBase<KEY, VALUE>& rhs);
};


template <typename KEY, typename VALUE>
typename WArrayMapBase<KEY, VALUE>::iterator begin(WArrayMapBase<KEY, VALUE>& ref_container)
{
  return begin(ref_container.GetData());
}

template <typename KEY, typename VALUE>
typename WArrayMapBase<KEY, VALUE>::const_iterator begin(const WArrayMapBase<KEY, VALUE>& container)
{
  return begin(container.GetData());
}
template <typename KEY, typename VALUE>
typename WArrayMapBase<KEY, VALUE>::const_iterator cbegin(const WArrayMapBase<KEY, VALUE>& container)
{
  return cbegin(container.GetData());
}

template <typename KEY, typename VALUE>
typename WArrayMapBase<KEY, VALUE>::reverse_iterator rbegin(WArrayMapBase<KEY, VALUE>& ref_container)
{
  return rbegin(ref_container.GetData());
}

template <typename KEY, typename VALUE>
typename WArrayMapBase<KEY, VALUE>::const_reverse_iterator rbegin(const WArrayMapBase<KEY, VALUE>& container)
{
  return rbegin(container.GetData());
}

template <typename KEY, typename VALUE>
typename WArrayMapBase<KEY, VALUE>::const_reverse_iterator crbegin(const WArrayMapBase<KEY, VALUE>& container)
{
  return crbegin(container.GetData());
}

template <typename KEY, typename VALUE>
typename WArrayMapBase<KEY, VALUE>::iterator end(WArrayMapBase<KEY, VALUE>& ref_container)
{
  return end(ref_container.GetData());
}

template <typename KEY, typename VALUE>
typename WArrayMapBase<KEY, VALUE>::const_iterator end(const WArrayMapBase<KEY, VALUE>& container)
{
  return end(container.GetData());
}

template <typename KEY, typename VALUE>
typename WArrayMapBase<KEY, VALUE>::const_iterator cend(const WArrayMapBase<KEY, VALUE>& container)
{
  return cend(container.GetData());
}

template <typename KEY, typename VALUE>
typename WArrayMapBase<KEY, VALUE>::reverse_iterator rend(WArrayMapBase<KEY, VALUE>& ref_container)
{
  return rend(ref_container.GetData());
}

template <typename KEY, typename VALUE>
typename WArrayMapBase<KEY, VALUE>::const_reverse_iterator rend(const WArrayMapBase<KEY, VALUE>& container)
{
  return rend(container.GetData());
}

template <typename KEY, typename VALUE>
typename WArrayMapBase<KEY, VALUE>::const_reverse_iterator crend(const WArrayMapBase<KEY, VALUE>& container)
{
  return crend(container.GetData());
}


#include <Foundation/Containers/Implementation/ArrayMap_inl.h>
