#pragma once

#include <Foundation/Memory/MemoryUtils.h>

#include <Foundation/Containers/Implementation/ArrayIterator.h>

// This #include is quite vital, do not remove it!
#include <Foundation/Strings/FormatString.h>

#include <Foundation/Math/Math.h>

#if W_ENABLED(W_INTEROP_STL_SPAN)
#  include <span>
#endif

/// Value used by containers for indices to indicate an invalid index.
#ifndef WInvalidIndex
#  define WInvalidIndex 0xFFFFFFFF
#endif

namespace WArrayPtrDetail
{
  template <typename U>
  struct ByteTypeHelper
  {
    using type = WUInt8;
  };

  template <typename U>
  struct ByteTypeHelper<const U>
  {
    using type = const WUInt8;
  };
} // namespace WArrayPtrDetail

/// This class encapsulates an array and it's size. It is recommended to use this class instead of plain C arrays.
///
/// No data is deallocated at destruction, the WArrayPtr only allows for easier access.
template <typename T>
class WArrayPtr
{
  template <typename U>
  friend class WArrayPtr;

public:
  W_DECLARE_POD_TYPE();

  static_assert(!std::is_same_v<T, void>, "WArrayPtr<void> is not allowed (anymore)");
  static_assert(!std::is_same_v<T, const void>, "WArrayPtr<void> is not allowed (anymore)");

  using ByteType = typename WArrayPtrDetail::ByteTypeHelper<T>::type;
  using ValueType = T;
  using PointerType = T*;

  /// Initializes the WArrayPtr to be empty.
  W_ALWAYS_INLINE WArrayPtr() // [tested]
    : m_pPtr(nullptr)
    , m_uiCount(0u)
  {
  }

  /// Copies the pointer and size of /a other. Does not allocate any data.
  W_ALWAYS_INLINE WArrayPtr(const WArrayPtr<T>& other) // [tested]
  {
    m_pPtr = other.m_pPtr;
    m_uiCount = other.m_uiCount;
  }

  /// Initializes the WArrayPtr with the given pointer and number of elements. No memory is allocated or copied.
  inline WArrayPtr(T* pPtr, WUInt32 uiCount) // [tested]
    : m_pPtr(pPtr)
    , m_uiCount(uiCount)
  {
    // If any of the arguments is invalid, we invalidate ourself.
    if (m_pPtr == nullptr || m_uiCount == 0)
    {
      m_pPtr = nullptr;
      m_uiCount = 0;
    }
  }

  /// Initializes the WArrayPtr to encapsulate the given array.
  template <size_t N>
  W_ALWAYS_INLINE WArrayPtr(T (&staticArray)[N]) // [tested]
    : m_pPtr(staticArray)
    , m_uiCount(static_cast<WUInt32>(N))
  {
  }

  /// Initializes the WArrayPtr to be a copy of \a other. No memory is allocated or copied.
  template <typename U>
  W_ALWAYS_INLINE WArrayPtr(const WArrayPtr<U>& other) // [tested]
    : m_pPtr(other.m_pPtr)
    , m_uiCount(other.m_uiCount)
  {
  }

#if W_ENABLED(W_INTEROP_STL_SPAN)
  template <typename U>
  W_ALWAYS_INLINE WArrayPtr(const std::span<U>& other)
    : m_pPtr(other.data())
    , m_uiCount((WUInt32)other.size())
  {
  }

  operator std::span<const T>() const
  {
    return std::span(GetPtr(), static_cast<size_t>(GetCount()));
  }

  operator std::span<T>()
  {
    return std::span(GetPtr(), static_cast<size_t>(GetCount()));
  }

  std::span<T> GetSpan()
  {
    return std::span(GetPtr(), static_cast<size_t>(GetCount()));
  }

  std::span<const T> GetSpan() const
  {
    return std::span(GetPtr(), static_cast<size_t>(GetCount()));
  }
#endif

  /// Convert to const version.
  operator WArrayPtr<const T>() const { return WArrayPtr<const T>(static_cast<const T*>(GetPtr()), GetCount()); } // [tested]

  /// Copies the pointer and size of /a other. Does not allocate any data.
  W_ALWAYS_INLINE void operator=(const WArrayPtr<T>& other) // [tested]
  {
    m_pPtr = other.m_pPtr;
    m_uiCount = other.m_uiCount;
  }

