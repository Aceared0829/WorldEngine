
/// Value used by containers for indices to indicate an invalid index.
#ifndef WInvalidIndex
#  define WInvalidIndex 0xFFFFFFFF
#endif

// ***** Const Iterator *****

template <typename K, typename H>
WHashSetBase<K, H>::ConstIterator::ConstIterator(const WHashSetBase<K, H>& hashSet)
  : m_pHashSet(&hashSet)
{
}

template <typename K, typename H>
void WHashSetBase<K, H>::ConstIterator::SetToBegin()
{
  if (m_pHashSet->IsEmpty())
  {
    m_uiCurrentIndex = m_pHashSet->m_uiCapacity;
    return;
  }
  while (!m_pHashSet->IsValidEntry(m_uiCurrentIndex))
  {
    ++m_uiCurrentIndex;
  }
}

template <typename K, typename H>
inline void WHashSetBase<K, H>::ConstIterator::SetToEnd()
{
  m_uiCurrentCount = m_pHashSet->m_uiCount;
  m_uiCurrentIndex = m_pHashSet->m_uiCapacity;
}

template <typename K, typename H>
W_ALWAYS_INLINE bool WHashSetBase<K, H>::ConstIterator::IsValid() const
{
  return m_uiCurrentCount < m_pHashSet->m_uiCount;
}

template <typename K, typename H>
W_ALWAYS_INLINE bool WHashSetBase<K, H>::ConstIterator::operator==(const typename WHashSetBase<K, H>::ConstIterator& rhs) const
{
  return m_uiCurrentIndex == rhs.m_uiCurrentIndex && m_pHashSet->m_pEntries == rhs.m_pHashSet->m_pEntries;
}

template <typename K, typename H>
W_FORCE_INLINE const K& WHashSetBase<K, H>::ConstIterator::Key() const
{
  return m_pHashSet->m_pEntries[m_uiCurrentIndex];
}

template <typename K, typename H>
void WHashSetBase<K, H>::ConstIterator::Next()
{
  ++m_uiCurrentCount;
  if (m_uiCurrentCount == m_pHashSet->m_uiCount)
  {
    m_uiCurrentIndex = m_pHashSet->m_uiCapacity;
    return;
  }

  for (++m_uiCurrentIndex; m_uiCurrentIndex < m_pHashSet->m_uiCapacity; ++m_uiCurrentIndex)
  {
    if (m_pHashSet->IsValidEntry(m_uiCurrentIndex))
    {
      return;
    }
  }
  SetToEnd();
}

template <typename K, typename H>
W_ALWAYS_INLINE void WHashSetBase<K, H>::ConstIterator::operator++()
{
  Next();
}


// ***** WHashSetBase *****

template <typename K, typename H>
WHashSetBase<K, H>::WHashSetBase(WAllocator* pAllocator)
{
  m_pEntries = nullptr;
  m_pEntryFlags = nullptr;
  m_uiCount = 0;
  m_uiCapacity = 0;
  m_pAllocator = pAllocator;
}

template <typename K, typename H>
WHashSetBase<K, H>::WHashSetBase(const WHashSetBase<K, H>& other, WAllocator* pAllocator)
{
  m_pEntries = nullptr;
  m_pEntryFlags = nullptr;
  m_uiCount = 0;
  m_uiCapacity = 0;
  m_pAllocator = pAllocator;

  *this = other;
}

template <typename K, typename H>
WHashSetBase<K, H>::WHashSetBase(WHashSetBase<K, H>&& other, WAllocator* pAllocator)
{
  m_pEntries = nullptr;
  m_pEntryFlags = nullptr;
  m_uiCount = 0;
  m_uiCapacity = 0;
  m_pAllocator = pAllocator;

  *this = std::move(other);
}

