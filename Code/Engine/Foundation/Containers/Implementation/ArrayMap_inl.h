#pragma once

template <typename KEY, typename VALUE>
inline WArrayMapBase<KEY, VALUE>::WArrayMapBase(WAllocator* pAllocator)
  : m_Data(pAllocator)
{
  m_bSorted = true;
}

template <typename KEY, typename VALUE>
inline WArrayMapBase<KEY, VALUE>::WArrayMapBase(const WArrayMapBase& rhs, WAllocator* pAllocator)
  : m_bSorted(rhs.m_bSorted)
  , m_Data(pAllocator)
{
  m_Data = rhs.m_Data;
}

template <typename KEY, typename VALUE>
inline void WArrayMapBase<KEY, VALUE>::operator=(const WArrayMapBase& rhs)
{
  m_bSorted = rhs.m_bSorted;
  m_Data = rhs.m_Data;
}

template <typename KEY, typename VALUE>
W_ALWAYS_INLINE WUInt32 WArrayMapBase<KEY, VALUE>::GetCount() const
{
  return m_Data.GetCount();
}

template <typename KEY, typename VALUE>
W_ALWAYS_INLINE bool WArrayMapBase<KEY, VALUE>::IsEmpty() const
{
  return m_Data.IsEmpty();
}

template <typename KEY, typename VALUE>
inline void WArrayMapBase<KEY, VALUE>::Clear()
{
  m_bSorted = true;
  m_Data.Clear();
}

template <typename KEY, typename VALUE>
template <typename CompatibleKeyType, typename CompatibleValueType>
inline WUInt32 WArrayMapBase<KEY, VALUE>::Insert(CompatibleKeyType&& key, CompatibleValueType&& value)
{
  Pair& ref = m_Data.ExpandAndGetRef();
  ref.key = std::forward<CompatibleKeyType>(key);
  ref.value = std::forward<CompatibleValueType>(value);
  m_bSorted = false;
  return m_Data.GetCount() - 1;
}

template <typename KEY, typename VALUE>
inline void WArrayMapBase<KEY, VALUE>::Sort() const
{
  if (m_bSorted)
    return;

  m_bSorted = true;
  m_Data.Sort();
}

template <typename KEY, typename VALUE>
template <typename CompatibleKeyType>
WUInt32 WArrayMapBase<KEY, VALUE>::Find(const CompatibleKeyType& key) const
{
  if (!m_bSorted)
  {
    m_Data.Sort();
  }

  WUInt32 lb = 0;
  WUInt32 ub = m_Data.GetCount();

  while (lb < ub)
  {
    const WUInt32 middle = lb + ((ub - lb) >> 1);

    if (m_Data[middle].key < key)
    {
      lb = middle + 1;
    }
    else if (key < m_Data[middle].key)
    {
      ub = middle;
    }
    else // equal
    {
      return middle;
    }
  }

  return WInvalidIndex;
}

template <typename KEY, typename VALUE>
template <typename CompatibleKeyType>
WUInt32 WArrayMapBase<KEY, VALUE>::LowerBound(const CompatibleKeyType& key) const
{
  if (!m_bSorted)
  {
    m_Data.Sort();
  }

  WUInt32 lb = 0;
  WUInt32 ub = m_Data.GetCount();

  while (lb < ub)
  {
    const WUInt32 middle = lb + ((ub - lb) >> 1);

    if (m_Data[middle].key < key)
    {
      lb = middle + 1;
    }
    else
    {
      ub = middle;
    }
  }

  if (lb == m_Data.GetCount())
    return WInvalidIndex;

  return lb;
}

template <typename KEY, typename VALUE>
template <typename CompatibleKeyType>
WUInt32 WArrayMapBase<KEY, VALUE>::UpperBound(const CompatibleKeyType& key) const
{
  if (!m_bSorted)
  {
    m_Data.Sort();
  }

  WUInt32 lb = 0;
  WUInt32 ub = m_Data.GetCount();

  while (lb < ub)
  {
    const WUInt32 middle = lb + ((ub - lb) >> 1);

    if (key < m_Data[middle].key)
    {
      ub = middle;
    }
    else
    {
      lb = middle + 1;
    }
  }

  if (ub == m_Data.GetCount())
    return WInvalidIndex;

  return ub;
}

template <typename KEY, typename VALUE>
W_ALWAYS_INLINE const KEY& WArrayMapBase<KEY, VALUE>::GetKey(WUInt32 uiIndex) const
{
  return m_Data[uiIndex].key;
}

template <typename KEY, typename VALUE>
W_ALWAYS_INLINE const VALUE& WArrayMapBase<KEY, VALUE>::GetValue(WUInt32 uiIndex) const
{
  return m_Data[uiIndex].value;
}

template <typename KEY, typename VALUE>
VALUE& WArrayMapBase<KEY, VALUE>::GetValue(WUInt32 uiIndex)
{
  return m_Data[uiIndex].value;
}

template <typename KEY, typename VALUE>
W_ALWAYS_INLINE WDynamicArray<typename WArrayMapBase<KEY, VALUE>::Pair>& WArrayMapBase<KEY, VALUE>::GetData()
{
  m_bSorted = false;
  return m_Data;
}

