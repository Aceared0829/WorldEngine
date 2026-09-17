
template <typename T>
WDynamicArrayBase<T>::WDynamicArrayBase(WAllocator* pAllocator)
  : m_pAllocator(pAllocator)
{
}

template <typename T>
WDynamicArrayBase<T>::WDynamicArrayBase(T* pInplaceStorage, WUInt32 uiCapacity, WAllocator* pAllocator)
  : m_pAllocator(pAllocator)
{
  m_pAllocator.SetFlags(Storage::External);
  this->m_uiCapacity = uiCapacity;
  this->m_pElements = pInplaceStorage;
}

template <typename T>
WDynamicArrayBase<T>::WDynamicArrayBase(const WDynamicArrayBase<T>& other, WAllocator* pAllocator)
  : m_pAllocator(pAllocator)
{
  WArrayBase<T, WDynamicArrayBase<T>>::operator=((WArrayPtr<const T>)other); // redirect this to the WArrayPtr version
}

template <typename T>
WDynamicArrayBase<T>::WDynamicArrayBase(WDynamicArrayBase<T>&& other, WAllocator* pAllocator)
  : m_pAllocator(pAllocator)
{
  *this = std::move(other);
}

template <typename T>
WDynamicArrayBase<T>::WDynamicArrayBase(const WArrayPtr<const T>& other, WAllocator* pAllocator)
  : m_pAllocator(pAllocator)
{
  WArrayBase<T, WDynamicArrayBase<T>>::operator=(other);
}

template <typename T>
WDynamicArrayBase<T>::~WDynamicArrayBase()
{
  this->Clear();

  if (m_pAllocator.GetFlags() == Storage::Owned)
  {
    // only delete our storage, if we own it
    W_DELETE_RAW_BUFFER(this->m_pAllocator, this->m_pElements);
  }

  this->m_uiCapacity = 0;
  this->m_pElements = nullptr;
}

template <typename T>
W_ALWAYS_INLINE void WDynamicArrayBase<T>::operator=(const WDynamicArrayBase<T>& rhs)
{
  WArrayBase<T, WDynamicArrayBase<T>>::operator=((WArrayPtr<const T>)rhs); // redirect this to the WArrayPtr version
}

template <typename T>
inline void WDynamicArrayBase<T>::operator=(WDynamicArrayBase<T>&& rhs) noexcept
{
  // Clear any existing data (calls destructors if necessary)
  this->Clear();

  if (this->m_pAllocator.GetPtr() == rhs.m_pAllocator.GetPtr() && rhs.m_pAllocator.GetFlags() == Storage::Owned) // only move the storage of rhs, if it owns it
  {
    if (this->m_pAllocator.GetFlags() == Storage::Owned)
    {
      // only delete our storage, if we own it
      W_DELETE_RAW_BUFFER(this->m_pAllocator, this->m_pElements);
    }

    // we now own this storage
    this->m_pAllocator.SetFlags(Storage::Owned);

    // move the data over from the other array
    this->m_uiCount = rhs.m_uiCount;
    this->m_uiCapacity = rhs.m_uiCapacity;
    this->m_pElements = rhs.m_pElements;

    // reset the other array to not reference the data anymore
    rhs.m_pElements = nullptr;
    rhs.m_uiCount = 0;
    rhs.m_uiCapacity = 0;
  }
  else
  {
    // Ensure we have enough data.
    this->Reserve(rhs.m_uiCount);
    this->m_uiCount = rhs.m_uiCount;

    WMemoryUtils::RelocateConstruct(
      this->GetElementsPtr(), rhs.GetElementsPtr() /* vital to remap rhs.m_pElements to absolute ptr */, rhs.m_uiCount);

    rhs.m_uiCount = 0;
  }
}

