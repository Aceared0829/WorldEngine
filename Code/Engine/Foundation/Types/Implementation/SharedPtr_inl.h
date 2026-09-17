
template <typename T>
W_ALWAYS_INLINE WSharedPtr<T>::WSharedPtr()
{
  m_pInstance = nullptr;
  m_pAllocator = nullptr;
}

template <typename T>
template <typename U>
W_ALWAYS_INLINE WSharedPtr<T>::WSharedPtr(const WInternal::NewInstance<U>& instance)
{
  m_pInstance = instance.m_pInstance;
  m_pAllocator = instance.m_pAllocator;

  AddReferenceIfValid();
}

template <typename T>
template <typename U>
W_ALWAYS_INLINE WSharedPtr<T>::WSharedPtr(U* pInstance, WAllocator* pAllocator)
{
  m_pInstance = pInstance;
  m_pAllocator = pAllocator;

  AddReferenceIfValid();
}

template <typename T>
W_ALWAYS_INLINE WSharedPtr<T>::WSharedPtr(const WSharedPtr<T>& other)
{
  m_pInstance = other.m_pInstance;
  m_pAllocator = other.m_pAllocator;

  AddReferenceIfValid();
}

template <typename T>
template <typename U>
W_ALWAYS_INLINE WSharedPtr<T>::WSharedPtr(const WSharedPtr<U>& other)
{
  m_pInstance = other.m_pInstance;
  m_pAllocator = other.m_pAllocator;

  AddReferenceIfValid();
}

template <typename T>
template <typename U>
W_ALWAYS_INLINE WSharedPtr<T>::WSharedPtr(WSharedPtr<U>&& other)
{
  m_pInstance = other.m_pInstance;
  m_pAllocator = other.m_pAllocator;

  other.m_pInstance = nullptr;
  other.m_pAllocator = nullptr;
}

template <typename T>
template <typename U>
W_ALWAYS_INLINE WSharedPtr<T>::WSharedPtr(WUniquePtr<U>&& other)
{
  m_pInstance = other.Release(m_pAllocator);

  AddReferenceIfValid();
}

template <typename T>
W_ALWAYS_INLINE WSharedPtr<T>::WSharedPtr(std::nullptr_t)
{
  m_pInstance = nullptr;
  m_pAllocator = nullptr;
}

template <typename T>
W_ALWAYS_INLINE WSharedPtr<T>::~WSharedPtr()
{
  ReleaseReferenceIfValid();
}

template <typename T>
template <typename U>
W_ALWAYS_INLINE WSharedPtr<T>& WSharedPtr<T>::operator=(const WInternal::NewInstance<U>& instance)
{
  ReleaseReferenceIfValid();

  m_pInstance = instance.m_pInstance;
  m_pAllocator = instance.m_pAllocator;

  AddReferenceIfValid();

  return *this;
}

template <typename T>
W_ALWAYS_INLINE WSharedPtr<T>& WSharedPtr<T>::operator=(const WSharedPtr<T>& other)
{
  if (m_pInstance != other.m_pInstance)
  {
    ReleaseReferenceIfValid();

    m_pInstance = other.m_pInstance;
    m_pAllocator = other.m_pAllocator;

    AddReferenceIfValid();
  }

  return *this;
}

template <typename T>
template <typename U>
W_ALWAYS_INLINE WSharedPtr<T>& WSharedPtr<T>::operator=(const WSharedPtr<U>& other)
{
  if (m_pInstance != other.m_pInstance)
  {
    ReleaseReferenceIfValid();

    m_pInstance = other.m_pInstance;
    m_pAllocator = other.m_pAllocator;

    AddReferenceIfValid();
  }

  return *this;
}

template <typename T>
template <typename U>
W_ALWAYS_INLINE WSharedPtr<T>& WSharedPtr<T>::operator=(WSharedPtr<U>&& other)
{
  if (m_pInstance != other.m_pInstance)
  {
    ReleaseReferenceIfValid();

    m_pInstance = other.m_pInstance;
    m_pAllocator = other.m_pAllocator;

    other.m_pInstance = nullptr;
    other.m_pAllocator = nullptr;
  }

  return *this;
}

