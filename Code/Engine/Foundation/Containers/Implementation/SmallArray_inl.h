
template <typename T, WUInt16 Size>
WSmallArrayBase<T, Size>::WSmallArrayBase() = default;

template <typename T, WUInt16 Size>
W_ALWAYS_INLINE WSmallArrayBase<T, Size>::WSmallArrayBase(const WSmallArrayBase<T, Size>& other, WAllocator* pAllocator)
{
  CopyFrom((WArrayPtr<const T>)other, pAllocator);
  m_uiUserData = other.m_uiUserData;
}

template <typename T, WUInt16 Size>
W_ALWAYS_INLINE WSmallArrayBase<T, Size>::WSmallArrayBase(const WArrayPtr<const T>& other, WAllocator* pAllocator)
{
  CopyFrom(other, pAllocator);
}

template <typename T, WUInt16 Size>
W_ALWAYS_INLINE WSmallArrayBase<T, Size>::WSmallArrayBase(WSmallArrayBase<T, Size>&& other, WAllocator* pAllocator)
{
  MoveFrom(std::move(other), pAllocator);
}

template <typename T, WUInt16 Size>
W_FORCE_INLINE WSmallArrayBase<T, Size>::~WSmallArrayBase()
{
  W_ASSERT_DEBUG(m_uiCount == 0, "The derived class did not destruct all objects. Count is {0}.", m_uiCount);
  W_ASSERT_DEBUG(m_pElements == nullptr, "The derived class did not free its memory.");
}

template <typename T, WUInt16 Size>
void WSmallArrayBase<T, Size>::CopyFrom(const WArrayPtr<const T>& other, WAllocator* pAllocator)
{
  W_ASSERT_DEV(other.GetCount() <= WSmallInvalidIndex, "Can't copy {} elements to small array. Maximum count is {}", other.GetCount(), WSmallInvalidIndex);

  if (GetData() == other.GetPtr())
  {
    if (m_uiCount == other.GetCount())
      return;

    W_ASSERT_DEV(m_uiCount > other.GetCount(), "Dangling array pointer. The given array pointer points to invalid memory.");
    T* pElements = GetElementsPtr();
    WMemoryUtils::Destruct(pElements + other.GetCount(), m_uiCount - other.GetCount());
    m_uiCount = static_cast<WUInt16>(other.GetCount());
    return;
  }

  const WUInt32 uiOldCount = m_uiCount;
  const WUInt32 uiNewCount = other.GetCount();

  if (uiNewCount > uiOldCount)
  {
    Reserve(static_cast<WUInt16>(uiNewCount), pAllocator);
    T* pElements = GetElementsPtr();
    WMemoryUtils::Copy(pElements, other.GetPtr(), uiOldCount);
    WMemoryUtils::CopyConstructArray(pElements + uiOldCount, other.GetPtr() + uiOldCount, uiNewCount - uiOldCount);
  }
  else
  {
    T* pElements = GetElementsPtr();
    WMemoryUtils::Copy(pElements, other.GetPtr(), uiNewCount);
    WMemoryUtils::Destruct(pElements + uiNewCount, uiOldCount - uiNewCount);
  }

  m_uiCount = static_cast<WUInt16>(uiNewCount);
}

template <typename T, WUInt16 Size>
void WSmallArrayBase<T, Size>::MoveFrom(WSmallArrayBase<T, Size>&& other, WAllocator* pAllocator)
{
  Clear();

  if (other.m_uiCapacity > Size)
  {
    if (m_uiCapacity > Size)
    {
      // only delete our own external storage
      W_DELETE_RAW_BUFFER(pAllocator, m_pElements);
    }

    m_uiCapacity = other.m_uiCapacity;
    m_pElements = other.m_pElements;
  }
  else
  {
    WMemoryUtils::RelocateConstruct(GetElementsPtr(), other.GetElementsPtr(), other.m_uiCount);
  }

  m_uiCount = other.m_uiCount;
  m_uiUserData = other.m_uiUserData;

  // reset the other array to not reference the data anymore
  other.m_pElements = nullptr;
  other.m_uiCount = 0;
  other.m_uiCapacity = 0;
}

template <typename T, WUInt16 Size>
W_ALWAYS_INLINE WSmallArrayBase<T, Size>::operator WArrayPtr<const T>() const
{
  return WArrayPtr<const T>(GetElementsPtr(), m_uiCount);
}

