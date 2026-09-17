#pragma once

template <class Container>
W_ALWAYS_INLINE WUInt32 WBitfield<Container>::GetBitInt(WUInt32 uiBitIndex) const
{
  return (uiBitIndex >> 5); // div 32
}

template <class Container>
W_ALWAYS_INLINE WUInt32 WBitfield<Container>::GetBitMask(WUInt32 uiBitIndex) const
{
  return 1 << (uiBitIndex & 0x1F); // modulo 32, shifted to bit position
}

template <class Container>
W_ALWAYS_INLINE WUInt32 WBitfield<Container>::GetCount() const
{
  return m_uiCount;
}

template <class Container>
template <typename> // Second template needed so that the compiler only instantiates it when called. Needed to prevent errors with containers that do not support this.
void WBitfield<Container>::SetCountUninitialized(WUInt32 uiBitCount)
{
  const WUInt32 uiInts = (uiBitCount + 31) >> 5;
  m_Container.SetCountUninitialized(uiInts);

  m_uiCount = uiBitCount;
}

template <class Container>
void WBitfield<Container>::SetCount(WUInt32 uiBitCount, bool bSetNew)
{
  if (m_uiCount == uiBitCount)
    return;

  const WUInt32 uiOldBits = m_uiCount;

  SetCountUninitialized(uiBitCount);

  // if there are new bits, initialize them
  if (uiBitCount > uiOldBits)
  {
    if (bSetNew)
      SetBitRange(uiOldBits, uiBitCount - uiOldBits);
    else
      ClearBitRange(uiOldBits, uiBitCount - uiOldBits);
  }
}

template <class Container>
W_ALWAYS_INLINE bool WBitfield<Container>::IsEmpty() const
{
  return m_uiCount == 0;
}

template <class Container>
bool WBitfield<Container>::IsAnyBitSet(WUInt32 uiFirstBit /*= 0*/, WUInt32 uiNumBits /*= 0xFFFFFFFF*/) const
{
  if (m_uiCount == 0 || uiNumBits == 0)
    return false;

  W_ASSERT_DEBUG(uiFirstBit < m_uiCount, "Cannot access bit {0}, the bitfield only has {1} bits.", uiFirstBit, m_uiCount);

  const WUInt32 uiLastBit = WMath::Min<WUInt32>(uiFirstBit + uiNumBits, m_uiCount) - 1;

  const WUInt32 uiFirstInt = GetBitInt(uiFirstBit);
  const WUInt32 uiLastInt = GetBitInt(uiLastBit);

  // all within the same int
  if (uiFirstInt == uiLastInt)
  {
    for (WUInt32 i = uiFirstBit; i <= uiLastBit; ++i)
    {
      if (IsBitSet(i))
        return true;
    }
  }
  else
  {
    const WUInt32 uiNextIntBit = (uiFirstInt + 1) * 32;
    const WUInt32 uiPrevIntBit = uiLastInt * 32;

    // check the bits in the first int individually
    for (WUInt32 i = uiFirstBit; i < uiNextIntBit; ++i)
    {
      if (IsBitSet(i))
        return true;
    }

    // check the bits in the ints in between with one operation
    for (WUInt32 i = uiFirstInt + 1; i < uiLastInt; ++i)
    {
      if ((m_Container[i] & 0xFFFFFFFF) != 0)
        return true;
    }

    // check the bits in the last int individually
    for (WUInt32 i = uiPrevIntBit; i <= uiLastBit; ++i)
    {
      if (IsBitSet(i))
        return true;
    }
  }

  return false;
}

template <class Container>
W_ALWAYS_INLINE bool WBitfield<Container>::IsNoBitSet(WUInt32 uiFirstBit /*= 0*/, WUInt32 uiLastBit /*= 0xFFFFFFFF*/) const
{
  return !IsAnyBitSet(uiFirstBit, uiLastBit);
}