template <typename K, typename H>
WHashSetBase<K, H>::~WHashSetBase()
{
  Clear();
  W_DELETE_RAW_BUFFER(m_pAllocator, m_pEntries);
  W_DELETE_RAW_BUFFER(m_pAllocator, m_pEntryFlags);
  m_uiCapacity = 0;
}

template <typename K, typename H>
void WHashSetBase<K, H>::operator=(const WHashSetBase<K, H>& rhs)
{
  Clear();
  Reserve(rhs.GetCount());

  WUInt32 uiCopied = 0;
  for (WUInt32 i = 0; uiCopied < rhs.GetCount(); ++i)
  {
    if (rhs.IsValidEntry(i))
    {
      Insert(rhs.m_pEntries[i]);
      ++uiCopied;
    }
  }
}

template <typename K, typename H>
void WHashSetBase<K, H>::operator=(WHashSetBase<K, H>&& rhs)
{
  // Clear any existing data (calls destructors if necessary)
  Clear();

  if (m_pAllocator != rhs.m_pAllocator)
  {
    Reserve(rhs.m_uiCapacity);

    WUInt32 uiCopied = 0;
    for (WUInt32 i = 0; uiCopied < rhs.GetCount(); ++i)
    {
      if (rhs.IsValidEntry(i))
      {
        Insert(std::move(rhs.m_pEntries[i]));
        ++uiCopied;
      }
    }

    rhs.Clear();
  }
  else
  {
    W_DELETE_RAW_BUFFER(m_pAllocator, m_pEntries);
    W_DELETE_RAW_BUFFER(m_pAllocator, m_pEntryFlags);

    // Move all data over.
    m_pEntries = rhs.m_pEntries;
    m_pEntryFlags = rhs.m_pEntryFlags;
    m_uiCount = rhs.m_uiCount;
    m_uiCapacity = rhs.m_uiCapacity;

    // Temp copy forgets all its state.
    rhs.m_pEntries = nullptr;
    rhs.m_pEntryFlags = nullptr;
    rhs.m_uiCount = 0;
    rhs.m_uiCapacity = 0;
  }
}

template <typename K, typename H>
bool WHashSetBase<K, H>::operator==(const WHashSetBase<K, H>& rhs) const
{
  if (m_uiCount != rhs.m_uiCount)
    return false;

  WUInt32 uiCompared = 0;
  for (WUInt32 i = 0; uiCompared < m_uiCount; ++i)
  {
    if (IsValidEntry(i))
    {
      if (!rhs.Contains(m_pEntries[i]))
        return false;

      ++uiCompared;
    }
  }

  return true;
}

template <typename K, typename H>
void WHashSetBase<K, H>::Reserve(WUInt32 uiCapacity)
{
  const WUInt64 uiCap64 = static_cast<WUInt64>(uiCapacity);
  WUInt64 uiNewCapacity64 = uiCap64 + (uiCap64 * 2 / 3);                  // ensure a maximum load of 60%

  uiNewCapacity64 = WMath::Min<WUInt64>(uiNewCapacity64, 0x80000000llu); // the largest power-of-two in 32 bit

  WUInt32 uiNewCapacity32 = static_cast<WUInt32>(uiNewCapacity64 & 0xFFFFFFFF);
  W_ASSERT_DEBUG(uiCapacity <= uiNewCapacity32, "WHashSet/Map do not support more than 2 billion entries.");

  if (m_uiCapacity >= uiNewCapacity32)
    return;

  uiNewCapacity32 = WMath::Max<WUInt32>(WMath::PowerOfTwo_Ceil(uiNewCapacity32), CAPACITY_ALIGNMENT);
  SetCapacity(uiNewCapacity32);
}