template <typename T, WUInt16 Size>
W_ALWAYS_INLINE WSmallArrayBase<T, Size>::operator WArrayPtr<T>()
{
  return WArrayPtr<T>(GetElementsPtr(), m_uiCount);
}

template <typename T, WUInt16 Size>
W_ALWAYS_INLINE bool WSmallArrayBase<T, Size>::operator==(const WSmallArrayBase<T, Size>& rhs) const
{
  return *this == rhs.GetArrayPtr();
}

#if W_DISABLED(W_USE_CPP20_OPERATORS)
template <typename T, WUInt16 Size>
bool WSmallArrayBase<T, Size>::operator==(const WArrayPtr<const T>& rhs) const
{
  if (m_uiCount != rhs.GetCount())
    return false;

  return WMemoryUtils::IsEqual(GetElementsPtr(), rhs.GetPtr(), m_uiCount);
}
#endif

template <typename T, WUInt16 Size>
W_ALWAYS_INLINE bool WSmallArrayBase<T, Size>::operator<(const WSmallArrayBase<T, Size>& rhs) const
{
  return GetArrayPtr() < rhs.GetArrayPtr();
}

template <typename T, WUInt16 Size>
W_ALWAYS_INLINE bool WSmallArrayBase<T, Size>::operator<(const WArrayPtr<const T>& rhs) const
{
  return GetArrayPtr() < rhs;
}

template <typename T, WUInt16 Size>
W_ALWAYS_INLINE const T& WSmallArrayBase<T, Size>::operator[](const WUInt32 uiIndex) const
{
  W_ASSERT_DEBUG(uiIndex < m_uiCount, "Out of bounds access. Array has {0} elements, trying to access element at index {1}.", m_uiCount, uiIndex);
  return GetElementsPtr()[uiIndex];
}

template <typename T, WUInt16 Size>
W_ALWAYS_INLINE T& WSmallArrayBase<T, Size>::operator[](const WUInt32 uiIndex)
{
  W_ASSERT_DEBUG(uiIndex < m_uiCount, "Out of bounds access. Array has {0} elements, trying to access element at index {1}.", m_uiCount, uiIndex);
  return GetElementsPtr()[uiIndex];
}

template <typename T, WUInt16 Size>
void WSmallArrayBase<T, Size>::SetCount(WUInt16 uiCount, WAllocator* pAllocator)
{
  const WUInt32 uiOldCount = m_uiCount;
  const WUInt32 uiNewCount = uiCount;

  if (uiNewCount > uiOldCount)
  {
    Reserve(static_cast<WUInt16>(uiNewCount), pAllocator);
    WMemoryUtils::Construct<ConstructAll>(GetElementsPtr() + uiOldCount, uiNewCount - uiOldCount);
  }
  else if (uiNewCount < uiOldCount)
  {
    WMemoryUtils::Destruct(GetElementsPtr() + uiNewCount, uiOldCount - uiNewCount);
  }

  m_uiCount = uiCount;
}

template <typename T, WUInt16 Size>
void WSmallArrayBase<T, Size>::SetCount(WUInt16 uiCount, const T& fillValue, WAllocator* pAllocator)
{
  const WUInt32 uiOldCount = m_uiCount;
  const WUInt32 uiNewCount = uiCount;

  if (uiNewCount > uiOldCount)
  {
    Reserve(uiCount, pAllocator);
    WMemoryUtils::CopyConstruct(GetElementsPtr() + uiOldCount, fillValue, uiNewCount - uiOldCount);
  }
  else if (uiNewCount < uiOldCount)
  {
    WMemoryUtils::Destruct(GetElementsPtr() + uiNewCount, uiOldCount - uiNewCount);
  }

  m_uiCount = uiCount;
}

template <typename T, WUInt16 Size>
void WSmallArrayBase<T, Size>::EnsureCount(WUInt16 uiCount, WAllocator* pAllocator)
{
  if (uiCount > m_uiCount)
  {
    SetCount(uiCount, pAllocator);
  }
}

