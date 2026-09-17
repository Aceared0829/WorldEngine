
template <typename T, WUInt32 SizeInBytes>
W_ALWAYS_INLINE WDataBlock<T, SizeInBytes>::WDataBlock(T* pData, WUInt32 uiCount)
{
  m_pData = pData;
  m_uiCount = uiCount;
}

template <typename T, WUInt32 SizeInBytes>
W_FORCE_INLINE T* WDataBlock<T, SizeInBytes>::ReserveBack()
{
  W_ASSERT_DEV(m_uiCount < CAPACITY, "Block is full.");
  return m_pData + m_uiCount++;
}

template <typename T, WUInt32 SizeInBytes>
W_FORCE_INLINE T* WDataBlock<T, SizeInBytes>::PopBack()
{
  W_ASSERT_DEV(m_uiCount > 0, "Block is empty");
  --m_uiCount;
  return m_pData + m_uiCount;
}

template <typename T, WUInt32 SizeInBytes>
W_ALWAYS_INLINE bool WDataBlock<T, SizeInBytes>::IsEmpty() const
{
  return m_uiCount == 0;
}

template <typename T, WUInt32 SizeInBytes>
W_ALWAYS_INLINE bool WDataBlock<T, SizeInBytes>::IsFull() const
{
  return m_uiCount == CAPACITY;
}

template <typename T, WUInt32 SizeInBytes>
W_FORCE_INLINE T& WDataBlock<T, SizeInBytes>::operator[](WUInt32 uiIndex) const
{
  W_ASSERT_DEV(uiIndex < m_uiCount, "Out of bounds access. Data block has {0} elements, trying to access element at index {1}.", m_uiCount, uiIndex);
  return m_pData[uiIndex];
}

///////////////////////////////////////////////////////////////////////////////////////////////////

template <WUInt32 BlockSize>
WLargeBlockAllocator<BlockSize>::WLargeBlockAllocator(WStringView sName, WAllocator* pParent, WAllocatorTrackingMode mode)
  : m_TrackingMode(mode)
  , m_SuperBlocks(pParent)
  , m_FreeBlocks(pParent)
{
  static_assert(BlockSize >= 4096, "Block size must be 4096 or bigger");

  m_Id = WMemoryTracker::RegisterAllocator(sName, mode, WPageAllocator::GetId());

  const WUInt32 uiPageSize = WSystemInformation::Get().GetMemoryPageSize();
  W_IGNORE_UNUSED(uiPageSize);
  W_ASSERT_DEV(uiPageSize <= BlockSize, "Memory Page size is bigger than block size.");
  W_ASSERT_DEV(BlockSize % uiPageSize == 0, "Blocksize ({0}) must be a multiple of page size ({1})", BlockSize, uiPageSize);
}

template <WUInt32 BlockSize>
WLargeBlockAllocator<BlockSize>::~WLargeBlockAllocator()
{
  WMemoryTracker::DeregisterAllocator(m_Id);

  for (WUInt32 i = 0; i < m_SuperBlocks.GetCount(); ++i)
  {
    WPageAllocator::DeallocatePage(m_SuperBlocks[i].m_pBasePtr);
  }
}

template <WUInt32 BlockSize>
template <typename T>
W_FORCE_INLINE WDataBlock<T, BlockSize> WLargeBlockAllocator<BlockSize>::AllocateBlock()
{
  struct Helper
  {
    enum
    {
      BLOCK_CAPACITY = WDataBlock<T, BlockSize>::CAPACITY
    };
  };

  static_assert(
    Helper::BLOCK_CAPACITY >= 1, "Type is too big for block allocation. Consider using regular heap allocation instead or increase the block size.");

  WDataBlock<T, BlockSize> block(static_cast<T*>(Allocate(alignof(T))), 0);
  return block;
}

template <WUInt32 BlockSize>
template <typename T>
W_FORCE_INLINE void WLargeBlockAllocator<BlockSize>::DeallocateBlock(WDataBlock<T, BlockSize>& inout_block)
{
  Deallocate(inout_block.m_pData);
  inout_block.m_pData = nullptr;
  inout_block.m_uiCount = 0;
}

template <WUInt32 BlockSize>
W_ALWAYS_INLINE WStringView WLargeBlockAllocator<BlockSize>::GetName() const
{
  return WMemoryTracker::GetAllocatorName(m_Id);
}

template <WUInt32 BlockSize>
W_ALWAYS_INLINE WAllocatorId WLargeBlockAllocator<BlockSize>::GetId() const
{
  return m_Id;
}

template <WUInt32 BlockSize>
W_ALWAYS_INLINE const WAllocator::Stats& WLargeBlockAllocator<BlockSize>::GetStats() const
{
  return WMemoryTracker::GetAllocatorStats(m_Id);
}

