
#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Types/ArrayPtr.h>

/// This class encapsulates a blob's storage and it's size. It is recommended to use this class instead of directly working on the void* of the
/// blob.
///
/// No data is deallocated at destruction, the WBlobPtr only allows for easier access.
template <typename T>
class WBlobPtr
{
public:
  W_DECLARE_POD_TYPE();

  static_assert(!std::is_same_v<T, void>, "WBlobPtr<void> is not allowed (anymore)");
  static_assert(!std::is_same_v<T, const void>, "WBlobPtr<void> is not allowed (anymore)");

  using ByteType = typename WArrayPtrDetail::ByteTypeHelper<T>::type;
  using ValueType = T;
  using PointerType = T*;

  /// Initializes the WBlobPtr to be empty.
  WBlobPtr() = default;

  /// Initializes the WBlobPtr with the given pointer and number of elements. No memory is allocated or copied.
  template <typename U>
  inline WBlobPtr(U* pPtr, WUInt64 uiCount)
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

  /// Initializes the WBlobPtr to encapsulate the given array.
  template <size_t N>
  W_ALWAYS_INLINE WBlobPtr(ValueType (&staticArray)[N])
    : m_pPtr(staticArray)
    , m_uiCount(static_cast<WUInt64>(N))
  {
  }

  /// Initializes the WBlobPtr to be a copy of \a other. No memory is allocated or copied.
  W_ALWAYS_INLINE WBlobPtr(const WBlobPtr<T>& other)
    : m_pPtr(other.m_pPtr)
    , m_uiCount(other.m_uiCount)
  {
  }

  /// Initializes the WBlobPtr to be a copy of \a other. No memory is allocated or copied.
  W_ALWAYS_INLINE WBlobPtr(const WArrayPtr<T>& other)
    : m_pPtr(other.GetPtr())
    , m_uiCount(other.GetCount())
  {
  }

  /// Convert to const version.
  operator WBlobPtr<const T>() const { return WBlobPtr<const T>(static_cast<const T*>(GetPtr()), GetCount()); }

  /// Copies the pointer and size of /a other. Does not allocate any data.
  W_ALWAYS_INLINE void operator=(const WBlobPtr<T>& other)
  {
    m_pPtr = other.m_pPtr;
    m_uiCount = other.m_uiCount;
  }

  /// Copies the pointer and size of /a other. Does not allocate any data.
  W_ALWAYS_INLINE void operator=(const WArrayPtr<T>& other)
  {
    m_pPtr = other.GetPtr();
    m_uiCount = other.GetCount();
  }

  /// Clears the array
  W_ALWAYS_INLINE void Clear()
  {
    m_pPtr = nullptr;
    m_uiCount = 0;
  }

  W_ALWAYS_INLINE void operator=(std::nullptr_t)
  {
    m_pPtr = nullptr;
    m_uiCount = 0;
  }

  /// Returns the pointer to the array.
  W_ALWAYS_INLINE PointerType GetPtr() const { return m_pPtr; }

  /// Returns the pointer to the array.
  W_ALWAYS_INLINE PointerType GetPtr() { return m_pPtr; }

  /// Returns the pointer behind the last element of the array
  W_ALWAYS_INLINE PointerType GetEndPtr() { return m_pPtr + m_uiCount; }

  /// Returns the pointer behind the last element of the array
  W_ALWAYS_INLINE PointerType GetEndPtr() const { return m_pPtr + m_uiCount; }

  /// Returns whether the array is empty.
  W_ALWAYS_INLINE bool IsEmpty() const { return GetCount() == 0; }

  /// Returns the number of elements in the array.
  W_ALWAYS_INLINE WUInt64 GetCount() const { return m_uiCount; }

