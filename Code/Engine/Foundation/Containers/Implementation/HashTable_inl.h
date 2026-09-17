
/// Value used by containers for indices to indicate an invalid index.
#ifndef WInvalidIndex
#  define WInvalidIndex 0xFFFFFFFF
#endif

// ***** Const Iterator *****

template <typename K, typename V, typename H>
WHashTableBaseConstIterator<K, V, H>::WHashTableBaseConstIterator(const WHashTableBase<K, V, H>& hashTable)
  : m_pHashTable(&hashTable)
{
}

template <typename K, typename V, typename H>
void WHashTableBaseConstIterator<K, V, H>::SetToBegin()
{
  if (m_pHashTable->IsEmpty())
  {
    m_uiCurrentIndex = m_pHashTable->m_uiCapacity;
    return;
  }
  while (!m_pHashTable->IsValidEntry(m_uiCurrentIndex))
  {
    ++m_uiCurrentIndex;
  }
}

template <typename K, typename V, typename H>
inline void WHashTableBaseConstIterator<K, V, H>::SetToEnd()
{
  m_uiCurrentCount = m_pHashTable->m_uiCount;
  m_uiCurrentIndex = m_pHashTable->m_uiCapacity;
}


template <typename K, typename V, typename H>
W_FORCE_INLINE bool WHashTableBaseConstIterator<K, V, H>::IsValid() const
{
  return m_uiCurrentCount < m_pHashTable->m_uiCount;
}

template <typename K, typename V, typename H>
W_FORCE_INLINE bool WHashTableBaseConstIterator<K, V, H>::operator==(const WHashTableBaseConstIterator<K, V, H>& rhs) const
{
  return m_uiCurrentIndex == rhs.m_uiCurrentIndex && m_pHashTable->m_pEntries == rhs.m_pHashTable->m_pEntries;
}

template <typename K, typename V, typename H>
W_ALWAYS_INLINE const K& WHashTableBaseConstIterator<K, V, H>::Key() const
{
  return m_pHashTable->m_pEntries[m_uiCurrentIndex].key;
}

template <typename K, typename V, typename H>
W_ALWAYS_INLINE const V& WHashTableBaseConstIterator<K, V, H>::Value() const
{
  return m_pHashTable->m_pEntries[m_uiCurrentIndex].value;
}

template <typename K, typename V, typename H>
void WHashTableBaseConstIterator<K, V, H>::Next()
{
  // if we already iterated over the amount of valid elements that the hash-table stores, early out
  if (m_uiCurrentCount >= m_pHashTable->m_uiCount)
    return;

  // increase the counter of how many elements we have seen
  ++m_uiCurrentCount;
  // increase the index of the element to look at
  ++m_uiCurrentIndex;

  // check that we don't leave the valid range of element indices
  while (m_uiCurrentIndex < m_pHashTable->m_uiCapacity)
  {
    if (m_pHashTable->IsValidEntry(m_uiCurrentIndex))
      return;

    ++m_uiCurrentIndex;
  }

  // if we fell through this loop, we reached the end of all elements in the container
  // set the m_uiCurrentCount to maximum, to enable early-out in the future and to make 'IsValid' return 'false'
  m_uiCurrentCount = m_pHashTable->m_uiCount;
}

template <typename K, typename V, typename H>
W_ALWAYS_INLINE void WHashTableBaseConstIterator<K, V, H>::operator++()
{
  Next();
}

#if W_ENABLED(W_USE_CPP20_OPERATORS)
// These functions are used for structured bindings.
// They describe how many elements can be accessed in the binding and which type they are.
namespace std
{
  template <typename K, typename V, typename H>
  struct tuple_size<WHashTableBaseConstIterator<K, V, H>> : integral_constant<size_t, 2>
  {
  };

  template <typename K, typename V, typename H>
  struct tuple_element<0, WHashTableBaseConstIterator<K, V, H>>
  {
    using type = const K&;
  };

  template <typename K, typename V, typename H>
  struct tuple_element<1, WHashTableBaseConstIterator<K, V, H>>
  {
    using type = const V&;
  };
} // namespace std
#endif

// ***** Iterator *****

template <typename K, typename V, typename H>
WHashTableBaseIterator<K, V, H>::WHashTableBaseIterator(const WHashTableBase<K, V, H>& hashTable)
  : WHashTableBaseConstIterator<K, V, H>(hashTable)
{
}

