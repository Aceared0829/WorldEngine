#pragma once

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Memory/AllocatorWrapper.h>

/// Implementation of a hashset.
///
/// The hashset stores values by using the hash as an index into the table.
/// This implementation uses linear-probing to resolve hash collisions which means all values are stored
/// in a linear array. Automatic resizing maintains a load factor below 60% for optimal performance.
///
/// Performance characteristics:
/// - Average case: O(1) - insertion, erasure, lookup
/// - Worst case: O(n) - when all keys hash to the same location (very rare with good hash functions)
/// - Resizing: O(n) - occurs when load factor exceeds 60%, amortized cost is still O(1) per operation
/// - Memory usage: More memory efficient than tree-based containers, ~1.67x element storage
/// - Iteration: O(n) in hash order (not sorted)
/// - Set operations (union, intersection, difference): O(n + m) average case
///
/// Use when:
/// - Fast lookup/insertion/removal is the primary concern
/// - You don't need sorted iteration
/// - Memory efficiency is important
/// - You have a good hash function for your key type
///
/// Consider WSet instead when:
/// - You need sorted iteration
/// - You need stable element addresses (no reallocation)
/// - Predictable O(log n) performance is more important than average O(1)
///
/// The hash function can be customized by providing a Hasher helper class like WHashHelper.
/// \see WHashHelper
template <typename KeyType, typename Hasher>
class WHashSetBase
{
public:
  /// Const iterator.
  class ConstIterator
  {
  public:
    /// Checks whether this iterator points to a valid element.
    bool IsValid() const; // [tested]

    /// Checks whether the two iterators point to the same element.
    bool operator==(const typename WHashSetBase<KeyType, Hasher>::ConstIterator& rhs) const;

    W_ADD_DEFAULT_OPERATOR_NOTEQUAL(const typename WHashSetBase<KeyType, Hasher>::ConstIterator&);

    /// Returns the 'key' of the element that this iterator points to.
    const KeyType& Key() const; // [tested]

    /// Returns the 'key' of the element that this iterator points to.
    W_ALWAYS_INLINE const KeyType& operator*() const { return Key(); } // [tested]

    /// Advances the iterator to the next element in the map. The iterator will not be valid anymore, if the end is reached.
    void Next(); // [tested]

    /// Shorthand for 'Next'
    void operator++(); // [tested]

  protected:
    friend class WHashSetBase<KeyType, Hasher>;

    explicit ConstIterator(const WHashSetBase<KeyType, Hasher>& hashSet);
    void SetToBegin();
    void SetToEnd();

    const WHashSetBase<KeyType, Hasher>* m_pHashSet = nullptr;
    WUInt32 m_uiCurrentIndex = 0; // current element index that this iterator points to.
    WUInt32 m_uiCurrentCount = 0; // current number of valid elements that this iterator has found so far.
  };

protected:
  /// Creates an empty hashset. Does not allocate any data yet.
  explicit WHashSetBase(WAllocator* pAllocator); // [tested]

  /// Creates a copy of the given hashset.
  WHashSetBase(const WHashSetBase<KeyType, Hasher>& rhs, WAllocator* pAllocator); // [tested]

  /// Moves data from an existing hashtable into this one.
  WHashSetBase(WHashSetBase<KeyType, Hasher>&& rhs, WAllocator* pAllocator); // [tested]

  /// Destructor.
  ~WHashSetBase(); // [tested]

  /// Copies the data from another hashset into this one.
  void operator=(const WHashSetBase<KeyType, Hasher>& rhs); // [tested]

  /// Moves data from an existing hashset into this one.
  void operator=(WHashSetBase<KeyType, Hasher>&& rhs); // [tested]

public:
  /// Compares this table to another table.
  bool operator==(const WHashSetBase<KeyType, Hasher>& rhs) const; // [tested]
  W_ADD_DEFAULT_OPERATOR_NOTEQUAL(const WHashSetBase<KeyType, Hasher>&);

  /// Expands the hashset by over-allocating the internal storage so that the load factor is lower or equal to 60% when inserting the
  /// given number of entries.
  void Reserve(WUInt32 uiCapacity); // [tested]

  /// Tries to compact the hashset to avoid wasting memory.
  ///
  /// The resulting capacity is at least 'GetCount' (no elements get removed).
  /// Will deallocate all data, if the hashset is empty.
  void Compact(); // [tested]

  /// Returns the number of active entries in the table.
  WUInt32 GetCount() const; // [tested]

  /// Returns true, if the hashset does not contain any elements.
  bool IsEmpty() const; // [tested]

  /// Clears the table.
  void Clear(); // [tested]

  /// Inserts the key. Returns whether the key was already existing.
  template <typename CompatibleKeyType>
  bool Insert(CompatibleKeyType&& key); // [tested]

  /// Removes the entry with the given key. Returns if an entry was removed.
  template <typename CompatibleKeyType>
  bool Remove(const CompatibleKeyType& key); // [tested]

  /// Erases the key at the given Iterator. Returns an iterator to the element after the given iterator.
  ConstIterator Remove(const ConstIterator& pos); // [tested]