template <typename K, typename H>
void WHashSetBase<K, H>::Compact()
{
  if (IsEmpty())
  {
    // completely deallocate all data, if the table is empty.
    W_DELETE_RAW_BUFFER(m_pAllocator, m_pEntries);
    W_DELETE_RAW_BUFFER(m_pAllocator, m_pEntryFlags);
    m_uiCapacity = 0;
  }
  else
  {
    const WUInt32 uiNewCapacity = (m_uiCount + (CAPACITY_ALIGNMENT - 1)) & ~(CAPACITY_ALIGNMENT - 1);
    if (m_uiCapacity != uiNewCapacity)
      SetCapacity(uiNewCapacity);
  }
}

template <typename K, typename H>
W_ALWAYS_INLINE WUInt32 WHashSetBase<K, H>::GetCount() const
{
  return m_uiCount;
}

template <typename K, typename H>
W_ALWAYS_INLINE bool WHashSetBase<K, H>::IsEmpty() const
{
  return m_uiCount == 0;
}

template <typename K, typename H>
void WHashSetBase<K, H>::Clear()
{
  for (WUInt32 i = 0; i < m_uiCapacity; ++i)
  {
    if (IsValidEntry(i))
    {
      WMemoryUtils::Destruct(&m_pEntries[i], 1);
    }
  }

  WMemoryUtils::ZeroFill(m_pEntryFlags, GetFlagsCapacity());
  m_uiCount = 0;
}

template <typename K, typename H>
template <typename CompatibleKeyType>
bool WHashSetBase<K, H>::Insert(CompatibleKeyType&& key)
{
  Reserve(m_uiCount + 1);

  WUInt32 uiIndex = H::Hash(key) & (m_uiCapacity - 1);
  WUInt32 uiDeletedIndex = WInvalidIndex;

  WUInt32 uiCounter = 0;
  while (!IsFreeEntry(uiIndex) && uiCounter < m_uiCapacity)
  {
    if (IsDeletedEntry(uiIndex))
    {
      if (uiDeletedIndex == WInvalidIndex)
        uiDeletedIndex = uiIndex;
    }
    else if (H::Equal(m_pEntries[uiIndex], key))
    {
      return true;
    }
    ++uiIndex;
    if (uiIndex == m_uiCapacity)
      uiIndex = 0;

    ++uiCounter;
  }

  // new entry
  uiIndex = uiDeletedIndex != WInvalidIndex ? uiDeletedIndex : uiIndex;

  // Constructions might either be a move or a copy.
  WMemoryUtils::CopyOrMoveConstruct(&m_pEntries[uiIndex], std::forward<CompatibleKeyType>(key));

  MarkEntryAsValid(uiIndex);
  ++m_uiCount;

  return false;
}

template <typename K, typename H>
template <typename CompatibleKeyType>
bool WHashSetBase<K, H>::Remove(const CompatibleKeyType& key)
{
  WUInt32 uiIndex = FindEntry(key);
  if (uiIndex != WInvalidIndex)
  {
    RemoveInternal(uiIndex);
    return true;
  }

  return false;
}

template <typename K, typename H>
typename WHashSetBase<K, H>::ConstIterator WHashSetBase<K, H>::Remove(const typename WHashSetBase<K, H>::ConstIterator& pos)
{
  ConstIterator it = pos;
  WUInt32 uiIndex = pos.m_uiCurrentIndex;
  ++it;
  --it.m_uiCurrentCount;
  RemoveInternal(uiIndex);
  return it;
}

template <typename K, typename H>
void WHashSetBase<K, H>::RemoveInternal(WUInt32 uiIndex)
{
  WMemoryUtils::Destruct(&m_pEntries[uiIndex], 1);

  WUInt32 uiNextIndex = uiIndex + 1;
  if (uiNextIndex == m_uiCapacity)
    uiNextIndex = 0;

  // if the next entry is free we are at the end of a chain and
  // can immediately mark this entry as free as well
  if (IsFreeEntry(uiNextIndex))
  {
    MarkEntryAsFree(uiIndex);

    // run backwards and free all deleted entries in this chain
    WUInt32 uiPrevIndex = (uiIndex != 0) ? uiIndex : m_uiCapacity;
    --uiPrevIndex;

    while (IsDeletedEntry(uiPrevIndex))
    {
      MarkEntryAsFree(uiPrevIndex);

      if (uiPrevIndex == 0)
        uiPrevIndex = m_uiCapacity;
      --uiPrevIndex;
    }
  }
  else
  {
    MarkEntryAsDeleted(uiIndex);
  }

  --m_uiCount;
}

