#pragma once

#include <Foundation/Containers/StaticArray.h>

/// A ring-buffer container that will use a static array of a given capacity to cycle through elements.
///
/// If you need a dynamic ring-buffer, use an WDeque.
template <typename T, WUInt32 Capacity>
class WStaticRingBuffer
{
public:
  static_assert(Capacity > 1, "ORLY?");

  /// Constructs an empty ring-buffer.
  WStaticRingBuffer(); // [tested]

  /// Copies the content from rhs into this ring-buffer.
  WStaticRingBuffer(const WStaticRingBuffer<T, Capacity>& rhs); // [tested]

  /// Destructs all remaining elements.
  ~WStaticRingBuffer(); // [tested]

  /// Copies the content from rhs into this ring-buffer.
  void operator=(const WStaticRingBuffer<T, Capacity>& rhs); // [tested]

  /// Compares two ring-buffers for equality.
  bool operator==(const WStaticRingBuffer<T, Capacity>& rhs) const; // [tested]
  W_ADD_DEFAULT_OPERATOR_NOTEQUAL(const WStaticRingBuffer<T, Capacity>&);

  /// Appends an element at the end of the ring-buffer. Asserts that CanAppend() is true.
  void PushBack(const T& element); // [tested]

  /// Appends an element at the end of the ring-buffer. Asserts that CanAppend() is true.
  void PushBack(T&& element); // [tested]

  /// Accesses the latest element in the ring-buffer.
  T& PeekBack(); // [tested]

  /// Accesses the latest element in the ring-buffer.
  const T& PeekBack() const; // [tested]

  /// Removes the oldest element from the ring-buffer.
  void PopFront(WUInt32 uiElements = 1); // [tested]

  /// Accesses the oldest element in the ring-buffer.
  const T& PeekFront() const; // [tested]

  /// Accesses the oldest element in the ring-buffer.
  T& PeekFront(); // [tested]

  /// Accesses the n-th element in the ring-buffer.
  const T& operator[](WUInt32 uiIndex) const; // [tested]

  /// Accesses the n-th element in the ring-buffer.
  T& operator[](WUInt32 uiIndex); // [tested]

  /// Returns the number of elements that are currently in the ring-buffer.
  WUInt32 GetCount() const; // [tested]

  /// Returns true if the ring-buffer currently contains no elements.
  bool IsEmpty() const; // [tested]

  /// Returns true, if the ring-buffer can store at least uiElements additional elements.
  bool CanAppend(WUInt32 uiElements = 1); // [tested]

  /// Destructs all elements in the ring-buffer.
  void Clear(); // [tested]

private:
  T* GetStaticArray();

  /// The fixed size array.
  struct alignas(alignof(T))
  {
    WUInt8 m_Data[Capacity * sizeof(T)];
  };

  T* m_pElements;
  WUInt32 m_uiCount;
  WUInt32 m_uiFirstElement;
};

#include <Foundation/Containers/Implementation/StaticRingBuffer_inl.h>
