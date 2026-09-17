
template <typename T, typename Derived>
WArrayBase<T, Derived>::WArrayBase() = default;

template <typename T, typename Derived>
WArrayBase<T, Derived>::~WArrayBase()
{
  W_ASSERT_DEBUG(m_uiCount == 0, "The derived class did not destruct all objects. Count is {0}.", m_uiCount);
  W_ASSERT_DEBUG(m_pElements == nullptr, "The derived class did not free its memory.");
}

template <typename T, typename Derived>
void WArrayBase<T, Derived>::operator=(const WArrayPtr<const T>& rhs)
{
  if (this->GetData() == rhs.GetPtr())
  {
    if (m_uiCount == rhs.GetCount())
      return;

    W_ASSERT_DEV(m_uiCount > rhs.GetCount(), "Dangling array pointer. The given array pointer points to invalid memory.");
    T* pElements = static_cast<Derived*>(this)->GetElementsPtr();
    WMemoryUtils::Destruct(pElements + rhs.GetCount(), m_uiCount - rhs.GetCount());
    m_uiCount = rhs.GetCount();
    return;
  }

  const WUInt32 uiOldCount = m_uiCount;
  const WUInt32 uiNewCount = rhs.GetCount();

  if (uiNewCount > uiOldCount)
  {
    static_cast<Derived*>(this)->Reserve(uiNewCount);
    T* pElements = static_cast<Derived*>(this)->GetElementsPtr();
    WMemoryUtils::Copy(pElements, rhs.GetPtr(), uiOldCount);
    WMemoryUtils::CopyConstructArray(pElements + uiOldCount, rhs.GetPtr() + uiOldCount, uiNewCount - uiOldCount);
  }
  else
  {
    T* pElements = static_cast<Derived*>(this)->GetElementsPtr();
    WMemoryUtils::Copy(pElements, rhs.GetPtr(), uiNewCount);
    WMemoryUtils::Destruct(pElements + uiNewCount, uiOldCount - uiNewCount);
  }

  m_uiCount = uiNewCount;
}

template <typename T, typename Derived>
W_ALWAYS_INLINE WArrayBase<T, Derived>::operator WArrayPtr<const T>() const
{
  return WArrayPtr<const T>(static_cast<const Derived*>(this)->GetElementsPtr(), m_uiCount);
}

template <typename T, typename Derived>
W_ALWAYS_INLINE WArrayBase<T, Derived>::operator WArrayPtr<T>()
{
  return WArrayPtr<T>(static_cast<Derived*>(this)->GetElementsPtr(), m_uiCount);
}

template <typename T, typename Derived>
bool WArrayBase<T, Derived>::operator==(const WArrayBase<T, Derived>& rhs) const
{
  if (m_uiCount != rhs.GetCount())
    return false;

  return WMemoryUtils::IsEqual(static_cast<const Derived*>(this)->GetElementsPtr(), rhs.GetData(), m_uiCount);
}

template <typename T, typename Derived>
W_ALWAYS_INLINE bool WArrayBase<T, Derived>::operator<(const WArrayBase<T, Derived>& rhs) const
{
  return GetArrayPtr() < rhs.GetArrayPtr();
}

#if W_DISABLED(W_USE_CPP20_OPERATORS)
template <typename T, typename Derived>
bool WArrayBase<T, Derived>::operator==(const WArrayPtr<const T>& rhs) const
{
  if (m_uiCount != rhs.GetCount())
    return false;

  return WMemoryUtils::IsEqual(static_cast<const Derived*>(this)->GetElementsPtr(), rhs.GetPtr(), m_uiCount);
}
#endif

template <typename T, typename Derived>
W_ALWAYS_INLINE bool WArrayBase<T, Derived>::operator<(const WArrayPtr<const T>& rhs) const
{
  return GetArrayPtr() < rhs;
}

template <typename T, typename Derived>
W_ALWAYS_INLINE const T& WArrayBase<T, Derived>::operator[](const WUInt32 uiIndex) const
{
  W_ASSERT_DEBUG(uiIndex < m_uiCount, "Out of bounds access. Array has {0} elements, trying to access element at index {1}.", m_uiCount, uiIndex);
  return static_cast<const Derived*>(this)->GetElementsPtr()[uiIndex];
}

template <typename T, typename Derived>
W_ALWAYS_INLINE T& WArrayBase<T, Derived>::operator[](const WUInt32 uiIndex)
{
  W_ASSERT_DEBUG(uiIndex < m_uiCount, "Out of bounds access. Array has {0} elements, trying to access element at index {1}.", m_uiCount, uiIndex);
  return static_cast<Derived*>(this)->GetElementsPtr()[uiIndex];
}

