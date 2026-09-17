#include <intrin.h>

W_ALWAYS_INLINE WInt32 WAtomicUtils::Read(const WInt32& iSrc)
{
  return _InterlockedOr((long*)(&iSrc), 0);
}

W_ALWAYS_INLINE WInt64 WAtomicUtils::Read(const WInt64& iSrc)
{
#if W_ENABLED(W_PLATFORM_32BIT)
  WInt64 old;
  do
  {
    old = iSrc;
  } while (_InterlockedCompareExchange64(const_cast<WInt64*>(&iSrc), old, old) != old);
  return old;
#else
  return _InterlockedOr64(const_cast<WInt64*>(&iSrc), 0);
#endif
}

W_ALWAYS_INLINE WInt32 WAtomicUtils::Increment(WInt32& ref_iDest)
{
  return _InterlockedIncrement(reinterpret_cast<long*>(&ref_iDest));
}

W_ALWAYS_INLINE WInt64 WAtomicUtils::Increment(WInt64& ref_iDest)
{
#if W_ENABLED(W_PLATFORM_32BIT)
  WInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, old + 1, old) != old);
  return old + 1;
#else
  return _InterlockedIncrement64(&ref_iDest);
#endif
}

W_ALWAYS_INLINE WInt32 WAtomicUtils::Decrement(WInt32& ref_iDest)
{
  return _InterlockedDecrement(reinterpret_cast<long*>(&ref_iDest));
}

W_ALWAYS_INLINE WInt64 WAtomicUtils::Decrement(WInt64& ref_iDest)
{
#if W_ENABLED(W_PLATFORM_32BIT)
  WInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, old - 1, old) != old);
  return old - 1;
#else
  return _InterlockedDecrement64(&ref_iDest);
#endif
}

W_ALWAYS_INLINE WInt32 WAtomicUtils::PostIncrement(WInt32& ref_iDest)
{
  return _InterlockedExchangeAdd(reinterpret_cast<long*>(&ref_iDest), 1);
}

W_ALWAYS_INLINE WInt64 WAtomicUtils::PostIncrement(WInt64& ref_iDest)
{
#if W_ENABLED(W_PLATFORM_32BIT)
  WInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, old + 1, old) != old);
  return old;
#else
  return _InterlockedExchangeAdd64(&ref_iDest, 1);
#endif
}

W_ALWAYS_INLINE WInt32 WAtomicUtils::PostDecrement(WInt32& ref_iDest)
{
  return _InterlockedExchangeAdd(reinterpret_cast<long*>(&ref_iDest), -1);
}

W_ALWAYS_INLINE WInt64 WAtomicUtils::PostDecrement(WInt64& ref_iDest)
{
#if W_ENABLED(W_PLATFORM_32BIT)
  WInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, old - 1, old) != old);
  return old;
#else
  return _InterlockedExchangeAdd64(&ref_iDest, -1);
#endif
}

W_ALWAYS_INLINE void WAtomicUtils::Add(WInt32& ref_iDest, WInt32 value)
{
  _InterlockedExchangeAdd(reinterpret_cast<long*>(&ref_iDest), value);
}

W_ALWAYS_INLINE void WAtomicUtils::Add(WInt64& ref_iDest, WInt64 value)
{
#if W_ENABLED(W_PLATFORM_32BIT)
  WInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, old + value, old) != old);
#else
  _InterlockedExchangeAdd64(&ref_iDest, value);
#endif
}


W_ALWAYS_INLINE void WAtomicUtils::And(WInt32& ref_iDest, WInt32 value)
{
  _InterlockedAnd(reinterpret_cast<long*>(&ref_iDest), value);
}

W_ALWAYS_INLINE void WAtomicUtils::And(WInt64& ref_iDest, WInt64 value)
{
#if W_ENABLED(W_PLATFORM_32BIT)
  WInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, old & value, old) != old);
#else
  _InterlockedAnd64(&ref_iDest, value);
#endif
}


W_ALWAYS_INLINE void WAtomicUtils::Or(WInt32& ref_iDest, WInt32 value)
{
  _InterlockedOr(reinterpret_cast<long*>(&ref_iDest), value);
}