  /// Creates a sub-array from this array.
  W_FORCE_INLINE WBlobPtr<T> GetSubArray(WUInt64 uiStart, WUInt64 uiCount) const // [tested]
  {
    W_ASSERT_DEV(
      uiStart + uiCount <= GetCount(), "uiStart+uiCount ({0}) has to be smaller or equal than the count ({1}).", uiStart + uiCount, GetCount());
    return WBlobPtr<T>(GetPtr() + uiStart, uiCount);
  }

  /// Creates a sub-array from this array.
  /// \note \code ap.GetSubArray(i) \endcode is equivalent to \code ap.GetSubArray(i, ap.GetCount() - i) \endcode.
  W_FORCE_INLINE WBlobPtr<T> GetSubArray(WUInt64 uiStart) const // [tested]
  {
    W_ASSERT_DEV(uiStart <= GetCount(), "uiStart ({0}) has to be smaller or equal than the count ({1}).", uiStart, GetCount());
    return WBlobPtr<T>(GetPtr() + uiStart, GetCount() - uiStart);
  }

  /// Reinterprets this array as a byte array.
  W_ALWAYS_INLINE WBlobPtr<const ByteType> ToByteBlob() const
  {
    return WBlobPtr<const ByteType>(reinterpret_cast<const ByteType*>(GetPtr()), GetCount() * sizeof(T));
  }

  /// Reinterprets this array as a byte array.
  W_ALWAYS_INLINE WBlobPtr<ByteType> ToByteBlob() { return WBlobPtr<ByteType>(reinterpret_cast<ByteType*>(GetPtr()), GetCount() * sizeof(T)); }

  /// Cast an BlobPtr to an BlobPtr to a different, but same size, type
  template <typename U>
  W_ALWAYS_INLINE WBlobPtr<U> Cast()
  {
    static_assert(sizeof(T) == sizeof(U), "Can only cast with equivalent element size.");
    return WBlobPtr<U>(reinterpret_cast<U*>(GetPtr()), GetCount());
  }

  /// Cast an BlobPtr to an BlobPtr to a different, but same size, type
  template <typename U>
  W_ALWAYS_INLINE WBlobPtr<const U> Cast() const
  {
    static_assert(sizeof(T) == sizeof(U), "Can only cast with equivalent element size.");
    return WBlobPtr<const U>(reinterpret_cast<const U*>(GetPtr()), GetCount());
  }

  /// Index access.
  W_FORCE_INLINE const ValueType& operator[](WUInt64 uiIndex) const // [tested]
  {
    W_ASSERT_DEBUG(uiIndex < GetCount(), "Cannot access element {0}, the array only holds {1} elements.", uiIndex, GetCount());
    return *static_cast<const ValueType*>(GetPtr() + uiIndex);
  }

  /// Index access.
  W_FORCE_INLINE ValueType& operator[](WUInt64 uiIndex) // [tested]
  {
    W_ASSERT_DEBUG(uiIndex < GetCount(), "Cannot access element {0}, the array only holds {1} elements.", uiIndex, GetCount());
    return *static_cast<ValueType*>(GetPtr() + uiIndex);
  }

  /// Compares the two arrays for equality.
  inline bool operator==(const WBlobPtr<const T>& other) const // [tested]
  {
    if (GetCount() != other.GetCount())
      return false;

    if (GetPtr() == other.GetPtr())
      return true;

    return WMemoryUtils::IsEqual(static_cast<const ValueType*>(GetPtr()), static_cast<const ValueType*>(other.GetPtr()), static_cast<size_t>(GetCount()));
  }

  /// Compares the two arrays for inequality.
  W_ALWAYS_INLINE bool operator!=(const WBlobPtr<const T>& other) const // [tested]
  {
    return !(*this == other);
  }

  /// Copies the data from \a other into this array. The arrays must have the exact same size.
  inline void CopyFrom(const WBlobPtr<const T>& other) // [tested]
  {
    W_ASSERT_DEV(GetCount() == other.GetCount(), "Count for copy does not match. Target has {0} elements, source {1} elements", GetCount(), other.GetCount());

    WMemoryUtils::Copy(static_cast<ValueType*>(GetPtr()), static_cast<const ValueType*>(other.GetPtr()), static_cast<size_t>(GetCount()));
  }

