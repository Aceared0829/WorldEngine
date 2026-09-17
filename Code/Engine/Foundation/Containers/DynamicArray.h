#pragma once

#include <Foundation/Containers/ArrayBase.h>
#include <Foundation/Memory/AllocatorWrapper.h>
#include <Foundation/Memory/TempAllocator.h>
#include <Foundation/Types/PointerWithFlags.h>

/// Implementation of a dynamically growing array.
///
/// Best-case performance for the PushBack operation is O(1) if the WDynamicArray doesn't need to be expanded.
/// In the worst case, PushBack is O(n).
/// Look-up is guaranteed to always be O(1).
template <typename T>
class WDynamicArrayBase : public WArrayBase<T, WDynamicArrayBase<T>>
{
protected:
  /// Creates an empty array. Does not allocate any data yet.
  explicit WDynamicArrayBase(WAllocator* pAllocator);                                 // [tested]

  WDynamicArrayBase(T* pInplaceStorage, WUInt32 uiCapacity, WAllocator* pAllocator); // [tested]

  /// Creates a copy of the given array.
  WDynamicArrayBase(const WDynamicArrayBase<T>& other, WAllocator* pAllocator); // [tested]

  /// Moves the given array into this one.
  WDynamicArrayBase(WDynamicArrayBase<T>&& other, WAllocator* pAllocator); // [tested]

  /// Creates a copy of the given array.
  WDynamicArrayBase(const WArrayPtr<const T>& other, WAllocator* pAllocator); // [tested]

  /// Destructor.
  ~WDynamicArrayBase(); // [tested]

  /// Copies the data from some other contiguous array into this one.
  void operator=(const WDynamicArrayBase<T>& rhs); // [tested]

  /// Moves the data from some other contiguous array into this one.
  void operator=(WDynamicArrayBase<T>&& rhs) noexcept; // [tested]

  T* GetElementsPtr();
  const T* GetElementsPtr() const;

  friend class WArrayBase<T, WDynamicArrayBase<T>>;

public:
  /// Expands the array so it can at least store the given capacity.
  void Reserve(WUInt32 uiCapacity); // [tested]

  /// Tries to compact the array to avoid wasting memory. The resulting capacity is at least 'GetCount' (no elements get removed). Will
  /// deallocate all data, if the array is empty.
  void Compact(); // [tested]

  /// Returns the allocator that is used by this instance.
  WAllocator* GetAllocator() const { return const_cast<WAllocator*>(m_pAllocator.GetPtr()); }

  /// Returns the amount of bytes that are currently allocated on the heap.
  WUInt64 GetHeapMemoryUsage() const; // [tested]

  /// swaps the contents of this array with another one
  void Swap(WDynamicArrayBase<T>& other); // [tested]

private:
  enum Storage
  {
    Owned = 0,
    External = 1
  };

  WPointerWithFlags<WAllocator, 1> m_pAllocator;

  enum
  {
    CAPACITY_ALIGNMENT = 16
  };

  void SetCapacity(WUInt32 uiCapacity);
};

/// \see WDynamicArrayBase
template <typename T, typename AllocatorWrapper = WDefaultAllocatorWrapper>
class WDynamicArray : public WDynamicArrayBase<T>
{
public:
  W_DECLARE_MEM_RELOCATABLE_TYPE();


  WDynamicArray();
  explicit WDynamicArray(WAllocator* pAllocator);

  WDynamicArray(const WDynamicArray<T, AllocatorWrapper>& other);
  WDynamicArray(const WDynamicArrayBase<T>& other);
  explicit WDynamicArray(const WArrayPtr<const T>& other);

  WDynamicArray(WDynamicArray<T, AllocatorWrapper>&& other);
  WDynamicArray(WDynamicArrayBase<T>&& other);

  void operator=(const WDynamicArray<T, AllocatorWrapper>& rhs);
  void operator=(const WDynamicArrayBase<T>& rhs);
  void operator=(const WArrayPtr<const T>& rhs);

  void operator=(WDynamicArray<T, AllocatorWrapper>&& rhs) noexcept;
  void operator=(WDynamicArrayBase<T>&& rhs) noexcept;

protected:
  WDynamicArray(T* pInplaceStorage, WUInt32 uiCapacity, WAllocator* pAllocator)
    : WDynamicArrayBase<T>(pInplaceStorage, uiCapacity, pAllocator)
  {
  }
};

/// A dynamic array that uses the temp allocator.
/// This is ideal for temporary arrays that are only used within a short scope.
/// The temp allocator is optimized for short-lived allocations and can be more efficient than the default allocator for this use case.
template <typename T>
class WTempArray : public WDynamicArray<T>
{
public:
  WTempArray();

  void operator=(const WDynamicArrayBase<T>& rhs);
  void operator=(const WArrayPtr<const T>& rhs);

  void operator=(WDynamicArrayBase<T>&& rhs) noexcept;
};

/// Overload of WMakeArrayPtr for const dynamic arrays of pointer pointing to const type.
template <typename T, typename AllocatorWrapper>
WArrayPtr<const T* const> WMakeArrayPtr(const WDynamicArray<T*, AllocatorWrapper>& dynArray);

/// Overload of WMakeArrayPtr for const dynamic arrays.
template <typename T, typename AllocatorWrapper>
WArrayPtr<const T> WMakeArrayPtr(const WDynamicArray<T, AllocatorWrapper>& dynArray);

/// Overload of WMakeArrayPtr for dynamic arrays.
template <typename T, typename AllocatorWrapper>
WArrayPtr<T> WMakeArrayPtr(WDynamicArray<T, AllocatorWrapper>& in_dynArray);


static_assert(WGetTypeClass<WDynamicArray<int>>::value == 2, "dynamic array is not memory relocatable");

#include <Foundation/Containers/Implementation/DynamicArray_inl.h>
