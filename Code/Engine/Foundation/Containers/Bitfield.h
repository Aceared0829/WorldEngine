#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Implementation/BitIterator.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Math/Constants.h>

/// A template interface, that turns any array class into a bitfield.
///
/// This class provides an interface to work with single bits, to store true/false values.
/// The underlying container is configurable, though it must support random access and a 'SetCount' function and it must use elements of type
/// WUInt32. In most cases a dynamic array should be used. For this case the WDynamicBitfield typedef is already available. There is also an
/// WHybridBitfield typedef.
template <class Container>
class WBitfield
{
public:
  WBitfield() = default;

  /// Returns the number of bits that this bitfield stores.
  WUInt32 GetCount() const; // [tested]

  /// Resizes the Bitfield to hold the given number of bits. This version does NOT initialize new bits!
  template <typename = void>                       // Template is used to only conditionally compile this function in when it is actually used.
  void SetCountUninitialized(WUInt32 uiBitCount); // [tested]

  /// Resizes the Bitfield to hold the given number of bits. If \a bSetNew is true, new bits are set to 1, otherwise they are cleared to 0.
  void SetCount(WUInt32 uiBitCount, bool bSetNew = false); // [tested]

  /// Returns true, if the bitfield does not store any bits.
  bool IsEmpty() const; // [tested]

  /// Returns true, if the bitfield is not empty and any bit is 1.
  bool IsAnyBitSet(WUInt32 uiFirstBit = 0, WUInt32 uiNumBits = 0xFFFFFFFF) const; // [tested]

  /// Returns true, if the bitfield is empty or all bits are set to zero.
  bool IsNoBitSet(WUInt32 uiFirstBit = 0, WUInt32 uiNumBits = 0xFFFFFFFF) const; // [tested]

  /// Returns true, if the bitfield is not empty and all bits are set to one.
  bool AreAllBitsSet(WUInt32 uiFirstBit = 0, WUInt32 uiNumBits = 0xFFFFFFFF) const; // [tested]

  /// Discards all bits and sets count to zero.
  void Clear(); // [tested]

  /// Sets the given bit to 1.
  void SetBit(WUInt32 uiBit); // [tested]

  /// Clears the given bit to 0.
  void ClearBit(WUInt32 uiBit); // [tested]

  /// Flips the given bit to the opposite value.
  void FlipBit(WUInt32 uiBit); // [tested]

  /// Sets the given bit to 1 or 0 depending on the given value.
  void SetBitValue(WUInt32 uiBit, bool bValue); // [tested]

  /// Returns true, if the given bit is set to 1.
  bool IsBitSet(WUInt32 uiBit) const; // [tested]

  /// Clears all bits to 0.
  void ClearAllBits(); // [tested]

  /// Sets all bits to 1.
  void SetAllBits(); // [tested]

  /// Sets the range starting at uiFirstBit up to (and including) uiLastBit to 1.
  void SetBitRange(WUInt32 uiFirstBit, WUInt32 uiNumBits); // [tested]

  /// Clears the range starting at uiFirstBit up to (and including) uiLastBit to 0.
  void ClearBitRange(WUInt32 uiFirstBit, WUInt32 uiNumBits); // [tested]

  /// Flips the range starting at uiFirstBit up to (and including) uiLastBit.
  void FlipBitRange(WUInt32 uiFirstBit, WUInt32 uiNumBits); // [tested]

  /// Swaps two bitfields
  void Swap(WBitfield<Container>& other); // [tested]
  struct ConstIterator
  {
    using iterator_category = std::forward_iterator_tag;
    using value_type = WUInt32;
    using sub_iterator = ::WBitIterator<WUInt32, true>;

    // Invalid iterator (end)
    W_FORCE_INLINE ConstIterator() = default; // [tested]

    // Start iterator.
    explicit ConstIterator(const WBitfield<Container>& bitfield); // [tested]

    /// Checks whether this iterator points to a valid element.
    bool IsValid() const; // [tested]

    /// Returns the 'value' of the element that this iterator points to.
    WUInt32 Value() const; // [tested]

    /// Advances the iterator to the next element in the map. The iterator will not be valid anymore, if the end is reached.
    void Next();                                       // [tested]

    bool operator==(const ConstIterator& other) const; // [tested]
    bool operator!=(const ConstIterator& other) const; // [tested]

    /// Returns 'Value()' to enable foreach.
    WUInt32 operator*() const; // [tested]

