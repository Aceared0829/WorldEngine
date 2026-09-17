
// ***** Const Iterator *****

template <typename IdType, typename ValueType>
WIdTableBase<IdType, ValueType>::ConstIterator::ConstIterator(const WIdTableBase<IdType, ValueType>& idTable)
  : m_IdTable(idTable)
  , m_CurrentIndex(0)
  , m_CurrentCount(0)
{
  if (m_IdTable.IsEmpty())
    return;

  while (m_IdTable.m_pEntries[m_CurrentIndex].id.m_InstanceIndex != m_CurrentIndex)
  {
    ++m_CurrentIndex;
  }
}

template <typename IdType, typename ValueType>
W_ALWAYS_INLINE bool WIdTableBase<IdType, ValueType>::ConstIterator::IsValid() const
{
  return m_CurrentCount < m_IdTable.m_Count;
}

template <typename IdType, typename ValueType>
W_FORCE_INLINE bool WIdTableBase<IdType, ValueType>::ConstIterator::operator==(
  const typename WIdTableBase<IdType, ValueType>::ConstIterator& it2) const
{
  return m_IdTable.m_pEntries == it2.m_IdTable.m_pEntries && m_CurrentIndex == it2.m_CurrentIndex;
}

template <typename IdType, typename ValueType>
W_ALWAYS_INLINE bool WIdTableBase<IdType, ValueType>::ConstIterator::operator!=(
  const typename WIdTableBase<IdType, ValueType>::ConstIterator& it2) const
{
  return !(*this == it2);
}

template <typename IdType, typename ValueType>
W_ALWAYS_INLINE IdType WIdTableBase<IdType, ValueType>::ConstIterator::Id() const
{
  return m_IdTable.m_pEntries[m_CurrentIndex].id;
}

template <typename IdType, typename ValueType>
W_FORCE_INLINE const ValueType& WIdTableBase<IdType, ValueType>::ConstIterator::Value() const
{
  return m_IdTable.m_pEntries[m_CurrentIndex].value;
}

template <typename IdType, typename ValueType>
void WIdTableBase<IdType, ValueType>::ConstIterator::Next()
{
  ++m_CurrentCount;
  if (m_CurrentCount == m_IdTable.m_Count)
    return;

  do
  {
    ++m_CurrentIndex;
  } while (m_IdTable.m_pEntries[m_CurrentIndex].id.m_InstanceIndex != m_CurrentIndex);
}

template <typename IdType, typename ValueType>
W_ALWAYS_INLINE void WIdTableBase<IdType, ValueType>::ConstIterator::operator++()
{
  Next();
}


// ***** Iterator *****

template <typename IdType, typename ValueType>
WIdTableBase<IdType, ValueType>::Iterator::Iterator(const WIdTableBase<IdType, ValueType>& idTable)
  : ConstIterator(idTable)
{
}

template <typename IdType, typename ValueType>
W_ALWAYS_INLINE ValueType& WIdTableBase<IdType, ValueType>::Iterator::Value()
{
  return this->m_IdTable.m_pEntries[this->m_CurrentIndex].value;
}


// ***** WIdTableBase *****

template <typename IdType, typename ValueType>
WIdTableBase<IdType, ValueType>::WIdTableBase(WAllocator* pAllocator)
{
  m_pEntries = nullptr;
  m_Count = 0;
  m_Capacity = 0;
  m_FreelistEnqueue = IndexType(-1);
  m_FreelistDequeue = 0;
  m_pAllocator = pAllocator;
}

template <typename IdType, typename ValueType>
WIdTableBase<IdType, ValueType>::WIdTableBase(const WIdTableBase<IdType, ValueType>& other, WAllocator* pAllocator)
{
  m_pEntries = nullptr;
  m_Count = 0;
  m_Capacity = 0;
  m_FreelistEnqueue = IndexType(-1);
  m_FreelistDequeue = 0;
  m_pAllocator = pAllocator;

  *this = other;
}

template <typename IdType, typename ValueType>
WIdTableBase<IdType, ValueType>::~WIdTableBase()
{
  for (IndexType i = 0; i < m_Capacity; ++i)
  {
    if (m_pEntries[i].id.m_InstanceIndex == i)
    {
      WMemoryUtils::Destruct(&m_pEntries[i].value, 1);
    }
  }

  W_DELETE_RAW_BUFFER(m_pAllocator, m_pEntries);
  m_Capacity = 0;
}