template <typename T, typename Derived>
void WArrayBase<T, Derived>::SetCount(WUInt32 uiCount)
{
  const WUInt32 uiOldCount = m_uiCount;
  const WUInt32 uiNewCount = uiCount;

  if (uiNewCount > uiOldCount)
  {
    static_cast<Derived*>(this)->Reserve(uiNewCount);
    WMemoryUtils::Construct<ConstructAll>(static_cast<Derived*>(this)->GetElementsPtr() + uiOldCount, uiNewCount - uiOldCount);
  }
  else if (uiNewCount < uiOldCount)
  {
    WMemoryUtils::Destruct(static_cast<Derived*>(this)->GetElementsPtr() + uiNewCount, uiOldCount - uiNewCount);
  }

  m_uiCount = uiCount;
}

template <typename T, typename Derived>
void WArrayBase<T, Derived>::SetCount(WUInt32 uiCount, const T& fillValue)
{
  const WUInt32 uiOldCount = m_uiCount;
  const WUInt32 uiNewCount = uiCount;

  if (uiNewCount > uiOldCount)
  {
    static_cast<Derived*>(this)->Reserve(uiNewCount);
    WMemoryUtils::CopyConstruct(static_cast<Derived*>(this)->GetElementsPtr() + uiOldCount, fillValue, uiNewCount - uiOldCount);
  }
  else if (uiNewCount < uiOldCount)
  {
    WMemoryUtils::Destruct(static_cast<Derived*>(this)->GetElementsPtr() + uiNewCount, uiOldCount - uiNewCount);
  }

  m_uiCount = uiCount;
}

template <typename T, typename Derived>
void WArrayBase<T, Derived>::EnsureCount(WUInt32 uiCount)
{
  if (uiCount > m_uiCount)
  {
    SetCount(uiCount);
  }
}

template <typename T, typename Derived>
template <typename> // Second template needed so that the compiler does only instantiate it when called. Otherwise the static_assert would trigger
// early.
void WArrayBase<T, Derived>::SetCountUninitialized(WUInt32 uiCount)
{
  static_assert(WIsPodType<T>::value == WTypeIsPod::value, "SetCountUninitialized is only supported for POD types. See W_DEFINE_AS_POD_TYPE() and W_DECLARE_POD_TYPE().");
  const WUInt32 uiOldCount = m_uiCount;
  const WUInt32 uiNewCount = uiCount;

  if (uiNewCount > uiOldCount)
  {
    static_cast<Derived*>(this)->Reserve(uiNewCount);
    // we already assert above that T is a POD type
    // don't construct anything, leave the memory untouched
  }

  m_uiCount = uiCount;
}

template <typename T, typename Derived>
W_ALWAYS_INLINE WUInt32 WArrayBase<T, Derived>::GetCount() const
{
  return m_uiCount;
}

template <typename T, typename Derived>
W_ALWAYS_INLINE bool WArrayBase<T, Derived>::IsEmpty() const
{
  return m_uiCount == 0;
}

template <typename T, typename Derived>
void WArrayBase<T, Derived>::Clear()
{
  WMemoryUtils::Destruct(static_cast<Derived*>(this)->GetElementsPtr(), m_uiCount);
  m_uiCount = 0;
}

template <typename T, typename Derived>
bool WArrayBase<T, Derived>::Contains(const T& value) const
{
  return IndexOf(value) != WInvalidIndex;
}

template <typename T, typename Derived>
void WArrayBase<T, Derived>::InsertAt(WUInt32 uiIndex, const T& value)
{
  W_ASSERT_DEV(uiIndex <= m_uiCount, "Invalid index. Array has {0} elements, trying to insert element at index {1}.", m_uiCount, uiIndex);

  static_cast<Derived*>(this)->Reserve(m_uiCount + 1);

  WMemoryUtils::Prepend(static_cast<Derived*>(this)->GetElementsPtr() + uiIndex, value, m_uiCount - uiIndex);
  m_uiCount++;
}

template <typename T, typename Derived>
void WArrayBase<T, Derived>::InsertAt(WUInt32 uiIndex, T&& value)
{
  W_ASSERT_DEV(uiIndex <= m_uiCount, "Invalid index. Array has {0} elements, trying to insert element at index {1}.", m_uiCount, uiIndex);

  static_cast<Derived*>(this)->Reserve(m_uiCount + 1);

  WMemoryUtils::Prepend(static_cast<Derived*>(this)->GetElementsPtr() + uiIndex, std::move(value), m_uiCount - uiIndex);
  m_uiCount++;
}