  /// Clears the array
  W_ALWAYS_INLINE void Clear()
  {
    m_pPtr = nullptr;
    m_uiCount = 0;
  }

  W_ALWAYS_INLINE void operator=(std::nullptr_t) // [tested]
  {
    m_pPtr = nullptr;
    m_uiCount = 0;
  }

  /// Returns the pointer to the array.
  W_ALWAYS_INLINE PointerType GetPtr() const // [tested]
  {
    return m_pPtr;
  }

  /// Returns the pointer to the array.
  W_ALWAYS_INLINE PointerType GetPtr() // [tested]
  {
    return m_pPtr;
  }

  /// Returns the pointer behind the last element of the array
  W_ALWAYS_INLINE PointerType GetEndPtr() { return m_pPtr + m_uiCount; }

  /// Returns the pointer behind the last element of the array
  W_ALWAYS_INLINE PointerType GetEndPtr() const { return m_pPtr + m_uiCount; }

  /// Returns whether the array is empty.
  W_ALWAYS_INLINE bool IsEmpty() const // [tested]
  {
    return GetCount() == 0;
  }

  /// Returns the number of elements in the array.
  W_ALWAYS_INLINE WUInt32 GetCount() const // [tested]
  {
    return m_uiCount;
  }

  /// Creates a sub-array from this array.
  W_FORCE_INLINE WArrayPtr<T> GetSubArray(WUInt32 uiStart, WUInt32 uiCount) const // [tested]
  {
    // the first check is necessary to also detect errors when uiStart+uiCount would overflow
    W_ASSERT_DEV(uiStart <= GetCount() && uiStart + uiCount <= GetCount(), "uiStart+uiCount ({0}) has to be smaller or equal than the count ({1}).",
      uiStart + uiCount, GetCount());
    return WArrayPtr<T>(GetPtr() + uiStart, uiCount);
  }

  /// Creates a sub-array from this array.
  /// \note \code ap.GetSubArray(i) \endcode is equivalent to \code ap.GetSubArray(i, ap.GetCount() - i) \endcode.
  W_FORCE_INLINE WArrayPtr<T> GetSubArray(WUInt32 uiStart) const // [tested]
  {
    W_ASSERT_DEV(uiStart <= GetCount(), "uiStart ({0}) has to be smaller or equal than the count ({1}).", uiStart, GetCount());
    return WArrayPtr<T>(GetPtr() + uiStart, GetCount() - uiStart);
  }

  /// Reinterprets this array as a byte array.
  W_ALWAYS_INLINE WArrayPtr<const ByteType> ToByteArray() const
  {
    return WArrayPtr<const ByteType>(reinterpret_cast<const ByteType*>(GetPtr()), GetCount() * sizeof(T));
  }

  /// Reinterprets this array as a byte array.
  W_ALWAYS_INLINE WArrayPtr<ByteType> ToByteArray() { return WArrayPtr<ByteType>(reinterpret_cast<ByteType*>(GetPtr()), GetCount() * sizeof(T)); }


  /// Cast an ArrayPtr to an ArrayPtr to a different, but same size, type
  template <typename U>
  W_ALWAYS_INLINE WArrayPtr<U> Cast()
  {
    static_assert(sizeof(T) == sizeof(U), "Can only cast with equivalent element size.");
    return WArrayPtr<U>(reinterpret_cast<U*>(GetPtr()), GetCount());
  }

  /// Cast an ArrayPtr to an ArrayPtr to a different, but same size, type
  template <typename U>
  W_ALWAYS_INLINE WArrayPtr<const U> Cast() const
  {
    static_assert(sizeof(T) == sizeof(U), "Can only cast with equivalent element size.");
    return WArrayPtr<const U>(reinterpret_cast<const U*>(GetPtr()), GetCount());
  }

  /// Index access.
  W_FORCE_INLINE const ValueType& operator[](WUInt32 uiIndex) const // [tested]
  {
    W_ASSERT_DEBUG(uiIndex < GetCount(), "Cannot access element {0}, the array only holds {1} elements.", uiIndex, GetCount());
    return *static_cast<const ValueType*>(GetPtr() + uiIndex);
  }

  /// Index access.
  W_FORCE_INLINE ValueType& operator[](WUInt32 uiIndex) // [tested]
  {
    W_ASSERT_DEBUG(uiIndex < GetCount(), "Cannot access element {0}, the array only holds {1} elements.", uiIndex, GetCount());
    return *static_cast<ValueType*>(GetPtr() + uiIndex);
  }