template <typename IdType, typename ValueType>
void WIdTableBase<IdType, ValueType>::operator=(const WIdTableBase<IdType, ValueType>& rhs)
{
  Clear();
  Reserve(rhs.m_Capacity);

  for (IndexType i = 0; i < rhs.m_Capacity; ++i)
  {
    Entry& entry = m_pEntries[i];

    entry.id = rhs.m_pEntries[i].id;
    if (entry.id.m_InstanceIndex == i)
    {
      WMemoryUtils::CopyConstruct(&entry.value, rhs.m_pEntries[i].value, 1);
    }
  }

  m_Count = rhs.m_Count;
  m_FreelistDequeue = rhs.m_FreelistDequeue;
}

template <typename IdType, typename ValueType>
void WIdTableBase<IdType, ValueType>::Reserve(IndexType capacity)
{
  if (m_Capacity >= capacity + CAPACITY_ALIGNMENT)
    return;

  const WUInt64 uiCurCap64 = static_cast<WUInt64>(this->m_Capacity);
  WUInt64 uiNewCapacity64 = uiCurCap64 + (uiCurCap64 / 2);

  uiNewCapacity64 = WMath::Max<WUInt64>(uiNewCapacity64, capacity + CAPACITY_ALIGNMENT);

  // the maximum value must leave room for the capacity alignment computation below (without overflowing the 32 bit range)
  uiNewCapacity64 = WMath::Min<WUInt64>(uiNewCapacity64, 0xFFFFFFFFllu - (CAPACITY_ALIGNMENT - 1));

  uiNewCapacity64 = (uiNewCapacity64 + (CAPACITY_ALIGNMENT - 1)) & ~(CAPACITY_ALIGNMENT - 1);

  SetCapacity(static_cast<IndexType>(uiNewCapacity64 & 0xFFFFFFFF));
}

template <typename IdType, typename ValueType>
W_ALWAYS_INLINE typename WIdTableBase<IdType, ValueType>::IndexType WIdTableBase<IdType, ValueType>::GetCount() const
{
  return m_Count;
}

template <typename IdType, typename ValueType>
W_ALWAYS_INLINE typename WIdTableBase<IdType, ValueType>::IndexType WIdTableBase<IdType, ValueType>::GetCapacity() const
{
  return m_Capacity;
}

template <typename IdType, typename ValueType>
W_ALWAYS_INLINE bool WIdTableBase<IdType, ValueType>::IsEmpty() const
{
  return m_Count == 0;
}

template <typename IdType, typename ValueType>
void WIdTableBase<IdType, ValueType>::Clear()
{
  for (IndexType i = 0; i < m_Capacity; ++i)
  {
    Entry& entry = m_pEntries[i];

    if (entry.id.m_InstanceIndex == i)
    {
      WMemoryUtils::Destruct(&entry.value, 1);
      ++entry.id.m_Generation;
    }

    entry.id.m_InstanceIndex = static_cast<decltype(entry.id.m_InstanceIndex)>(i + 1);
  }

  m_FreelistDequeue = 0;
  m_FreelistEnqueue = m_Capacity - 1;
  m_Count = 0;
}

template <typename IdType, typename ValueType>
IdType WIdTableBase<IdType, ValueType>::Insert(const ValueType& value)
{
  Reserve(m_Count + 1);

  const IndexType uiNewIndex = m_FreelistDequeue;
  Entry& entry = m_pEntries[uiNewIndex];

  m_FreelistDequeue = entry.id.m_InstanceIndex;
  entry.id.m_InstanceIndex = static_cast<decltype(entry.id.m_InstanceIndex)>(uiNewIndex);

  WMemoryUtils::CopyConstruct(&entry.value, value, 1);

  ++m_Count;

  return entry.id;
}

template <typename IdType, typename ValueType>
IdType WIdTableBase<IdType, ValueType>::Insert(ValueType&& value)
{
  Reserve(m_Count + 1);

  const IndexType uiNewIndex = m_FreelistDequeue;
  Entry& entry = m_pEntries[uiNewIndex];

  m_FreelistDequeue = entry.id.m_InstanceIndex;
  entry.id.m_InstanceIndex = static_cast<decltype(entry.id.m_InstanceIndex)>(uiNewIndex);

  WMemoryUtils::MoveConstruct<ValueType>(&entry.value, std::move(value));

  ++m_Count;

  return entry.id;
}

