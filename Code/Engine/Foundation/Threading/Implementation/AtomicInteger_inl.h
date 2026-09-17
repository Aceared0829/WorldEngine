
template <typename T>
W_ALWAYS_INLINE WAtomicInteger<T>::WAtomicInteger()
  : m_Value(0)
{
}

template <typename T>
W_ALWAYS_INLINE WAtomicInteger<T>::WAtomicInteger(T value)
  : m_Value(static_cast<UnderlyingType>(value))
{
}

template <typename T>
W_ALWAYS_INLINE WAtomicInteger<T>::WAtomicInteger(const WAtomicInteger<T>& value)
  : m_Value(WAtomicUtils::Read(value.m_Value))
{
}

template <typename T>
W_ALWAYS_INLINE WAtomicInteger<T>& WAtomicInteger<T>::operator=(const T value)
{
  Set(value);
  return *this;
}

template <typename T>
W_ALWAYS_INLINE WAtomicInteger<T>& WAtomicInteger<T>::operator=(const WAtomicInteger<T>& value)
{
  Set(WAtomicUtils::Read(value.m_Value));
  return *this;
}

template <typename T>
W_ALWAYS_INLINE T WAtomicInteger<T>::Increment()
{
  return static_cast<T>(WAtomicUtils::Increment(m_Value));
}

template <typename T>
W_ALWAYS_INLINE T WAtomicInteger<T>::Decrement()
{
  return static_cast<T>(WAtomicUtils::Decrement(m_Value));
}

template <typename T>
W_ALWAYS_INLINE T WAtomicInteger<T>::PostIncrement()
{
  return static_cast<T>(WAtomicUtils::PostIncrement(m_Value));
}

template <typename T>
W_ALWAYS_INLINE T WAtomicInteger<T>::PostDecrement()
{
  return static_cast<T>(WAtomicUtils::PostDecrement(m_Value));
}

template <typename T>
W_ALWAYS_INLINE void WAtomicInteger<T>::Add(T x)
{
  WAtomicUtils::Add(m_Value, static_cast<UnderlyingType>(x));
}

template <typename T>
W_ALWAYS_INLINE void WAtomicInteger<T>::Subtract(T x)
{
  WAtomicUtils::Add(m_Value, -static_cast<UnderlyingType>(x));
}

template <typename T>
W_ALWAYS_INLINE void WAtomicInteger<T>::And(T x)
{
  WAtomicUtils::And(m_Value, static_cast<UnderlyingType>(x));
}

template <typename T>
W_ALWAYS_INLINE void WAtomicInteger<T>::Or(T x)
{
  WAtomicUtils::Or(m_Value, static_cast<UnderlyingType>(x));
}

template <typename T>
W_ALWAYS_INLINE void WAtomicInteger<T>::Xor(T x)
{
  WAtomicUtils::Xor(m_Value, static_cast<UnderlyingType>(x));
}

template <typename T>
W_ALWAYS_INLINE void WAtomicInteger<T>::Min(T x)
{
  WAtomicUtils::Min(m_Value, static_cast<UnderlyingType>(x));
}

template <typename T>
W_ALWAYS_INLINE void WAtomicInteger<T>::Max(T x)
{
  WAtomicUtils::Max(m_Value, static_cast<UnderlyingType>(x));
}

template <typename T>
W_ALWAYS_INLINE T WAtomicInteger<T>::Set(T x)
{
  return static_cast<T>(WAtomicUtils::Set(m_Value, static_cast<UnderlyingType>(x)));
}

template <typename T>
W_ALWAYS_INLINE bool WAtomicInteger<T>::TestAndSet(T expected, T x)
{
  return WAtomicUtils::TestAndSet(m_Value, static_cast<UnderlyingType>(expected), static_cast<UnderlyingType>(x));
}

template <typename T>
W_ALWAYS_INLINE T WAtomicInteger<T>::CompareAndSwap(T expected, T x)
{
  return static_cast<T>(WAtomicUtils::CompareAndSwap(m_Value, static_cast<UnderlyingType>(expected), static_cast<UnderlyingType>(x)));
}

template <typename T>
W_ALWAYS_INLINE WAtomicInteger<T>::operator T() const
{
  return static_cast<T>(WAtomicUtils::Read(m_Value));
}

//////////////////////////////////////////////////////////////////////////

W_ALWAYS_INLINE WAtomicBool::WAtomicBool() = default;
W_ALWAYS_INLINE WAtomicBool::~WAtomicBool() = default;

W_ALWAYS_INLINE WAtomicBool::WAtomicBool(bool value)
{
  Set(value);
}

W_ALWAYS_INLINE WAtomicBool::WAtomicBool(const WAtomicBool& rhs)
{
  Set(static_cast<bool>(rhs));
}

W_ALWAYS_INLINE bool WAtomicBool::Set(bool value)
{
  return m_iAtomicInt.Set(value ? 1 : 0) != 0;
}

W_ALWAYS_INLINE void WAtomicBool::operator=(bool value)
{
  Set(value);
}

W_ALWAYS_INLINE void WAtomicBool::operator=(const WAtomicBool& rhs)
{
  Set(static_cast<bool>(rhs));
}

W_ALWAYS_INLINE WAtomicBool::operator bool() const
{
  return static_cast<WInt32>(m_iAtomicInt) != 0;
}

W_ALWAYS_INLINE bool WAtomicBool::TestAndSet(bool bExpected, bool bNewValue)
{
  return m_iAtomicInt.TestAndSet(bExpected ? 1 : 0, bNewValue ? 1 : 0) != 0;
}