template <WUInt32 BlockSize>
void* WLargeBlockAllocator<BlockSize>::Allocate(size_t uiAlign)
{
  W_ASSERT_RELEASE(WMath::IsPowerOf2((WUInt32)uiAlign), "Alignment must be power of two");

  WTime fAllocationTime = WTime::Now();

  W_LOCK(m_Mutex);

  void* ptr = nullptr;

  if (!m_FreeBlocks.IsEmpty())
  {
    // Re-use a super block
    WUInt32 uiFreeBlockIndex = m_FreeBlocks.PeekBack();
    m_FreeBlocks.PopBack();

    const WUInt32 uiSuperBlockIndex = uiFreeBlockIndex / SuperBlock::NUM_BLOCKS;
    const WUInt32 uiInnerBlockIndex = uiFreeBlockIndex & (SuperBlock::NUM_BLOCKS - 1);
    SuperBlock& superBlock = m_SuperBlocks[uiSuperBlockIndex];
    ++superBlock.m_uiUsedBlocks;

    ptr = WMemoryUtils::AddByteOffset(superBlock.m_pBasePtr, uiInnerBlockIndex * BlockSize);
  }
  else
  {
    // Allocate a new super block
    void* pMemory = WPageAllocator::AllocatePage(SuperBlock::SIZE_IN_BYTES);
    W_CHECK_ALIGNMENT(pMemory, uiAlign);

    SuperBlock superBlock;
    superBlock.m_pBasePtr = pMemory;
    superBlock.m_uiUsedBlocks = 1;

    m_SuperBlocks.PushBack(superBlock);

    const WUInt32 uiBlockBaseIndex = (m_SuperBlocks.GetCount() - 1) * SuperBlock::NUM_BLOCKS;
    for (WUInt32 i = SuperBlock::NUM_BLOCKS - 1; i > 0; --i)
    {
      m_FreeBlocks.PushBack(uiBlockBaseIndex + i);
    }

    ptr = pMemory;
  }

  if (m_TrackingMode >= WAllocatorTrackingMode::AllocationStats)
  {
    WMemoryTracker::AddAllocation(m_Id, m_TrackingMode, ptr, BlockSize, uiAlign, WTime::Now() - fAllocationTime);
  }

  return ptr;
}

template <WUInt32 BlockSize>
void WLargeBlockAllocator<BlockSize>::Deallocate(void* ptr)
{
  W_LOCK(m_Mutex);

  if (m_TrackingMode >= WAllocatorTrackingMode::AllocationStats)
  {
    WMemoryTracker::RemoveAllocation(m_Id, ptr);
  }

  // find super block
  bool bFound = false;
  WUInt32 uiSuperBlockIndex = m_SuperBlocks.GetCount();
  std::ptrdiff_t diff = 0;

  for (; uiSuperBlockIndex-- > 0;)
  {
    diff = (char*)ptr - (char*)m_SuperBlocks[uiSuperBlockIndex].m_pBasePtr;
    if (diff >= 0 && diff < SuperBlock::SIZE_IN_BYTES)
    {
      bFound = true;
      break;
    }
  }

  W_IGNORE_UNUSED(bFound);
  W_ASSERT_DEV(bFound, "'{0}' was not allocated with this allocator", WArgP(ptr));

  SuperBlock& superBlock = m_SuperBlocks[uiSuperBlockIndex];
  --superBlock.m_uiUsedBlocks;

  if (superBlock.m_uiUsedBlocks == 0 && m_FreeBlocks.GetCount() > SuperBlock::NUM_BLOCKS * 4)
  {
    // give memory back
    WPageAllocator::DeallocatePage(superBlock.m_pBasePtr);

    m_SuperBlocks.RemoveAtAndSwap(uiSuperBlockIndex);
    const WUInt32 uiLastSuperBlockIndex = m_SuperBlocks.GetCount();

    // patch free list
    for (WUInt32 i = 0; i < m_FreeBlocks.GetCount(); ++i)
    {
      const WUInt32 uiIndex = m_FreeBlocks[i];
      const WUInt32 uiSBIndex = uiIndex / SuperBlock::NUM_BLOCKS;

      if (uiSBIndex == uiSuperBlockIndex)
      {
        // points to the block we just removed
        m_FreeBlocks.RemoveAtAndSwap(i);
        --i;
      }
      else if (uiSBIndex == uiLastSuperBlockIndex)
      {
        // points to the block we just swapped
        m_FreeBlocks[i] = uiSuperBlockIndex * SuperBlock::NUM_BLOCKS + (uiIndex & (SuperBlock::NUM_BLOCKS - 1));
      }
    }
  }
  else
  {
    // add block to free list
    const WUInt32 uiInnerBlockIndex = (WUInt32)(diff / BlockSize);
    m_FreeBlocks.PushBack(uiSuperBlockIndex * SuperBlock::NUM_BLOCKS + uiInnerBlockIndex);
  }
}
