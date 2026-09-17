template <WAllocatorTrackingMode TrackingMode, bool OverwriteMemoryOnReset>
WLinearAllocator<TrackingMode, OverwriteMemoryOnReset>::WLinearAllocator(WStringView sName, WAllocator* pParent, WUInt32 uiInitialSize)
  : SUPER(sName, pParent)
  , m_DestructData(pParent)
  , m_PtrToDestructDataIndexTable(pParent)
{
  this->m_allocator.SetNextBucketSize(uiInitialSize);
}

template <WAllocatorTrackingMode TrackingMode, bool OverwriteMemoryOnReset>
WLinearAllocator<TrackingMode, OverwriteMemoryOnReset>::~WLinearAllocator()
{
  Reset();
}

template <WAllocatorTrackingMode TrackingMode, bool OverwriteMemoryOnReset>
void* WLinearAllocator<TrackingMode, OverwriteMemoryOnReset>::Allocate(size_t uiSize, size_t uiAlign, WMemoryUtils::DestructorFunction destructorFunc)
{
  W_LOCK(m_Mutex);

  void* ptr = SUPER::Allocate(uiSize, uiAlign, destructorFunc);

  if (destructorFunc != nullptr)
  {
    WUInt32 uiIndex = m_DestructData.GetCount();
    m_PtrToDestructDataIndexTable.Insert(ptr, uiIndex);

    auto& data = m_DestructData.ExpandAndGetRef();
    data.m_Func = destructorFunc;
    data.m_Ptr = ptr;
  }

  return ptr;
}

template <WAllocatorTrackingMode TrackingMode, bool OverwriteMemoryOnReset>
void WLinearAllocator<TrackingMode, OverwriteMemoryOnReset>::Deallocate(void* pPtr)
{
  W_LOCK(m_Mutex);

  WUInt32 uiIndex;
  if (m_PtrToDestructDataIndexTable.Remove(pPtr, &uiIndex))
  {
    auto& data = m_DestructData[uiIndex];
    data.m_Func = nullptr;
    data.m_Ptr = nullptr;
  }

  SUPER::Deallocate(pPtr);
}

W_MSVC_ANALYSIS_WARNING_PUSH

// Disable warning for incorrect operator (compiler complains about the TrackingMode bitwise and in the case that flags = None)
// even with the added guard of a check that it can't be 0.
W_MSVC_ANALYSIS_WARNING_DISABLE(6313)

template <WAllocatorTrackingMode TrackingMode, bool OverwriteMemoryOnReset>
void WLinearAllocator<TrackingMode, OverwriteMemoryOnReset>::Reset()
{
  W_LOCK(m_Mutex);

  for (WUInt32 i = m_DestructData.GetCount(); i-- > 0;)
  {
    auto& data = m_DestructData[i];
    if (data.m_Func != nullptr)
      data.m_Func(data.m_Ptr);
  }

  m_DestructData.Clear();
  m_PtrToDestructDataIndexTable.Clear();

  this->m_allocator.Reset();
  if constexpr (TrackingMode >= WAllocatorTrackingMode::AllocationStats)
  {
    WMemoryTracker::RemoveAllAllocations(this->m_Id);
  }
  else if constexpr (TrackingMode >= WAllocatorTrackingMode::Basics)
  {
    WAllocator::Stats stats;
    this->m_allocator.FillStats(stats);

    WMemoryTracker::SetAllocatorStats(this->m_Id, stats);
  }
}
W_MSVC_ANALYSIS_WARNING_POP
