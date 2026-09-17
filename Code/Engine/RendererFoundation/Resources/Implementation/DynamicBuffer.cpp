#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/DynamicBuffer.h>

namespace
{
  struct CompareRangesByStart
  {
    bool Less(const WGAL::ModifiedRange& a, const WGAL::ModifiedRange& b) const
    {
      return a.m_uiMin < b.m_uiMin;
    }
  };

  struct CompareRangesByStartReverse
  {
    bool Less(const WGAL::ModifiedRange& a, const WGAL::ModifiedRange& b) const
    {
      return a.m_uiMin > b.m_uiMin;
    }
  };

  struct CompareRangesByCount
  {
    bool Less(const WGAL::ModifiedRange& a, const WGAL::ModifiedRange& b) const
    {
      return a.GetCount() < b.GetCount();
    }
  };
} // namespace

////////////////////////////////////////////////////////////////////////

WGALDynamicBuffer::~WGALDynamicBuffer()
{
  Deinitialize();
}

void WGALDynamicBuffer::Initialize(const WGALBufferCreationDescription& desc, WStringView sDebugName)
{
  W_ASSERT_DEV(desc.m_uiStructSize > 0, "Struct size must be greater than 0");
  W_IGNORE_UNUSED(sDebugName);

  m_Desc = desc;

  m_Data.SetCountUninitialized(desc.m_uiTotalSize);
  m_uiCapacity = desc.m_uiTotalSize / desc.m_uiStructSize;

  m_sDebugName = sDebugName;
}

void WGALDynamicBuffer::Deinitialize()
{
  Clear();

  if (m_hBufferForRendering != m_hBufferForUpload)
  {
    WGALDevice::GetDefaultDevice()->DestroyBuffer(m_hBufferForRendering);
  }

  WGALDevice::GetDefaultDevice()->DestroyBuffer(m_hBufferForUpload);
}

void WGALDynamicBuffer::Clear()
{
  m_Data.SetCountUninitialized(m_Desc.m_uiTotalSize);
  m_uiNextOffset = 0;

  for (auto& tempData : m_TempData)
  {
    tempData.m_pAllocator->Deallocate(tempData.m_pData);
  }
  m_TempData.Clear();

  m_Allocations.Clear();
  m_FreeRanges.Clear();
  m_DirtyRange.Reset();
}

WUInt32 WGALDynamicBuffer::Allocate(WUInt64 uiUserData, WUInt32 uiCount, WBitflags<AllocateFlags> allocateFlags, WAllocator* pTempAllocator)
{
  W_LOCK(m_Mutex);

  WUInt32 uiOffset = WInvalidIndex;

  for (WUInt32 i = 0; i < m_FreeRanges.GetCount(); ++i)
  {
    auto& freeRange = m_FreeRanges[i];
    const WUInt32 uiFreeCount = freeRange.GetCount();

    if (uiFreeCount >= uiCount)
    {
      uiOffset = freeRange.m_uiMin;

      if (uiFreeCount == uiCount)
      {
        m_FreeRanges.RemoveAtAndCopy(i);
      }
      else
      {
        freeRange.m_uiMin += uiCount;
      }

      break;
    }
  }

  if (uiOffset == WInvalidIndex)
  {
    uiOffset = m_uiNextOffset;
    m_uiNextOffset += uiCount;

    if (m_uiNextOffset > m_uiCapacity)
    {
      AllocateTempData(uiOffset, m_uiNextOffset, pTempAllocator);
    }
  }

  WUInt32 uiDataIndex = 0;
  const WUInt32 uiByteEndOffset = (uiOffset + uiCount) * m_Desc.m_uiStructSize;
  if (uiByteEndOffset > m_Data.GetCount())
  {
    for (WUInt32 i = 0; i < m_TempData.GetCount(); ++i)
    {
      auto& tempData = m_TempData[i];
      if (uiByteEndOffset <= (tempData.m_uiStartByteOffset + tempData.m_uiByteSize))
      {
        uiDataIndex = i + 1;
        break;
      }
    }
  }

  m_Allocations.Insert(uiOffset, Allocation{uiUserData, uiCount, uiDataIndex});

  if (allocateFlags.IsSet(AllocateFlags::ZeroFill))
  {
    WUInt32 uiDummyCount = 0;
    auto data = MapForWriting(uiOffset, uiDummyCount);
    WMemoryUtils::ZeroFill(data.GetPtr(), data.GetCount());
  }

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  CheckSelf();
#endif

  return uiOffset;
}