template <typename T, WUInt16 Size>
template <typename> // Second template needed so that the compiler does only instantiate it when called. Otherwise the static_assert would trigger early.
void WSmallArrayBase<T, Size>::SetCountUninitialized(WUInt16 uiCount, WAllocator* pAllocator)
{
  static_assert(WIsPodType<T>::value == WTypeIsPod::value, "SetCountUninitialized is only supported for POD types. See W_DEFINE_AS_POD_TYPE() and W_DECLARE_POD_TYPE().");
  const WUInt16 uiOldCount = m_uiCount;
  const WUInt16 uiNewCount = uiCount;

  if (uiNewCount > uiOldCount)
  {
    Reserve(uiNewCount, pAllocator);
    WMemoryUtils::Construct<SkipTrivialTypes>(GetElementsPtr() + uiOldCount, uiNewCount - uiOldCount);
  }
  else if (uiNewCount < uiOldCount)
  {
    WMemoryUtils::Destruct(GetElementsPtr() + uiNewCount, uiOldCount - uiNewCount);
  }

  m_uiCount = uiCount;
}

template <typename T, WUInt16 Size>
W_ALWAYS_INLINE WUInt32 WSmallArrayBase<T, Size>::GetCount() const
{
  return m_uiCount;
}

template <typename T, WUInt16 Size>
W_ALWAYS_INLINE bool WSmallArrayBase<T, Size>::IsEmpty() const
{
  return m_uiCount == 0;
}

template <typename T, WUInt16 Size>
void WSmallArrayBase<T, Size>::Clear()
{
  WMemoryUtils::Destruct(GetElementsPtr(), m_uiCount);
  m_uiCount = 0;
}

template <typename T, WUInt16 Size>
bool WSmallArrayBase<T, Size>::Contains(const T& value) const
{
  return IndexOf(value) != WInvalidIndex;
}

template <typename T, WUInt16 Size>
void WSmallArrayBase<T, Size>::Insert(const T& value, WUInt32 uiIndex, WAllocator* pAllocator)
{
  W_ASSERT_DEV(uiIndex <= m_uiCount, "Invalid index. Array has {0} elements, trying to insert element at index {1}.", m_uiCount, uiIndex);

  Reserve(m_uiCount + 1, pAllocator);

  WMemoryUtils::Prepend(GetElementsPtr() + uiIndex, value, m_uiCount - uiIndex);
  m_uiCount++;
}

template <typename T, WUInt16 Size>
void WSmallArrayBase<T, Size>::Insert(T&& value, WUInt32 uiIndex, WAllocator* pAllocator)
{
  W_ASSERT_DEV(uiIndex <= m_uiCount, "Invalid index. Array has {0} elements, trying to insert element at index {1}.", m_uiCount, uiIndex);

  Reserve(m_uiCount + 1, pAllocator);

  WMemoryUtils::Prepend(GetElementsPtr() + uiIndex, std::move(value), m_uiCount - uiIndex);
  m_uiCount++;
}

template <typename T, WUInt16 Size>
bool WSmallArrayBase<T, Size>::RemoveAndCopy(const T& value)
{
  WUInt32 uiIndex = IndexOf(value);

  if (uiIndex == WInvalidIndex)
    return false;

  RemoveAtAndCopy(uiIndex);
  return true;
}

template <typename T, WUInt16 Size>
bool WSmallArrayBase<T, Size>::RemoveAndSwap(const T& value)
{
  WUInt32 uiIndex = IndexOf(value);

  if (uiIndex == WInvalidIndex)
    return false;

  RemoveAtAndSwap(uiIndex);
  return true;
}

template <typename T, WUInt16 Size>
void WSmallArrayBase<T, Size>::RemoveAtAndCopy(WUInt32 uiIndex, WUInt16 uiNumElements /*= 1*/)
{
  W_ASSERT_DEV(uiIndex + uiNumElements <= m_uiCount, "Out of bounds access. Array has {0} elements, trying to remove element at index {1}.", m_uiCount, uiIndex + uiNumElements - 1);

  T* pElements = GetElementsPtr();

  m_uiCount -= uiNumElements;
  WMemoryUtils::RelocateOverlapped(pElements + uiIndex, pElements + uiIndex + uiNumElements, m_uiCount - uiIndex);
}

template <typename T, WUInt16 Size>
void WSmallArrayBase<T, Size>::RemoveAtAndSwap(WUInt32 uiIndex, WUInt16 uiNumElements /*= 1*/)
{
  W_ASSERT_DEV(uiIndex + uiNumElements <= m_uiCount, "Out of bounds access. Array has {0} elements, trying to remove element at index {1}.", m_uiCount, uiIndex + uiNumElements - 1);

  T* pElements = GetElementsPtr();

  for (WUInt32 i = 0; i < uiNumElements; ++i)
  {
    m_uiCount--;

    if (m_uiCount != uiIndex)
    {
      pElements[uiIndex] = std::move(pElements[m_uiCount]);
    }
    WMemoryUtils::Destruct(pElements + m_uiCount, 1);
    ++uiIndex;
  }
}

