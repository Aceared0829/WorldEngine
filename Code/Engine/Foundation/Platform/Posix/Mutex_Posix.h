
W_ALWAYS_INLINE WMutex::WMutex()
{
  pthread_mutexattr_t mutexAttributes;
  pthread_mutexattr_init(&mutexAttributes);
  pthread_mutexattr_settype(&mutexAttributes, PTHREAD_MUTEX_RECURSIVE);

  pthread_mutex_init(&m_hHandle, &mutexAttributes);

  pthread_mutexattr_destroy(&mutexAttributes);
}

W_ALWAYS_INLINE WMutex::~WMutex()
{
  pthread_mutex_destroy(&m_hHandle);
}

W_ALWAYS_INLINE void WMutex::Lock()
{
  pthread_mutex_lock(&m_hHandle);
  ++m_iLockCount;
}

W_ALWAYS_INLINE WResult WMutex::TryLock()
{
  if (pthread_mutex_trylock(&m_hHandle) == 0)
  {
    ++m_iLockCount;
    return W_SUCCESS;
  }

  return W_FAILURE;
}
W_ALWAYS_INLINE void WMutex::Unlock()
{
  --m_iLockCount;
  pthread_mutex_unlock(&m_hHandle);
}