void WGALDynamicBuffer::Deallocate(WUInt32 uiOffset)
{
  W_LOCK(m_Mutex);

  auto it = m_Allocations.Find(uiOffset);
  W_ASSERT_DEV(it.IsValid(), "Invalid offset");

  const WUInt32 uiCount = it.Value().m_uiCount;

  if (it.Key() == m_Allocations.GetReverseIterator().Key())
  {
    m_uiNextOffset = uiOffset;

    // Remove free range in front of the last allocation
    for (WUInt32 i = 0; i < m_FreeRanges.GetCount(); ++i)
    {
      auto& freeRange = m_FreeRanges[i];
      if (freeRange.m_uiMax + 1 == m_uiNextOffset)
      {
        m_uiNextOffset = freeRange.m_uiMin;
        m_FreeRanges.RemoveAtAndCopy(i);
        break;
      }
    }
  }
  else
  {
    m_FreeRanges.PushBack(WGAL::ModifiedRange{uiOffset, uiOffset + uiCount - 1});

    // Merge adjacent free ranges
    m_FreeRanges.Sort(CompareRangesByStart());
    for (WUInt32 i = 0; i < m_FreeRanges.GetCount() - 1; ++i)
    {
      auto& currentFreeRange = m_FreeRanges[i];
      auto& nextFreeRange = m_FreeRanges[i + 1];

      if (currentFreeRange.m_uiMax + 1 == nextFreeRange.m_uiMin)
      {
        currentFreeRange.m_uiMax = nextFreeRange.m_uiMax;
        m_FreeRanges.RemoveAtAndCopy(i + 1);
        --i;
      }
    }

    // Sort by count to make sure that the smaller holes are filled first
    m_FreeRanges.Sort(CompareRangesByCount());
  }

  m_Allocations.Remove(it);

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  CheckSelf();
#endif
}

WByteArrayPtr WGALDynamicBuffer::MapForWriting(WUInt32 uiOffset, WUInt32& out_uiCount)
{
  W_LOCK(m_Mutex);

  auto it = m_Allocations.Find(uiOffset);
  W_ASSERT_DEV(it.IsValid(), "Invalid offset");

  auto& allocation = it.Value();
  out_uiCount = allocation.m_uiCount;

  // Mark dirty
  m_DirtyRange.SetToIncludeRange(uiOffset, uiOffset + out_uiCount - 1);

  const WUInt32 uiByteOffset = uiOffset * m_Desc.m_uiStructSize;
  const WUInt32 uiByteSize = out_uiCount * m_Desc.m_uiStructSize;

  if (allocation.m_uiDataIndex > 0)
  {
    auto& tempData = m_TempData[allocation.m_uiDataIndex - 1];
    const WUInt32 uiLocalByteOffset = uiByteOffset - tempData.m_uiStartByteOffset;
    W_ASSERT_DEBUG(uiLocalByteOffset + uiByteSize <= tempData.m_uiByteSize, "Implementation error");
    return WByteArrayPtr(tempData.m_pData + uiLocalByteOffset, uiByteSize);
  }

  return m_Data.GetByteArrayPtr().GetSubArray(uiByteOffset, uiByteSize);
}

WConstByteArrayPtr WGALDynamicBuffer::MapForReading(WUInt32 uiOffset, WUInt32& out_uiCount) const
{
  W_LOCK(m_Mutex);

  auto it = m_Allocations.Find(uiOffset);
  W_ASSERT_DEV(it.IsValid(), "Invalid offset");

  auto& allocation = it.Value();
  out_uiCount = allocation.m_uiCount;

  const WUInt32 uiByteOffset = uiOffset * m_Desc.m_uiStructSize;
  const WUInt32 uiByteSize = out_uiCount * m_Desc.m_uiStructSize;

  if (allocation.m_uiDataIndex > 0)
  {
    auto& tempData = m_TempData[allocation.m_uiDataIndex - 1];
    const WUInt32 uiLocalByteOffset = uiByteOffset - tempData.m_uiStartByteOffset;
    W_ASSERT_DEBUG(uiLocalByteOffset + uiByteSize <= tempData.m_uiByteSize, "Implementation error");
    return WConstByteArrayPtr(tempData.m_pData + uiLocalByteOffset, uiByteSize);
  }

  return m_Data.GetByteArrayPtr().GetSubArray(uiByteOffset, uiByteSize);
}

