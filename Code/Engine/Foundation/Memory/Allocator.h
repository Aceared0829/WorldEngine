#pragma once

/// \file

#include <Foundation/Time/Time.h>
#include <Foundation/Types/ArrayPtr.h>
#include <Foundation/Types/Id.h>
#include <utility>


#ifdef new
#  undef new
#endif

#ifdef delete
#  undef delete
#endif

using WAllocatorId = WGenericId<24, 8>;

/// Base class for all memory allocators.
///
/// This abstract base class defines the interface for all allocators in WorldEngine. Allocators are responsible
/// for memory management and can implement different allocation strategies (heap, linear, frame, etc.).
///
/// Key concepts:
/// - All allocators provide aligned memory allocation
/// - Optional tracking of allocation statistics and debugging information
/// - Virtual interface allows swapping allocator implementations
/// - Thread safety depends on specific allocator implementation
///
/// Usage:
/// - Use W_NEW/W_DELETE macros instead of calling Allocate/Deallocate directly
/// - Different allocator types optimize for different usage patterns
/// - Always pair allocations with deallocations using the same allocator instance
class W_FOUNDATION_DLL WAllocator
{
public:
  struct Stats
  {
    W_DECLARE_POD_TYPE();

    WUInt64 m_uiNumAllocations = 0;         ///< total number of allocations
    WUInt64 m_uiNumDeallocations = 0;       ///< total number of deallocations
    WUInt64 m_uiAllocationSize = 0;         ///< total allocation size in bytes

    WUInt64 m_uiPerFrameAllocationSize = 0; ///< allocation size in bytes in this frame
    WTime m_PerFrameAllocationTime;         ///< time spend on allocations in this frame
  };

  WAllocator();
  virtual ~WAllocator();

  /// Interface, do not use this directly, always use the new/delete macros below
  ///
  /// Allocates aligned memory of the specified size. The destructorFunc parameter is used for
  /// automatic cleanup when the allocator is reset or destroyed (mainly used by linear allocators).
  virtual void* Allocate(size_t uiSize, size_t uiAlign, WMemoryUtils::DestructorFunction destructorFunc = nullptr) = 0;

  /// Deallocates memory previously allocated by this allocator.
  ///
  /// The pointer must have been returned by a previous call to Allocate() on this allocator instance.
  /// Passing nullptr is safe and will be ignored.
  virtual void Deallocate(void* pPtr) = 0;

  /// Reallocates memory, potentially moving the data to a new location.
  ///
  /// Default implementation allocates new memory, copies old data, and deallocates the old memory.
  /// Some allocators may provide more efficient implementations.
  virtual void* Reallocate(void* pPtr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign);

  /// Returns the number of bytes allocated at this address.
  ///
  /// This information is only available if allocation tracking is enabled (see WAllocatorTrackingMode
  /// and W_ALLOC_TRACKING_DEFAULT). Returns 0 when tracking is disabled or for invalid pointers.
  /// Primarily used for debugging and memory analysis.
  virtual size_t AllocatedSize(const void* pPtr) = 0;

  virtual WAllocatorId GetId() const = 0;
  virtual Stats GetStats() const = 0;

private:
  W_DISALLOW_COPY_AND_ASSIGN(WAllocator);
};

#include <Foundation/Memory/Implementation/Allocator_inl.h>

/// creates a new instance of type using the given allocator
#define W_NEW(allocator, type, ...) \
  WInternal::NewInstance<type>(     \
    new ((allocator)->Allocate(sizeof(type), alignof(type), WMemoryUtils::MakeDestructorFunction<type>())) type(__VA_ARGS__), (allocator))

/// deletes the instance stored in ptr using the given allocator and sets ptr to nullptr
#define W_DELETE(allocator, ptr)       \
  {                                     \
    WInternal::Delete(allocator, ptr); \
    ptr = nullptr;                      \
  }

/// creates a new array of type using the given allocator with count elements, calls default constructor for non-POD types
#define W_NEW_ARRAY(allocator, type, count) WInternal::CreateArray<type>(allocator, count)

/// Calls destructor on every element for non-POD types and deletes the array stored in arrayPtr using the given allocator
#define W_DELETE_ARRAY(allocator, arrayPtr)      \
  {                                               \
    WInternal::DeleteArray(allocator, arrayPtr); \
    arrayPtr.Clear();                             \
  }

/// creates a raw buffer of type using the given allocator with count elements, but does NOT call the default constructor
#define W_NEW_RAW_BUFFER(allocator, type, count) WInternal::CreateRawBuffer<type>(allocator, count)

/// deletes a raw buffer stored in ptr using the given allocator, but does NOT call destructor
#define W_DELETE_RAW_BUFFER(allocator, ptr)     \
  {                                              \
    WInternal::DeleteRawBuffer(allocator, ptr); \
    ptr = nullptr;                               \
  }

/// extends a given raw buffer to the new size, taking care of calling constructors / assignment operators.
#define W_EXTEND_RAW_BUFFER(allocator, ptr, oldSize, newSize) WInternal::ExtendRawBuffer(ptr, allocator, oldSize, newSize)



/// creates a new instance of type using the default allocator
#define W_DEFAULT_NEW(type, ...) W_NEW(WFoundation::GetDefaultAllocator(), type, __VA_ARGS__)

/// deletes the instance stored in ptr using the default allocator and sets ptr to nullptr
#define W_DEFAULT_DELETE(ptr) W_DELETE(WFoundation::GetDefaultAllocator(), ptr)

/// creates a new array of type using the default allocator with count elements, calls default constructor for non-POD types
#define W_DEFAULT_NEW_ARRAY(type, count) W_NEW_ARRAY(WFoundation::GetDefaultAllocator(), type, count)

/// calls destructor on every element for non-POD types and deletes the array stored in arrayPtr using the default allocator
#define W_DEFAULT_DELETE_ARRAY(arrayPtr) W_DELETE_ARRAY(WFoundation::GetDefaultAllocator(), arrayPtr)

/// creates a raw buffer of type using the default allocator with count elements, but does NOT call the default constructor
#define W_DEFAULT_NEW_RAW_BUFFER(type, count) W_NEW_RAW_BUFFER(WFoundation::GetDefaultAllocator(), type, count)

/// deletes a raw buffer stored in ptr using the default allocator, but does NOT call destructor
#define W_DEFAULT_DELETE_RAW_BUFFER(ptr) W_DELETE_RAW_BUFFER(WFoundation::GetDefaultAllocator(), ptr)
