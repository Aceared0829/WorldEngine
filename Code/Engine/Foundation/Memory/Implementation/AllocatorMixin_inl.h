namespace WInternal
{
  template <typename AllocationPolicy, WAllocatorTrackingMode TrackingMode>
  class WAllocatorImpl : public WAllocator
  {
  public:
    WAllocatorImpl(WStringView sName, WAllocator* pParent);
    ~WAllocatorImpl();

    // WAllocator implementation
    virtual void* Allocate(size_t uiSize, size_t uiAlign, WMemoryUtils::DestructorFunction destructorFunc = nullptr) override;
    virtual void Deallocate(void* pPtr) override;
    virtual size_t AllocatedSize(const void* pPtr) override;
    virtual WAllocatorId GetId() const override;
    virtual Stats GetStats() const override;

    WAllocator* GetParent() const;

  protected:
    AllocationPolicy m_allocator;

    WAllocatorId m_Id;
    WThreadID m_ThreadID;
  };

  template <typename AllocationPolicy, WAllocatorTrackingMode TrackingMode, bool HasReallocate>
  class WAllocatorMixinReallocate : public WAllocatorImpl<AllocationPolicy, TrackingMode>
  {
  public:
    WAllocatorMixinReallocate(WStringView sName, WAllocator* pParent);
  };

  template <typename AllocationPolicy, WAllocatorTrackingMode TrackingMode>
  class WAllocatorMixinReallocate<AllocationPolicy, TrackingMode, true> : public WAllocatorImpl<AllocationPolicy, TrackingMode>
  {
  public:
    WAllocatorMixinReallocate(WStringView sName, WAllocator* pParent);
    virtual void* Reallocate(void* pPtr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign) override;
  };
}; // namespace WInternal

template <typename A, WAllocatorTrackingMode TrackingMode>
W_FORCE_INLINE WInternal::WAllocatorImpl<A, TrackingMode>::WAllocatorImpl(WStringView sName, WAllocator* pParent /* = nullptr */)
  : m_allocator(pParent)
  , m_ThreadID(WThreadUtils::GetCurrentThreadID())
{
  if constexpr (TrackingMode >= WAllocatorTrackingMode::Basics)
  {
    this->m_Id = WMemoryTracker::RegisterAllocator(sName, TrackingMode, pParent != nullptr ? pParent->GetId() : WAllocatorId());
  }
}

template <typename A, WAllocatorTrackingMode TrackingMode>
WInternal::WAllocatorImpl<A, TrackingMode>::~WAllocatorImpl()
{
  if constexpr (TrackingMode >= WAllocatorTrackingMode::Basics)
  {
    WMemoryTracker::DeregisterAllocator(this->m_Id);
  }
}

template <typename A, WAllocatorTrackingMode TrackingMode>
void* WInternal::WAllocatorImpl<A, TrackingMode>::Allocate(size_t uiSize, size_t uiAlign, WMemoryUtils::DestructorFunction destructorFunc)
{
  W_IGNORE_UNUSED(destructorFunc);

  // zero size allocations always return nullptr without tracking (since deallocate nullptr is ignored)
  if (uiSize == 0)
    return nullptr;

  W_ASSERT_DEBUG(WMath::IsPowerOf2((WUInt32)uiAlign), "Alignment must be power of two");

  [[maybe_unused]] WTime fAllocationTime;

  if constexpr (TrackingMode >= WAllocatorTrackingMode::AllocationStats)
  {
    fAllocationTime = WTime::Now();
  }

  void* ptr = m_allocator.Allocate(uiSize, uiAlign);
  W_ASSERT_DEV(ptr != nullptr, "Could not allocate {0} bytes. Out of memory?", uiSize);

  if constexpr (TrackingMode >= WAllocatorTrackingMode::AllocationStats)
  {
    WMemoryTracker::AddAllocation(this->m_Id, TrackingMode, ptr, uiSize, uiAlign, WTime::Now() - fAllocationTime);
  }

  return ptr;
}

template <typename A, WAllocatorTrackingMode TrackingMode>
void WInternal::WAllocatorImpl<A, TrackingMode>::Deallocate(void* pPtr)
{
  if constexpr (TrackingMode >= WAllocatorTrackingMode::AllocationStats)
  {
    WMemoryTracker::RemoveAllocation(this->m_Id, pPtr);
  }

  m_allocator.Deallocate(pPtr);
}

template <typename A, WAllocatorTrackingMode TrackingMode>
size_t WInternal::WAllocatorImpl<A, TrackingMode>::AllocatedSize(const void* pPtr)
{
  if constexpr (TrackingMode >= WAllocatorTrackingMode::AllocationStats)
  {
    return WMemoryTracker::GetAllocationInfo(this->m_Id, pPtr).m_uiSize;
  }
  else
  {
    return 0;
  }
}

template <typename A, WAllocatorTrackingMode TrackingMode>
WAllocatorId WInternal::WAllocatorImpl<A, TrackingMode>::GetId() const
{
  return this->m_Id;
}

template <typename A, WAllocatorTrackingMode TrackingMode>
WAllocator::Stats WInternal::WAllocatorImpl<A, TrackingMode>::GetStats() const
{
  if constexpr (TrackingMode >= WAllocatorTrackingMode::Basics)
  {
    return WMemoryTracker::GetAllocatorStats(this->m_Id);
  }
  else
  {
    return Stats();
  }
}

template <typename A, WAllocatorTrackingMode TrackingMode>
W_ALWAYS_INLINE WAllocator* WInternal::WAllocatorImpl<A, TrackingMode>::GetParent() const
{
  return m_allocator.GetParent();
}

template <typename A, WAllocatorTrackingMode TrackingMode, bool HasReallocate>
WInternal::WAllocatorMixinReallocate<A, TrackingMode, HasReallocate>::WAllocatorMixinReallocate(WStringView sName, WAllocator* pParent)
  : WAllocatorImpl<A, TrackingMode>(sName, pParent)
{
}

template <typename A, WAllocatorTrackingMode TrackingMode>
WInternal::WAllocatorMixinReallocate<A, TrackingMode, true>::WAllocatorMixinReallocate(WStringView sName, WAllocator* pParent)
  : WAllocatorImpl<A, TrackingMode>(sName, pParent)
{
}

template <typename A, WAllocatorTrackingMode TrackingMode>
void* WInternal::WAllocatorMixinReallocate<A, TrackingMode, true>::Reallocate(void* pPtr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign)
{
  [[maybe_unused]] WTime fAllocationTime;

  if constexpr (TrackingMode >= WAllocatorTrackingMode::AllocationStats)
  {
    WMemoryTracker::RemoveAllocation(this->m_Id, pPtr);
    fAllocationTime = WTime::Now();
  }

  void* pNewMem = this->m_allocator.Reallocate(pPtr, uiCurrentSize, uiNewSize, uiAlign);

  if constexpr (TrackingMode >= WAllocatorTrackingMode::AllocationStats)
  {
    WMemoryTracker::AddAllocation(this->m_Id, TrackingMode, pNewMem, uiNewSize, uiAlign, WTime::Now() - fAllocationTime);
  }

  return pNewMem;
}