    /// Shorthand for 'Next'.
    void operator++(); // [tested]

  private:
    void FindNextChunk(WUInt32 uiStartChunk);

  private:
    WUInt32 m_uiChunk = 0;
    sub_iterator m_Iterator;
    const WBitfield<Container>* m_pBitfield = nullptr;
  };

  /// Returns a constant iterator to the very first set bit.
  /// Note that due to the way iterating through bits is accelerated, changes to the bitfield while iterating through the bits has undefined behaviour.
  ConstIterator GetIterator() const; // [tested]

  /// Returns an invalid iterator. Needed to support range based for loops.
  ConstIterator GetEndIterator() const; // [tested]

private:
  friend struct ConstIterator;

  WUInt32 GetBitInt(WUInt32 uiBitIndex) const;
  WUInt32 GetBitMask(WUInt32 uiBitIndex) const;

  WUInt32 m_uiCount = 0;
  Container m_Container;
};

/// This should be the main type of bitfield to use, although other internal container types are possible.
using WDynamicBitfield = WBitfield<WDynamicArray<WUInt32>>;

/// An WBitfield that uses a hybrid array as internal container.
template <WUInt32 BITS>
using WHybridBitfield = WBitfield<WHybridArray<WUInt32, (BITS + 31) / 32>>;

//////////////////////////////////////////////////////////////////////////
// begin() /end() for range-based for-loop support
template <typename Container>
typename WBitfield<Container>::ConstIterator begin(const WBitfield<Container>& container)
{
  return container.GetIterator();
}

template <typename Container>
typename WBitfield<Container>::ConstIterator cbegin(const WBitfield<Container>& container)
{
  return container.GetIterator();
}

template <typename Container>
typename WBitfield<Container>::ConstIterator end(const WBitfield<Container>& container)
{
  return container.GetEndIterator();
}