template <typename K, typename V, typename H>
WHashTableBaseIterator<K, V, H>::WHashTableBaseIterator(const WHashTableBaseIterator<K, V, H>& rhs)
  : WHashTableBaseConstIterator<K, V, H>(*rhs.m_pHashTable)
{
  this->m_uiCurrentIndex = rhs.m_uiCurrentIndex;
  this->m_uiCurrentCount = rhs.m_uiCurrentCount;
}

template <typename K, typename V, typename H>
W_ALWAYS_INLINE void WHashTableBaseIterator<K, V, H>::operator=(const WHashTableBaseIterator& rhs) // [tested]
{
  this->m_pHashTable = rhs.m_pHashTable;
  this->m_uiCurrentIndex = rhs.m_uiCurrentIndex;
  this->m_uiCurrentCount = rhs.m_uiCurrentCount;
}

template <typename K, typename V, typename H>
W_FORCE_INLINE V& WHashTableBaseIterator<K, V, H>::Value()
{
  return this->m_pHashTable->m_pEntries[this->m_uiCurrentIndex].value;
}

template <typename K, typename V, typename H>
W_FORCE_INLINE V& WHashTableBaseIterator<K, V, H>::Value() const
{
  return this->m_pHashTable->m_pEntries[this->m_uiCurrentIndex].value;
}


#if W_ENABLED(W_USE_CPP20_OPERATORS)
// These functions are used for structured bindings.
// They describe how many elements can be accessed in the binding and which type they are.
namespace std
{
  template <typename K, typename V, typename H>
  struct tuple_size<WHashTableBaseIterator<K, V, H>> : integral_constant<size_t, 2>
  {
  };

  template <typename K, typename V, typename H>
  struct tuple_element<0, WHashTableBaseIterator<K, V, H>>
  {
    using type = const K&;
  };

  template <typename K, typename V, typename H>
  struct tuple_element<1, WHashTableBaseIterator<K, V, H>>
  {
    using type = V&;
  };
} // namespace std
#endif

// ***** WHashTableBase *****

template <typename K, typename V, typename H>
WHashTableBase<K, V, H>::WHashTableBase(WAllocator* pAllocator)
{
  m_pEntries = nullptr;
  m_pEntryFlags = nullptr;
  m_uiCount = 0;
  m_uiCapacity = 0;
  m_pAllocator = pAllocator;
}

template <typename K, typename V, typename H>
WHashTableBase<K, V, H>::WHashTableBase(const WHashTableBase<K, V, H>& other, WAllocator* pAllocator)
{
  m_pEntries = nullptr;
  m_pEntryFlags = nullptr;
  m_uiCount = 0;
  m_uiCapacity = 0;
  m_pAllocator = pAllocator;

  *this = other;
}

template <typename K, typename V, typename H>
WHashTableBase<K, V, H>::WHashTableBase(WHashTableBase<K, V, H>&& other, WAllocator* pAllocator)
{
  m_pEntries = nullptr;
  m_pEntryFlags = nullptr;
  m_uiCount = 0;
  m_uiCapacity = 0;
  m_pAllocator = pAllocator;

  *this = std::move(other);
}

template <typename K, typename V, typename H>
WHashTableBase<K, V, H>::~WHashTableBase()
{
  Clear();
  W_DELETE_RAW_BUFFER(m_pAllocator, m_pEntries);
  W_DELETE_RAW_BUFFER(m_pAllocator, m_pEntryFlags);
  m_uiCapacity = 0;
}

template <typename K, typename V, typename H>
void WHashTableBase<K, V, H>::operator=(const WHashTableBase<K, V, H>& rhs)
{
  Clear();
  Reserve(rhs.GetCount());

  WUInt32 uiCopied = 0;
  for (WUInt32 i = 0; uiCopied < rhs.GetCount(); ++i)
  {
    if (rhs.IsValidEntry(i))
    {
      Insert(rhs.m_pEntries[i].key, rhs.m_pEntries[i].value);
      ++uiCopied;
    }
  }
}

template <typename K, typename V, typename H>
void WHashTableBase<K, V, H>::operator=(WHashTableBase<K, V, H>&& rhs)
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
        Insert(std::move(rhs.m_pEntries[i].key), std::move(rhs.m_pEntries[i].value));
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

