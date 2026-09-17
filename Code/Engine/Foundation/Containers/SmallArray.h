#pragma once

#include <Foundation/Algorithm/Sorting.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Memory/AllocatorWrapper.h>
#include <Foundation/Types/ArrayPtr.h>

constexpr WUInt32 WSmallInvalidIndex = 0xFFFF;

/// Implementation of a dynamically growing array with in-place storage and small memory overhead.
///
/// Contrary to WDynamicArray and WHybridArray, the WSmallArray only uses a 16-bit index,
/// so it can only hold 64K items (65535). It is meant for use cases, where the maximum required size
/// is low, but the size of the container itself should also be as small as possible.
///
/// Best-case performance for the PushBack operation is in O(1) if the WSmallArray does not need to be expanded.
/// In the worst case, PushBack is O(n).
/// Look-up is guaranteed to always be O(1).
template <typename T, WUInt16 Size>
class WSmallArrayBase
{
public:
  // Only if the stored type is either POD or relocatable the hybrid array itself is also relocatable.
  W_DECLARE_MEM_RELOCATABLE_TYPE_CONDITIONAL(T);

  WSmallArrayBase();                                                                // [tested]
  WSmallArrayBase(const WSmallArrayBase<T, Size>& other, WAllocator* pAllocator); // [tested]
  WSmallArrayBase(const WArrayPtr<const T>& other, WAllocator* pAllocator);       // [tested]
  WSmallArrayBase(WSmallArrayBase<T, Size>&& other, WAllocator* pAllocator);      // [tested]

  ~WSmallArrayBase();                                                               // [tested]

  // Can't use regular assignment operators since we need to pass an allocator. Use CopyFrom or MoveFrom methods instead.
  void operator=(const WSmallArrayBase<T, Size>& rhs) = delete;
  void operator=(WSmallArrayBase<T, Size>&& rhs) = delete;

  /// Copies the data from some other array into this one.
  void CopyFrom(const WArrayPtr<const T>& other, WAllocator* pAllocator); // [tested]

  /// Moves the data from some other array into this one.
  void MoveFrom(WSmallArrayBase<T, Size>&& other, WAllocator* pAllocator); // [tested]

  /// Conversion to const WArrayPtr.
  operator WArrayPtr<const T>() const; // [tested]

  /// Conversion to WArrayPtr.
  operator WArrayPtr<T>(); // [tested]

  /// Compares this array to another contiguous array type.
  bool operator==(const WSmallArrayBase<T, Size>& rhs) const; // [tested]
  W_ADD_DEFAULT_OPERATOR_NOTEQUAL(const WSmallArrayBase<T, Size>&);

#if W_DISABLED(W_USE_CPP20_OPERATORS)
  bool operator==(const WArrayPtr<const T>& rhs) const; // [tested]
  W_ADD_DEFAULT_OPERATOR_NOTEQUAL(const WArrayPtr<const T>&);
#endif

  /// Compares this array to another contiguous array type.
  bool operator<(const WSmallArrayBase<T, Size>& rhs) const; // [tested]
  bool operator<(const WArrayPtr<const T>& rhs) const;       // [tested]

  /// Returns the element at the given index. Does bounds checks in debug builds.
  const T& operator[](WUInt32 uiIndex) const; // [tested]

  /// Returns the element at the given index. Does bounds checks in debug builds.
  T& operator[](WUInt32 uiIndex); // [tested]

  /// Resizes the array to have exactly uiCount elements. Default constructs extra elements if the array is grown.
  void SetCount(WUInt16 uiCount, WAllocator* pAllocator); // [tested]

  /// Resizes the array to have exactly uiCount elements. Constructs all new elements by copying the FillValue.
  void SetCount(WUInt16 uiCount, const T& fillValue, WAllocator* pAllocator); // [tested]