  /// Compares the two arrays for equality.
  template <typename = typename std::enable_if<std::is_const<T>::value == false>>
  inline bool operator==(const WArrayPtr<const T>& other) const // [tested]
  {
    if (GetCount() != other.GetCount())
      return false;

    if (GetPtr() == other.GetPtr())
      return true;

    return WMemoryUtils::IsEqual(static_cast<const ValueType*>(GetPtr()), static_cast<const ValueType*>(other.GetPtr()), GetCount());
  }

#if W_DISABLED(W_USE_CPP20_OPERATORS)
  template <typename = typename std::enable_if<std::is_const<T>::value == false>>
  inline bool operator!=(const WArrayPtr<const T>& other) const // [tested]
  {
    return !(*this == other);
  }
#endif

  /// Compares the two arrays for equality.
  inline bool operator==(const WArrayPtr<T>& other) const // [tested]
  {
    if (GetCount() != other.GetCount())
      return false;

    if (GetPtr() == other.GetPtr())
      return true;

    return WMemoryUtils::IsEqual(static_cast<const ValueType*>(GetPtr()), static_cast<const ValueType*>(other.GetPtr()), GetCount());
  }
  W_ADD_DEFAULT_OPERATOR_NOTEQUAL(const WArrayPtr<T>&);

  /// Compares the two arrays for less.
  inline bool operator<(const WArrayPtr<const T>& other) const // [tested]
  {
    if (GetCount() != other.GetCount())
      return GetCount() < other.GetCount();

    for (WUInt32 i = 0; i < GetCount(); ++i)
    {
      if (GetPtr()[i] < other.GetPtr()[i])
        return true;

      if (other.GetPtr()[i] < GetPtr()[i])
        return false;
    }

    return false;
  }

  /// Copies the data from \a other into this array. The arrays must have the exact same size.
  inline void CopyFrom(const WArrayPtr<const T>& other) // [tested]
  {
    W_ASSERT_DEV(GetCount() == other.GetCount(), "Count for copy does not match. Target has {0} elements, source {1} elements", GetCount(), other.GetCount());

    WMemoryUtils::Copy(static_cast<ValueType*>(GetPtr()), static_cast<const ValueType*>(other.GetPtr()), GetCount());
  }

  W_ALWAYS_INLINE void Swap(WArrayPtr<T>& other)
  {
    ::WMath::Swap(m_pPtr, other.m_pPtr);
    ::WMath::Swap(m_uiCount, other.m_uiCount);
  }

  /// Checks whether the given value can be found in the array. O(n) complexity.
  W_ALWAYS_INLINE bool Contains(const T& value) const // [tested]
  {
    return IndexOf(value) != WInvalidIndex;
  }

  /// Searches for the first occurrence of the given value and returns its index or WInvalidIndex if not found.
  inline WUInt32 IndexOf(const T& value, WUInt32 uiStartIndex = 0) const // [tested]
  {
    for (WUInt32 i = uiStartIndex; i < m_uiCount; ++i)
    {
      if (WMemoryUtils::IsEqual(m_pPtr + i, &value))
        return i;
    }

    return WInvalidIndex;
  }

  /// Searches for the last occurrence of the given value and returns its index or WInvalidIndex if not found.
  inline WUInt32 LastIndexOf(const T& value, WUInt32 uiStartIndex = WInvalidIndex) const // [tested]
  {
    for (WUInt32 i = ::WMath::Min(uiStartIndex, m_uiCount); i-- > 0;)
    {
      if (WMemoryUtils::IsEqual(m_pPtr + i, &value))
        return i;
    }
    return WInvalidIndex;
  }

  using const_iterator = const T*;
  using const_reverse_iterator = const_reverse_pointer_iterator<T>;
  using iterator = T*;
  using reverse_iterator = reverse_pointer_iterator<T>;

private:
  PointerType m_pPtr;
  WUInt32 m_uiCount;
};

//////////////////////////////////////////////////////////////////////////

using WByteArrayPtr = WArrayPtr<WUInt8>;
using WConstByteArrayPtr = WArrayPtr<const WUInt8>;

//////////////////////////////////////////////////////////////////////////

