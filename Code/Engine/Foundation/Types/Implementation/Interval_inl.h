
template <class Type>
constexpr WInterval<Type>::WInterval(Type startAndEndValue)
  : m_StartValue(startAndEndValue)
  , m_EndValue(startAndEndValue)
{
}

template <class Type>
constexpr WInterval<Type>::WInterval(Type start, Type end)
  : m_StartValue(start)
  , m_EndValue(WMath::Max(start, end))
{
}

template <class Type>
void WInterval<Type>::SetStartAdjustEnd(Type value)
{
  m_StartValue = value;
  m_EndValue = WMath::Max(m_EndValue, m_StartValue);
}

template <class Type>
void WInterval<Type>::SetEndAdjustStart(Type value)
{
  m_EndValue = value;
  m_StartValue = WMath::Min(m_StartValue, m_EndValue);
}

template <class Type>
void WInterval<Type>::ClampToIntervalAdjustEnd(Type minValue, Type maxValue, Type minimumSeparation /*= Type()*/)
{
  // clamp the start value to the valid range, leave minimumSeparation at the end
  m_StartValue = WMath::Clamp(m_StartValue, minValue, maxValue - minimumSeparation);

  // clamp the start value to the remaining range
  m_EndValue = WMath::Clamp(m_EndValue, m_StartValue, maxValue);
}

template <class Type>
void WInterval<Type>::ClampToIntervalAdjustStart(Type minValue, Type maxValue, Type minimumSeparation /*= Type()*/)
{
  // clamp the end value to the valid range, leave minimumSeparation at the start
  m_EndValue = WMath::Clamp(m_EndValue, minValue + minimumSeparation, maxValue);

  // clamp the start value to the remaining range
  m_StartValue = WMath::Clamp(m_StartValue, minValue, m_EndValue);
}

template <class Type>
Type WInterval<Type>::GetSeparation() const
{
  return m_EndValue - m_StartValue;
}

template <class Type>
bool WInterval<Type>::operator==(const WInterval<Type>& rhs) const
{
  return m_StartValue == rhs.m_StartValue && m_EndValue == rhs.m_EndValue;
}

template <class Type>
bool WInterval<Type>::operator!=(const WInterval<Type>& rhs) const
{
  return !operator==(rhs);
}