template <class Container>
bool WBitfield<Container>::AreAllBitsSet(WUInt32 uiFirstBit /*= 0*/, WUInt32 uiNumBits /*= 0xFFFFFFFF*/) const
{
  if (m_uiCount == 0 || uiNumBits == 0)
    return false;

  W_ASSERT_DEBUG(uiFirstBit < m_uiCount, "Cannot access bit {0}, the bitfield only has {1} bits.", uiFirstBit, m_uiCount);

  const WUInt32 uiLastBit = WMath::Min<WUInt32>(uiFirstBit + uiNumBits, m_uiCount - 1);

  const WUInt32 uiFirstInt = GetBitInt(uiFirstBit);
  const WUInt32 uiLastInt = GetBitInt(uiLastBit);

  // all within the same int
  if (uiFirstInt == uiLastInt)
  {
    for (WUInt32 i = uiFirstBit; i <= uiLastBit; ++i)
    {
      if (!IsBitSet(i))
        return false;
    }
  }
  else
  {
    const WUInt32 uiNextIntBit = (uiFirstInt + 1) * 32;
    const WUInt32 uiPrevIntBit = uiLastInt * 32;

    // check the bits in the first int individually
    for (WUInt32 i = uiFirstBit; i < uiNextIntBit; ++i)
    {
      if (!IsBitSet(i))
        return false;
    }

    // check the bits in the ints in between with one operation
    for (WUInt32 i = uiFirstInt + 1; i < uiLastInt; ++i)
    {
      if (m_Container[i] != 0xFFFFFFFF)
        return false;
    }

    // check the bits in the last int individually
    for (WUInt32 i = uiPrevIntBit; i <= uiLastBit; ++i)
    {
      if (!IsBitSet(i))
        return false;
    }
  }

  return true;
}

template <class Container>
W_ALWAYS_INLINE void WBitfield<Container>::Clear()
{
  m_uiCount = 0;
  m_Container.Clear();
}

template <class Container>
void WBitfield<Container>::SetBit(WUInt32 uiBit)
{
  W_ASSERT_DEBUG(uiBit < m_uiCount, "Cannot access bit {0}, the bitfield only has {1} bits.", uiBit, m_uiCount);

  m_Container[GetBitInt(uiBit)] |= GetBitMask(uiBit);
}

template <class Container>
void WBitfield<Container>::ClearBit(WUInt32 uiBit)
{
  W_ASSERT_DEBUG(uiBit < m_uiCount, "Cannot access bit {0}, the bitfield only has {1} bits.", uiBit, m_uiCount);

  m_Container[GetBitInt(uiBit)] &= ~GetBitMask(uiBit);
}

template <class Container>
void WBitfield<Container>::FlipBit(WUInt32 uiBit)
{
  W_ASSERT_DEBUG(uiBit < m_uiCount, "Cannot access bit {0}, the bitfield only has {1} bits.", uiBit, m_uiCount);

  m_Container[GetBitInt(uiBit)] ^= GetBitMask(uiBit);
}

template <class Container>
W_ALWAYS_INLINE void WBitfield<Container>::SetBitValue(WUInt32 uiBit, bool bValue)
{
  if (bValue)
  {
    SetBit(uiBit);
  }
  else
  {
    ClearBit(uiBit);
  }
}

template <class Container>
bool WBitfield<Container>::IsBitSet(WUInt32 uiBit) const
{
  W_ASSERT_DEBUG(uiBit < m_uiCount, "Cannot access bit {0}, the bitfield only has {1} bits.", uiBit, m_uiCount);

  return (m_Container[GetBitInt(uiBit)] & GetBitMask(uiBit)) != 0;
}

template <class Container>
void WBitfield<Container>::ClearAllBits()
{
  for (WUInt32 i = 0; i < m_Container.GetCount(); ++i)
    m_Container[i] = 0;
}

template <class Container>
void WBitfield<Container>::SetAllBits()
{
  for (WUInt32 i = 0; i < m_Container.GetCount(); ++i)
    m_Container[i] = 0xFFFFFFFF;
}