template <typename K, typename V, typename H>
bool WHashTableBase<K, V, H>::operator==(const WHashTableBase<K, V, H>& rhs) const
{
  if (m_uiCount != rhs.m_uiCount)
    return false;

  WUInt32 uiCompared = 0;
  for (WUInt32 i = 0; uiCompared < m_uiCount; ++i)
  {
    if (IsValidEntry(i))
    {
      const V* pRhsValue = nullptr;
      if (!rhs.TryGetValue(m_pEntries[i].key, pRhsValue))
        return false;

      if (m_pEntries[i].value != *pRhsValue)
        return false;

      ++uiCompared;
    }
  }

  return true;
}

template <typename K, typename V, typename H>
void WHashTableBase<K, V, H>::Reserve(WUInt32 uiCapacity)
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

template <typename K, typename V, typename H>
void WHashTableBase<K, V, H>::Compact()
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
    const WUInt32 uiNewCapacity = WMath::PowerOfTwo_Ceil(m_uiCount + (CAPACITY_ALIGNMENT - 1)) & ~(CAPACITY_ALIGNMENT - 1);
    if (m_uiCapacity != uiNewCapacity)
      SetCapacity(uiNewCapacity);
  }
}

template <typename K, typename V, typename H>
W_ALWAYS_INLINE WUInt32 WHashTableBase<K, V, H>::GetCount() const
{
  return m_uiCount;
}

template <typename K, typename V, typename H>
W_ALWAYS_INLINE bool WHashTableBase<K, V, H>::IsEmpty() const
{
  return m_uiCount == 0;
}

template <typename K, typename V, typename H>
void WHashTableBase<K, V, H>::Clear()
{
  for (WUInt32 i = 0; i < m_uiCapacity; ++i)
  {
    if (IsValidEntry(i))
    {
      WMemoryUtils::Destruct(&m_pEntries[i].key, 1);
      WMemoryUtils::Destruct(&m_pEntries[i].value, 1);
    }
  }

  WMemoryUtils::ZeroFill(m_pEntryFlags, GetFlagsCapacity());
  m_uiCount = 0;
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType, typename CompatibleValueType>
bool WHashTableBase<K, V, H>::Insert(CompatibleKeyType&& key, CompatibleValueType&& value, V* out_pOldValue /*= nullptr*/)
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
    else if (H::Equal(m_pEntries[uiIndex].key, key))
    {
      if (out_pOldValue != nullptr)
        *out_pOldValue = std::move(m_pEntries[uiIndex].value);

      m_pEntries[uiIndex].value = std::forward<CompatibleValueType>(value); // Either move or copy assignment.
      return true;
    }
    ++uiIndex;
    if (uiIndex == m_uiCapacity)
      uiIndex = 0;

    ++uiCounter;
  }

  // new entry
  uiIndex = uiDeletedIndex != WInvalidIndex ? uiDeletedIndex : uiIndex;

  // Both constructions might either be a move or a copy.
  WMemoryUtils::CopyOrMoveConstruct(&m_pEntries[uiIndex].key, std::forward<CompatibleKeyType>(key));
  WMemoryUtils::CopyOrMoveConstruct(&m_pEntries[uiIndex].value, std::forward<CompatibleValueType>(value));

  MarkEntryAsValid(uiIndex);
  ++m_uiCount;

  return false;
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
bool WHashTableBase<K, V, H>::Remove(const CompatibleKeyType& key, V* out_pOldValue /*= nullptr*/)
{
  WUInt32 uiIndex = FindEntry(key);
  if (uiIndex != WInvalidIndex)
  {
    if (out_pOldValue != nullptr)
      *out_pOldValue = std::move(m_pEntries[uiIndex].value);

    RemoveInternal(uiIndex);
    return true;
  }

  return false;
}

template <typename K, typename V, typename H>
typename WHashTableBase<K, V, H>::Iterator WHashTableBase<K, V, H>::Remove(const typename WHashTableBase<K, V, H>::Iterator& pos)
{
  W_ASSERT_DEBUG(pos.m_pHashTable == this, "Iterator from wrong hashtable");
  Iterator it = pos;
  WUInt32 uiIndex = pos.m_uiCurrentIndex;
  ++it;
  --it.m_uiCurrentCount;
  RemoveInternal(uiIndex);
  return it;
}

