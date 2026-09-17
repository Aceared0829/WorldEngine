
#pragma once

#include <Foundation/Containers/SmallArray.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/TagRegistry.h>

class WTag;
using WTagSetBlockStorage = WUInt64;

/// A dynamic collection of tags featuring fast lookups.
///
/// This class can be used to store a (dynamic) collection of tags. Tags are registered within
/// the global tag registry and allocated a bit index. The tag set allows comparatively fast lookups
/// to check if a given tag is in the set or not.
/// Adding a tag may have some overhead depending whether the block storage for the tag
/// bit indices needs to be expanded or not (if the storage needs to be expanded the hybrid array will be resized).
/// Typical storage requirements for a given tag set instance should be small since the block storage is a sliding
/// window. The standard class which can be used is WTagSet, usage of WTagSetTemplate is only necessary
/// if the allocator needs to be overridden.
template <typename BlockStorageAllocator = WDefaultAllocatorWrapper>
class WTagSetTemplate
{
public:
  WTagSetTemplate();

  bool operator==(const WTagSetTemplate& other) const;
  bool operator!=(const WTagSetTemplate& other) const;

  /// Adds the given tag to the set.
  void Set(const WTag& tag); // [tested]

  /// Removes the given tag.
  void Remove(const WTag& tag); // [tested]

  /// Returns true, if the given tag is in the set.
  bool IsSet(const WTag& tag) const; // [tested]

  /// Returns true if this tag set contains any tag set in the given other tag set.
  bool IsAnySet(const WTagSetTemplate& otherSet) const; // [tested]

  /// Returns how many tags are in this set.
  WUInt32 GetNumTagsSet() const;

  /// True if the tag set never contained any tag or was cleared.
  bool IsEmpty() const;

  /// Removes all tags from the set
  void Clear();

  /// Adds the tag with the given name. If the tag does not exist, it will be registered.
  void SetByName(WStringView sTag);

  /// Removes the given tag. If it doesn't exist, nothing happens.
  void RemoveByName(WStringView sTag);

  /// Checks whether the named tag is part of this set. Returns false if the tag does not exist.
  bool IsSetByName(WStringView sTag) const;

  /// Checks whether the named tag is part of this set. Returns false if the tag does not exist.
  bool IsSetByName(const WTempHashedString& sTag) const;

  /// Allows to iterate over all tags in this set
  class Iterator
  {
  public:
    Iterator(const WTagSetTemplate<BlockStorageAllocator>* pSet, bool bEnd = false);

    /// Returns a reference to the current tag
    const WTag& operator*() const;

    /// Returns a pointer to the current tag
    const WTag* operator->() const;

    /// Returns whether the iterator is still pointing to a valid item
    W_ALWAYS_INLINE bool IsValid() const { return m_uiIndex != 0xFFFFFFFF; }

    W_ALWAYS_INLINE bool operator!=(const Iterator& rhs) const { return m_pTagSet != rhs.m_pTagSet || m_uiIndex != rhs.m_uiIndex; }

    /// Advances the iterator to the next item
    void operator++();

  private:
    bool IsBitSet() const;

    const WTagSetTemplate<BlockStorageAllocator>* m_pTagSet;
    WUInt32 m_uiIndex = 0;
  };

  /// Returns an iterator to list all tags in this set
  Iterator GetIterator() const { return Iterator(this); }

  /// Writes the tag set state to a stream. Tags itself are serialized as strings.
  void Save(WStreamWriter& inout_stream) const;

  /// Reads the tag set state from a stream and registers the tags with the given registry.
  void Load(WStreamReader& inout_stream, WTagRegistry& inout_registry);

private:
  friend class Iterator;

  bool IsTagInAllocatedRange(const WTag& Tag) const;

  void Reallocate(WUInt32 uiNewTagBlockStart, WUInt32 uiNewMaxBlockIndex);

  WSmallArray<WTagSetBlockStorage, 1, BlockStorageAllocator> m_TagBlocks;

  struct UserData
  {
    WUInt16 m_uiTagBlockStart;
    WUInt16 m_uiTagCount;
  };

  WUInt16 GetTagBlockStart() const;
  WUInt16 GetTagBlockEnd() const;
  void SetTagBlockStart(WUInt16 uiTagBlockStart);

  WUInt16 GetTagCount() const;
  void SetTagCount(WUInt16 uiTagCount);
  void IncreaseTagCount();
  void DecreaseTagCount();
};

/// Default tag set, uses WDefaultAllocatorWrapper for allocations.
using WTagSet = WTagSetTemplate<>;

W_DECLARE_REFLECTABLE_TYPE(W_FOUNDATION_DLL, WTagSet);

template <typename BlockStorageAllocator>
typename WTagSetTemplate<BlockStorageAllocator>::Iterator cbegin(const WTagSetTemplate<BlockStorageAllocator>& cont)
{
  return typename WTagSetTemplate<BlockStorageAllocator>::Iterator(&cont);
}

template <typename BlockStorageAllocator>
typename WTagSetTemplate<BlockStorageAllocator>::Iterator cend(const WTagSetTemplate<BlockStorageAllocator>& cont)
{
  return typename WTagSetTemplate<BlockStorageAllocator>::Iterator(&cont, true);
}

template <typename BlockStorageAllocator>
typename WTagSetTemplate<BlockStorageAllocator>::Iterator begin(const WTagSetTemplate<BlockStorageAllocator>& cont)
{
  return typename WTagSetTemplate<BlockStorageAllocator>::Iterator(&cont);
}

template <typename BlockStorageAllocator>
typename WTagSetTemplate<BlockStorageAllocator>::Iterator end(const WTagSetTemplate<BlockStorageAllocator>& cont)
{
  return typename WTagSetTemplate<BlockStorageAllocator>::Iterator(&cont, true);
}

#include <Foundation/Types/Implementation/TagSet_inl.h>
