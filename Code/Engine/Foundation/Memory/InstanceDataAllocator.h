#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Blob.h>
#include <Foundation/Containers/DynamicArray.h>

/// Structure to describe an instance data type.
///
/// Many resources, such as VMs, state machines and visual scripts of various types have shared state (their configuration)
/// as well as per-instance state (for their execution).
///
/// This structure describes the type of instance data used by a such a resource (or a node inside it).
/// Instance data is allocated through the WInstanceDataAllocator.
///
/// Use the templated Fill() method to fill the desc from a data type.
struct W_FOUNDATION_DLL WInstanceDataDesc
{
  WUInt32 m_uiTypeSize = 0;
  WUInt32 m_uiTypeAlignment = 0;
  WMemoryUtils::ConstructorFunction m_ConstructorFunction = nullptr;
  WMemoryUtils::DestructorFunction m_DestructorFunction = nullptr;

  template <typename T>
  W_ALWAYS_INLINE void FillFromType()
  {
    m_uiTypeSize = sizeof(T);
    m_uiTypeAlignment = alignof(T);
    m_ConstructorFunction = WMemoryUtils::MakeConstructorFunction<SkipTrivialTypes, T>();
    m_DestructorFunction = WMemoryUtils::MakeDestructorFunction<T>();
  }
};

/// Manages complex multi-type instance data allocation with proper construction and destruction.
///
/// This allocator is designed for systems that need to allocate heterogeneous data structures
/// in a single memory block, such as VM instances, state machines, or script execution contexts.
/// It calculates proper alignments for mixed data types and handles construction/destruction
/// automatically.
///
/// Typical workflow:
/// 1. Use AddDesc() to register all data types needed for an instance
/// 2. Call AllocateAndConstruct() to create initialized memory
/// 3. Use GetInstanceData() to access individual data by offset
/// 4. Call DestructAndDeallocate() when the instance is no longer needed
class W_FOUNDATION_DLL WInstanceDataAllocator
{
public:
  /// Adds the given desc to internal list of data that needs to be allocated and returns the byte offset.
  [[nodiscard]] WUInt32 AddDesc(const WInstanceDataDesc& desc);

  /// Resets all internal state.
  void ClearDescs();

  /// Constructs the instance data objects, within the pre-allocated memory block.
  void Construct(WByteBlobPtr blobPtr) const;

  /// Destructs the instance data objects.
  void Destruct(WByteBlobPtr blobPtr) const;

  /// Allocates memory and constructs the instance data objects inside it. The returned WBlob must be stored somewhere.
  [[nodiscard]] WBlob AllocateAndConstruct() const;

  /// Destructs and deallocates the instance data objects and the given memory block.
  void DestructAndDeallocate(WBlob& ref_blob) const;

  /// The total size in bytes taken up by all instance data objects that were added.
  WUInt32 GetTotalDataSize() const { return m_uiTotalDataSize; }

  /// Retrieves a void pointer to the instance data within the given blob at the given offset, or nullptr if the offset is invalid.
  W_ALWAYS_INLINE static void* GetInstanceData(const WByteBlobPtr& blobPtr, WUInt32 uiOffset)
  {
    return (uiOffset != WInvalidIndex) ? blobPtr.GetPtr() + uiOffset : nullptr;
  }

private:
  WDynamicArray<WInstanceDataDesc> m_Descs;
  WUInt32 m_uiTotalDataSize = 0;
};