  W_ALWAYS_INLINE void Swap(WBlobPtr<T>& other)
  {
    WMath::Swap(m_pPtr, other.m_pPtr);
    WMath::Swap(m_uiCount, other.m_uiCount);
  }

  using const_iterator = const T*;
  using const_reverse_iterator = const_reverse_pointer_iterator<T>;
  using iterator = T*;
  using reverse_iterator = reverse_pointer_iterator<T>;

private:
  PointerType m_pPtr = nullptr;
  WUInt64 m_uiCount = 0u;
};

//////////////////////////////////////////////////////////////////////////

using WByteBlobPtr = WBlobPtr<WUInt8>;
using WConstByteBlobPtr = WBlobPtr<const WUInt8>;

//////////////////////////////////////////////////////////////////////////

/// Helper function to create WBlobPtr from a pointer of some type and a count.
template <typename T>
W_ALWAYS_INLINE WBlobPtr<T> WMakeBlobPtr(T* pPtr, WUInt64 uiCount)
{
  return WBlobPtr<T>(pPtr, uiCount);
}

/// Helper function to create WBlobPtr from a static array the a size known at compile-time.
template <typename T, WUInt64 N>
W_ALWAYS_INLINE WBlobPtr<T> WMakeBlobPtr(T (&staticArray)[N])
{
  return WBlobPtr<T>(staticArray);
}

/// Helper function to create WConstByteBlobPtr from a pointer of some type and a count.
template <typename T>
W_ALWAYS_INLINE WConstByteBlobPtr WMakeByteBlobPtr(const T* pPtr, WUInt32 uiCount)
{
  return WConstByteBlobPtr(static_cast<const WUInt8*>(pPtr), uiCount * sizeof(T));
}

/// Helper function to create WByteBlobPtr from a pointer of some type and a count.
template <typename T>
W_ALWAYS_INLINE WByteBlobPtr WMakeByteBlobPtr(T* pPtr, WUInt32 uiCount)
{
  return WByteBlobPtr(reinterpret_cast<WUInt8*>(pPtr), uiCount * sizeof(T));
}

/// Helper function to create WByteBlobPtr from a void pointer and a count.
W_ALWAYS_INLINE WByteBlobPtr WMakeByteBlobPtr(void* pPtr, WUInt32 uiBytes)
{
  return WByteBlobPtr(reinterpret_cast<WUInt8*>(pPtr), uiBytes);
}

/// Helper function to create WConstByteBlobPtr from a const void pointer and a count.
W_ALWAYS_INLINE WConstByteBlobPtr WMakeByteBlobPtr(const void* pPtr, WUInt32 uiBytes)
{
  return WConstByteBlobPtr(static_cast<const WUInt8*>(pPtr), uiBytes);
}

//////////////////////////////////////////////////////////////////////////

template <typename T>
typename WBlobPtr<T>::iterator begin(WBlobPtr<T>& in_container)
{
  return in_container.GetPtr();
}

template <typename T>
typename WBlobPtr<T>::const_iterator begin(const WBlobPtr<T>& container)
{
  return container.GetPtr();
}

template <typename T>
typename WBlobPtr<T>::const_iterator cbegin(const WBlobPtr<T>& container)
{
  return container.GetPtr();
}

template <typename T>
typename WBlobPtr<T>::reverse_iterator rbegin(WBlobPtr<T>& in_container)
{
  return typename WBlobPtr<T>::reverse_iterator(in_container.GetPtr() + in_container.GetCount() - 1);
}

template <typename T>
typename WBlobPtr<T>::const_reverse_iterator rbegin(const WBlobPtr<T>& container)
{
  return typename WBlobPtr<T>::const_reverse_iterator(container.GetPtr() + container.GetCount() - 1);
}