template <typename T>
template <typename U>
W_ALWAYS_INLINE WSharedPtr<T>& WSharedPtr<T>::operator=(WUniquePtr<U>&& other)
{
  ReleaseReferenceIfValid();

  m_pInstance = other.Release(m_pAllocator);

  AddReferenceIfValid();

  return *this;
}

template <typename T>
W_ALWAYS_INLINE WSharedPtr<T>& WSharedPtr<T>::operator=(std::nullptr_t)
{
  ReleaseReferenceIfValid();

  return *this;
}

template <typename T>
W_ALWAYS_INLINE T* WSharedPtr<T>::Borrow() const
{
  return m_pInstance;
}

template <typename T>
W_ALWAYS_INLINE void WSharedPtr<T>::Clear()
{
  ReleaseReferenceIfValid();
}

template <typename T>
W_ALWAYS_INLINE T& WSharedPtr<T>::operator*() const
{
  return *m_pInstance;
}

template <typename T>
W_ALWAYS_INLINE T* WSharedPtr<T>::operator->() const
{
  return m_pInstance;
}

template <typename T>
W_ALWAYS_INLINE WSharedPtr<T>::operator const T*() const
{
  return m_pInstance;
}

template <typename T>
W_ALWAYS_INLINE WSharedPtr<T>::operator T*()
{
  return m_pInstance;
}

template <typename T>
W_ALWAYS_INLINE WSharedPtr<T>::operator bool() const
{
  return m_pInstance != nullptr;
}

template <typename T>
W_ALWAYS_INLINE bool WSharedPtr<T>::operator==(const WSharedPtr<T>& rhs) const
{
  return m_pInstance == rhs.m_pInstance;
}

template <typename T>
W_ALWAYS_INLINE bool WSharedPtr<T>::operator!=(const WSharedPtr<T>& rhs) const
{
  return m_pInstance != rhs.m_pInstance;
}

template <typename T>
W_ALWAYS_INLINE bool WSharedPtr<T>::operator<(const WSharedPtr<T>& rhs) const
{
  return m_pInstance < rhs.m_pInstance;
}

template <typename T>
W_ALWAYS_INLINE bool WSharedPtr<T>::operator<=(const WSharedPtr<T>& rhs) const
{
  return !(rhs < *this);
}

template <typename T>
W_ALWAYS_INLINE bool WSharedPtr<T>::operator>(const WSharedPtr<T>& rhs) const
{
  return rhs < *this;
}

template <typename T>
W_ALWAYS_INLINE bool WSharedPtr<T>::operator>=(const WSharedPtr<T>& rhs) const
{
  return !(*this < rhs);
}

template <typename T>
W_ALWAYS_INLINE bool WSharedPtr<T>::operator==(std::nullptr_t) const
{
  return m_pInstance == nullptr;
}

template <typename T>
W_ALWAYS_INLINE bool WSharedPtr<T>::operator!=(std::nullptr_t) const
{
  return m_pInstance != nullptr;
}

template <typename T>
W_ALWAYS_INLINE bool WSharedPtr<T>::operator<(std::nullptr_t) const
{
  return m_pInstance < nullptr;
}

template <typename T>
W_ALWAYS_INLINE bool WSharedPtr<T>::operator<=(std::nullptr_t) const
{
  return m_pInstance <= nullptr;
}

template <typename T>
W_ALWAYS_INLINE bool WSharedPtr<T>::operator>(std::nullptr_t) const
{
  return m_pInstance > nullptr;
}

template <typename T>
W_ALWAYS_INLINE bool WSharedPtr<T>::operator>=(std::nullptr_t) const
{
  return m_pInstance >= nullptr;
}

template <typename T>
W_ALWAYS_INLINE void WSharedPtr<T>::AddReferenceIfValid()
{
  if (m_pInstance != nullptr)
  {
    m_pInstance->AddRef();
  }
}

template <typename T>
W_ALWAYS_INLINE void WSharedPtr<T>::ReleaseReferenceIfValid()
{
  if (m_pInstance != nullptr)
  {
    if (m_pInstance->ReleaseRef() == 0)
    {
      auto pNonConstInstance = const_cast<typename WTypeTraits<T>::NonConstType*>(m_pInstance);
      W_ASSERT_DEV(m_pAllocator != nullptr, "Fake shared pointers should never be released");
      W_DELETE(m_pAllocator, pNonConstInstance);
    }

    m_pInstance = nullptr;
    m_pAllocator = nullptr;
  }
}