  /// Returns if an entry with given key exists in the table.
  template <typename CompatibleKeyType>
  bool Contains(const CompatibleKeyType& key) const; // [tested]

  /// Checks whether all keys of the given set are in the container.
  bool ContainsSet(const WHashSetBase<KeyType, Hasher>& operand) const; // [tested]

  /// Makes this set the union of itself and the operand.
  void Union(const WHashSetBase<KeyType, Hasher>& operand); // [tested]

  /// Makes this set the difference of itself and the operand, i.e. subtracts operand.
  void Difference(const WHashSetBase<KeyType, Hasher>& operand); // [tested]

  /// Makes this set the intersection of itself and the operand.
  void Intersection(const WHashSetBase<KeyType, Hasher>& operand); // [tested]

  /// Returns a constant Iterator to the very first element.
  ConstIterator GetIterator() const; // [tested]

  /// Returns a constant Iterator to the first element that is not part of the hashset. Needed to implement range based for loop
  /// support.
  ConstIterator GetEndIterator() const;

  /// Returns the allocator that is used by this instance.
  WAllocator* GetAllocator() const;

  /// Returns the amount of bytes that are currently allocated on the heap.
  WUInt64 GetHeapMemoryUsage() const; // [tested]

  /// Swaps this map with the other one.
  void Swap(WHashSetBase<KeyType, Hasher>& other); // [tested]

  /// Searches for key, returns a ConstIterator to it or an invalid iterator, if no such key is found. O(1) operation.
  template <typename CompatibleKeyType>
  ConstIterator Find(const CompatibleKeyType& key) const;

private:
  KeyType* m_pEntries;
  WUInt32* m_pEntryFlags;

  WUInt32 m_uiCount;
  WUInt32 m_uiCapacity;

  WAllocator* m_pAllocator;

  enum
  {
    FREE_ENTRY = 0,
    VALID_ENTRY = 1,
    DELETED_ENTRY = 2,
    FLAGS_MASK = 3,
    CAPACITY_ALIGNMENT = 32
  };

  void SetCapacity(WUInt32 uiCapacity);

  void RemoveInternal(WUInt32 uiIndex);

  template <typename CompatibleKeyType>
  WUInt32 FindEntry(const CompatibleKeyType& key) const;

  template <typename CompatibleKeyType>
  WUInt32 FindEntry(WUInt32 uiHash, const CompatibleKeyType& key) const;

  WUInt32 GetFlagsCapacity() const;
  WUInt32 GetFlags(WUInt32* pFlags, WUInt32 uiEntryIndex) const;
  void SetFlags(WUInt32 uiEntryIndex, WUInt32 uiFlags);

  bool IsFreeEntry(WUInt32 uiEntryIndex) const;
  bool IsValidEntry(WUInt32 uiEntryIndex) const;
  bool IsDeletedEntry(WUInt32 uiEntryIndex) const;

  void MarkEntryAsFree(WUInt32 uiEntryIndex);
  void MarkEntryAsValid(WUInt32 uiEntryIndex);
  void MarkEntryAsDeleted(WUInt32 uiEntryIndex);
};

/// \see WHashSetBase
template <typename KeyType, typename Hasher = WHashHelper<KeyType>, typename AllocatorWrapper = WDefaultAllocatorWrapper>
class WHashSet : public WHashSetBase<KeyType, Hasher>
{
public:
  WHashSet();
  explicit WHashSet(WAllocator* pAllocator);

  WHashSet(const WHashSet<KeyType, Hasher, AllocatorWrapper>& other);
  WHashSet(const WHashSetBase<KeyType, Hasher>& other);

  WHashSet(WHashSet<KeyType, Hasher, AllocatorWrapper>&& other);
  WHashSet(WHashSetBase<KeyType, Hasher>&& other);

  void operator=(const WHashSet<KeyType, Hasher, AllocatorWrapper>& rhs);
  void operator=(const WHashSetBase<KeyType, Hasher>& rhs);

  void operator=(WHashSet<KeyType, Hasher, AllocatorWrapper>&& rhs);
  void operator=(WHashSetBase<KeyType, Hasher>&& rhs);
};

template <typename KeyType, typename Hasher>
typename WHashSetBase<KeyType, Hasher>::ConstIterator begin(const WHashSetBase<KeyType, Hasher>& set)
{
  return set.GetIterator();
}

template <typename KeyType, typename Hasher>
typename WHashSetBase<KeyType, Hasher>::ConstIterator cbegin(const WHashSetBase<KeyType, Hasher>& set)
{
  return set.GetIterator();
}

template <typename KeyType, typename Hasher>
typename WHashSetBase<KeyType, Hasher>::ConstIterator end(const WHashSetBase<KeyType, Hasher>& set)
{
  return set.GetEndIterator();
}

template <typename KeyType, typename Hasher>
typename WHashSetBase<KeyType, Hasher>::ConstIterator cend(const WHashSetBase<KeyType, Hasher>& set)
{
  return set.GetEndIterator();
}

#include <Foundation/Containers/Implementation/HashSet_inl.h>