/// Helper function to create WArrayPtr from a pointer of some type and a count.
template <typename T>
W_ALWAYS_INLINE WArrayPtr<T> WMakeArrayPtr(T* pPtr, WUInt32 uiCount)
{
  return WArrayPtr<T>(pPtr, uiCount);
}

/// Helper function to create WArrayPtr from a static array the a size known at compile-time.
template <typename T, WUInt32 N>
W_ALWAYS_INLINE WArrayPtr<T> WMakeArrayPtr(T (&staticArray)[N])
{
  return WArrayPtr<T>(staticArray);
}

/// Helper function to create WConstByteArrayPtr from a pointer of some type and a count.
template <typename T>
W_ALWAYS_INLINE WConstByteArrayPtr WMakeByteArrayPtr(const T* pPtr, WUInt32 uiCount)
{
  return WConstByteArrayPtr(reinterpret_cast<const WUInt8*>(pPtr), uiCount * sizeof(T));
}

/// Helper function to create WByteArrayPtr from a pointer of some type and a count.
template <typename T>
W_ALWAYS_INLINE WByteArrayPtr WMakeByteArrayPtr(T* pPtr, WUInt32 uiCount)
{
  return WByteArrayPtr(reinterpret_cast<WUInt8*>(pPtr), uiCount * sizeof(T));
}

/// Helper function to create WByteArrayPtr from a void pointer and a count.
W_ALWAYS_INLINE WByteArrayPtr WMakeByteArrayPtr(void* pPtr, WUInt32 uiBytes)
{
  return WByteArrayPtr(static_cast<WUInt8*>(pPtr), uiBytes);
}

/// Helper function to create WConstByteArrayPtr from a const void pointer and a count.
W_ALWAYS_INLINE WConstByteArrayPtr WMakeByteArrayPtr(const void* pPtr, WUInt32 uiBytes)
{
  return WConstByteArrayPtr(static_cast<const WUInt8*>(pPtr), uiBytes);
}

//////////////////////////////////////////////////////////////////////////

template <typename T>
typename WArrayPtr<T>::iterator begin(WArrayPtr<T>& ref_container)
{
  return ref_container.GetPtr();
}

template <typename T>
typename WArrayPtr<T>::const_iterator begin(const WArrayPtr<T>& container)
{
  return container.GetPtr();
}

template <typename T>
typename WArrayPtr<T>::const_iterator cbegin(const WArrayPtr<T>& container)
{
  return container.GetPtr();
}

template <typename T>
typename WArrayPtr<T>::reverse_iterator rbegin(WArrayPtr<T>& ref_container)
{
  return typename WArrayPtr<T>::reverse_iterator(ref_container.GetPtr() + ref_container.GetCount() - 1);
}

template <typename T>
typename WArrayPtr<T>::const_reverse_iterator rbegin(const WArrayPtr<T>& container)
{
  return typename WArrayPtr<T>::const_reverse_iterator(container.GetPtr() + container.GetCount() - 1);
}

template <typename T>
typename WArrayPtr<T>::const_reverse_iterator crbegin(const WArrayPtr<T>& container)
{
  return typename WArrayPtr<T>::const_reverse_iterator(container.GetPtr() + container.GetCount() - 1);
}

template <typename T>
typename WArrayPtr<T>::iterator end(WArrayPtr<T>& ref_container)
{
  return ref_container.GetPtr() + ref_container.GetCount();
}

template <typename T>
typename WArrayPtr<T>::const_iterator end(const WArrayPtr<T>& container)
{
  return container.GetPtr() + container.GetCount();
}

template <typename T>
typename WArrayPtr<T>::const_iterator cend(const WArrayPtr<T>& container)
{
  return container.GetPtr() + container.GetCount();
}

template <typename T>
typename WArrayPtr<T>::reverse_iterator rend(WArrayPtr<T>& ref_container)
{
  return typename WArrayPtr<T>::reverse_iterator(ref_container.GetPtr() - 1);
}

template <typename T>
typename WArrayPtr<T>::const_reverse_iterator rend(const WArrayPtr<T>& container)
{
  return typename WArrayPtr<T>::const_reverse_iterator(container.GetPtr() - 1);
}

template <typename T>
typename WArrayPtr<T>::const_reverse_iterator crend(const WArrayPtr<T>& container)
{
  return typename WArrayPtr<T>::const_reverse_iterator(container.GetPtr() - 1);
}