template <typename K, typename H>
template <typename CompatibleKeyType>
W_FORCE_INLINE bool WHashSetBase<K, H>::Contains(const CompatibleKeyType& key) const
{
  return FindEntry(key) != WInvalidIndex;
}

template <typename K, typename H>
template <typename CompatibleKeyType>
W_FORCE_INLINE typename WHashSetBase<K, H>::ConstIterator WHashSetBase<K, H>::Find(const CompatibleKeyType& key) const
{
  WUInt32 uiIndex = FindEntry(key);
  if (uiIndex == WInvalidIndex)
  {
    return GetEndIterator();
  }

  ConstIterator it(*this);
  it.m_uiCurrentIndex = uiIndex;
  it.m_uiCurrentCount = 0; // we do not know the 'count' (which is used as an optimization), so we just use 0

  return it;
}

template <typename K, typename H>
bool WHashSetBase<K, H>::ContainsSet(const WHashSetBase<K, H>& operand) const
{
  for (const K& key : operand)
  {
    if (!Contains(key))
      return false;
  }

  return true;
}

template <typename K, typename H>
void WHashSetBase<K, H>::Union(const WHashSetBase<K, H>& operand)
{
  Reserve(GetCount() + operand.GetCount());
  for (const auto& key : operand)
  {
    Insert(key);
  }
}

template <typename K, typename H>
void WHashSetBase<K, H>::Difference(const WHashSetBase<K, H>& operand)
{
  for (const auto& key : operand)
  {
    Remove(key);
  }
}

template <typename K, typename H>
void WHashSetBase<K, H>::Intersection(const WHashSetBase<K, H>& operand)
{
  for (auto it = GetIterator(); it.IsValid();)
  {
    if (!operand.Contains(it.Key()))
      it = Remove(it);
    else
      ++it;
  }
}

template <typename K, typename H>
W_FORCE_INLINE typename WHashSetBase<K, H>::ConstIterator WHashSetBase<K, H>::GetIterator() const
{
  ConstIterator iterator(*this);
  iterator.SetToBegin();
  return iterator;
}

template <typename K, typename H>
W_FORCE_INLINE typename WHashSetBase<K, H>::ConstIterator WHashSetBase<K, H>::GetEndIterator() const
{
  ConstIterator iterator(*this);
  iterator.SetToEnd();
  return iterator;
}

template <typename K, typename H>
W_ALWAYS_INLINE WAllocator* WHashSetBase<K, H>::GetAllocator() const
{
  return m_pAllocator;
}

template <typename K, typename H>
WUInt64 WHashSetBase<K, H>::GetHeapMemoryUsage() const
{
  return ((WUInt64)m_uiCapacity * sizeof(K)) + (sizeof(WUInt32) * (WUInt64)GetFlagsCapacity());
}

// private methods
template <typename K, typename H>
void WHashSetBase<K, H>::SetCapacity(WUInt32 uiCapacity)
{
  W_ASSERT_DEBUG(WMath::IsPowerOf2(uiCapacity), "uiCapacity must be a power of two to avoid modulo during lookup.");
  const WUInt32 uiOldCapacity = m_uiCapacity;
  m_uiCapacity = uiCapacity;

  K* pOldEntries = m_pEntries;
  WUInt32* pOldEntryFlags = m_pEntryFlags;

  m_pEntries = W_NEW_RAW_BUFFER(m_pAllocator, K, m_uiCapacity);
  m_pEntryFlags = W_NEW_RAW_BUFFER(m_pAllocator, WUInt32, GetFlagsCapacity());
  WMemoryUtils::ZeroFill(m_pEntryFlags, GetFlagsCapacity());

  m_uiCount = 0;
  for (WUInt32 i = 0; i < uiOldCapacity; ++i)
  {
    if (GetFlags(pOldEntryFlags, i) == VALID_ENTRY)
    {
      W_VERIFY(!Insert(std::move(pOldEntries[i])), "Implementation error");

      WMemoryUtils::Destruct(&pOldEntries[i], 1);
    }
  }

  W_DELETE_RAW_BUFFER(m_pAllocator, pOldEntries);
  W_DELETE_RAW_BUFFER(m_pAllocator, pOldEntryFlags);
}