template <typename T>
void WDynamicArrayBase<T>::Swap(WDynamicArrayBase<T>& other)
{
  if (this->m_pAllocator.GetFlags() == Storage::External && other.m_pAllocator.GetFlags() == Storage::External)
  {
    constexpr WUInt32 InplaceStorageSize = 64;

    struct alignas(alignof(T)) Tmp
    {
      WUInt8 m_StaticData[InplaceStorageSize * sizeof(T)];
    };

    const WUInt32 localSize = this->m_uiCount;
    const WUInt32 otherLocalSize = other.m_uiCount;

    if (localSize <= InplaceStorageSize && otherLocalSize <= InplaceStorageSize && localSize <= other.m_uiCapacity &&
        otherLocalSize <= this->m_uiCapacity)
    {

      Tmp tmp;
      WMemoryUtils::RelocateConstruct(reinterpret_cast<T*>(tmp.m_StaticData), this->GetElementsPtr(), localSize);
      WMemoryUtils::RelocateConstruct(this->GetElementsPtr(), other.GetElementsPtr(), otherLocalSize);
      WMemoryUtils::RelocateConstruct(other.GetElementsPtr(), reinterpret_cast<T*>(tmp.m_StaticData), localSize);

      WMath::Swap(this->m_pAllocator, other.m_pAllocator);
      WMath::Swap(this->m_uiCount, other.m_uiCount);

      return; // successfully swapped in place
    }

    // temp buffer was insufficient -> fallthrough
  }

  if (this->m_pAllocator.GetFlags() == Storage::External)
  {
    // enforce using own storage
    this->Reserve(this->m_uiCapacity + 1);
  }

  if (other.m_pAllocator.GetFlags() == Storage::External)
  {
    // enforce using own storage
    other.Reserve(other.m_uiCapacity + 1);
  }

  // no external storage involved -> swap pointers
  WMath::Swap(this->m_pAllocator, other.m_pAllocator);
  this->DoSwap(other);
}

template <typename T>
void WDynamicArrayBase<T>::SetCapacity(WUInt32 uiCapacity)
{
  // do NOT early out here, it is vital that this function does its thing even if the old capacity would be sufficient

  if (this->m_pAllocator.GetFlags() == Storage::Owned && uiCapacity > this->m_uiCapacity)
  {
    this->m_pElements = W_EXTEND_RAW_BUFFER(this->m_pAllocator, this->m_pElements, this->m_uiCount, uiCapacity);
  }
  else
  {
    T* pOldElements = GetElementsPtr();

    T* pNewElements = W_NEW_RAW_BUFFER(this->m_pAllocator, T, uiCapacity);
    WMemoryUtils::RelocateConstruct(pNewElements, pOldElements, this->m_uiCount);

    if (this->m_pAllocator.GetFlags() == Storage::Owned)
    {
      W_DELETE_RAW_BUFFER(this->m_pAllocator, pOldElements);
    }

    // after any resize, we definitely own the storage
    this->m_pAllocator.SetFlags(Storage::Owned);
    this->m_pElements = pNewElements;
  }

  this->m_uiCapacity = uiCapacity;
}

template <typename T>
void WDynamicArrayBase<T>::Reserve(WUInt32 uiCapacity)
{
  if (this->m_uiCapacity >= uiCapacity)
    return;

  const WUInt64 uiCurCap64 = static_cast<WUInt64>(this->m_uiCapacity);
  WUInt64 uiNewCapacity64 = uiCurCap64 + (uiCurCap64 / 2);

  uiNewCapacity64 = WMath::Max<WUInt64>(uiNewCapacity64, uiCapacity);

  constexpr WUInt64 uiMaxCapacity = 0xFFFFFFFFllu - (CAPACITY_ALIGNMENT - 1);

  // the maximum value must leave room for the capacity alignment computation below (without overflowing the 32 bit range)
  uiNewCapacity64 = WMath::Min<WUInt64>(uiNewCapacity64, uiMaxCapacity);

  uiNewCapacity64 = (uiNewCapacity64 + (CAPACITY_ALIGNMENT - 1)) & ~(CAPACITY_ALIGNMENT - 1);

  W_ASSERT_DEV(uiCapacity <= uiNewCapacity64, "The requested capacity of {} elements exceeds the maximum possible capacity of {} elements.", uiCapacity, uiMaxCapacity);

  SetCapacity(static_cast<WUInt32>(uiNewCapacity64 & 0xFFFFFFFF));
}

template <typename T>
void WDynamicArrayBase<T>::Compact()
{
  if (m_pAllocator.GetFlags() == Storage::External)
    return;

  if (this->IsEmpty())
  {
    // completely deallocate all data, if the array is empty.
    W_DELETE_RAW_BUFFER(this->m_pAllocator, this->m_pElements);
    this->m_uiCapacity = 0;
  }
  else
  {
    const WUInt32 uiNewCapacity = (this->m_uiCount + (CAPACITY_ALIGNMENT - 1)) & ~(CAPACITY_ALIGNMENT - 1);
    if (this->m_uiCapacity != uiNewCapacity)
      SetCapacity(uiNewCapacity);
  }
}

template <typename T>
W_ALWAYS_INLINE T* WDynamicArrayBase<T>::GetElementsPtr()
{
  return this->m_pElements;
}

