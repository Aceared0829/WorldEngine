
template <typename T, WUInt32 C>
WStaticArray<T, C>::WStaticArray()
{
  W_ASSERT_DEBUG(this->m_pElements == nullptr, "static arrays should not use m_pElements");
  this->m_uiCapacity = C;
}

template <typename T, WUInt32 C>
WStaticArray<T, C>::WStaticArray(const WStaticArray<T, C>& rhs)
{
  W_ASSERT_DEBUG(this->m_pElements == nullptr, "static arrays should not use m_pElements");
  this->m_uiCapacity = C;
  *this = (WArrayPtr<const T>)rhs; // redirect this to the WArrayPtr version
}

template <typename T, WUInt32 C>
template <WUInt32 OtherCapacity>
WStaticArray<T, C>::WStaticArray(const WStaticArray<T, OtherCapacity>& rhs)
{
  static_assert(OtherCapacity <= C);

  W_ASSERT_DEBUG(this->m_pElements == nullptr, "static arrays should not use m_pElements");
  this->m_uiCapacity = C;

  *this = (WArrayPtr<const T>)rhs; // redirect this to the WArrayPtr version
}

template <typename T, WUInt32 C>
WStaticArray<T, C>::WStaticArray(const WArrayPtr<const T>& rhs)
{
  W_ASSERT_DEBUG(this->m_pElements == nullptr, "static arrays should not use m_pElements");
  this->m_uiCapacity = C;

  *this = rhs;
}

template <typename T, WUInt32 C>
WStaticArray<T, C>::~WStaticArray()
{
  this->Clear();
  W_ASSERT_DEBUG(this->m_pElements == nullptr, "static arrays should not use m_pElements");
}

template <typename T, WUInt32 C>
W_ALWAYS_INLINE T* WStaticArray<T, C>::GetStaticArray()
{
  return reinterpret_cast<T*>(m_Data);
}

template <typename T, WUInt32 C>
W_FORCE_INLINE const T* WStaticArray<T, C>::GetStaticArray() const
{
  return reinterpret_cast<const T*>(m_Data);
}

template <typename T, WUInt32 C>
W_FORCE_INLINE void WStaticArray<T, C>::Reserve(WUInt32 uiCapacity)
{
  W_IGNORE_UNUSED(uiCapacity);
  W_ASSERT_DEV(uiCapacity <= C, "The static array has a fixed capacity of {0}, cannot reserve more elements than that.", C);
  // Nothing to do here
}

template <typename T, WUInt32 C>
W_ALWAYS_INLINE void WStaticArray<T, C>::operator=(const WStaticArray<T, C>& rhs)
{
  *this = (WArrayPtr<const T>)rhs; // redirect this to the WArrayPtr version
}

template <typename T, WUInt32 C>
template <WUInt32 OtherCapacity>
W_ALWAYS_INLINE void WStaticArray<T, C>::operator=(const WStaticArray<T, OtherCapacity>& rhs)
{
  *this = (WArrayPtr<const T>)rhs; // redirect this to the WArrayPtr version
}

template <typename T, WUInt32 C>
W_ALWAYS_INLINE void WStaticArray<T, C>::operator=(const WArrayPtr<const T>& rhs)
{
  WArrayBase<T, WStaticArray<T, C>>::operator=(rhs);
}

template <typename T, WUInt32 C>
W_FORCE_INLINE T* WStaticArray<T, C>::GetElementsPtr()
{
  return GetStaticArray();
}

template <typename T, WUInt32 C>
W_FORCE_INLINE const T* WStaticArray<T, C>::GetElementsPtr() const
{
  return GetStaticArray();
}