template <typename IdType, typename ValueType>
bool WIdTableBase<IdType, ValueType>::Remove(const IdType id, ValueType* out_pOldValue /*= nullptr*/)
{
  if (m_Capacity <= id.m_InstanceIndex)
    return false;

  const IndexType uiIndex = id.m_InstanceIndex;
  Entry& entry = m_pEntries[uiIndex];
  if (!entry.id.IsIndexAndGenerationEqual(id))
    return false;

  if (out_pOldValue != nullptr)
    *out_pOldValue = std::move(m_pEntries[uiIndex].value);

  WMemoryUtils::Destruct(&entry.value, 1);

  entry.id.m_InstanceIndex = m_pEntries[m_FreelistEnqueue].id.m_InstanceIndex;
  ++entry.id.m_Generation;

  // at wrap around, prevent generation from becoming 0, to ensure that a zero initialized array could ever contain a valid ID
  if (entry.id.m_Generation == 0)
    entry.id.m_Generation = 1;

  m_pEntries[m_FreelistEnqueue].id.m_InstanceIndex = static_cast<decltype(entry.id.m_InstanceIndex)>(uiIndex);
  m_FreelistEnqueue = uiIndex;

  --m_Count;
  return true;
}

template <typename IdType, typename ValueType>
W_FORCE_INLINE bool WIdTableBase<IdType, ValueType>::TryGetValue(const IdType id, ValueType& out_value) const
{
  const IndexType index = id.m_InstanceIndex;
  if (index < m_Capacity && m_pEntries[index].id.IsIndexAndGenerationEqual(id))
  {
    out_value = m_pEntries[index].value;
    return true;
  }
  return false;
}

template <typename IdType, typename ValueType>
W_FORCE_INLINE bool WIdTableBase<IdType, ValueType>::TryGetValue(const IdType id, ValueType*& out_pValue) const
{
  const IndexType index = id.m_InstanceIndex;
  if (index < m_Capacity && m_pEntries[index].id.IsIndexAndGenerationEqual(id))
  {
    out_pValue = &m_pEntries[index].value;
    return true;
  }
  return false;
}

template <typename IdType, typename ValueType>
W_FORCE_INLINE const ValueType& WIdTableBase<IdType, ValueType>::operator[](const IdType id) const
{
  W_ASSERT_DEBUG(id.m_InstanceIndex < m_Capacity, "Out of bounds access. Table has {0} elements, trying to access element at index {1}.",
    m_Capacity, id.m_InstanceIndex);
  const Entry& entry = m_pEntries[id.m_InstanceIndex];
  W_ASSERT_DEBUG(entry.id.IsIndexAndGenerationEqual(id), "Stale access. Trying to access a value (generation: {0}) that has been removed and replaced by a new value (generation: {1})", entry.id.m_Generation, id.m_Generation);

  return entry.value;
}

template <typename IdType, typename ValueType>
W_FORCE_INLINE ValueType& WIdTableBase<IdType, ValueType>::operator[](const IdType id)
{
  W_ASSERT_DEBUG(id.m_InstanceIndex < m_Capacity, "Out of bounds access. Table has {0} elements, trying to access element at index {1}.", m_Capacity, id.m_InstanceIndex);

  Entry& entry = m_pEntries[id.m_InstanceIndex];

  W_ASSERT_DEBUG(entry.id.IsIndexAndGenerationEqual(id), "Stale access. Trying to access a value (generation: {0}) that has been removed and replaced by a new value (generation: {1})", static_cast<int>(entry.id.m_Generation), id.m_Generation);

  return entry.value;
}

template <typename IdType, typename ValueType>
W_FORCE_INLINE const ValueType& WIdTableBase<IdType, ValueType>::GetValueUnchecked(const IndexType index) const
{
  W_ASSERT_DEBUG(index < m_Capacity, "Out of bounds access. Table has {0} elements, trying to access element at index {1}.", m_Capacity, index);
  return m_pEntries[index].value;
}

template <typename IdType, typename ValueType>
W_FORCE_INLINE ValueType& WIdTableBase<IdType, ValueType>::GetValueUnchecked(const IndexType index)
{
  W_ASSERT_DEBUG(index < m_Capacity, "Out of bounds access. Table has {0} elements, trying to access element at index {1}.", m_Capacity, index);
  return m_pEntries[index].value;
}

