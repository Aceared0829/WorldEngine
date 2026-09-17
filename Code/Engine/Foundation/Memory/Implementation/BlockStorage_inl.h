
template <typename T, WUInt32 BlockSize, WBlockStorageType::Enum StorageType>
W_FORCE_INLINE WBlockStorage<T, BlockSize, StorageType>::ConstIterator::ConstIterator(
  const WBlockStorage<T, BlockSize, StorageType>& storage, WUInt32 uiStartIndex, WUInt32 uiCount)
  : m_Storage(storage)
{
  m_uiCurrentIndex = uiStartIndex;
  m_uiEndIndex = WMath::Max(uiStartIndex + uiCount, uiCount);

  if (StorageType == WBlockStorageType::FreeList)
  {
    WUInt32 uiEndIndex = WMath::Min(m_uiEndIndex, m_Storage.m_uiCount);
    while (m_uiCurrentIndex < uiEndIndex && !m_Storage.m_UsedEntries.IsBitSet(m_uiCurrentIndex))
    {
      ++m_uiCurrentIndex;
    }
  }
}

template <typename T, WUInt32 BlockSize, WBlockStorageType::Enum StorageType>
W_FORCE_INLINE T& WBlockStorage<T, BlockSize, StorageType>::ConstIterator::CurrentElement() const
{
  const WUInt32 uiBlockIndex = m_uiCurrentIndex / WDataBlock<T, BlockSize>::CAPACITY;
  const WUInt32 uiInnerIndex = m_uiCurrentIndex - uiBlockIndex * WDataBlock<T, BlockSize>::CAPACITY;
  return m_Storage.m_Blocks[uiBlockIndex][uiInnerIndex];
}

template <typename T, WUInt32 BlockSize, WBlockStorageType::Enum StorageType>
W_ALWAYS_INLINE const T& WBlockStorage<T, BlockSize, StorageType>::ConstIterator::operator*() const
{
  return CurrentElement();
}

template <typename T, WUInt32 BlockSize, WBlockStorageType::Enum StorageType>
W_ALWAYS_INLINE const T* WBlockStorage<T, BlockSize, StorageType>::ConstIterator::operator->() const
{
  return &CurrentElement();
}

template <typename T, WUInt32 BlockSize, WBlockStorageType::Enum StorageType>
W_ALWAYS_INLINE WBlockStorage<T, BlockSize, StorageType>::ConstIterator::operator const T*() const
{
  return &CurrentElement();
}

template <typename T, WUInt32 BlockSize, WBlockStorageType::Enum StorageType>
W_FORCE_INLINE void WBlockStorage<T, BlockSize, StorageType>::ConstIterator::Next()
{
  ++m_uiCurrentIndex;

  if (StorageType == WBlockStorageType::FreeList)
  {
    WUInt32 uiEndIndex = WMath::Min(m_uiEndIndex, m_Storage.m_uiCount);
    while (m_uiCurrentIndex < uiEndIndex && !m_Storage.m_UsedEntries.IsBitSet(m_uiCurrentIndex))
    {
      ++m_uiCurrentIndex;
    }
  }
}

template <typename T, WUInt32 BlockSize, WBlockStorageType::Enum StorageType>
W_FORCE_INLINE bool WBlockStorage<T, BlockSize, StorageType>::ConstIterator::IsValid() const
{
  return m_uiCurrentIndex < WMath::Min(m_uiEndIndex, m_Storage.m_uiCount);
}