template <typename T, WUInt16 Size>
WUInt32 WSmallArrayBase<T, Size>::IndexOf(const T& value, WUInt32 uiStartIndex) const
{
  const T* pElements = GetElementsPtr();

  for (WUInt32 i = uiStartIndex; i < m_uiCount; i++)
  {
    if (WMemoryUtils::IsEqual(pElements + i, &value))
      return i;
  }
  return WInvalidIndex;
}

template <typename T, WUInt16 Size>
WUInt32 WSmallArrayBase<T, Size>::LastIndexOf(const T& value, WUInt32 uiStartIndex) const
{
  const T* pElements = GetElementsPtr();

  for (WUInt32 i = WMath::Min<WUInt32>(uiStartIndex, m_uiCount); i-- > 0;)
  {
    if (WMemoryUtils::IsEqual(pElements + i, &value))
      return i;
  }
  return WInvalidIndex;
}

template <typename T, WUInt16 Size>
T& WSmallArrayBase<T, Size>::ExpandAndGetRef(WAllocator* pAllocator)
{
  Reserve(m_uiCount + 1, pAllocator);

  T* pElements = GetElementsPtr();

  WMemoryUtils::Construct<SkipTrivialTypes>(pElements + m_uiCount, 1);

  T& ReturnRef = *(pElements + m_uiCount);

  m_uiCount++;

  return ReturnRef;
}

template <typename T, WUInt16 Size>
void WSmallArrayBase<T, Size>::PushBack(const T& value, WAllocator* pAllocator)
{
  Reserve(m_uiCount + 1, pAllocator);

  WMemoryUtils::CopyConstruct(GetElementsPtr() + m_uiCount, value, 1);
  m_uiCount++;
}

template <typename T, WUInt16 Size>
void WSmallArrayBase<T, Size>::PushBack(T&& value, WAllocator* pAllocator)
{
  Reserve(m_uiCount + 1, pAllocator);

  WMemoryUtils::MoveConstruct<T>(GetElementsPtr() + m_uiCount, std::move(value));
  m_uiCount++;
}

template <typename T, WUInt16 Size>
void WSmallArrayBase<T, Size>::PushBackUnchecked(const T& value)
{
  W_ASSERT_DEBUG(m_uiCount < m_uiCapacity, "Appending unchecked to array with insufficient capacity.");

  WMemoryUtils::CopyConstruct(GetElementsPtr() + m_uiCount, value, 1);
  m_uiCount++;
}

template <typename T, WUInt16 Size>
void WSmallArrayBase<T, Size>::PushBackUnchecked(T&& value)
{
  W_ASSERT_DEBUG(m_uiCount < m_uiCapacity, "Appending unchecked to array with insufficient capacity.");

  WMemoryUtils::MoveConstruct<T>(GetElementsPtr() + m_uiCount, std::move(value));
  m_uiCount++;
}

template <typename T, WUInt16 Size>
void WSmallArrayBase<T, Size>::PushBackRange(const WArrayPtr<const T>& range, WAllocator* pAllocator)
{
  const WUInt32 uiRangeCount = range.GetCount();
  Reserve(m_uiCount + uiRangeCount, pAllocator);

  WMemoryUtils::CopyConstructArray(GetElementsPtr() + m_uiCount, range.GetPtr(), uiRangeCount);
  m_uiCount += uiRangeCount;
}

template <typename T, WUInt16 Size>
void WSmallArrayBase<T, Size>::PopBack(WUInt32 uiCountToRemove /* = 1 */)
{
  W_ASSERT_DEBUG(m_uiCount >= uiCountToRemove, "Out of bounds access. Array has {0} elements, trying to pop {1} elements.", m_uiCount, uiCountToRemove);

  m_uiCount -= static_cast<WUInt16>(uiCountToRemove);
  WMemoryUtils::Destruct(GetElementsPtr() + m_uiCount, uiCountToRemove);
}

