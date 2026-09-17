
W_ALWAYS_INLINE WRational::WRational()

  = default;

W_ALWAYS_INLINE WRational::WRational(WUInt32 uiNumerator, WUInt32 uiDenominator)
  : m_uiNumerator(uiNumerator)
  , m_uiDenominator(uiDenominator)
{
}

W_ALWAYS_INLINE bool WRational::IsIntegral() const
{
  if (m_uiNumerator == 0 && m_uiDenominator == 0)
    return true;

  return ((m_uiNumerator / m_uiDenominator) * m_uiDenominator) == m_uiNumerator;
}

W_ALWAYS_INLINE bool WRational::operator==(const WRational& other) const
{
  return m_uiNumerator == other.m_uiNumerator && m_uiDenominator == other.m_uiDenominator;
}

W_ALWAYS_INLINE bool WRational::operator!=(const WRational& other) const
{
  return m_uiNumerator != other.m_uiNumerator || m_uiDenominator != other.m_uiDenominator;
}

W_ALWAYS_INLINE WUInt32 WRational::GetNumerator() const
{
  return m_uiNumerator;
}

W_ALWAYS_INLINE WUInt32 WRational::GetDenominator() const
{
  return m_uiDenominator;
}

W_ALWAYS_INLINE WUInt32 WRational::GetIntegralResult() const
{
  if (m_uiNumerator == 0 && m_uiDenominator == 0)
    return 0;

  return m_uiNumerator / m_uiDenominator;
}

W_ALWAYS_INLINE double WRational::GetFloatingPointResult() const
{
  if (m_uiNumerator == 0 && m_uiDenominator == 0)
    return 0.0;

  return static_cast<double>(m_uiNumerator) / static_cast<double>(m_uiDenominator);
}

W_ALWAYS_INLINE bool WRational::IsValid() const
{
  return m_uiDenominator != 0 || (m_uiNumerator == 0 && m_uiDenominator == 0);
}

W_ALWAYS_INLINE WRational WRational::ReduceIntegralFraction() const
{
  W_ASSERT_DEV(IsValid() && IsIntegral(), "ReduceIntegralFraction can only be called on valid, integral rational numbers");

  return WRational(m_uiNumerator / m_uiDenominator, 1);
}
