
template <typename T>
W_ALWAYS_INLINE WUniquePtr<T>::WUniquePtr() = default;

template <typename T>
template <typename U>
W_ALWAYS_INLINE WUniquePtr<T>::WUniquePtr(const WInternal::NewInstance<U>& instance)
{
  m_pInstance = instance.m_pInstance;
  m_pAllocator = instance.m_pAllocator;
}

template <typename T>
template <typename U>
W_ALWAYS_INLINE WUniquePtr<T>::WUniquePtr(U* pInstance, WAllocator* pAllocator)
{
  m_pInstance = pInstance;
  m_pAllocator = pAllocator;
}

template <typename T>
template <typename U>
W_ALWAYS_INLINE WUniquePtr<T>::WUniquePtr(WUniquePtr<U>&& other)
{
  m_pInstance = other.m_pInstance;
  m_pAllocator = other.m_pAllocator;

  other.m_pInstance = nullptr;
  other.m_pAllocator = nullptr;
}

template <typename T>
W_ALWAYS_INLINE WUniquePtr<T>::WUniquePtr(std::nullptr_t)
{
}

template <typename T>
W_ALWAYS_INLINE WUniquePtr<T>::~WUniquePtr()
{
  Clear();
}

template <typename T>
template <typename U>
W_ALWAYS_INLINE WUniquePtr<T>& WUniquePtr<T>::operator=(const WInternal::NewInstance<U>& instance)
{
  Clear();

  m_pInstance = instance.m_pInstance;
  m_pAllocator = instance.m_pAllocator;

  return *this;
}

template <typename T>
template <typename U>
W_ALWAYS_INLINE WUniquePtr<T>& WUniquePtr<T>::operator=(WUniquePtr<U>&& other)
{
  Clear();

  m_pInstance = other.m_pInstance;
  m_pAllocator = other.m_pAllocator;

  other.m_pInstance = nullptr;
  other.m_pAllocator = nullptr;

  return *this;
}

template <typename T>
W_ALWAYS_INLINE WUniquePtr<T>& WUniquePtr<T>::operator=(std::nullptr_t)
{
  Clear();

  return *this;
}

template <typename T>
W_ALWAYS_INLINE T* WUniquePtr<T>::Release()
{
  T* pInstance = m_pInstance;

  m_pInstance = nullptr;
  m_pAllocator = nullptr;

  return pInstance;
}

template <typename T>
W_ALWAYS_INLINE T* WUniquePtr<T>::Release(WAllocator*& out_pAllocator)
{
  T* pInstance = m_pInstance;
  out_pAllocator = m_pAllocator;

  m_pInstance = nullptr;
  m_pAllocator = nullptr;

  return pInstance;
}

template <typename T>
W_ALWAYS_INLINE T* WUniquePtr<T>::Borrow() const
{
  return m_pInstance;
}

template <typename T>
W_ALWAYS_INLINE void WUniquePtr<T>::Clear()
{
  if (m_pAllocator != nullptr)
  {
    W_DELETE(m_pAllocator, m_pInstance);
  }

  m_pInstance = nullptr;
  m_pAllocator = nullptr;
}

template <typename T>
W_ALWAYS_INLINE T& WUniquePtr<T>::operator*() const
{
  return *m_pInstance;
}

template <typename T>
W_ALWAYS_INLINE T* WUniquePtr<T>::operator->() const
{
  return m_pInstance;
}

template <typename T>
W_ALWAYS_INLINE WUniquePtr<T>::operator bool() const
{
  return m_pInstance != nullptr;
}

template <typename T>
W_ALWAYS_INLINE bool WUniquePtr<T>::operator==(const WUniquePtr<T>& rhs) const
{
  return m_pInstance == rhs.m_pInstance;
}

template <typename T>
W_ALWAYS_INLINE bool WUniquePtr<T>::operator!=(const WUniquePtr<T>& rhs) const
{
  return m_pInstance != rhs.m_pInstance;
}

template <typename T>
W_ALWAYS_INLINE bool WUniquePtr<T>::operator<(const WUniquePtr<T>& rhs) const
{
  return m_pInstance < rhs.m_pInstance;
}

template <typename T>
W_ALWAYS_INLINE bool WUniquePtr<T>::operator<=(const WUniquePtr<T>& rhs) const
{
  return !(rhs < *this);
}

template <typename T>
W_ALWAYS_INLINE bool WUniquePtr<T>::operator>(const WUniquePtr<T>& rhs) const
{
  return rhs < *this;
}

template <typename T>
W_ALWAYS_INLINE bool WUniquePtr<T>::operator>=(const WUniquePtr<T>& rhs) const
{
  return !(*this < rhs);
}

template <typename T>
W_ALWAYS_INLINE bool WUniquePtr<T>::operator==(std::nullptr_t) const
{
  return m_pInstance == nullptr;
}

template <typename T>
W_ALWAYS_INLINE bool WUniquePtr<T>::operator!=(std::nullptr_t) const
{
  return m_pInstance != nullptr;
}

template <typename T>
W_ALWAYS_INLINE bool WUniquePtr<T>::operator<(std::nullptr_t) const
{
  return m_pInstance < nullptr;
}

template <typename T>
W_ALWAYS_INLINE bool WUniquePtr<T>::operator<=(std::nullptr_t) const
{
  return m_pInstance <= nullptr;
}

template <typename T>
W_ALWAYS_INLINE bool WUniquePtr<T>::operator>(std::nullptr_t) const
{
  return m_pInstance > nullptr;
}

template <typename T>
W_ALWAYS_INLINE bool WUniquePtr<T>::operator>=(std::nullptr_t) const
{
  return m_pInstance >= nullptr;
}

//////////////////////////////////////////////////////////////////////////
// free functions

template <typename T>
W_ALWAYS_INLINE bool operator==(const WUniquePtr<T>& lhs, const T* rhs)
{
  return lhs.Borrow() == rhs;
}

template <typename T>
W_ALWAYS_INLINE bool operator==(const WUniquePtr<T>& lhs, T* rhs)
{
  return lhs.Borrow() == rhs;
}

template <typename T>
W_ALWAYS_INLINE bool operator!=(const WUniquePtr<T>& lhs, const T* rhs)
{
  return lhs.Borrow() != rhs;
}

template <typename T>
W_ALWAYS_INLINE bool operator!=(const WUniquePtr<T>& lhs, T* rhs)
{
  return lhs.Borrow() != rhs;
}

template <typename T>
W_ALWAYS_INLINE bool operator==(const T* lhs, const WUniquePtr<T>& rhs)
{
  return lhs == rhs.Borrow();
}

template <typename T>
W_ALWAYS_INLINE bool operator==(T* lhs, const WUniquePtr<T>& rhs)
{
  return lhs == rhs.Borrow();
}

template <typename T>
W_ALWAYS_INLINE bool operator!=(const T* lhs, const WUniquePtr<T>& rhs)
{
  return lhs != rhs.Borrow();
}

template <typename T>
W_ALWAYS_INLINE bool operator!=(T* lhs, const WUniquePtr<T>& rhs)
{
  return lhs != rhs.Borrow();
}