WUInt32 WGALDynamicBuffer::AllocateTempData(WUInt32 uiStartOffset, WUInt32 uiNewCount, WAllocator* pTempAllocator)
{
  constexpr WUInt32 uiExpGrowthLimit = 16 * 1024 * 1024;

  uiNewCount = WMath::Max(uiNewCount, 256U);
  if (uiNewCount < uiExpGrowthLimit)
  {
    uiNewCount = WMath::PowerOfTwo_Ceil(uiNewCount);
  }
  else
  {
    uiNewCount = WMemoryUtils::AlignSize(uiNewCount, uiExpGrowthLimit);
  }

  m_Desc.m_uiTotalSize = uiNewCount * m_Desc.m_uiStructSize;
  m_uiCapacity = uiNewCount;

  if (pTempAllocator == nullptr)
  {
    pTempAllocator = WFoundation::GetAlignedAllocator();
  }

  TempData& tempData = m_TempData.ExpandAndGetRef();
  tempData.m_pAllocator = pTempAllocator;
  tempData.m_uiByteSize = (uiNewCount - uiStartOffset) * m_Desc.m_uiStructSize;
  tempData.m_uiStartByteOffset = uiStartOffset * m_Desc.m_uiStructSize;
  tempData.m_pData = static_cast<WUInt8*>(pTempAllocator->Allocate(tempData.m_uiByteSize, 16));

  m_DirtyRange.SetToIncludeRange(0, (uiNewCount - 1));

  return m_TempData.GetCount();
}

void WGALDynamicBuffer::UploadChangesForNextFrame()
{
  W_LOCK(m_Mutex);

  if (m_DirtyRange.IsValid() == false)
    return;

  // Assemble final data buffer
  m_Data.SetCountUninitialized(m_Desc.m_uiTotalSize);
  for (auto& tempData : m_TempData)
  {
    WMemoryUtils::Copy(&m_Data[tempData.m_uiStartByteOffset], tempData.m_pData, tempData.m_uiByteSize);
    tempData.m_pAllocator->Deallocate(tempData.m_pData);
  }
  m_TempData.Clear();

  // Patch data indices
  for (auto it = m_Allocations.GetReverseIterator(); it.IsValid(); ++it)
  {
    auto& allocation = it.Value();
    if (allocation.m_uiDataIndex == 0)
      break;

    allocation.m_uiDataIndex = 0;
  }

  auto pDevice = WGALDevice::GetDefaultDevice();

  if (m_hBufferForUpload.IsInvalidated() == false && pDevice->GetBuffer(m_hBufferForUpload)->GetDescription().m_uiTotalSize != m_Desc.m_uiTotalSize)
  {
    pDevice->DestroyBuffer(m_hBufferForUpload);
    m_hBufferForUpload.Invalidate();
  }

  if (m_hBufferForUpload.IsInvalidated())
  {
    m_hBufferForUpload = pDevice->CreateBuffer(m_Desc, m_Data);

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
    pDevice->GetBuffer(m_hBufferForUpload)->SetDebugName(m_sDebugName);
#endif
  }
  else
  {
    const WUInt32 uiByteOffset = m_DirtyRange.m_uiMin * m_Desc.m_uiStructSize;
    const WUInt32 uiByteSize = m_DirtyRange.GetCount() * m_Desc.m_uiStructSize;
    auto data = m_Data.GetArrayPtr().GetSubArray(uiByteOffset, uiByteSize);

    pDevice->UpdateBufferForNextFrame(m_hBufferForUpload, data, uiByteOffset);
  }

  m_DirtyRange.Reset();
}