template <typename T, WUInt16 Size>
W_FORCE_INLINE T& WSmallArrayBase<T, Size>::PeekBack()
{
  W_ASSERT_DEBUG(m_uiCount > 0, "Out of bounds access. Trying to peek into an empty array.");
  return GetElementsPtr()[m_uiCount - 1];
}

template <typename T, WUInt16 Size>
W_FORCE_INLINE const T& WSmallArrayBase<T, Size>::PeekBack() const
{
  W_ASSERT_DEBUG(m_uiCount > 0, "Out of bounds access. Trying to peek into an empty array.");
  return GetElementsPtr()[m_uiCount - 1];
}

template <typename T, WUInt16 Size>
template <typename Comparer>
void WSmallArrayBase<T, Size>::Sort(const Comparer& comparer)
{
  if (m_uiCount > 1)
  {
    WArrayPtr<T> ar = GetArrayPtr();
    WSorting::QuickSort(ar, comparer);
  }
}

template <typename T, WUInt16 Size>
void WSmallArrayBase<T, Size>::Sort()
{
  if (m_uiCount > 1)
  {
    WArrayPtr<T> ar = GetArrayPtr();
    WSorting::QuickSort(ar, WCompareHelper<T>());
  }
}

template <typename T, WUInt16 Size>
W_ALWAYS_INLINE T* WSmallArrayBase<T, Size>::GetData()
{
  if (IsEmpty())
    return nullptr;

  return GetElementsPtr();
}

template <typename T, WUInt16 Size>
W_ALWAYS_INLINE const T* WSmallArrayBase<T, Size>::GetData() const
{
  if (IsEmpty())
    return nullptr;

  return GetElementsPtr();
}

template <typename T, WUInt16 Size>
W_ALWAYS_INLINE WArrayPtr<T> WSmallArrayBase<T, Size>::GetArrayPtr()
{
  return WArrayPtr<T>(GetData(), GetCount());
}

template <typename T, WUInt16 Size>
W_ALWAYS_INLINE WArrayPtr<const T> WSmallArrayBase<T, Size>::GetArrayPtr() const
{
  return WArrayPtr<const T>(GetData(), GetCount());
}

template <typename T, WUInt16 Size>
W_ALWAYS_INLINE WArrayPtr<typename WArrayPtr<T>::ByteType> WSmallArrayBase<T, Size>::GetByteArrayPtr()
{
  return GetArrayPtr().ToByteArray();
}

template <typename T, WUInt16 Size>
W_ALWAYS_INLINE WArrayPtr<typename WArrayPtr<const T>::ByteType> WSmallArrayBase<T, Size>::GetByteArrayPtr() const
{
  return GetArrayPtr().ToByteArray();
}

template <typename T, WUInt16 Size>
void WSmallArrayBase<T, Size>::Reserve(WUInt16 uiCapacity, WAllocator* pAllocator)
{
  if (m_uiCapacity >= uiCapacity)
    return;

  const WUInt32 uiCurCap = static_cast<WUInt32>(m_uiCapacity);
  WUInt32 uiNewCapacity = uiCurCap + (uiCurCap / 2);

  uiNewCapacity = WMath::Max<WUInt32>(uiNewCapacity, uiCapacity);
  uiNewCapacity = WMemoryUtils::AlignSize<WUInt32>(uiNewCapacity, CAPACITY_ALIGNMENT);
  uiNewCapacity = WMath::Min<WUInt32>(uiNewCapacity, 0xFFFFu);

  SetCapacity(static_cast<WUInt16>(uiNewCapacity), pAllocator);
}

template <typename T, WUInt16 Size>
void WSmallArrayBase<T, Size>::Compact(WAllocator* pAllocator)
{
  if (IsEmpty())
  {
    if (m_uiCapacity > Size)
    {
      // completely deallocate all data, if the array is empty.
      W_DELETE_RAW_BUFFER(pAllocator, m_pElements);
    }

    m_uiCapacity = Size;
    m_pElements = nullptr;
  }
  else if (m_uiCapacity > Size)
  {
    WUInt32 uiNewCapacity = WMemoryUtils::AlignSize<WUInt32>(m_uiCount, CAPACITY_ALIGNMENT);
    uiNewCapacity = WMath::Min<WUInt32>(uiNewCapacity, 0xFFFFu);

    if (m_uiCapacity != uiNewCapacity)
      SetCapacity(static_cast<WUInt16>(uiNewCapacity), pAllocator);
  }
}

