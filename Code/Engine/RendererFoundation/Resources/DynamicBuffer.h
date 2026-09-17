#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>

/// A dynamic buffer can be used when a lot of data needs to be stored in a single large buffer with dynamic size.
///
/// This class supports allocation and deallocation of single elements or ranges of multiple elements.
/// An allocation is identified by an offset and can have additional user data attached.
/// The indented usage patterns is that data is allocated and written to the buffer during game play or extraction code.
/// After all data has been written UploadChangesForNextFrame needs to be called to upload changed data to the GPU buffer.
/// The renderer would then call GetBufferForRendering to get the correct buffer for rendering.
class W_RENDERERFOUNDATION_DLL WGALDynamicBuffer
{
public:
  /// Deallocates all data.
  void Clear();

  struct AllocateFlags
  {
    using StorageType = WUInt32;

    enum Enum
    {
      None,
      ZeroFill = W_BIT(0),

      Default = None
    };

    struct Bits
    {
      StorageType ZeroFill : 1;
    };
  };

  /// Allocates a single or multiple elements and returns the offset to the first element.
  /// This offset is used to identify the allocation and should also be used in a shader to read the data from the buffer.
  ///
  /// The user data can be used to store additional information, typically the owner of the allocation, like e.g. a component handle.
  template <typename U>
  WUInt32 Allocate(const U& userData, WUInt32 uiCount = 1, WBitflags<AllocateFlags> allocateFlags = AllocateFlags::None, WAllocator* pTempAllocator = nullptr)
  {
    static_assert(sizeof(U) <= sizeof(WUInt64), "userData is too large");
    WUInt64 uiUserData = 0;
    *reinterpret_cast<U*>(&uiUserData) = userData;

    return Allocate(uiUserData, uiCount, allocateFlags, pTempAllocator);
  }

  /// Removes an allocation at the given offset. The offset must have been returned by Allocate.
  /// This will create unused space in the buffer that can be filled by subsequent allocations or closed later by compaction.
  void Deallocate(WUInt32 uiOffset);

  /// Maps a range of elements for writing.
  template <typename T>
  WArrayPtr<T> MapForWriting(WUInt32 uiOffset)
  {
    WUInt32 uiCount = 0;
    WByteArrayPtr byteData = MapForWriting(uiOffset, uiCount);
    W_ASSERT_DEBUG(sizeof(T) == m_Desc.m_uiStructSize, "Invalid Type");
    return WArrayPtr<T>(reinterpret_cast<T*>(byteData.GetPtr()), uiCount);
  }

  /// Maps a range of bytes for writing.
  WByteArrayPtr MapBytesForWriting(WUInt32 uiOffset)
  {
    WUInt32 uiCount = 0;
    WByteArrayPtr byteData = MapForWriting(uiOffset, uiCount);
    W_ASSERT_DEBUG(byteData.GetCount() == uiCount * m_Desc.m_uiStructSize, "Implementation error");
    return byteData;
  }

  /// Maps a range of elements for reading.
  template <typename T>
  WArrayPtr<const T> MapForReading(WUInt32 uiOffset) const
  {
    WUInt32 uiCount = 0;
    WConstByteArrayPtr byteData = MapForReading(uiOffset, uiCount);
    W_ASSERT_DEBUG(sizeof(T) == m_Desc.m_uiStructSize, "Invalid Type");
    return WArrayPtr<const T>(reinterpret_cast<const T*>(byteData.GetPtr()), uiCount);
  }

  /// Upload all changed data to the GPU buffer for the next rendering frame, aka the next time BeginFrame is called on the GALDevice.
  void UploadChangesForNextFrame();

  struct ChangedAllocation
  {
    WUInt64 m_uiUserData = 0;
    WUInt32 m_uiNewOffset = 0;
  };

  /// Tries to compact the buffer by moving allocations to free ranges. All moved allocations are returned in out_changedAllocations.
  ///
  /// The user data can be used to update the owner of the allocation.
  /// To prevent too many changes per frame only uiMaxSteps are executed which corresponds to the number of allocations which can be moved.
  void RunCompactionSteps(WDynamicArray<ChangedAllocation>& out_changedAllocations, WUInt32 uiMaxSteps = 16);

  /// This should be called inside the rendering code to retrieve the underlying buffer for rendering.
  ///
  /// It is ensured that it will always return the same buffer until the next time BeginFrame is called on the GALDevice even if the buffer
  /// has been resized due to more allocations on the game play or extraction side.
  const WGALBufferHandle& GetBufferForRendering() const { return m_hBufferForRendering; }

  /// Returns the description that was used to create this dynamic buffer.
  const WGALBufferCreationDescription& GetDescription() const { return m_Desc; }

  /// Returns the debug name that was used to create this dynamic buffer.
  WStringView GetDebugName() const { return m_sDebugName; }

private:
  friend class WMemoryUtils;
  friend class WGALDevice;

  WGALDynamicBuffer() = default;
  ~WGALDynamicBuffer();

  void Initialize(const WGALBufferCreationDescription& desc, WStringView sDebugName);
  void Deinitialize();

  WUInt32 Allocate(WUInt64 uiUserData, WUInt32 uiCount, WBitflags<AllocateFlags> allocateFlags, WAllocator* pTempAllocator);
  WByteArrayPtr MapForWriting(WUInt32 uiOffset, WUInt32& out_uiCount);
  WConstByteArrayPtr MapForReading(WUInt32 uiOffset, WUInt32& out_uiCount) const;

  WUInt32 AllocateTempData(WUInt32 uiStartOffset, WUInt32 uiNewCount, WAllocator* pTempAllocator);

  void SwapBuffers()
  {
    m_hBufferForRendering = m_hBufferForUpload;
  }

  mutable WMutex m_Mutex;

  WUInt32 m_uiCapacity = 0;   ///< in number of elements
  WUInt32 m_uiNextOffset = 0; ///< in number of elements

  WDynamicArray<WUInt8, WAlignedAllocatorWrapper> m_Data;

  struct TempData
  {
    WAllocator* m_pAllocator = nullptr;
    WUInt8* m_pData = nullptr;
    WUInt32 m_uiStartByteOffset = 0;
    WUInt32 m_uiByteSize = 0;
  };

  WSmallArray<TempData, 2> m_TempData;

  struct Allocation
  {
    WUInt64 m_uiUserData = 0;
    WUInt32 m_uiCount = 0;
    WUInt32 m_uiDataIndex = 0; ///< 0 is full buffer, greater than 0 are temp buffers
  };

  WMap<WUInt32, Allocation> m_Allocations;

  WDynamicArray<WGAL::ModifiedRange> m_FreeRanges;
  WGAL::ModifiedRange m_DirtyRange;

  WGALBufferCreationDescription m_Desc;

  WGALBufferHandle m_hBufferForUpload;
  WGALBufferHandle m_hBufferForRendering;

  WString m_sDebugName;

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  void CheckSelf() const;
#endif
};

W_DECLARE_FLAGS_OPERATORS(WGALDynamicBuffer::AllocateFlags);