template <typename K, typename H>
template <typename CompatibleKeyType>
W_FORCE_INLINE WUInt32 WHashSetBase<K, H>::FindEntry(const CompatibleKeyType& key) const
{
  return FindEntry(H::Hash(key), key);
}

template <typename K, typename H>
template <typename CompatibleKeyType>
inline WUInt32 WHashSetBase<K, H>::FindEntry(WUInt32 uiHash, const CompatibleKeyType& key) const
{
  if (m_uiCapacity > 0)
  {
    WUInt32 uiIndex = uiHash & (m_uiCapacity - 1);
    WUInt32 uiCounter = 0;
    while (!IsFreeEntry(uiIndex) && uiCounter < m_uiCapacity)
    {
      if (IsValidEntry(uiIndex) && H::Equal(m_pEntries[uiIndex], key))
        return uiIndex;

      ++uiIndex;
      if (uiIndex == m_uiCapacity)
        uiIndex = 0;

      ++uiCounter;
    }
  }
  // not found
  return WInvalidIndex;
}

#define W_HASHSET_USE_BITFLAGS W_ON

template <typename K, typename H>
W_FORCE_INLINE WUInt32 WHashSetBase<K, H>::GetFlagsCapacity() const
{
#if W_ENABLED(W_HASHSET_USE_BITFLAGS)
  return (m_uiCapacity + 15) / 16;
#else
  return m_uiCapacity;
#endif
}

template <typename K, typename H>
WUInt32 WHashSetBase<K, H>::GetFlags(WUInt32* pFlags, WUInt32 uiEntryIndex) const
{
#if W_ENABLED(W_HASHSET_USE_BITFLAGS)
  const WUInt32 uiIndex = uiEntryIndex / 16;
  const WUInt32 uiSubIndex = (uiEntryIndex & 15) * 2;
  return (pFlags[uiIndex] >> uiSubIndex) & FLAGS_MASK;
#else
  return pFlags[uiEntryIndex] & FLAGS_MASK;
#endif
}

template <typename K, typename H>
void WHashSetBase<K, H>::SetFlags(WUInt32 uiEntryIndex, WUInt32 uiFlags)
{
#if W_ENABLED(W_HASHSET_USE_BITFLAGS)
  const WUInt32 uiIndex = uiEntryIndex / 16;
  const WUInt32 uiSubIndex = (uiEntryIndex & 15) * 2;
  W_ASSERT_DEBUG(uiIndex < GetFlagsCapacity(), "Out of bounds access");
  m_pEntryFlags[uiIndex] &= ~(FLAGS_MASK << uiSubIndex);
  m_pEntryFlags[uiIndex] |= (uiFlags << uiSubIndex);
#else
  W_ASSERT_DEBUG(uiEntryIndex < GetFlagsCapacity(), "Out of bounds access");
  m_pEntryFlags[uiEntryIndex] = uiFlags;
#endif
}

template <typename K, typename H>
W_FORCE_INLINE bool WHashSetBase<K, H>::IsFreeEntry(WUInt32 uiEntryIndex) const
{
  return GetFlags(m_pEntryFlags, uiEntryIndex) == FREE_ENTRY;
}