template <typename T, WUInt16 Size>
W_ALWAYS_INLINE WUInt64 WSmallArrayBase<T, Size>::GetHeapMemoryUsage() const
{
  return m_uiCapacity <= Size ? 0 : m_uiCapacity * sizeof(T);
}

template <typename T, WUInt16 Size>
template <typename U>
W_ALWAYS_INLINE const U& WSmallArrayBase<T, Size>::GetUserData() const
{
  static_assert(sizeof(U) <= sizeof(WUInt32));
  return reinterpret_cast<const U&>(m_uiUserData);
}

template <typename T, WUInt16 Size>
template <typename U>
W_ALWAYS_INLINE U& WSmallArrayBase<T, Size>::GetUserData()
{
  static_assert(sizeof(U) <= sizeof(WUInt32));
  return reinterpret_cast<U&>(m_uiUserData);
}

template <typename T, WUInt16 Size>
void WSmallArrayBase<T, Size>::SetCapacity(WUInt16 uiCapacity, WAllocator* pAllocator)
{
  if (m_uiCapacity > Size && uiCapacity > m_uiCapacity)
  {
    m_pElements = W_EXTEND_RAW_BUFFER(pAllocator, m_pElements, m_uiCount, uiCapacity);
    m_uiCapacity = uiCapacity;
  }
  else
  {
    // special case when migrating from in-place to external storage or shrinking
    T* pOldElements = GetElementsPtr();

    const WUInt32 uiOldCapacity = m_uiCapacity;
    const WUInt32 uiNewCapacity = uiCapacity;
    m_uiCapacity = WMath::Max(uiCapacity, Size);

    if (uiNewCapacity > Size)
    {
      // new external storage
      T* pNewElements = W_NEW_RAW_BUFFER(pAllocator, T, uiCapacity);
      WMemoryUtils::RelocateConstruct(pNewElements, pOldElements, m_uiCount);
      m_pElements = pNewElements;
    }
    else
    {
      // Re-use inplace storage
      WMemoryUtils::RelocateConstruct(GetElementsPtr(), pOldElements, m_uiCount);
    }

    if (uiOldCapacity > Size)
    {
      W_DELETE_RAW_BUFFER(pAllocator, pOldElements);
    }
  }
}

template <typename T, WUInt16 Size>
W_ALWAYS_INLINE T* WSmallArrayBase<T, Size>::GetElementsPtr()
{
  return m_uiCapacity <= Size ? reinterpret_cast<T*>(m_StaticData) : m_pElements;
}

template <typename T, WUInt16 Size>
W_ALWAYS_INLINE const T* WSmallArrayBase<T, Size>::GetElementsPtr() const
{
  return m_uiCapacity <= Size ? reinterpret_cast<const T*>(m_StaticData) : m_pElements;
}

//////////////////////////////////////////////////////////////////////////

template <typename T, WUInt16 Size, typename AllocatorWrapper /*= WDefaultAllocatorWrapper*/>
WSmallArray<T, Size, AllocatorWrapper>::WSmallArray() = default;

template <typename T, WUInt16 Size, typename AllocatorWrapper /*= WDefaultAllocatorWrapper*/>
W_ALWAYS_INLINE WSmallArray<T, Size, AllocatorWrapper>::WSmallArray(const WSmallArray<T, Size, AllocatorWrapper>& other)
  : SUPER(other, AllocatorWrapper::GetAllocator())
{
}

template <typename T, WUInt16 Size, typename AllocatorWrapper /*= WDefaultAllocatorWrapper*/>
W_ALWAYS_INLINE WSmallArray<T, Size, AllocatorWrapper>::WSmallArray(const WArrayPtr<const T>& other)
  : SUPER(other, AllocatorWrapper::GetAllocator())
{
}

template <typename T, WUInt16 Size, typename AllocatorWrapper /*= WDefaultAllocatorWrapper*/>
W_ALWAYS_INLINE WSmallArray<T, Size, AllocatorWrapper>::WSmallArray(WSmallArray<T, Size, AllocatorWrapper>&& other)
  : SUPER(static_cast<SUPER&&>(other), AllocatorWrapper::GetAllocator())
{
}

template <typename T, WUInt16 Size, typename AllocatorWrapper /*= WDefaultAllocatorWrapper*/>
WSmallArray<T, Size, AllocatorWrapper>::~WSmallArray()
{
  SUPER::Clear();
  SUPER::Compact(AllocatorWrapper::GetAllocator());
}

