#pragma once

#include <Foundation/Containers/DynamicArray.h>

/// A hybrid array uses in-place storage to handle the first few elements without any allocation. It dynamically resizes when more elements are needed.
///
/// It is often more efficient to use a hybrid array, rather than a dynamic array, when the number of needed elements is typically low or when the array is used only temporarily. In this case costly allocations can often be prevented entirely.
/// However, if the number of elements is unpredictable or usually very large, prefer a dynamic array, to avoid wasting (stack) memory for a hybrid array that is rarely large enough to be used.
/// The WHybridArray is derived from WDynamicArray and can therefore be passed to functions that expect an WDynamicArray, even for output.
template <typename T, WUInt32 Size, typename AllocatorWrapper = WDefaultAllocatorWrapper>
class WHybridArray : public WDynamicArray<T, AllocatorWrapper>
{
public:
  /// Creates an empty array. Does not allocate any data yet.
  WHybridArray(); // [tested]

  /// Creates an empty array. Does not allocate any data yet.
  explicit WHybridArray(WAllocator* pAllocator); // [tested]

  /// Creates a copy of the given array.
  WHybridArray(const WHybridArray<T, Size, AllocatorWrapper>& other); // [tested]

  /// Creates a copy of the given array.
  explicit WHybridArray(const WArrayPtr<const T>& other); // [tested]

  /// Moves the given array.
  WHybridArray(WHybridArray<T, Size, AllocatorWrapper>&& other) noexcept; // [tested]

  /// Copies the data from some other contiguous array into this one.
  void operator=(const WHybridArray<T, Size, AllocatorWrapper>& rhs); // [tested]

  /// Copies the data from some other contiguous array into this one.
  void operator=(const WArrayPtr<const T>& rhs); // [tested]

  /// Moves the data from some other contiguous array into this one.
  void operator=(WHybridArray<T, Size, AllocatorWrapper>&& rhs) noexcept; // [tested]

protected:
  /// The fixed size array.
  struct alignas(alignof(T))
  {
    WUInt8 m_StaticData[Size * sizeof(T)];
  };

  W_ALWAYS_INLINE T* GetStaticArray() { return reinterpret_cast<T*>(m_StaticData); }

  W_ALWAYS_INLINE const T* GetStaticArray() const { return reinterpret_cast<const T*>(m_StaticData); }
};

/// A hybrid array that uses the temp allocator if it exceeds the in-place storage.
/// This is ideal for temporary arrays that are only used within a short scope and are not expected to grow beyond the in-place storage size in most cases.
/// The temp allocator is optimized for short-lived allocations and can be more efficient than the default allocator for this use case.
template <typename T, WUInt32 Size>
class WTempHybridArray : public WHybridArray<T, Size>
{
public:
  WTempHybridArray();

  template <typename AllocatorWrapper>
  WTempHybridArray(const WHybridArray<T, Size, AllocatorWrapper>& other);
  explicit WTempHybridArray(const WArrayPtr<const T>& other);

  template <typename AllocatorWrapper>
  void operator=(const WHybridArray<T, Size, AllocatorWrapper>& rhs);
  void operator=(const WArrayPtr<const T>& rhs);

  void operator=(WHybridArray<T, Size>&& rhs) noexcept;
};

#include <Foundation/Containers/Implementation/HybridArray_inl.h>