  /// Resizes the array to have exactly uiCount elements. Extra elements might be uninitialized.
  template <typename = void>                                             // Template is used to only conditionally compile this function in when it is actually used.
  void SetCountUninitialized(WUInt16 uiCount, WAllocator* pAllocator); // [tested]

  /// Ensures the container has at least \a uiCount elements. Ie. calls SetCount() if the container has fewer elements, does nothing
  /// otherwise.
  void EnsureCount(WUInt16 uiCount, WAllocator* pAllocator); // [tested]

  /// Returns the number of active elements in the array.
  WUInt32 GetCount() const; // [tested]

  /// Returns true, if the array does not contain any elements.
  bool IsEmpty() const; // [tested]

  /// Clears the array.
  void Clear(); // [tested]

  /// Checks whether the given value can be found in the array. O(n) complexity.
  bool Contains(const T& value) const; // [tested]

  /// Inserts value at index by shifting all following elements.
  void Insert(const T& value, WUInt32 uiIndex, WAllocator* pAllocator); // [tested]

  /// Inserts value at index by shifting all following elements.
  void Insert(T&& value, WUInt32 uiIndex, WAllocator* pAllocator); // [tested]

  /// Removes the first occurrence of value and fills the gap by shifting all following elements
  bool RemoveAndCopy(const T& value); // [tested]

  /// Removes the first occurrence of value and fills the gap by swapping in the last element
  bool RemoveAndSwap(const T& value); // [tested]

  /// Removes the element at index and fills the gap by shifting all following elements
  void RemoveAtAndCopy(WUInt32 uiIndex, WUInt16 uiNumElements = 1); // [tested]

  /// Removes the element at index and fills the gap by swapping in the last element
  void RemoveAtAndSwap(WUInt32 uiIndex, WUInt16 uiNumElements = 1); // [tested]

  /// Searches for the first occurrence of the given value and returns its index or WInvalidIndex if not found.
  WUInt32 IndexOf(const T& value, WUInt32 uiStartIndex = 0) const; // [tested]

  /// Searches for the last occurrence of the given value and returns its index or WInvalidIndex if not found.
  WUInt32 LastIndexOf(const T& value, WUInt32 uiStartIndex = WSmallInvalidIndex) const; // [tested]

  /// Grows the array by one element and returns a reference to the newly created element.
  T& ExpandAndGetRef(WAllocator* pAllocator); // [tested]

  /// Pushes value at the end of the array.
  void PushBack(const T& value, WAllocator* pAllocator); // [tested]

  /// Pushes value at the end of the array.
  void PushBack(T&& value, WAllocator* pAllocator); // [tested]

  /// Pushes value at the end of the array. Does NOT ensure capacity.
  void PushBackUnchecked(const T& value); // [tested]

  /// Pushes value at the end of the array. Does NOT ensure capacity.
  void PushBackUnchecked(T&& value); // [tested]

  /// Pushes all elements in range at the end of the array. Increases the capacity if necessary.
  void PushBackRange(const WArrayPtr<const T>& range, WAllocator* pAllocator); // [tested]

  /// Removes count elements from the end of the array.
  void PopBack(WUInt32 uiCountToRemove = 1); // [tested]

  /// Returns the last element of the array.
  T& PeekBack(); // [tested]

  /// Returns the last element of the array.
  const T& PeekBack() const; // [tested]

  /// Sort with explicit comparer
  template <typename Comparer>
  void Sort(const Comparer& comparer); // [tested]

  /// Sort with default comparer
  void Sort(); // [tested]

  /// Returns a pointer to the array data, or nullptr if the array is empty.
  T* GetData();

  /// Returns a pointer to the array data, or nullptr if the array is empty.
  const T* GetData() const;

  /// Returns an array pointer to the array data, or an empty array pointer if the array is empty.
  WArrayPtr<T> GetArrayPtr(); // [tested]

  /// Returns an array pointer to the array data, or an empty array pointer if the array is empty.
  WArrayPtr<const T> GetArrayPtr() const; // [tested]