W_ALWAYS_INLINE void WAtomicUtils::Or(WInt64& ref_iDest, WInt64 value)
{
#if W_ENABLED(W_PLATFORM_32BIT)
  WInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, old | value, old) != old);
#else
  _InterlockedOr64(&ref_iDest, value);
#endif
}


W_ALWAYS_INLINE void WAtomicUtils::Xor(WInt32& ref_iDest, WInt32 value)
{
  _InterlockedXor(reinterpret_cast<long*>(&ref_iDest), value);
}

W_ALWAYS_INLINE void WAtomicUtils::Xor(WInt64& ref_iDest, WInt64 value)
{
#if W_ENABLED(W_PLATFORM_32BIT)
  WInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, old ^ value, old) != old);
#else
  _InterlockedXor64(&ref_iDest, value);
#endif
}


inline void WAtomicUtils::Min(WInt32& ref_iDest, WInt32 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    WInt32 iOldValue = ref_iDest;
    WInt32 iNewValue = value < iOldValue ? value : iOldValue; // do Min manually here, to break #include cycles

    if (_InterlockedCompareExchange(reinterpret_cast<long*>(&ref_iDest), iNewValue, iOldValue) == iOldValue)
      break;
  }
}

inline void WAtomicUtils::Min(WInt64& ref_iDest, WInt64 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    WInt64 iOldValue = ref_iDest;
    WInt64 iNewValue = value < iOldValue ? value : iOldValue; // do Min manually here, to break #include cycles

    if (_InterlockedCompareExchange64(&ref_iDest, iNewValue, iOldValue) == iOldValue)
      break;
  }
}

inline void WAtomicUtils::Max(WInt32& ref_iDest, WInt32 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    WInt32 iOldValue = ref_iDest;
    WInt32 iNewValue = iOldValue < value ? value : iOldValue; // do Max manually here, to break #include cycles

    if (_InterlockedCompareExchange(reinterpret_cast<long*>(&ref_iDest), iNewValue, iOldValue) == iOldValue)
      break;
  }
}

inline void WAtomicUtils::Max(WInt64& ref_iDest, WInt64 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    WInt64 iOldValue = ref_iDest;
    WInt64 iNewValue = iOldValue < value ? value : iOldValue; // do Max manually here, to break #include cycles

    if (_InterlockedCompareExchange64(&ref_iDest, iNewValue, iOldValue) == iOldValue)
      break;
  }
}


inline WInt32 WAtomicUtils::Set(WInt32& ref_iDest, WInt32 value)
{
  return _InterlockedExchange(reinterpret_cast<long*>(&ref_iDest), value);
}

W_ALWAYS_INLINE WInt64 WAtomicUtils::Set(WInt64& ref_iDest, WInt64 value)
{
#if W_ENABLED(W_PLATFORM_32BIT)
  WInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, value, old) != old);
  return old;
#else
  return _InterlockedExchange64(&ref_iDest, value);
#endif
}


W_ALWAYS_INLINE bool WAtomicUtils::TestAndSet(WInt32& ref_iDest, WInt32 iExpected, WInt32 value)
{
  return _InterlockedCompareExchange(reinterpret_cast<long*>(&ref_iDest), value, iExpected) == iExpected;
}

W_ALWAYS_INLINE bool WAtomicUtils::TestAndSet(WInt64& ref_iDest, WInt64 iExpected, WInt64 value)
{
  return _InterlockedCompareExchange64(&ref_iDest, value, iExpected) == iExpected;
}

W_ALWAYS_INLINE bool WAtomicUtils::TestAndSet(void** pDest, void* pExpected, void* value)
{
  return _InterlockedCompareExchangePointer(pDest, value, pExpected) == pExpected;
}

W_ALWAYS_INLINE WInt32 WAtomicUtils::CompareAndSwap(WInt32& ref_iDest, WInt32 iExpected, WInt32 value)
{
  return _InterlockedCompareExchange(reinterpret_cast<long*>(&ref_iDest), value, iExpected);
}

W_ALWAYS_INLINE WInt64 WAtomicUtils::CompareAndSwap(WInt64& ref_iDest, WInt64 iExpected, WInt64 value)
{
  return _InterlockedCompareExchange64(&ref_iDest, value, iExpected);
}
