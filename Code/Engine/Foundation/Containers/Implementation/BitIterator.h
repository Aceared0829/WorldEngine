#pragma once

#include <Foundation/Math/Math.h>

/// Chooses either WUInt32 or WUInt64 as the storage type for a given type T depending on its size. Required as WMath::FirstBitLow only supports WUInt32 or WUInt64.
/// \tparam T Type for which the storage should be inferred.
template <typename T, typename = std::void_t<>>
struct WBitIteratorStorage;
template <typename T>
struct WBitIteratorStorage<T, std::enable_if_t<sizeof(T) <= 4>>
{
  using Type = WUInt32;
};
template <typename T>
struct WBitIteratorStorage<T, std::enable_if_t<sizeof(T) >= 5>>
{
  using Type = WUInt64;
};

/// Configurable bit iterator. Allows for iterating over the bits in an integer, returning either the bit index or value.
/// \tparam DataType The type of data that is being iterated over.
/// \tparam ReturnsIndex If set, returns the index of the bit. Otherwise returns the value of the bit, i.e. W_BIT(value).
/// \tparam ReturnType Returned value type of the iterator. Defaults to same as DataType.
/// \tparam StorageType The storage type that the bit operations are performed on (either WUInt32 or WUInt64). Auto-computed.
template <typename DataType, bool ReturnsIndex = true, typename ReturnType = DataType, typename StorageType = typename WBitIteratorStorage<DataType>::Type>
struct WBitIterator
{
  using iterator_category = std::forward_iterator_tag;
  using value_type = DataType;
  static_assert(sizeof(DataType) <= 8);

  // Invalid iterator (end)
  W_ALWAYS_INLINE WBitIterator() = default;

  // Start iterator.
  W_ALWAYS_INLINE explicit WBitIterator(DataType data)
  {
    m_uiMask = static_cast<StorageType>(data);
  }

  W_ALWAYS_INLINE bool IsValid() const
  {
    return m_uiMask != 0;
  }

  W_ALWAYS_INLINE ReturnType Value() const
  {
    if constexpr (ReturnsIndex)
    {
      return static_cast<ReturnType>(WMath::FirstBitLow(m_uiMask));
    }
    else
    {
      return static_cast<ReturnType>(W_BIT(WMath::FirstBitLow(m_uiMask)));
    }
  }

  W_ALWAYS_INLINE void Next()
  {
    // Clear the lowest set bit. Why this works: https://www.geeksforgeeks.org/turn-off-the-rightmost-set-bit/
    m_uiMask = m_uiMask & (m_uiMask - 1);
  }

  W_ALWAYS_INLINE bool operator==(const WBitIterator& other) const
  {
    return m_uiMask == other.m_uiMask;
  }

  W_ALWAYS_INLINE bool operator!=(const WBitIterator& other) const
  {
    return m_uiMask != other.m_uiMask;
  }

  W_ALWAYS_INLINE ReturnType operator*() const
  {
    return Value();
  }

  W_ALWAYS_INLINE void operator++()
  {
    Next();
  }

  StorageType m_uiMask = 0;
};