  /// Returns a byte array pointer to the array data, or an empty array pointer if the array is empty.
  WArrayPtr<typename WArrayPtr<T>::ByteType> GetByteArrayPtr(); // [tested]

  /// Returns a byte array pointer to the array data, or an empty array pointer if the array is empty.
  WArrayPtr<typename WArrayPtr<const T>::ByteType> GetByteArrayPtr() const; // [tested]

  /// Expands the array so it can at least store the given capacity.
  void Reserve(WUInt16 uiCapacity, WAllocator* pAllocator); // [tested]

  /// Tries to compact the array to avoid wasting memory. The resulting capacity is at least 'GetCount' (no elements get removed). Will
  /// deallocate all data, if the array is empty.
  void Compact(WAllocator* pAllocator); // [tested]

  /// Returns the reserved number of elements that the array can hold without reallocating.
  WUInt32 GetCapacity() const { return m_uiCapacity; }

  /// Returns the amount of bytes that are currently allocated on the heap.
  WUInt64 GetHeapMemoryUsage() const; // [tested]

  using value_type = T;
  using const_reference = const T&;
  using const_iterator = const T*;
  using const_reverse_iterator = const_reverse_pointer_iterator<T>;
  using iterator = T*;
  using reverse_iterator = reverse_pointer_iterator<T>;

  template <typename U>
  const U& GetUserData() const; // [tested]

  template <typename U>
  U& GetUserData();             // [tested]

protected:
  enum
  {
    CAPACITY_ALIGNMENT = 4
  };

  void SetCapacity(WUInt16 uiCapacity, WAllocator* pAllocator);

  T* GetElementsPtr();
  const T* GetElementsPtr() const;

  WUInt16 m_uiCount = 0;
  WUInt16 m_uiCapacity = Size;

  WUInt32 m_uiUserData = 0;

  union
  {
    struct alignas(alignof(T))
    {
      WUInt8 m_StaticData[Size * sizeof(T)];
    };

    T* m_pElements = nullptr;
  };
};

//////////////////////////////////////////////////////////////////////////

/// \see WSmallArrayBase
template <typename T, WUInt16 Size, typename AllocatorWrapper = WDefaultAllocatorWrapper>
class WSmallArray : public WSmallArrayBase<T, Size>
{
  using SUPER = WSmallArrayBase<T, Size>;

public:
  // Only if the stored type is either POD or relocatable the hybrid array itself is also relocatable.
  W_DECLARE_MEM_RELOCATABLE_TYPE_CONDITIONAL(T);

  WSmallArray();

  WSmallArray(const WSmallArray<T, Size, AllocatorWrapper>& other);
  explicit WSmallArray(const WArrayPtr<const T>& other);
  WSmallArray(WSmallArray<T, Size, AllocatorWrapper>&& other);

  ~WSmallArray();

  void operator=(const WSmallArray<T, Size, AllocatorWrapper>& rhs);
  void operator=(const WArrayPtr<const T>& rhs);
  void operator=(WSmallArray<T, Size, AllocatorWrapper>&& rhs) noexcept;

  void SetCount(WUInt16 uiCount);                      // [tested]
  void SetCount(WUInt16 uiCount, const T& fillValue);  // [tested]
  void EnsureCount(WUInt16 uiCount);                   // [tested]

  template <typename = void>
  void SetCountUninitialized(WUInt16 uiCount);         // [tested]

  void InsertAt(WUInt32 uiIndex, const T& value);      // [tested]
  void InsertAt(WUInt32 uiIndex, T&& value);           // [tested]

  T& ExpandAndGetRef();                                 // [tested]
  void PushBack(const T& value);                        // [tested]
  void PushBack(T&& value);                             // [tested]
  void PushBackRange(const WArrayPtr<const T>& range); // [tested]

  void Reserve(WUInt16 uiCapacity);
  void Compact();
};

#include <Foundation/Containers/Implementation/SmallArray_inl.h>