template <typename T>
W_ALWAYS_INLINE const T* WDynamicArrayBase<T>::GetElementsPtr() const
{
  return this->m_pElements;
}

template <typename T>
WUInt64 WDynamicArrayBase<T>::GetHeapMemoryUsage() const
{
  if (this->m_pAllocator.GetFlags() == Storage::External)
    return 0;

  return (WUInt64)this->m_uiCapacity * (WUInt64)sizeof(T);
}

//////////////////////////////////////////////////////////////////////////

template <typename T, typename A>
WDynamicArray<T, A>::WDynamicArray()
  : WDynamicArrayBase<T>(A::GetAllocator())
{
}

template <typename T, typename A>
WDynamicArray<T, A>::WDynamicArray(WAllocator* pAllocator)
  : WDynamicArrayBase<T>(pAllocator)
{
}

template <typename T, typename A>
WDynamicArray<T, A>::WDynamicArray(const WDynamicArray<T, A>& other)
  : WDynamicArrayBase<T>(other, A::GetAllocator())
{
}

template <typename T, typename A>
WDynamicArray<T, A>::WDynamicArray(const WDynamicArrayBase<T>& other)
  : WDynamicArrayBase<T>(other, A::GetAllocator())
{
}

template <typename T, typename A>
WDynamicArray<T, A>::WDynamicArray(const WArrayPtr<const T>& other)
  : WDynamicArrayBase<T>(other, A::GetAllocator())
{
}

template <typename T, typename A>
WDynamicArray<T, A>::WDynamicArray(WDynamicArray<T, A>&& other)
  : WDynamicArrayBase<T>(std::move(other), other.GetAllocator())
{
}

template <typename T, typename A>
WDynamicArray<T, A>::WDynamicArray(WDynamicArrayBase<T>&& other)
  : WDynamicArrayBase<T>(std::move(other), other.GetAllocator())
{
}

template <typename T, typename A>
void WDynamicArray<T, A>::operator=(const WDynamicArray<T, A>& rhs)
{
  WDynamicArrayBase<T>::operator=(rhs);
}

template <typename T, typename A>
void WDynamicArray<T, A>::operator=(const WDynamicArrayBase<T>& rhs)
{
  WDynamicArrayBase<T>::operator=(rhs);
}

template <typename T, typename A>
void WDynamicArray<T, A>::operator=(const WArrayPtr<const T>& rhs)
{
  WArrayBase<T, WDynamicArrayBase<T>>::operator=(rhs);
}

template <typename T, typename A>
void WDynamicArray<T, A>::operator=(WDynamicArray<T, A>&& rhs) noexcept
{
  WDynamicArrayBase<T>::operator=(std::move(rhs));
}

template <typename T, typename A>
void WDynamicArray<T, A>::operator=(WDynamicArrayBase<T>&& rhs) noexcept
{
  WDynamicArrayBase<T>::operator=(std::move(rhs));
}

//////////////////////////////////////////////////////////////////////////

template <typename T>
WTempArray<T>::WTempArray()
  : WDynamicArray<T>(WTempAllocator::Get())
{
}

template <typename T>
void WTempArray<T>::operator=(const WDynamicArrayBase<T>& rhs)
{
  WDynamicArrayBase<T>::operator=(rhs);
}

template <typename T>
void WTempArray<T>::operator=(const WArrayPtr<const T>& rhs)
{
  WArrayBase<T, WDynamicArrayBase<T>>::operator=(rhs);
}

template <typename T>
void WTempArray<T>::operator=(WDynamicArrayBase<T>&& rhs) noexcept
{
  WDynamicArrayBase<T>::operator=(std::move(rhs));
}

//////////////////////////////////////////////////////////////////////////

template <typename T, typename AllocatorWrapper>
WArrayPtr<const T* const> WMakeArrayPtr(const WDynamicArray<T*, AllocatorWrapper>& dynArray)
{
  return WArrayPtr<const T* const>(dynArray.GetData(), dynArray.GetCount());
}

template <typename T, typename AllocatorWrapper>
WArrayPtr<const T> WMakeArrayPtr(const WDynamicArray<T, AllocatorWrapper>& dynArray)
{
  return WArrayPtr<const T>(dynArray.GetData(), dynArray.GetCount());
}

template <typename T, typename AllocatorWrapper>
WArrayPtr<T> WMakeArrayPtr(WDynamicArray<T, AllocatorWrapper>& in_dynArray)
{
  return WArrayPtr<T>(in_dynArray.GetData(), in_dynArray.GetCount());
}