template <typename T, typename Derived>
void WArrayBase<T, Derived>::InsertRangeAt(WUInt32 uiIndex, const WArrayPtr<const T>& range)
{
  const WUInt32 uiRangeCount = range.GetCount();
  static_cast<Derived*>(this)->Reserve(m_uiCount + uiRangeCount);

  WMemoryUtils::Prepend(static_cast<Derived*>(this)->GetElementsPtr() + uiIndex, range.GetPtr(), uiRangeCount, m_uiCount - uiIndex);
  m_uiCount += uiRangeCount;
}

template <typename T, typename Derived>
bool WArrayBase<T, Derived>::RemoveAndCopy(const T& value)
{
  WUInt32 uiIndex = IndexOf(value);

  if (uiIndex == WInvalidIndex)
    return false;

  RemoveAtAndCopy(uiIndex);
  return true;
}

template <typename T, typename Derived>
bool WArrayBase<T, Derived>::RemoveAndSwap(const T& value)
{
  WUInt32 uiIndex = IndexOf(value);

  if (uiIndex == WInvalidIndex)
    return false;

  RemoveAtAndSwap(uiIndex);
  return true;
}

template <typename T, typename Derived>
void WArrayBase<T, Derived>::RemoveAtAndCopy(WUInt32 uiIndex, WUInt32 uiNumElements /*= 1*/)
{
  W_ASSERT_DEV(uiIndex + uiNumElements <= m_uiCount, "Out of bounds access. Array has {0} elements, trying to remove element at index {1}.", m_uiCount, uiIndex + uiNumElements - 1);

  T* pElements = static_cast<Derived*>(this)->GetElementsPtr();

  m_uiCount -= uiNumElements;
  WMemoryUtils::RelocateOverlapped(pElements + uiIndex, pElements + uiIndex + uiNumElements, m_uiCount - uiIndex);
}