template <class Container>
void WBitfield<Container>::SetBitRange(WUInt32 uiFirstBit, WUInt32 uiNumBits)
{
  if (m_uiCount == 0 || uiNumBits == 0)
    return;

  W_ASSERT_DEBUG(uiFirstBit < m_uiCount, "Cannot access bit {0}, the bitfield only has {1} bits.", uiFirstBit, m_uiCount);

  const WUInt32 uiLastBit = uiFirstBit + uiNumBits - 1;

  const WUInt32 uiFirstInt = GetBitInt(uiFirstBit);
  const WUInt32 uiLastInt = GetBitInt(uiLastBit);

  // all within the same int
  if (uiFirstInt == uiLastInt)
  {
    for (WUInt32 i = uiFirstBit; i <= uiLastBit; ++i)
      SetBit(i);

    return;
  }

  const WUInt32 uiNextIntBit = (uiFirstInt + 1) * 32;
  const WUInt32 uiPrevIntBit = uiLastInt * 32;

  // set the bits in the first int individually
  for (WUInt32 i = uiFirstBit; i < uiNextIntBit; ++i)
    SetBit(i);

  // set the bits in the ints in between with one operation
  for (WUInt32 i = uiFirstInt + 1; i < uiLastInt; ++i)
    m_Container[i] = 0xFFFFFFFF;

  // set the bits in the last int individually
  for (WUInt32 i = uiPrevIntBit; i <= uiLastBit; ++i)
    SetBit(i);
}

template <class Container>
void WBitfield<Container>::ClearBitRange(WUInt32 uiFirstBit, WUInt32 uiNumBits)
{
  if (m_uiCount == 0 || uiNumBits == 0)
    return;

  W_ASSERT_DEBUG(uiFirstBit < m_uiCount, "Cannot access bit {0}, the bitfield only has {1} bits.", uiFirstBit, m_uiCount);

  const WUInt32 uiLastBit = uiFirstBit + uiNumBits - 1;

  const WUInt32 uiFirstInt = GetBitInt(uiFirstBit);
  const WUInt32 uiLastInt = GetBitInt(uiLastBit);

  // all within the same int
  if (uiFirstInt == uiLastInt)
  {
    for (WUInt32 i = uiFirstBit; i <= uiLastBit; ++i)
      ClearBit(i);

    return;
  }

  const WUInt32 uiNextIntBit = (uiFirstInt + 1) * 32;
  const WUInt32 uiPrevIntBit = uiLastInt * 32;

  // set the bits in the first int individually
  for (WUInt32 i = uiFirstBit; i < uiNextIntBit; ++i)
    ClearBit(i);

  // set the bits in the ints in between with one operation
  for (WUInt32 i = uiFirstInt + 1; i < uiLastInt; ++i)
    m_Container[i] = 0;

  // set the bits in the last int individually
  for (WUInt32 i = uiPrevIntBit; i <= uiLastBit; ++i)
    ClearBit(i);
}

template <class Container>
void WBitfield<Container>::FlipBitRange(WUInt32 uiFirstBit, WUInt32 uiNumBits)
{
  if (m_uiCount == 0 || uiNumBits == 0)
    return;

  W_ASSERT_DEBUG(uiFirstBit < m_uiCount, "Cannot access bit {0}, the bitfield only has {1} bits.", uiFirstBit, m_uiCount);

  const WUInt32 uiLastBit = uiFirstBit + uiNumBits - 1;

  const WUInt32 uiFirstInt = GetBitInt(uiFirstBit);
  const WUInt32 uiLastInt = GetBitInt(uiLastBit);

  // all within the same int
  if (uiFirstInt == uiLastInt)
  {
    for (WUInt32 i = uiFirstBit; i <= uiLastBit; ++i)
      FlipBit(i);

    return;
  }

  const WUInt32 uiNextIntBit = (uiFirstInt + 1) * 32;
  const WUInt32 uiPrevIntBit = uiLastInt * 32;

  // flip the bits in the first int individually
  for (WUInt32 i = uiFirstBit; i < uiNextIntBit; ++i)
    FlipBit(i);

  // flip the bits in the ints in between with one operation
  for (WUInt32 i = uiFirstInt + 1; i < uiLastInt; ++i)
    m_Container[i] = ~m_Container[i];

  // flip the bits in the last int individually
  for (WUInt32 i = uiPrevIntBit; i <= uiLastBit; ++i)
    FlipBit(i);
}

template <class Container>
void WBitfield<Container>::Swap(WBitfield<Container>& other)
{
  WMath::Swap(m_uiCount, other.m_uiCount);
  m_Container.Swap(other.m_Container);
}