template <typename T, WUInt16 Size, typename AllocatorWrapper /*= WDefaultAllocatorWrapper*/>
W_ALWAYS_INLINE void WSmallArray<T, Size, AllocatorWrapper>::operator=(const WSmallArray<T, Size, AllocatorWrapper>& rhs)
{
  *this = ((WArrayPtr<const T>)rhs); // redirect this to the WArrayPtr version
  this->m_uiUserData = rhs.m_uiUserData;
}

template <typename T, WUInt16 Size, typename AllocatorWrapper /*= WDefaultAllocatorWrapper*/>
W_ALWAYS_INLINE void WSmallArray<T, Size, AllocatorWrapper>::operator=(const WArrayPtr<const T>& rhs)
{
  SUPER::CopyFrom(rhs, AllocatorWrapper::GetAllocator());
}

template <typename T, WUInt16 Size, typename AllocatorWrapper /*= WDefaultAllocatorWrapper*/>
W_ALWAYS_INLINE void WSmallArray<T, Size, AllocatorWrapper>::operator=(WSmallArray<T, Size, AllocatorWrapper>&& rhs) noexcept
{
  SUPER::MoveFrom(std::move(rhs), AllocatorWrapper::GetAllocator());
}

template <typename T, WUInt16 Size, typename AllocatorWrapper /*= WDefaultAllocatorWrapper*/>
W_ALWAYS_INLINE void WSmallArray<T, Size, AllocatorWrapper>::SetCount(WUInt16 uiCount)
{
  SUPER::SetCount(uiCount, AllocatorWrapper::GetAllocator());
}

template <typename T, WUInt16 Size, typename AllocatorWrapper /*= WDefaultAllocatorWrapper*/>
W_ALWAYS_INLINE void WSmallArray<T, Size, AllocatorWrapper>::SetCount(WUInt16 uiCount, const T& fillValue)
{
  SUPER::SetCount(uiCount, fillValue, AllocatorWrapper::GetAllocator());
}

template <typename T, WUInt16 Size, typename AllocatorWrapper /*= WDefaultAllocatorWrapper*/>
W_ALWAYS_INLINE void WSmallArray<T, Size, AllocatorWrapper>::EnsureCount(WUInt16 uiCount)
{
  SUPER::EnsureCount(uiCount, AllocatorWrapper::GetAllocator());
}

template <typename T, WUInt16 Size, typename AllocatorWrapper /*= WDefaultAllocatorWrapper*/>
template <typename> // Second template needed so that the compiler does only instantiate it when called. Otherwise the static_assert would trigger early.
W_ALWAYS_INLINE void WSmallArray<T, Size, AllocatorWrapper>::SetCountUninitialized(WUInt16 uiCount)
{
  SUPER::SetCountUninitialized(uiCount, AllocatorWrapper::GetAllocator());
}

template <typename T, WUInt16 Size, typename AllocatorWrapper /*= WDefaultAllocatorWrapper*/>
W_ALWAYS_INLINE void WSmallArray<T, Size, AllocatorWrapper>::InsertAt(WUInt32 uiIndex, const T& value)
{
  SUPER::Insert(value, uiIndex, AllocatorWrapper::GetAllocator());
}

template <typename T, WUInt16 Size, typename AllocatorWrapper /*= WDefaultAllocatorWrapper*/>
W_ALWAYS_INLINE void WSmallArray<T, Size, AllocatorWrapper>::InsertAt(WUInt32 uiIndex, T&& value)
{
  SUPER::Insert(std::move(value), uiIndex, AllocatorWrapper::GetAllocator());
}

template <typename T, WUInt16 Size, typename AllocatorWrapper /*= WDefaultAllocatorWrapper*/>
W_ALWAYS_INLINE T& WSmallArray<T, Size, AllocatorWrapper>::ExpandAndGetRef()
{
  return SUPER::ExpandAndGetRef(AllocatorWrapper::GetAllocator());
}

template <typename T, WUInt16 Size, typename AllocatorWrapper /*= WDefaultAllocatorWrapper*/>
W_ALWAYS_INLINE void WSmallArray<T, Size, AllocatorWrapper>::PushBack(const T& value)
{
  SUPER::PushBack(value, AllocatorWrapper::GetAllocator());
}