template <typename K, typename V, typename H>
void WHashTableBase<K, V, H>::RemoveInternal(WUInt32 uiIndex)
{
  WMemoryUtils::Destruct(&m_pEntries[uiIndex].key, 1);
  WMemoryUtils::Destruct(&m_pEntries[uiIndex].value, 1);

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

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
inline bool WHashTableBase<K, V, H>::TryGetValue(const CompatibleKeyType& key, V& out_value) const
{
  WUInt32 uiIndex = FindEntry(key);
  if (uiIndex != WInvalidIndex)
  {
    W_ASSERT_DEBUG(m_pEntries != nullptr, "No entries present"); // To fix static analysis
    out_value = m_pEntries[uiIndex].value;
    return true;
  }

  return false;
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
inline bool WHashTableBase<K, V, H>::TryGetValue(const CompatibleKeyType& key, const V*& out_pValue) const
{
  WUInt32 uiIndex = FindEntry(key);
  if (uiIndex != WInvalidIndex)
  {
    out_pValue = &m_pEntries[uiIndex].value;
    W_ANALYSIS_ASSUME(out_pValue != nullptr);
    return true;
  }

  return false;
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
inline bool WHashTableBase<K, V, H>::TryGetValue(const CompatibleKeyType& key, V*& out_pValue) const
{
  WUInt32 uiIndex = FindEntry(key);
  if (uiIndex != WInvalidIndex)
  {
    out_pValue = &m_pEntries[uiIndex].value;
    W_ANALYSIS_ASSUME(out_pValue != nullptr);
    return true;
  }

  return false;
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
inline typename WHashTableBase<K, V, H>::ConstIterator WHashTableBase<K, V, H>::Find(const CompatibleKeyType& key) const
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

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
inline typename WHashTableBase<K, V, H>::Iterator WHashTableBase<K, V, H>::Find(const CompatibleKeyType& key)
{
  WUInt32 uiIndex = FindEntry(key);
  if (uiIndex == WInvalidIndex)
  {
    return GetEndIterator();
  }

  Iterator it(*this);
  it.m_uiCurrentIndex = uiIndex;
  it.m_uiCurrentCount = 0; // we do not know the 'count' (which is used as an optimization), so we just use 0
  return it;
}


template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
inline const V* WHashTableBase<K, V, H>::GetValue(const CompatibleKeyType& key) const
{
  WUInt32 uiIndex = FindEntry(key);
  return (uiIndex != WInvalidIndex) ? &m_pEntries[uiIndex].value : nullptr;
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
inline V* WHashTableBase<K, V, H>::GetValue(const CompatibleKeyType& key)
{
  WUInt32 uiIndex = FindEntry(key);
  return (uiIndex != WInvalidIndex) ? &m_pEntries[uiIndex].value : nullptr;
}

template <typename K, typename V, typename H>
inline V& WHashTableBase<K, V, H>::operator[](const K& key)
{
  return FindOrAdd(key, nullptr);
}

template <typename K, typename V, typename H>
V& WHashTableBase<K, V, H>::FindOrAdd(const K& key, bool* out_pExisted)
{
  const WUInt32 uiHash = H::Hash(key);
  WUInt32 uiIndex = FindEntry(uiHash, key);

  if (out_pExisted)
  {
    *out_pExisted = uiIndex != WInvalidIndex;
  }

  if (uiIndex == WInvalidIndex)
  {
    Reserve(m_uiCount + 1);

    // search for suitable insertion index again, table might have been resized
    uiIndex = uiHash & (m_uiCapacity - 1);
    while (IsValidEntry(uiIndex))
    {
      ++uiIndex;
      if (uiIndex == m_uiCapacity)
        uiIndex = 0;
    }

    // new entry
    WMemoryUtils::CopyConstruct(&m_pEntries[uiIndex].key, key, 1);
    WMemoryUtils::Construct<ConstructAll>(&m_pEntries[uiIndex].value, 1);
    MarkEntryAsValid(uiIndex);
    ++m_uiCount;
  }

  W_ASSERT_DEBUG(m_pEntries != nullptr, "Entries should be present");
  return m_pEntries[uiIndex].value;
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
W_FORCE_INLINE bool WHashTableBase<K, V, H>::Contains(const CompatibleKeyType& key) const
{
  return FindEntry(key) != WInvalidIndex;
}

template <typename K, typename V, typename H>
W_ALWAYS_INLINE typename WHashTableBase<K, V, H>::Iterator WHashTableBase<K, V, H>::GetIterator()
{
  Iterator iterator(*this);
  iterator.SetToBegin();
  return iterator;
}

template <typename K, typename V, typename H>
W_ALWAYS_INLINE typename WHashTableBase<K, V, H>::Iterator WHashTableBase<K, V, H>::GetEndIterator()
{
  Iterator iterator(*this);
  iterator.SetToEnd();
  return iterator;
}

template <typename K, typename V, typename H>
W_ALWAYS_INLINE typename WHashTableBase<K, V, H>::ConstIterator WHashTableBase<K, V, H>::GetIterator() const
{
  ConstIterator iterator(*this);
  iterator.SetToBegin();
  return iterator;
}

template <typename K, typename V, typename H>
W_ALWAYS_INLINE typename WHashTableBase<K, V, H>::ConstIterator WHashTableBase<K, V, H>::GetEndIterator() const
{
  ConstIterator iterator(*this);
  iterator.SetToEnd();
  return iterator;
}

template <typename K, typename V, typename H>
W_ALWAYS_INLINE WAllocator* WHashTableBase<K, V, H>::GetAllocator() const
{
  return m_pAllocator;
}

template <typename K, typename V, typename H>
WUInt64 WHashTableBase<K, V, H>::GetHeapMemoryUsage() const
{
  return ((WUInt64)m_uiCapacity * sizeof(Entry)) + (sizeof(WUInt32) * (WUInt64)GetFlagsCapacity());
}

// private methods
template <typename K, typename V, typename H>
void WHashTableBase<K, V, H>::SetCapacity(WUInt32 uiCapacity)
{
  W_ASSERT_DEBUG(WMath::IsPowerOf2(uiCapacity), "uiCapacity must be a power of two to avoid modulo during lookup.");
  const WUInt32 uiOldCapacity = m_uiCapacity;
  m_uiCapacity = uiCapacity;

  Entry* pOldEntries = m_pEntries;
  WUInt32* pOldEntryFlags = m_pEntryFlags;

  m_pEntries = W_NEW_RAW_BUFFER(m_pAllocator, Entry, m_uiCapacity);
  m_pEntryFlags = W_NEW_RAW_BUFFER(m_pAllocator, WUInt32, GetFlagsCapacity());
  WMemoryUtils::ZeroFill(m_pEntryFlags, GetFlagsCapacity());

  m_uiCount = 0;
  for (WUInt32 i = 0; i < uiOldCapacity; ++i)
  {
    if (GetFlags(pOldEntryFlags, i) == VALID_ENTRY)
    {
      W_VERIFY(!Insert(std::move(pOldEntries[i].key), std::move(pOldEntries[i].value)), "Implementation error");

      WMemoryUtils::Destruct(&pOldEntries[i].key, 1);
      WMemoryUtils::Destruct(&pOldEntries[i].value, 1);
    }
  }

  W_DELETE_RAW_BUFFER(m_pAllocator, pOldEntries);
  W_DELETE_RAW_BUFFER(m_pAllocator, pOldEntryFlags);
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
W_ALWAYS_INLINE WUInt32 WHashTableBase<K, V, H>::FindEntry(const CompatibleKeyType& key) const
{
  return FindEntry(H::Hash(key), key);
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
inline WUInt32 WHashTableBase<K, V, H>::FindEntry(WUInt32 uiHash, const CompatibleKeyType& key) const
{
  if (m_uiCapacity > 0)
  {
    WUInt32 uiIndex = uiHash & (m_uiCapacity - 1);
    WUInt32 uiCounter = 0;
    while (!IsFreeEntry(uiIndex) && uiCounter < m_uiCapacity)
    {
      if (IsValidEntry(uiIndex) && H::Equal(m_pEntries[uiIndex].key, key))
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

#define W_HASHTABLE_USE_BITFLAGS W_ON

template <typename K, typename V, typename H>
W_FORCE_INLINE WUInt32 WHashTableBase<K, V, H>::GetFlagsCapacity() const
{
#if W_ENABLED(W_HASHTABLE_USE_BITFLAGS)
  return (m_uiCapacity + 15) / 16;
#else
  return m_uiCapacity;
#endif
}

template <typename K, typename V, typename H>
W_ALWAYS_INLINE WUInt32 WHashTableBase<K, V, H>::GetFlags(WUInt32* pFlags, WUInt32 uiEntryIndex) const
{
#if W_ENABLED(W_HASHTABLE_USE_BITFLAGS)
  const WUInt32 uiIndex = uiEntryIndex / 16;
  const WUInt32 uiSubIndex = (uiEntryIndex & 15) * 2;
  return (pFlags[uiIndex] >> uiSubIndex) & FLAGS_MASK;
#else
  return pFlags[uiEntryIndex] & FLAGS_MASK;
#endif
}

template <typename K, typename V, typename H>
void WHashTableBase<K, V, H>::SetFlags(WUInt32 uiEntryIndex, WUInt32 uiFlags)
{
#if W_ENABLED(W_HASHTABLE_USE_BITFLAGS)
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

template <typename K, typename V, typename H>
W_FORCE_INLINE bool WHashTableBase<K, V, H>::IsFreeEntry(WUInt32 uiEntryIndex) const
{
  return GetFlags(m_pEntryFlags, uiEntryIndex) == FREE_ENTRY;
}

template <typename K, typename V, typename H>
W_FORCE_INLINE bool WHashTableBase<K, V, H>::IsValidEntry(WUInt32 uiEntryIndex) const
{
  return GetFlags(m_pEntryFlags, uiEntryIndex) == VALID_ENTRY;
}

template <typename K, typename V, typename H>
W_FORCE_INLINE bool WHashTableBase<K, V, H>::IsDeletedEntry(WUInt32 uiEntryIndex) const
{
  return GetFlags(m_pEntryFlags, uiEntryIndex) == DELETED_ENTRY;
}

template <typename K, typename V, typename H>
W_FORCE_INLINE void WHashTableBase<K, V, H>::MarkEntryAsFree(WUInt32 uiEntryIndex)
{
  SetFlags(uiEntryIndex, FREE_ENTRY);
}

template <typename K, typename V, typename H>
W_FORCE_INLINE void WHashTableBase<K, V, H>::MarkEntryAsValid(WUInt32 uiEntryIndex)
{
  SetFlags(uiEntryIndex, VALID_ENTRY);
}

template <typename K, typename V, typename H>
W_FORCE_INLINE void WHashTableBase<K, V, H>::MarkEntryAsDeleted(WUInt32 uiEntryIndex)
{
  SetFlags(uiEntryIndex, DELETED_ENTRY);
}


template <typename K, typename V, typename H, typename A>
WHashTable<K, V, H, A>::WHashTable()
  : WHashTableBase<K, V, H>(A::GetAllocator())
{
}

template <typename K, typename V, typename H, typename A>
WHashTable<K, V, H, A>::WHashTable(WAllocator* pAllocator)
  : WHashTableBase<K, V, H>(pAllocator)
{
}

template <typename K, typename V, typename H, typename A>
WHashTable<K, V, H, A>::WHashTable(const WHashTable<K, V, H, A>& other)
  : WHashTableBase<K, V, H>(other, A::GetAllocator())
{
}

template <typename K, typename V, typename H, typename A>
WHashTable<K, V, H, A>::WHashTable(const WHashTableBase<K, V, H>& other)
  : WHashTableBase<K, V, H>(other, A::GetAllocator())
{
}

template <typename K, typename V, typename H, typename A>
WHashTable<K, V, H, A>::WHashTable(WHashTable<K, V, H, A>&& other)
  : WHashTableBase<K, V, H>(std::move(other), other.GetAllocator())
{
}

template <typename K, typename V, typename H, typename A>
WHashTable<K, V, H, A>::WHashTable(WHashTableBase<K, V, H>&& other)
  : WHashTableBase<K, V, H>(std::move(other), other.GetAllocator())
{
}

template <typename K, typename V, typename H, typename A>
void WHashTable<K, V, H, A>::operator=(const WHashTable<K, V, H, A>& rhs)
{
  WHashTableBase<K, V, H>::operator=(rhs);
}

template <typename K, typename V, typename H, typename A>
void WHashTable<K, V, H, A>::operator=(const WHashTableBase<K, V, H>& rhs)
{
  WHashTableBase<K, V, H>::operator=(rhs);
}

template <typename K, typename V, typename H, typename A>
void WHashTable<K, V, H, A>::operator=(WHashTable<K, V, H, A>&& rhs)
{
  WHashTableBase<K, V, H>::operator=(std::move(rhs));
}

template <typename K, typename V, typename H, typename A>
void WHashTable<K, V, H, A>::operator=(WHashTableBase<K, V, H>&& rhs)
{
  WHashTableBase<K, V, H>::operator=(std::move(rhs));
}

template <typename KeyType, typename ValueType, typename Hasher>
void WHashTableBase<KeyType, ValueType, Hasher>::Swap(WHashTableBase<KeyType, ValueType, Hasher>& other)
{
  WMath::Swap(this->m_pEntries, other.m_pEntries);
  WMath::Swap(this->m_pEntryFlags, other.m_pEntryFlags);
  WMath::Swap(this->m_uiCount, other.m_uiCount);
  WMath::Swap(this->m_uiCapacity, other.m_uiCapacity);
  WMath::Swap(this->m_pAllocator, other.m_pAllocator);
}