template <class Container>
W_ALWAYS_INLINE typename WBitfield<Container>::ConstIterator WBitfield<Container>::GetIterator() const
{
  return ConstIterator(*this);
};

template <class Container>
W_ALWAYS_INLINE typename WBitfield<Container>::ConstIterator WBitfield<Container>::GetEndIterator() const
{
  return ConstIterator();
};

//////////////////////////////////////////////////////////////////////////
// WBitfield<Container>::ConstIterator

template <class Container>
WBitfield<Container>::ConstIterator::ConstIterator(const WBitfield<Container>& bitfield)
{
  m_pBitfield = &bitfield;
  FindNextChunk(0);
}

template <class Container>
W_ALWAYS_INLINE bool WBitfield<Container>::ConstIterator::IsValid() const
{
  return m_pBitfield != nullptr;
}

template <class Container>
W_ALWAYS_INLINE WUInt32 WBitfield<Container>::ConstIterator::Value() const
{
  return *m_Iterator + (m_uiChunk << 5);
}

template <class Container>
W_ALWAYS_INLINE void WBitfield<Container>::ConstIterator::Next()
{
  ++m_Iterator;
  if (!m_Iterator.IsValid())
  {
    FindNextChunk(m_uiChunk + 1);
  }
}

template <class Container>
W_ALWAYS_INLINE bool WBitfield<Container>::ConstIterator::operator==(const ConstIterator& other) const
{
  return m_pBitfield == other.m_pBitfield && m_Iterator == other.m_Iterator && m_uiChunk == other.m_uiChunk;
}

template <class Container>
W_ALWAYS_INLINE bool WBitfield<Container>::ConstIterator::operator!=(const ConstIterator& other) const
{
  return m_pBitfield != other.m_pBitfield || m_Iterator != other.m_Iterator || m_uiChunk != other.m_uiChunk;
}

template <class Container>
W_ALWAYS_INLINE WUInt32 WBitfield<Container>::ConstIterator::operator*() const
{
  return Value();
}

template <class Container>
W_ALWAYS_INLINE void WBitfield<Container>::ConstIterator::operator++()
{
  Next();
}