template <typename T, typename Derived>
void WArrayBase<T, Derived>::RemoveAtAndSwap(WUInt32 uiIndex, WUInt32 uiNumElements /*= 1*/)
{
  W_ASSERT_DEV(uiIndex + uiNumElements <= m_uiCount, "Out of bounds access. Array has {0} elements, trying to remove element at index {1}.", m_uiCount, uiIndex + uiNumElements - 1);

  T* pElements = static_cast<Derived*>(this)->GetElementsPtr();

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

template <typename T, typename Derived>
WUInt32 WArrayBase<T, Derived>::IndexOf(const T& value, WUInt32 uiStartIndex) const
{
  const T* pElements = static_cast<const Derived*>(this)->GetElementsPtr();

  for (WUInt32 i = uiStartIndex; i < m_uiCount; i++)
  {
    if (WMemoryUtils::IsEqual(pElements + i, &value))
      return i;
  }
  return WInvalidIndex;
}

template <typename T, typename Derived>
WUInt32 WArrayBase<T, Derived>::LastIndexOf(const T& value, WUInt32 uiStartIndex) const
{
  const T* pElements = static_cast<const Derived*>(this)->GetElementsPtr();

  for (WUInt32 i = WMath::Min(uiStartIndex, m_uiCount); i-- > 0;)
  {
    if (WMemoryUtils::IsEqual(pElements + i, &value))
      return i;
  }
  return WInvalidIndex;
}

template <typename T, typename Derived>
T& WArrayBase<T, Derived>::ExpandAndGetRef()
{
  static_cast<Derived*>(this)->Reserve(m_uiCount + 1);

  T* pElements = static_cast<Derived*>(this)->GetElementsPtr();

  WMemoryUtils::Construct<SkipTrivialTypes>(pElements + m_uiCount, 1);

  T& ReturnRef = *(pElements + m_uiCount);

  m_uiCount++;

  return ReturnRef;
}

template <typename T, typename Derived>
T* WArrayBase<T, Derived>::ExpandBy(WUInt32 uiNumNewItems)
{
  this->SetCount(this->GetCount() + uiNumNewItems);
  return GetArrayPtr().GetEndPtr() - uiNumNewItems;
}

template <typename T, typename Derived>
void WArrayBase<T, Derived>::PushBack(const T& value)
{
  static_cast<Derived*>(this)->Reserve(m_uiCount + 1);

  WMemoryUtils::CopyConstruct(static_cast<Derived*>(this)->GetElementsPtr() + m_uiCount, value, 1);
  m_uiCount++;
}

template <typename T, typename Derived>
void WArrayBase<T, Derived>::PushBack(T&& value)
{
  static_cast<Derived*>(this)->Reserve(m_uiCount + 1);

  WMemoryUtils::MoveConstruct<T>(static_cast<Derived*>(this)->GetElementsPtr() + m_uiCount, std::move(value));
  m_uiCount++;
}

template <typename T, typename Derived>
void WArrayBase<T, Derived>::PushBackUnchecked(const T& value)
{
  W_ASSERT_DEBUG(m_uiCount < m_uiCapacity, "Appending unchecked to array with insufficient capacity.");

  WMemoryUtils::CopyConstruct(static_cast<Derived*>(this)->GetElementsPtr() + m_uiCount, value, 1);
  m_uiCount++;
}

template <typename T, typename Derived>
void WArrayBase<T, Derived>::PushBackUnchecked(T&& value)
{
  W_ASSERT_DEBUG(m_uiCount < m_uiCapacity, "Appending unchecked to array with insufficient capacity.");

  WMemoryUtils::MoveConstruct<T>(static_cast<Derived*>(this)->GetElementsPtr() + m_uiCount, std::move(value));
  m_uiCount++;
}

template <typename T, typename Derived>
void WArrayBase<T, Derived>::PushBackRange(const WArrayPtr<const T>& range)
{
  const WUInt32 uiRangeCount = range.GetCount();
  static_cast<Derived*>(this)->Reserve(m_uiCount + uiRangeCount);

  WMemoryUtils::CopyConstructArray(static_cast<Derived*>(this)->GetElementsPtr() + m_uiCount, range.GetPtr(), uiRangeCount);
  m_uiCount += uiRangeCount;
}

template <typename T, typename Derived>
void WArrayBase<T, Derived>::PopBack(WUInt32 uiCountToRemove /* = 1 */)
{
  W_ASSERT_DEV(m_uiCount >= uiCountToRemove, "Out of bounds access. Array has {0} elements, trying to pop {1} elements.", m_uiCount, uiCountToRemove);

  m_uiCount -= uiCountToRemove;
  WMemoryUtils::Destruct(static_cast<Derived*>(this)->GetElementsPtr() + m_uiCount, uiCountToRemove);
}

template <typename T, typename Derived>
W_FORCE_INLINE T& WArrayBase<T, Derived>::PeekBack()
{
  W_ASSERT_DEBUG(m_uiCount > 0, "Out of bounds access. Trying to peek into an empty array.");
  return static_cast<Derived*>(this)->GetElementsPtr()[m_uiCount - 1];
}

template <typename T, typename Derived>
W_FORCE_INLINE const T& WArrayBase<T, Derived>::PeekBack() const
{
  W_ASSERT_DEBUG(m_uiCount > 0, "Out of bounds access. Trying to peek into an empty array.");
  return static_cast<const Derived*>(this)->GetElementsPtr()[m_uiCount - 1];
}

template <typename T, typename Derived>
template <typename Comparer>
void WArrayBase<T, Derived>::Sort(const Comparer& comparer)
{
  if (m_uiCount > 1)
  {
    WArrayPtr<T> ar = *this;
    WSorting::QuickSort(ar, comparer);
  }
}

template <typename T, typename Derived>
void WArrayBase<T, Derived>::Sort()
{
  if (m_uiCount > 1)
  {
    WArrayPtr<T> ar = *this;
    WSorting::QuickSort(ar, WCompareHelper<T>());
  }
}

template <typename T, typename Derived>
W_ALWAYS_INLINE T* WArrayBase<T, Derived>::GetData()
{
  if (IsEmpty())
    return nullptr;

  return static_cast<Derived*>(this)->GetElementsPtr();
}

template <typename T, typename Derived>
W_ALWAYS_INLINE const T* WArrayBase<T, Derived>::GetData() const
{
  if (IsEmpty())
    return nullptr;

  return static_cast<const Derived*>(this)->GetElementsPtr();
}

template <typename T, typename Derived>
W_ALWAYS_INLINE WArrayPtr<T> WArrayBase<T, Derived>::GetArrayPtr()
{
  return WArrayPtr<T>(GetData(), GetCount());
}

template <typename T, typename Derived>
W_ALWAYS_INLINE WArrayPtr<const T> WArrayBase<T, Derived>::GetArrayPtr() const
{
  return WArrayPtr<const T>(GetData(), GetCount());
}

template <typename T, typename Derived>
W_ALWAYS_INLINE WArrayPtr<typename WArrayPtr<T>::ByteType> WArrayBase<T, Derived>::GetByteArrayPtr()
{
  return GetArrayPtr().ToByteArray();
}

template <typename T, typename Derived>
W_ALWAYS_INLINE WArrayPtr<typename WArrayPtr<const T>::ByteType> WArrayBase<T, Derived>::GetByteArrayPtr() const
{
  return GetArrayPtr().ToByteArray();
}

template <typename T, typename Derived>
void WArrayBase<T, Derived>::DoSwap(WArrayBase<T, Derived>& other)
{
  WMath::Swap(this->m_pElements, other.m_pElements);
  WMath::Swap(this->m_uiCapacity, other.m_uiCapacity);
  WMath::Swap(this->m_uiCount, other.m_uiCount);
}
