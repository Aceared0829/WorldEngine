#include <Foundation/Math/Math.h>

W_ALWAYS_INLINE WInt32 WAtomicUtils::Read(const WInt32& src)
{
  return __sync_fetch_and_or(const_cast<WInt32*>(&src), 0);
}

W_ALWAYS_INLINE WInt64 WAtomicUtils::Read(const WInt64& src)
{
  return __sync_fetch_and_or_8(const_cast<WInt64*>(&src), 0);
}

W_ALWAYS_INLINE WInt32 WAtomicUtils::Increment(WInt32& dest)
{
  return __sync_add_and_fetch(&dest, 1);
}

W_ALWAYS_INLINE WInt64 WAtomicUtils::Increment(WInt64& dest)
{
  return __sync_add_and_fetch_8(&dest, 1);
}


W_ALWAYS_INLINE WInt32 WAtomicUtils::Decrement(WInt32& dest)
{
  return __sync_sub_and_fetch(&dest, 1);
}

W_ALWAYS_INLINE WInt64 WAtomicUtils::Decrement(WInt64& dest)
{
  return __sync_sub_and_fetch_8(&dest, 1);
}

W_ALWAYS_INLINE WInt32 WAtomicUtils::PostIncrement(WInt32& dest)
{
  return __sync_fetch_and_add(&dest, 1);
}

W_ALWAYS_INLINE WInt64 WAtomicUtils::PostIncrement(WInt64& dest)
{
  return __sync_fetch_and_add_8(&dest, 1);
}


W_ALWAYS_INLINE WInt32 WAtomicUtils::PostDecrement(WInt32& dest)
{
  return __sync_fetch_and_sub(&dest, 1);
}

W_ALWAYS_INLINE WInt64 WAtomicUtils::PostDecrement(WInt64& dest)
{
  return __sync_fetch_and_sub_8(&dest, 1);
}

W_ALWAYS_INLINE void WAtomicUtils::Add(WInt32& dest, WInt32 value)
{
  __sync_fetch_and_add(&dest, value);
}

W_ALWAYS_INLINE void WAtomicUtils::Add(WInt64& dest, WInt64 value)
{
  __sync_fetch_and_add_8(&dest, value);
}


W_ALWAYS_INLINE void WAtomicUtils::And(WInt32& dest, WInt32 value)
{
  __sync_fetch_and_and(&dest, value);
}

W_ALWAYS_INLINE void WAtomicUtils::And(WInt64& dest, WInt64 value)
{
  __sync_fetch_and_and_8(&dest, value);
}


W_ALWAYS_INLINE void WAtomicUtils::Or(WInt32& dest, WInt32 value)
{
  __sync_fetch_and_or(&dest, value);
}

W_ALWAYS_INLINE void WAtomicUtils::Or(WInt64& dest, WInt64 value)
{
  __sync_fetch_and_or_8(&dest, value);
}


W_ALWAYS_INLINE void WAtomicUtils::Xor(WInt32& dest, WInt32 value)
{
  __sync_fetch_and_xor(&dest, value);
}

W_ALWAYS_INLINE void WAtomicUtils::Xor(WInt64& dest, WInt64 value)
{
  __sync_fetch_and_xor_8(&dest, value);
}


W_FORCE_INLINE void WAtomicUtils::Min(WInt32& dest, WInt32 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    WInt32 iOldValue = dest;
    WInt32 iNewValue = WMath::Min(iOldValue, value);

    if (__sync_bool_compare_and_swap(&dest, iOldValue, iNewValue))
      break;
  }
}

W_FORCE_INLINE void WAtomicUtils::Min(WInt64& dest, WInt64 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    WInt64 iOldValue = dest;
    WInt64 iNewValue = WMath::Min(iOldValue, value);

    if (__sync_bool_compare_and_swap_8(&dest, iOldValue, iNewValue))
      break;
  }
}


W_FORCE_INLINE void WAtomicUtils::Max(WInt32& dest, WInt32 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    WInt32 iOldValue = dest;
    WInt32 iNewValue = WMath::Max(iOldValue, value);

    if (__sync_bool_compare_and_swap(&dest, iOldValue, iNewValue))
      break;
  }
}

W_FORCE_INLINE void WAtomicUtils::Max(WInt64& dest, WInt64 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    WInt64 iOldValue = dest;
    WInt64 iNewValue = WMath::Max(iOldValue, value);

    if (__sync_bool_compare_and_swap_8(&dest, iOldValue, iNewValue))
      break;
  }
}


W_ALWAYS_INLINE WInt32 WAtomicUtils::Set(WInt32& dest, WInt32 value)
{
  return __sync_lock_test_and_set(&dest, value);
}

W_ALWAYS_INLINE WInt64 WAtomicUtils::Set(WInt64& dest, WInt64 value)
{
  return __sync_lock_test_and_set_8(&dest, value);
}


W_ALWAYS_INLINE bool WAtomicUtils::TestAndSet(WInt32& dest, WInt32 expected, WInt32 value)
{
  return __sync_bool_compare_and_swap(&dest, expected, value);
}

W_ALWAYS_INLINE bool WAtomicUtils::TestAndSet(WInt64& dest, WInt64 expected, WInt64 value)
{
  return __sync_bool_compare_and_swap_8(&dest, expected, value);
}

W_ALWAYS_INLINE bool WAtomicUtils::TestAndSet(void** dest, void* expected, void* value)
{
#if W_ENABLED(W_PLATFORM_64BIT)
  WUInt64* puiTemp = reinterpret_cast<WUInt64*>(dest);
  return __sync_bool_compare_and_swap(puiTemp, reinterpret_cast<WUInt64>(expected), reinterpret_cast<WUInt64>(value));
#else
  WUInt32* puiTemp = reinterpret_cast<WUInt32*>(dest);
  return __sync_bool_compare_and_swap(puiTemp, reinterpret_cast<WUInt32>(expected), reinterpret_cast<WUInt32>(value));
#endif
}

W_ALWAYS_INLINE WInt32 WAtomicUtils::CompareAndSwap(WInt32& dest, WInt32 expected, WInt32 value)
{
  return __sync_val_compare_and_swap(&dest, expected, value);
}

W_ALWAYS_INLINE WInt64 WAtomicUtils::CompareAndSwap(WInt64& dest, WInt64 expected, WInt64 value)
{
  return __sync_val_compare_and_swap_8(&dest, expected, value);
}