template <typename T, WUInt16 Size, typename AllocatorWrapper /*= WDefaultAllocatorWrapper*/>
W_ALWAYS_INLINE void WSmallArray<T, Size, AllocatorWrapper>::PushBack(T&& value)
{
  SUPER::PushBack(std::move(value), AllocatorWrapper::GetAllocator());
}

template <typename T, WUInt16 Size, typename AllocatorWrapper /*= WDefaultAllocatorWrapper*/>
W_ALWAYS_INLINE void WSmallArray<T, Size, AllocatorWrapper>::PushBackRange(const WArrayPtr<const T>& range)
{
  SUPER::PushBackRange(range, AllocatorWrapper::GetAllocator());
}

template <typename T, WUInt16 Size, typename AllocatorWrapper /*= WDefaultAllocatorWrapper*/>
W_ALWAYS_INLINE void WSmallArray<T, Size, AllocatorWrapper>::Reserve(WUInt16 uiCapacity)
{
  SUPER::Reserve(uiCapacity, AllocatorWrapper::GetAllocator());
}

template <typename T, WUInt16 Size, typename AllocatorWrapper /*= WDefaultAllocatorWrapper*/>
W_ALWAYS_INLINE void WSmallArray<T, Size, AllocatorWrapper>::Compact()
{
  SUPER::Compact(AllocatorWrapper::GetAllocator());
}

//////////////////////////////////////////////////////////////////////////

template <typename T, WUInt16 Size>
typename WSmallArrayBase<T, Size>::iterator begin(WSmallArrayBase<T, Size>& ref_container)
{
  return ref_container.GetData();
}

template <typename T, WUInt16 Size>
typename WSmallArrayBase<T, Size>::const_iterator begin(const WSmallArrayBase<T, Size>& container)
{
  return container.GetData();
}

template <typename T, WUInt16 Size>
typename WSmallArrayBase<T, Size>::const_iterator cbegin(const WSmallArrayBase<T, Size>& container)
{
  return container.GetData();
}

template <typename T, WUInt16 Size>
typename WSmallArrayBase<T, Size>::reverse_iterator rbegin(WSmallArrayBase<T, Size>& ref_container)
{
  return typename WSmallArrayBase<T, Size>::reverse_iterator(ref_container.GetData() + ref_container.GetCount() - 1);
}

template <typename T, WUInt16 Size>
typename WSmallArrayBase<T, Size>::const_reverse_iterator rbegin(const WSmallArrayBase<T, Size>& container)
{
  return typename WSmallArrayBase<T, Size>::const_reverse_iterator(container.GetData() + container.GetCount() - 1);
}

template <typename T, WUInt16 Size>
typename WSmallArrayBase<T, Size>::const_reverse_iterator crbegin(const WSmallArrayBase<T, Size>& container)
{
  return typename WSmallArrayBase<T, Size>::const_reverse_iterator(container.GetData() + container.GetCount() - 1);
}

template <typename T, WUInt16 Size>
typename WSmallArrayBase<T, Size>::iterator end(WSmallArrayBase<T, Size>& ref_container)
{
  return ref_container.GetData() + ref_container.GetCount();
}

template <typename T, WUInt16 Size>
typename WSmallArrayBase<T, Size>::const_iterator end(const WSmallArrayBase<T, Size>& container)
{
  return container.GetData() + container.GetCount();
}

template <typename T, WUInt16 Size>
typename WSmallArrayBase<T, Size>::const_iterator cend(const WSmallArrayBase<T, Size>& container)
{
  return container.GetData() + container.GetCount();
}

template <typename T, WUInt16 Size>
typename WSmallArrayBase<T, Size>::reverse_iterator rend(WSmallArrayBase<T, Size>& ref_container)
{
  return typename WSmallArrayBase<T, Size>::reverse_iterator(ref_container.GetData() - 1);
}

template <typename T, WUInt16 Size>
typename WSmallArrayBase<T, Size>::const_reverse_iterator rend(const WSmallArrayBase<T, Size>& container)
{
  return typename WSmallArrayBase<T, Size>::const_reverse_iterator(container.GetData() - 1);
}

template <typename T, WUInt16 Size>
typename WSmallArrayBase<T, Size>::const_reverse_iterator crend(const WSmallArrayBase<T, Size>& container)
{
  return typename WSmallArrayBase<T, Size>::const_reverse_iterator(container.GetData() - 1);
}