void WGALDynamicBuffer::RunCompactionSteps(WDynamicArray<ChangedAllocation>& out_changedAllocations, WUInt32 uiMaxSteps)
{
  W_LOCK(m_Mutex);

  out_changedAllocations.Clear();

  if (m_FreeRanges.IsEmpty() || m_Allocations.IsEmpty())
    return;

  // Don't try to compact when we still have temporary data
  if (m_TempData.IsEmpty() == false)
    return;

  m_FreeRanges.Sort(CompareRangesByStartReverse());
  W_SCOPE_EXIT(m_FreeRanges.Sort(CompareRangesByCount()));

  auto MoveAllocation = [&](const Allocation& allocation, WUInt32 uiOldOffset, WUInt32 uiNewOffset)
  {
    out_changedAllocations.PushBack(ChangedAllocation{allocation.m_uiUserData, uiNewOffset});

    const WUInt32 uiOldByteOffset = uiOldOffset * m_Desc.m_uiStructSize;
    const WUInt32 uiNewByteOffset = uiNewOffset * m_Desc.m_uiStructSize;
    const WUInt32 uiByteSize = allocation.m_uiCount * m_Desc.m_uiStructSize;
    WMemoryUtils::Copy(&m_Data[uiNewByteOffset], &m_Data[uiOldByteOffset], uiByteSize);

    m_DirtyRange.SetToIncludeRange(uiNewOffset, uiNewOffset + allocation.m_uiCount - 1);

    m_Allocations.Insert(uiNewOffset, allocation);
    m_Allocations.Remove(uiOldOffset);
  };

  for (WUInt32 i = 0; i < uiMaxSteps; ++i)
  {
    if (m_FreeRanges.IsEmpty())
      return;

    auto& freeRange = m_FreeRanges.PeekBack();
    const WUInt32 uiFreeCount = freeRange.GetCount();
    const WUInt32 uiNewOffset = freeRange.m_uiMin;

    // first check whether the last allocation fits into the hole
    auto revIt = m_Allocations.GetReverseIterator();
    if (revIt.IsValid())
    {
      if (revIt.Value().m_uiCount == uiFreeCount)
      {
        m_FreeRanges.PopBack();

        m_uiNextOffset = revIt.Key();
        MoveAllocation(revIt.Value(), revIt.Key(), uiNewOffset);
        continue;
      }
    }

    // if not start to move the allocations forward
    auto it = m_Allocations.Find(freeRange.m_uiMax + 1);
    W_ASSERT_DEV(it.IsValid(), "Implementation error");
    {
      const WUInt32 uiNewFreeRangeMin = uiNewOffset + it.Value().m_uiCount;

      if (it.Key() == revIt.Key())
      {
        // This was the last allocation
        m_uiNextOffset = uiNewFreeRangeMin;
        m_FreeRanges.PopBack();
      }
      else
      {
        freeRange.m_uiMin = uiNewFreeRangeMin;
        freeRange.m_uiMax = freeRange.m_uiMin + uiFreeCount - 1;

        // merge adjacent free ranges
        if (m_FreeRanges.GetCount() > 1)
        {
          const WUInt32 uiSecondIndex = m_FreeRanges.GetCount() - 2;
          auto& secondFreeRange = m_FreeRanges[uiSecondIndex];
          if (freeRange.m_uiMax + 1 == secondFreeRange.m_uiMin)
          {
            freeRange.m_uiMax = secondFreeRange.m_uiMax;
            m_FreeRanges.RemoveAtAndCopy(uiSecondIndex);
          }
        }
      }

      MoveAllocation(it.Value(), it.Key(), uiNewOffset);
    }
  }

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  CheckSelf();
#endif
}

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
void WGALDynamicBuffer::CheckSelf() const
{
#  if 0
  if (m_uiNextOffset == 0 && m_Allocations.IsEmpty() && m_FreeRanges.IsEmpty())
    return;

  WDynamicBitfield check;
  check.SetCount(m_uiNextOffset, false);

  for (auto it : m_Allocations)
  {
    const WUInt32 uiStart = it.Key();
    const WUInt32 uiCount = it.Value().m_uiCount;
    W_ASSERT_DEBUG(!check.IsAnyBitSet(uiStart, uiCount), "Overlapping allocation detected");
    check.SetBitRange(uiStart, uiCount);
  }

  for (auto range : m_FreeRanges)
  {
    const WUInt32 uiStart = range.m_uiMin;
    const WUInt32 uiCount = range.GetCount();
    W_ASSERT_DEBUG(!check.IsAnyBitSet(uiStart, uiCount), "Overlapping free range detected");
    check.SetBitRange(uiStart, uiCount);
  }

  W_ASSERT_DEBUG(check.AreAllBitsSet(), "Some memory is neither allocated nor free");
#  endif
}
#endif
