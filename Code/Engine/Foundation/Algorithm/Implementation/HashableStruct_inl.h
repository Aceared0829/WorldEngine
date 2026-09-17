#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Memory/MemoryUtils.h>

template <typename T>
W_ALWAYS_INLINE WHashableStruct<T>::WHashableStruct()
{
  WMemoryUtils::ZeroFill<T>(static_cast<T*>(this), 1);
}

template <typename T>
W_ALWAYS_INLINE WHashableStruct<T>::WHashableStruct(const WHashableStruct<T>& other)
{
  WMemoryUtils::RawByteCopy(this, &other, sizeof(T));
}

template <typename T>
W_ALWAYS_INLINE void WHashableStruct<T>::operator=(const WHashableStruct<T>& other)
{
  if (this != &other)
  {
    WMemoryUtils::RawByteCopy(this, &other, sizeof(T));
  }
}
template <typename T>
bool WHashableStruct<T>::operator==(const WHashableStruct<T>& other) const
{
  return WMemoryUtils::RawByteCompare(this, &other, sizeof(T)) == 0;
}

template <typename T>
bool WHashableStruct<T>::operator!=(const WHashableStruct<T>& other) const
{
  return WMemoryUtils::RawByteCompare(this, &other, sizeof(T)) != 0;
}

template <typename T>
bool WHashableStruct<T>::operator<(const WHashableStruct<T>& other) const
{
  return WMemoryUtils::RawByteCompare(this, &other, sizeof(T)) < 0;
}

template <typename T>
W_ALWAYS_INLINE WUInt32 WHashableStruct<T>::CalculateHash() const
{
  return WHashingUtils::xxHash32(this, sizeof(T));
}
