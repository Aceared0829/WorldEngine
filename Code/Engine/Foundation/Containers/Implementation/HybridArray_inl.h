
template <typename T, WUInt32 Size, typename AllocatorWrapper /*= WDefaultAllocatorWrapper*/>
WHybridArray<T, Size, AllocatorWrapper>::WHybridArray()
  : WDynamicArray<T, AllocatorWrapper>(GetStaticArray(), Size, AllocatorWrapper::GetAllocator())
{
}

template <typename T, WUInt32 Size, typename AllocatorWrapper /*= WDefaultAllocatorWrapper*/>
WHybridArray<T, Size, AllocatorWrapper>::WHybridArray(WAllocator* pAllocator)
  : WDynamicArray<T, AllocatorWrapper>(GetStaticArray(), Size, pAllocator)
{
}

template <typename T, WUInt32 Size, typename AllocatorWrapper /*= WDefaultAllocatorWrapper*/>
WHybridArray<T, Size, AllocatorWrapper>::WHybridArray(const WHybridArray<T, Size, AllocatorWrapper>& other)
  : WDynamicArray<T, AllocatorWrapper>(GetStaticArray(), Size, AllocatorWrapper::GetAllocator())
{
  *this = other;
}

template <typename T, WUInt32 Size, typename AllocatorWrapper /*= WDefaultAllocatorWrapper*/>
WHybridArray<T, Size, AllocatorWrapper>::WHybridArray(const WArrayPtr<const T>& other)
  : WDynamicArray<T, AllocatorWrapper>(GetStaticArray(), Size, AllocatorWrapper::GetAllocator())
{
  *this = other;
}

template <typename T, WUInt32 Size, typename AllocatorWrapper /*= WDefaultAllocatorWrapper*/>
WHybridArray<T, Size, AllocatorWrapper>::WHybridArray(WHybridArray<T, Size, AllocatorWrapper>&& other) noexcept
  : WDynamicArray<T, AllocatorWrapper>(GetStaticArray(), Size, other.GetAllocator())
{
  *this = std::move(other);
}

template <typename T, WUInt32 Size, typename AllocatorWrapper /*= WDefaultAllocatorWrapper*/>
void WHybridArray<T, Size, AllocatorWrapper>::operator=(const WHybridArray<T, Size, AllocatorWrapper>& rhs)
{
  WDynamicArray<T, AllocatorWrapper>::operator=(rhs);
}

template <typename T, WUInt32 Size, typename AllocatorWrapper /*= WDefaultAllocatorWrapper*/>
void WHybridArray<T, Size, AllocatorWrapper>::operator=(const WArrayPtr<const T>& rhs)
{
  WDynamicArray<T, AllocatorWrapper>::operator=(rhs);
}

template <typename T, WUInt32 Size, typename AllocatorWrapper /*= WDefaultAllocatorWrapper*/>
void WHybridArray<T, Size, AllocatorWrapper>::operator=(WHybridArray<T, Size, AllocatorWrapper>&& rhs) noexcept
{
  WDynamicArray<T, AllocatorWrapper>::operator=(std::move(rhs));
}

//////////////////////////////////////////////////////////////////////////

template <typename T, WUInt32 Size>
WTempHybridArray<T, Size>::WTempHybridArray()
  : WHybridArray<T, Size>(WTempAllocator::Get())
{
}

template <typename T, WUInt32 Size>
template <typename AllocatorWrapper>
WTempHybridArray<T, Size>::WTempHybridArray(const WHybridArray<T, Size, AllocatorWrapper>& other)
  : WHybridArray<T, Size>(WTempAllocator::Get())
{
  *this = other;
}

template <typename T, WUInt32 Size>
WTempHybridArray<T, Size>::WTempHybridArray(const WArrayPtr<const T>& other)
  : WHybridArray<T, Size>(WTempAllocator::Get())
{
  *this = other;
}

template <typename T, WUInt32 Size>
template <typename AllocatorWrapper>
void WTempHybridArray<T, Size>::operator=(const WHybridArray<T, Size, AllocatorWrapper>& rhs)
{
  WDynamicArray<T>::operator=(rhs);
}

template <typename T, WUInt32 Size>
void WTempHybridArray<T, Size>::operator=(const WArrayPtr<const T>& rhs)
{
  WDynamicArray<T>::operator=(rhs);
}

template <typename T, WUInt32 Size>
void WTempHybridArray<T, Size>::operator=(WHybridArray<T, Size>&& rhs) noexcept
{
  WDynamicArray<T>::operator=(std::move(rhs));
}