template <typename IdType, typename ValueType>
W_FORCE_INLINE IdType WIdTableBase<IdType, ValueType>::GetIdUnchecked(const IndexType index) const
{
  W_ASSERT_DEBUG(index < m_Capacity, "Out of bounds access. Table has {0} elements, trying to access element at index {1}.", m_Capacity, index);
  return m_pEntries[index].id;
}

template <typename IdType, typename ValueType>
W_FORCE_INLINE bool WIdTableBase<IdType, ValueType>::Contains(const IdType id) const
{
  const IndexType index = id.m_InstanceIndex;
  return index < m_Capacity && m_pEntries[index].id.IsIndexAndGenerationEqual(id);
}

template <typename IdType, typename ValueType>
W_ALWAYS_INLINE typename WIdTableBase<IdType, ValueType>::Iterator WIdTableBase<IdType, ValueType>::GetIterator()
{
  return Iterator(*this);
}

template <typename IdType, typename ValueType>
W_ALWAYS_INLINE typename WIdTableBase<IdType, ValueType>::ConstIterator WIdTableBase<IdType, ValueType>::GetIterator() const
{
  return ConstIterator(*this);
}

template <typename IdType, typename ValueType>
W_ALWAYS_INLINE WAllocator* WIdTableBase<IdType, ValueType>::GetAllocator() const
{
  return m_pAllocator;
}

template <typename IdType, typename ValueType>
bool WIdTableBase<IdType, ValueType>::IsFreelistValid() const
{
  if (m_pEntries == nullptr)
    return true;

  IndexType uiIndex = m_FreelistDequeue;
  const Entry* pEntry = m_pEntries + uiIndex;

  while (pEntry->id.m_InstanceIndex < m_Capacity)
  {
    uiIndex = pEntry->id.m_InstanceIndex;
    pEntry = m_pEntries + uiIndex;
  }

  return uiIndex == m_FreelistEnqueue;
}


// private methods
template <typename IdType, typename ValueType>
void WIdTableBase<IdType, ValueType>::SetCapacity(IndexType uiCapacity)
{
  Entry* pNewEntries = W_NEW_RAW_BUFFER(m_pAllocator, Entry, (size_t)uiCapacity);

  for (IndexType i = 0; i < m_Capacity; ++i)
  {
    pNewEntries[i].id = m_pEntries[i].id;

    if (m_pEntries[i].id.m_InstanceIndex == i)
    {
      WMemoryUtils::RelocateConstruct(&pNewEntries[i].value, &m_pEntries[i].value, 1);
    }
  }

  W_DELETE_RAW_BUFFER(m_pAllocator, m_pEntries);
  m_pEntries = pNewEntries;

  InitializeFreelist(m_Capacity, uiCapacity);
  m_Capacity = uiCapacity;
}

template <typename IdType, typename ValueType>
inline void WIdTableBase<IdType, ValueType>::InitializeFreelist(IndexType uiStart, IndexType uiEnd)
{
  for (IndexType i = uiStart; i < uiEnd; ++i)
  {
    IdType& id = m_pEntries[i].id;
    id = IdType(i + 1, 1); // initialize generation with 1, to prevent 0 from being a valid ID
  }

  m_FreelistEnqueue = uiEnd - 1;
}


template <typename IdType, typename V, typename A>
WIdTable<IdType, V, A>::WIdTable()
  : WIdTableBase<IdType, V>(A::GetAllocator())
{
}

template <typename IdType, typename V, typename A>
WIdTable<IdType, V, A>::WIdTable(WAllocator* pAllocator)
  : WIdTableBase<IdType, V>(pAllocator)
{
}

template <typename IdType, typename V, typename A>
WIdTable<IdType, V, A>::WIdTable(const WIdTable<IdType, V, A>& other)
  : WIdTableBase<IdType, V>(other, A::GetAllocator())
{
}

template <typename IdType, typename V, typename A>
WIdTable<IdType, V, A>::WIdTable(const WIdTableBase<IdType, V>& other)
  : WIdTableBase<IdType, V>(other, A::GetAllocator())
{
}

template <typename IdType, typename V, typename A>
void WIdTable<IdType, V, A>::operator=(const WIdTable<IdType, V, A>& rhs)
{
  WIdTableBase<IdType, V>::operator=(rhs);
}

template <typename IdType, typename V, typename A>
void WIdTable<IdType, V, A>::operator=(const WIdTableBase<IdType, V>& rhs)
{
  WIdTableBase<IdType, V>::operator=(rhs);
}