template <typename KEY, typename VALUE>
W_ALWAYS_INLINE const WDynamicArray<typename WArrayMapBase<KEY, VALUE>::Pair>& WArrayMapBase<KEY, VALUE>::GetData() const
{
  return m_Data;
}

template <typename KEY, typename VALUE>
template <typename CompatibleKeyType>
VALUE& WArrayMapBase<KEY, VALUE>::FindOrAdd(const CompatibleKeyType& key, bool* out_pExisted)
{
  WUInt32 index = Find<CompatibleKeyType>(key);

  if (out_pExisted)
    *out_pExisted = index != WInvalidIndex;

  if (index == WInvalidIndex)
  {
    index = Insert(key, VALUE());
  }

  return GetValue(index);
}

template <typename KEY, typename VALUE>
template <typename CompatibleKeyType>
W_ALWAYS_INLINE VALUE& WArrayMapBase<KEY, VALUE>::operator[](const CompatibleKeyType& key)
{
  return FindOrAdd(key);
}

template <typename KEY, typename VALUE>
W_ALWAYS_INLINE const typename WArrayMapBase<KEY, VALUE>::Pair& WArrayMapBase<KEY, VALUE>::GetPair(WUInt32 uiIndex) const
{
  return m_Data[uiIndex];
}

template <typename KEY, typename VALUE>
void WArrayMapBase<KEY, VALUE>::RemoveAtAndCopy(WUInt32 uiIndex, bool bKeepSorted)
{
  if (bKeepSorted && m_bSorted)
  {
    m_Data.RemoveAtAndCopy(uiIndex);
  }
  else
  {
    m_Data.RemoveAtAndSwap(uiIndex);
    m_bSorted = false;
  }
}

template <typename KEY, typename VALUE>
template <typename CompatibleKeyType>
bool WArrayMapBase<KEY, VALUE>::RemoveAndCopy(const CompatibleKeyType& key, bool bKeepSorted)
{
  const WUInt32 uiIndex = Find(key);

  if (uiIndex == WInvalidIndex)
    return false;

  RemoveAtAndCopy(uiIndex, bKeepSorted);
  return true;
}

template <typename KEY, typename VALUE>
template <typename CompatibleKeyType>
W_ALWAYS_INLINE bool WArrayMapBase<KEY, VALUE>::Contains(const CompatibleKeyType& key) const
{
  return Find(key) != WInvalidIndex;
}

template <typename KEY, typename VALUE>
template <typename CompatibleKeyType>
bool WArrayMapBase<KEY, VALUE>::Contains(const CompatibleKeyType& key, const VALUE& value) const
{
  WUInt32 atpos = LowerBound(key);

  if (atpos == WInvalidIndex)
    return false;

  while (atpos < m_Data.GetCount())
  {
    if (m_Data[atpos].key != key)
      return false;

    if (m_Data[atpos].value == value)
      return true;

    ++atpos;
  }

  return false;
}


template <typename KEY, typename VALUE>
W_ALWAYS_INLINE void WArrayMapBase<KEY, VALUE>::Reserve(WUInt32 uiSize)
{
  m_Data.Reserve(uiSize);
}

template <typename KEY, typename VALUE>
W_ALWAYS_INLINE void WArrayMapBase<KEY, VALUE>::Compact()
{
  m_Data.Compact();
}

template <typename KEY, typename VALUE>
bool WArrayMapBase<KEY, VALUE>::operator==(const WArrayMapBase<KEY, VALUE>& rhs) const
{
  Sort();
  rhs.Sort();

  return m_Data == rhs.m_Data;
}

template <typename KEY, typename VALUE, typename A>
WArrayMap<KEY, VALUE, A>::WArrayMap()
  : WArrayMapBase<KEY, VALUE>(A::GetAllocator())
{
}

template <typename KEY, typename VALUE, typename A>
WArrayMap<KEY, VALUE, A>::WArrayMap(WAllocator* pAllocator)
  : WArrayMapBase<KEY, VALUE>(pAllocator)
{
}

template <typename KEY, typename VALUE, typename A>
WArrayMap<KEY, VALUE, A>::WArrayMap(const WArrayMap<KEY, VALUE, A>& rhs)
  : WArrayMapBase<KEY, VALUE>(rhs, A::GetAllocator())
{
}

template <typename KEY, typename VALUE, typename A>
WArrayMap<KEY, VALUE, A>::WArrayMap(const WArrayMapBase<KEY, VALUE>& rhs)
  : WArrayMapBase<KEY, VALUE>(rhs, A::GetAllocator())
{
}

template <typename KEY, typename VALUE, typename A>
void WArrayMap<KEY, VALUE, A>::operator=(const WArrayMap<KEY, VALUE, A>& rhs)
{
  WArrayMapBase<KEY, VALUE>::operator=(rhs);
}

template <typename KEY, typename VALUE, typename A>
void WArrayMap<KEY, VALUE, A>::operator=(const WArrayMapBase<KEY, VALUE>& rhs)
{
  WArrayMapBase<KEY, VALUE>::operator=(rhs);
}