template <class Container>
void WBitfield<Container>::ConstIterator::FindNextChunk(WUInt32 uiStartChunk)
{
  if (uiStartChunk < m_pBitfield->m_Container.GetCount())
  {
    const WUInt32 uiLastChunk = m_pBitfield->m_Container.GetCount() - 1;
    for (WUInt32 i = uiStartChunk; i < uiLastChunk; ++i)
    {
      if (m_pBitfield->m_Container[i] != 0)
      {
        m_uiChunk = i;
        m_Iterator = sub_iterator(m_pBitfield->m_Container[i]);
        return;
      }
    }

    const WUInt32 uiMask = 0xFFFFFFFF >> (32 - (m_pBitfield->m_uiCount - (uiLastChunk << 5)));
    if ((m_pBitfield->m_Container[uiLastChunk] & uiMask) != 0)
    {
      m_uiChunk = uiLastChunk;
      m_Iterator = sub_iterator(m_pBitfield->m_Container[uiLastChunk] & uiMask);
      return;
    }
  }

  // End iterator.
  m_pBitfield = nullptr;
  m_uiChunk = 0;
  m_Iterator = sub_iterator();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

template <typename T>
W_ALWAYS_INLINE WStaticBitfield<T>::WStaticBitfield()
{
  static_assert(std::is_unsigned<T>::value, "Storage type must be unsigned");
}

template <typename T>
W_ALWAYS_INLINE WStaticBitfield<T> WStaticBitfield<T>::MakeFromMask(StorageType bits)
{
  return WStaticBitfield<T>(bits);
}

template <typename T>
W_ALWAYS_INLINE bool WStaticBitfield<T>::IsAnyBitSet() const
{
  return m_Storage != 0;
}

template <typename T>
W_ALWAYS_INLINE bool WStaticBitfield<T>::IsNoBitSet() const
{
  return m_Storage == 0;
}

template <typename T>
bool WStaticBitfield<T>::AreAllBitsSet() const
{
  const T inv = ~m_Storage;
  return inv == 0;
}

template <typename T>
void WStaticBitfield<T>::ClearBitRange(WUInt32 uiFirstBit, WUInt32 uiNumBits)
{
  W_ASSERT_DEBUG(uiFirstBit < GetStorageTypeBitCount(), "Cannot access first bit {0}, the bitfield only has {1} bits.", uiFirstBit, GetStorageTypeBitCount());

  T mask = (uiNumBits / 8 >= sizeof(T)) ? (~static_cast<T>(0)) : ((static_cast<T>(1) << uiNumBits) - 1);
  mask <<= uiFirstBit;
  mask = ~mask;
  m_Storage &= mask;
}

template <typename T>
void WStaticBitfield<T>::SetBitRange(WUInt32 uiFirstBit, WUInt32 uiNumBits)
{
  W_ASSERT_DEBUG(uiFirstBit < GetStorageTypeBitCount(), "Cannot access first bit {0}, the bitfield only has {1} bits.", uiFirstBit, GetStorageTypeBitCount());

  T mask = (uiNumBits / 8 >= sizeof(T)) ? (~static_cast<T>(0)) : ((static_cast<T>(1) << uiNumBits) - 1);
  mask <<= uiFirstBit;
  m_Storage |= mask;
}

template <typename T>
W_ALWAYS_INLINE WUInt32 WStaticBitfield<T>::GetNumBitsSet() const
{
  return WMath::CountBits(m_Storage);
}

template <typename T>
W_ALWAYS_INLINE WUInt32 WStaticBitfield<T>::GetHighestBitSet() const
{
  return m_Storage == 0 ? GetStorageTypeBitCount() : WMath::FirstBitHigh(m_Storage);
}

template <typename T>
W_ALWAYS_INLINE WUInt32 WStaticBitfield<T>::GetLowestBitSet() const
{
  return m_Storage == 0 ? GetStorageTypeBitCount() : WMath::FirstBitLow(m_Storage);
}

template <typename T>
W_ALWAYS_INLINE void WStaticBitfield<T>::SetAllBits()
{
  m_Storage = WMath::MaxValue<T>(); // possible because we assert that T is unsigned
}

template <typename T>
W_ALWAYS_INLINE void WStaticBitfield<T>::ClearAllBits()
{
  m_Storage = 0;
}

template <typename T>
W_ALWAYS_INLINE bool WStaticBitfield<T>::IsBitSet(WUInt32 uiBit) const
{
  W_ASSERT_DEBUG(uiBit < GetStorageTypeBitCount(), "Cannot access bit {0}, the bitfield only has {1} bits.", uiBit, GetStorageTypeBitCount());

  return (m_Storage & (static_cast<T>(1u) << uiBit)) != 0;
}

template <typename T>
W_ALWAYS_INLINE void WStaticBitfield<T>::ClearBit(WUInt32 uiBit)
{
  W_ASSERT_DEBUG(uiBit < GetStorageTypeBitCount(), "Cannot access bit {0}, the bitfield only has {1} bits.", uiBit, GetStorageTypeBitCount());

  m_Storage &= ~(static_cast<T>(1u) << uiBit);
}

template <typename T>
W_ALWAYS_INLINE void WStaticBitfield<T>::SetBitValue(WUInt32 uiBit, bool bValue)
{
  if (bValue)
  {
    SetBit(uiBit);
  }
  else
  {
    ClearBit(uiBit);
  }
}

template <typename T>
W_ALWAYS_INLINE void WStaticBitfield<T>::SetBit(WUInt32 uiBit)
{
  W_ASSERT_DEBUG(uiBit < GetStorageTypeBitCount(), "Cannot access bit {0}, the bitfield only has {1} bits.", uiBit, GetStorageTypeBitCount());

  m_Storage |= static_cast<T>(1u) << uiBit;
}

template <typename T>
W_ALWAYS_INLINE void WStaticBitfield<T>::SetValue(T value)
{
  m_Storage = value;
}

template <typename T>
W_ALWAYS_INLINE T WStaticBitfield<T>::GetValue() const
{
  return m_Storage;
}

template <typename T>
W_ALWAYS_INLINE void WStaticBitfield<T>::Swap(WStaticBitfield<T>& other)
{
  WMath::Swap(m_Storage, other.m_Storage);
}