template <typename T, WUInt32 BlockSize, WBlockStorageType::Enum StorageType>
W_ALWAYS_INLINE void WBlockStorage<T, BlockSize, StorageType>::ConstIterator::operator++()
{
  Next();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T, WUInt32 BlockSize, WBlockStorageType::Enum StorageType>
W_FORCE_INLINE WBlockStorage<T, BlockSize, StorageType>::Iterator::Iterator(
  const WBlockStorage<T, BlockSize, StorageType>& storage, WUInt32 uiStartIndex, WUInt32 uiCount)
  : ConstIterator(storage, uiStartIndex, uiCount)
{
}

template <typename T, WUInt32 BlockSize, WBlockStorageType::Enum StorageType>
W_ALWAYS_INLINE T& WBlockStorage<T, BlockSize, StorageType>::Iterator::operator*()
{
  return this->CurrentElement();
}

template <typename T, WUInt32 BlockSize, WBlockStorageType::Enum StorageType>
W_ALWAYS_INLINE T* WBlockStorage<T, BlockSize, StorageType>::Iterator::operator->()
{
  return &(this->CurrentElement());
}

template <typename T, WUInt32 BlockSize, WBlockStorageType::Enum StorageType>
W_ALWAYS_INLINE WBlockStorage<T, BlockSize, StorageType>::Iterator::operator T*()
{
  return &(this->CurrentElement());
}

///////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T, WUInt32 BlockSize, WBlockStorageType::Enum StorageType>
W_FORCE_INLINE WBlockStorage<T, BlockSize, StorageType>::WBlockStorage(
  WLargeBlockAllocator<BlockSize>* pBlockAllocator, WAllocator* pAllocator)
  : m_pBlockAllocator(pBlockAllocator)
  , m_Blocks(pAllocator)

{
}

template <typename T, WUInt32 BlockSize, WBlockStorageType::Enum StorageType>
WBlockStorage<T, BlockSize, StorageType>::~WBlockStorage()
{
  Clear();
}

template <typename T, WUInt32 BlockSize, WBlockStorageType::Enum StorageType>
void WBlockStorage<T, BlockSize, StorageType>::Clear()
{
  for (WUInt32 uiBlockIndex = 0; uiBlockIndex < m_Blocks.GetCount(); ++uiBlockIndex)
  {
    WDataBlock<T, BlockSize>& block = m_Blocks[uiBlockIndex];

    if (StorageType == WBlockStorageType::Compact)
    {
      WMemoryUtils::Destruct(block.m_pData, block.m_uiCount);
    }
    else
    {
      for (WUInt32 uiInnerIndex = 0; uiInnerIndex < block.m_uiCount; ++uiInnerIndex)
      {
        WUInt32 uiIndex = uiBlockIndex * WDataBlock<T, BlockSize>::CAPACITY + uiInnerIndex;
        if (m_UsedEntries.IsBitSet(uiIndex))
        {
          WMemoryUtils::Destruct(&block.m_pData[uiInnerIndex], 1);
        }
      }
    }

    m_pBlockAllocator->DeallocateBlock(block);
  }

  m_Blocks.Clear();
}

template <typename T, WUInt32 BlockSize, WBlockStorageType::Enum StorageType>
T* WBlockStorage<T, BlockSize, StorageType>::Create()
{
  T* pNewObject = nullptr;
  WUInt32 uiNewIndex = WInvalidIndex;

  if (StorageType == WBlockStorageType::FreeList && m_uiFreelistStart != WInvalidIndex)
  {
    uiNewIndex = m_uiFreelistStart;

    const WUInt32 uiBlockIndex = uiNewIndex / WDataBlock<T, BlockSize>::CAPACITY;
    const WUInt32 uiInnerIndex = uiNewIndex - uiBlockIndex * WDataBlock<T, BlockSize>::CAPACITY;

    pNewObject = &(m_Blocks[uiBlockIndex][uiInnerIndex]);

    m_uiFreelistStart = *reinterpret_cast<WUInt32*>(pNewObject);
  }
  else
  {
    WDataBlock<T, BlockSize>* pBlock = nullptr;

    if (m_Blocks.GetCount() > 0)
    {
      pBlock = &m_Blocks.PeekBack();
    }

    if (pBlock == nullptr || pBlock->IsFull())
    {
      m_Blocks.PushBack(m_pBlockAllocator->template AllocateBlock<T>());
      pBlock = &m_Blocks.PeekBack();
    }

    pNewObject = pBlock->ReserveBack();
    uiNewIndex = m_uiCount;

    ++m_uiCount;
  }

  WMemoryUtils::Construct<SkipTrivialTypes>(pNewObject, 1);

  if (StorageType == WBlockStorageType::FreeList)
  {
    m_UsedEntries.SetCount(m_uiCount);
    m_UsedEntries.SetBit(uiNewIndex);
  }

  return pNewObject;
}

template <typename T, WUInt32 BlockSize, WBlockStorageType::Enum StorageType>
W_FORCE_INLINE void WBlockStorage<T, BlockSize, StorageType>::Delete(T* pObject)
{
  T* pDummy;
  Delete(pObject, pDummy);
}

template <typename T, WUInt32 BlockSize, WBlockStorageType::Enum StorageType>
void WBlockStorage<T, BlockSize, StorageType>::Delete(T* pObject, T*& out_pMovedObject)
{
  Delete(pObject, out_pMovedObject, WTraitInt<StorageType>());
}

template <typename T, WUInt32 BlockSize, WBlockStorageType::Enum StorageType>
W_ALWAYS_INLINE WUInt32 WBlockStorage<T, BlockSize, StorageType>::GetCount() const
{
  return m_uiCount;
}

template <typename T, WUInt32 BlockSize, WBlockStorageType::Enum StorageType>
W_ALWAYS_INLINE typename WBlockStorage<T, BlockSize, StorageType>::Iterator WBlockStorage<T, BlockSize, StorageType>::GetIterator(
  WUInt32 uiStartIndex /*= 0*/, WUInt32 uiCount /*= WInvalidIndex*/)
{
  return Iterator(*this, uiStartIndex, uiCount);
}

template <typename T, WUInt32 BlockSize, WBlockStorageType::Enum StorageType>
W_ALWAYS_INLINE typename WBlockStorage<T, BlockSize, StorageType>::ConstIterator WBlockStorage<T, BlockSize, StorageType>::GetIterator(
  WUInt32 uiStartIndex /*= 0*/, WUInt32 uiCount /*= WInvalidIndex*/) const
{
  return ConstIterator(*this, uiStartIndex, uiCount);
}

template <typename T, WUInt32 BlockSize, WBlockStorageType::Enum StorageType>
W_FORCE_INLINE void WBlockStorage<T, BlockSize, StorageType>::Delete(T* pObject, T*& out_pMovedObject, WTraitInt<WBlockStorageType::Compact>)
{
  WDataBlock<T, BlockSize>& lastBlock = m_Blocks.PeekBack();
  T* pLast = lastBlock.PopBack();

  --m_uiCount;
  if (pObject != pLast)
  {
    WMemoryUtils::Relocate(pObject, pLast, 1);
  }
  else
  {
    WMemoryUtils::Destruct(pLast, 1);
  }

  out_pMovedObject = pLast;

  if (lastBlock.IsEmpty())
  {
    m_pBlockAllocator->DeallocateBlock(lastBlock);
    m_Blocks.PopBack();
  }
}

template <typename T, WUInt32 BlockSize, WBlockStorageType::Enum StorageType>
W_FORCE_INLINE void WBlockStorage<T, BlockSize, StorageType>::Delete(T* pObject, T*& out_pMovedObject, WTraitInt<WBlockStorageType::FreeList>)
{
  WUInt32 uiIndex = WInvalidIndex;
  for (WUInt32 uiBlockIndex = 0; uiBlockIndex < m_Blocks.GetCount(); ++uiBlockIndex)
  {
    std::ptrdiff_t diff = pObject - m_Blocks[uiBlockIndex].m_pData;
    if (diff >= 0 && diff < WDataBlock<T, BlockSize>::CAPACITY)
    {
      uiIndex = uiBlockIndex * WDataBlock<T, BlockSize>::CAPACITY + (WInt32)diff;
      break;
    }
  }

  W_ASSERT_DEV(uiIndex != WInvalidIndex, "Invalid object {0} was not found in block storage.", WArgP(pObject));

  m_UsedEntries.ClearBit(uiIndex);

  out_pMovedObject = pObject;
  WMemoryUtils::Destruct(pObject, 1);

  *reinterpret_cast<WUInt32*>(pObject) = m_uiFreelistStart;
  m_uiFreelistStart = uiIndex;
}