template <typename Container>
typename WBitfield<Container>::ConstIterator cend(const WBitfield<Container>& container)
{
  return container.GetEndIterator();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

template <typename T>
class WStaticBitfield
{
public:
  using StorageType = T;
  using ConstIterator = WBitIterator<StorageType, true, WUInt32>;

  static constexpr WUInt32 GetStorageTypeBitCount() { return WMath::NumBits<T>(); }

  /// Initializes the bitfield to all zero.
  WStaticBitfield();

  static WStaticBitfield<T> MakeFromMask(StorageType bits);

  /// Returns true, if the bitfield is not zero.
  bool IsAnyBitSet() const; // [tested]

  /// Returns true, if the bitfield is all zero.
  bool IsNoBitSet() const; // [tested]

  /// Returns true, if the bitfield is not empty and all bits are set to one.
  bool AreAllBitsSet() const; // [tested]

  /// Sets the given bit to 1.
  void SetBit(WUInt32 uiBit); // [tested]

  /// Clears the given bit to 0.
  void ClearBit(WUInt32 uiBit); // [tested]

  /// Sets the given bit to 1 or 0 depending on the given value.
  void SetBitValue(WUInt32 uiBit, bool bValue); // [tested]

  /// Returns true, if the given bit is set to 1.
  bool IsBitSet(WUInt32 uiBit) const; // [tested]

  /// Clears all bits to 0. Same as Clear().
  void ClearAllBits(); // [tested]

  /// Sets all bits to 1.
  void SetAllBits(); // [tested]

  /// Sets the range starting at uiFirstBit up to (and including) uiLastBit to 1.
  void SetBitRange(WUInt32 uiFirstBit, WUInt32 uiNumBits); // [tested]

  /// Clears the range starting at uiFirstBit up to (and including) uiLastBit to 0.
  void ClearBitRange(WUInt32 uiFirstBit, WUInt32 uiNumBits); // [tested]

  /// Returns the index of the lowest bit that is set. Returns the max index+1 in case no bit is set, at all.
  WUInt32 GetLowestBitSet() const; // [tested]

  /// Returns the index of the highest bit that is set. Returns the max index+1 in case no bit is set, at all.
  WUInt32 GetHighestBitSet() const; // [tested]

  /// Returns the count of how many bits are set in total.
  WUInt32 GetNumBitsSet() const; // [tested]

  /// Returns the raw uint that stores all bits.
  T GetValue() const; // [tested]

  /// Sets the raw uint that stores all bits.
  void SetValue(T value); // [tested]

  /// Swaps two bitfields
  void Swap(WStaticBitfield<T>& other); // [tested]

  /// Modifies \a this to also contain the bits from \a rhs.
  W_ALWAYS_INLINE void operator|=(const WStaticBitfield<T>& rhs) { m_Storage |= rhs.m_Storage; }

  /// Modifies \a this to only contain the bits that were set in \a this and \a rhs.
  W_ALWAYS_INLINE void operator&=(const WStaticBitfield<T>& rhs) { m_Storage &= rhs.m_Storage; }

  WResult Serialize(WStreamWriter& inout_writer) const
  {
    inout_writer.WriteVersion(s_Version);
    inout_writer << m_Storage;
    return W_SUCCESS;
  }

  WResult Deserialize(WStreamReader& inout_reader)
  {
    /*auto version =*/inout_reader.ReadVersion(s_Version);
    inout_reader >> m_Storage;
    return W_SUCCESS;
  }

  /// Returns a constant iterator to the very first set bit.
  /// Note that due to the way iterating through bits is accelerated, changes to the bitfield while iterating through the bits has undefined behaviour.
  ConstIterator GetIterator() const // [tested]
  {
    return ConstIterator(m_Storage);
  };

  /// Returns an invalid iterator. Needed to support range based for loops.
  ConstIterator GetEndIterator() const // [tested]
  {
    return ConstIterator();
  };

private:
  static constexpr WTypeVersion s_Version = 1;

  WStaticBitfield(StorageType initValue)
    : m_Storage(initValue)
  {
  }

  template <typename U>
  friend WStaticBitfield<U> operator|(WStaticBitfield<U> lhs, WStaticBitfield<U> rhs);

  template <typename U>
  friend WStaticBitfield<U> operator&(WStaticBitfield<U> lhs, WStaticBitfield<U> rhs);

  template <typename U>
  friend WStaticBitfield<U> operator^(WStaticBitfield<U> lhs, WStaticBitfield<U> rhs);

  template <typename U>
  friend bool operator==(WStaticBitfield<U> lhs, WStaticBitfield<U> rhs);

  template <typename U>
  friend bool operator!=(WStaticBitfield<U> lhs, WStaticBitfield<U> rhs);

  StorageType m_Storage = 0;
};

template <typename T>
inline WStaticBitfield<T> operator|(WStaticBitfield<T> lhs, WStaticBitfield<T> rhs)
{
  return WStaticBitfield<T>(lhs.m_Storage | rhs.m_Storage);
}

template <typename T>
inline WStaticBitfield<T> operator&(WStaticBitfield<T> lhs, WStaticBitfield<T> rhs)
{
  return WStaticBitfield<T>(lhs.m_Storage & rhs.m_Storage);
}

template <typename T>
inline WStaticBitfield<T> operator^(WStaticBitfield<T> lhs, WStaticBitfield<T> rhs)
{
  return WStaticBitfield<T>(lhs.m_Storage ^ rhs.m_Storage);
}

template <typename T>
inline bool operator==(WStaticBitfield<T> lhs, WStaticBitfield<T> rhs)
{
  return lhs.m_Storage == rhs.m_Storage;
}

template <typename T>
inline bool operator!=(WStaticBitfield<T> lhs, WStaticBitfield<T> rhs)
{
  return lhs.m_Storage != rhs.m_Storage;
}

//////////////////////////////////////////////////////////////////////////
// begin() /end() for range-based for-loop support
template <typename Container>
typename WStaticBitfield<Container>::ConstIterator begin(const WStaticBitfield<Container>& container)
{
  return container.GetIterator();
}

template <typename Container>
typename WStaticBitfield<Container>::ConstIterator cbegin(const WStaticBitfield<Container>& container)
{
  return container.GetIterator();
}

template <typename Container>
typename WStaticBitfield<Container>::ConstIterator end(const WStaticBitfield<Container>& container)
{
  return container.GetEndIterator();
}

template <typename Container>
typename WStaticBitfield<Container>::ConstIterator cend(const WStaticBitfield<Container>& container)
{
  return container.GetEndIterator();
}

using WStaticBitfield8 = WStaticBitfield<WUInt8>;
using WStaticBitfield16 = WStaticBitfield<WUInt16>;
using WStaticBitfield32 = WStaticBitfield<WUInt32>;
using WStaticBitfield64 = WStaticBitfield<WUInt64>;

#include <Foundation/Containers/Implementation/Bitfield_inl.h>