template <typename T>
typename WBlobPtr<T>::const_reverse_iterator crbegin(const WBlobPtr<T>& container)
{
  return typename WBlobPtr<T>::const_reverse_iterator(container.GetPtr() + container.GetCount() - 1);
}

template <typename T>
typename WBlobPtr<T>::iterator end(WBlobPtr<T>& in_container)
{
  return in_container.GetPtr() + in_container.GetCount();
}

template <typename T>
typename WBlobPtr<T>::const_iterator end(const WBlobPtr<T>& container)
{
  return container.GetPtr() + container.GetCount();
}

template <typename T>
typename WBlobPtr<T>::const_iterator cend(const WBlobPtr<T>& container)
{
  return container.GetPtr() + container.GetCount();
}

template <typename T>
typename WBlobPtr<T>::reverse_iterator rend(WBlobPtr<T>& in_container)
{
  return typename WBlobPtr<T>::reverse_iterator(in_container.GetPtr() - 1);
}

template <typename T>
typename WBlobPtr<T>::const_reverse_iterator rend(const WBlobPtr<T>& container)
{
  return typename WBlobPtr<T>::const_reverse_iterator(container.GetPtr() - 1);
}

template <typename T>
typename WBlobPtr<T>::const_reverse_iterator crend(const WBlobPtr<T>& container)
{
  return typename WBlobPtr<T>::const_reverse_iterator(container.GetPtr() - 1);
}

/// WBlob allows to store simple binary data larger than 4GB.
/// This storage class is used by WImage to allow processing of large textures for example.
/// In the current implementation the start of the allocated memory is guaranteed to be 64 byte aligned.
class W_FOUNDATION_DLL WBlob
{
public:
  W_DECLARE_MEM_RELOCATABLE_TYPE();

  /// Default constructor. Does not allocate any memory.
  WBlob();

  /// Move constructor. Moves the storage pointer from the other blob to this blob.
  WBlob(WBlob&& other);

  /// Move assignment. Moves the storage pointer from the other blob to this blob.
  void operator=(WBlob&& rhs);

  /// Default destructor. Will call Clear() to deallocate the memory.
  ~WBlob();

  /// Sets the blob to the content of pSource.
  /// This will allocate the necessary memory if needed and then copy uiSize bytes from pSource.
  void SetFrom(const void* pSource, WUInt64 uiSize);

  /// Deallocates the memory allocated by this instance.
  void Clear();

  /// \bried Is data blob empty
  bool IsEmpty() const;

  /// Allocates uiCount bytes for storage in this object. The bytes will have undefined content.
  void SetCountUninitialized(WUInt64 uiCount);

  /// Convenience method to clear the content of the blob to all 0 bytes.
  void ZeroFill();

  /// Returns a blob pointer to the blob data, or an empty blob pointer if the blob is empty.
  template <typename T>
  WBlobPtr<T> GetBlobPtr()
  {
    return WBlobPtr<T>(static_cast<T*>(m_pStorage), m_uiSize);
  }

  /// Returns a blob pointer to the blob data, or an empty blob pointer if the blob is empty.
  template <typename T>
  WBlobPtr<const T> GetBlobPtr() const
  {
    return WBlobPtr<const T>(static_cast<T*>(m_pStorage), m_uiSize);
  }

  /// Returns a blob pointer to the blob data, or an empty blob pointer if the blob is empty.
  WByteBlobPtr GetByteBlobPtr() { return WByteBlobPtr(reinterpret_cast<WUInt8*>(m_pStorage), m_uiSize); }

  /// Returns a blob pointer to the blob data, or an empty blob pointer if the blob is empty.
  WConstByteBlobPtr GetByteBlobPtr() const { return WConstByteBlobPtr(reinterpret_cast<const WUInt8*>(m_pStorage), m_uiSize); }

private:
  void* m_pStorage = nullptr;
  WUInt64 m_uiSize = 0;
};