template <typename K, typename H>
W_FORCE_INLINE bool WHashSetBase<K, H>::IsValidEntry(WUInt32 uiEntryIndex) const
{
  W_ASSERT_DEBUG(uiEntryIndex < m_uiCapacity, "Out of bounds access");
  return GetFlags(m_pEntryFlags, uiEntryIndex) == VALID_ENTRY;
}

template <typename K, typename H>
W_FORCE_INLINE bool WHashSetBase<K, H>::IsDeletedEntry(WUInt32 uiEntryIndex) const
{
  return GetFlags(m_pEntryFlags, uiEntryIndex) == DELETED_ENTRY;
}

template <typename K, typename H>
W_FORCE_INLINE void WHashSetBase<K, H>::MarkEntryAsFree(WUInt32 uiEntryIndex)
{
  SetFlags(uiEntryIndex, FREE_ENTRY);
}

template <typename K, typename H>
W_FORCE_INLINE void WHashSetBase<K, H>::MarkEntryAsValid(WUInt32 uiEntryIndex)
{
  SetFlags(uiEntryIndex, VALID_ENTRY);
}

template <typename K, typename H>
W_FORCE_INLINE void WHashSetBase<K, H>::MarkEntryAsDeleted(WUInt32 uiEntryIndex)
{
  SetFlags(uiEntryIndex, DELETED_ENTRY);
}


template <typename K, typename H, typename A>
WHashSet<K, H, A>::WHashSet()
  : WHashSetBase<K, H>(A::GetAllocator())
{
}

template <typename K, typename H, typename A>
WHashSet<K, H, A>::WHashSet(WAllocator* pAllocator)
  : WHashSetBase<K, H>(pAllocator)
{
}

template <typename K, typename H, typename A>
WHashSet<K, H, A>::WHashSet(const WHashSet<K, H, A>& other)
  : WHashSetBase<K, H>(other, A::GetAllocator())
{
}

template <typename K, typename H, typename A>
WHashSet<K, H, A>::WHashSet(const WHashSetBase<K, H>& other)
  : WHashSetBase<K, H>(other, A::GetAllocator())
{
}

template <typename K, typename H, typename A>
WHashSet<K, H, A>::WHashSet(WHashSet<K, H, A>&& other)
  : WHashSetBase<K, H>(std::move(other), other.GetAllocator())
{
}

template <typename K, typename H, typename A>
WHashSet<K, H, A>::WHashSet(WHashSetBase<K, H>&& other)
  : WHashSetBase<K, H>(std::move(other), other.GetAllocator())
{
}

template <typename K, typename H, typename A>
void WHashSet<K, H, A>::operator=(const WHashSet<K, H, A>& rhs)
{
  WHashSetBase<K, H>::operator=(rhs);
}

template <typename K, typename H, typename A>
void WHashSet<K, H, A>::operator=(const WHashSetBase<K, H>& rhs)
{
  WHashSetBase<K, H>::operator=(rhs);
}

template <typename K, typename H, typename A>
void WHashSet<K, H, A>::operator=(WHashSet<K, H, A>&& rhs)
{
  WHashSetBase<K, H>::operator=(std::move(rhs));
}

template <typename K, typename H, typename A>
void WHashSet<K, H, A>::operator=(WHashSetBase<K, H>&& rhs)
{
  WHashSetBase<K, H>::operator=(std::move(rhs));
}

template <typename KeyType, typename Hasher>
void WHashSetBase<KeyType, Hasher>::Swap(WHashSetBase<KeyType, Hasher>& other)
{
  WMath::Swap(this->m_pEntries, other.m_pEntries);
  WMath::Swap(this->m_pEntryFlags, other.m_pEntryFlags);
  WMath::Swap(this->m_uiCount, other.m_uiCount);
  WMath::Swap(this->m_uiCapacity, other.m_uiCapacity);
  WMath::Swap(this->m_pAllocator, other.m_pAllocator);
}
