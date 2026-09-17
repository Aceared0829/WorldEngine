#pragma once

template <typename Type>
constexpr W_ALWAYS_INLINE Type WAngleTemplate<Type>::Pi()
{
  return static_cast<Type>(3.1415926535897932384626433832795);
}

template <typename Type>
constexpr W_ALWAYS_INLINE Type WAngleTemplate<Type>::DegToRadMultiplier()
{
  return Pi() / (Type)180;
}

template <typename Type>
constexpr W_ALWAYS_INLINE Type WAngleTemplate<Type>::RadToDegMultiplier()
{
  return ((Type)180) / Pi();
}

template <typename Type>
constexpr Type WAngleTemplate<Type>::DegToRad(Type f)
{
  return f * DegToRadMultiplier();
}

template <typename Type>
constexpr Type WAngleTemplate<Type>::RadToDeg(Type f)
{
  return f * RadToDegMultiplier();
}

template <typename Type>
constexpr inline WAngleTemplate<Type> WAngleTemplate<Type>::MakeFromDegree(Type fDegree)
{
  return WAngleTemplate<Type>(DegToRad(fDegree));
}

template <typename Type>
constexpr W_ALWAYS_INLINE WAngleTemplate<Type> WAngleTemplate<Type>::MakeFromRadian(Type fRadian)
{
  return WAngleTemplate<Type>(fRadian);
}

template <typename Type>
constexpr inline Type WAngleTemplate<Type>::GetDegree() const
{
  return RadToDeg(m_fRadian);
}

template <typename Type>
constexpr W_ALWAYS_INLINE Type WAngleTemplate<Type>::GetRadian() const
{
  return m_fRadian;
}

template <typename Type>
inline WAngleTemplate<Type> WAngleTemplate<Type>::GetNormalizedRange() const
{
  WAngleTemplate<Type> out(m_fRadian);
  out.NormalizeRange();
  return out;
}

template <typename Type>
inline bool WAngleTemplate<Type>::IsEqualSimple(WAngleTemplate<Type> rhs, WAngleTemplate<Type> epsilon) const
{
  const WAngleTemplate<Type> diff = AngleBetween(*this, rhs);

  return ((diff.m_fRadian >= -epsilon.m_fRadian) && (diff.m_fRadian <= epsilon.m_fRadian));
}

template <typename Type>
inline bool WAngleTemplate<Type>::IsEqualNormalized(WAngleTemplate<Type> rhs, WAngleTemplate<Type> epsilon) const
{
  // equality between normalized angles
  const WAngleTemplate<Type> aNorm = GetNormalizedRange();
  const WAngleTemplate<Type> bNorm = rhs.GetNormalizedRange();

  return aNorm.IsEqualSimple(bNorm, epsilon);
}

template <typename Type>
constexpr W_ALWAYS_INLINE WAngleTemplate<Type> WAngleTemplate<Type>::operator-() const
{
  return WAngleTemplate<Type>(-m_fRadian);
}

template <typename Type>
W_ALWAYS_INLINE void WAngleTemplate<Type>::operator+=(WAngleTemplate<Type> r)
{
  m_fRadian += r.m_fRadian;
}

template <typename Type>
W_ALWAYS_INLINE void WAngleTemplate<Type>::operator-=(WAngleTemplate<Type> r)
{
  m_fRadian -= r.m_fRadian;
}

template <typename Type>
constexpr inline WAngleTemplate<Type> WAngleTemplate<Type>::operator+(WAngleTemplate<Type> r) const
{
  return WAngleTemplate<Type>(m_fRadian + r.m_fRadian);
}

template <typename Type>
constexpr inline WAngleTemplate<Type> WAngleTemplate<Type>::operator-(WAngleTemplate<Type> r) const
{
  return WAngleTemplate<Type>(m_fRadian - r.m_fRadian);
}

template <typename Type>
constexpr W_ALWAYS_INLINE bool WAngleTemplate<Type>::operator==(const WAngleTemplate<Type>& r) const
{
  return m_fRadian == r.m_fRadian;
}

template <typename Type>
constexpr W_ALWAYS_INLINE bool WAngleTemplate<Type>::operator!=(const WAngleTemplate<Type>& r) const
{
  return m_fRadian != r.m_fRadian;
}

template <typename Type>
constexpr W_ALWAYS_INLINE bool WAngleTemplate<Type>::operator<(const WAngleTemplate<Type>& r) const
{
  return m_fRadian < r.m_fRadian;
}

template <typename Type>
constexpr W_ALWAYS_INLINE bool WAngleTemplate<Type>::operator>(const WAngleTemplate<Type>& r) const
{
  return m_fRadian > r.m_fRadian;
}

template <typename Type>
constexpr W_ALWAYS_INLINE bool WAngleTemplate<Type>::operator<=(const WAngleTemplate<Type>& r) const
{
  return m_fRadian <= r.m_fRadian;
}

template <typename Type>
constexpr W_ALWAYS_INLINE bool WAngleTemplate<Type>::operator>=(const WAngleTemplate<Type>& r) const
{
  return m_fRadian >= r.m_fRadian;
}

template <typename Type>
constexpr inline WAngleTemplate<Type> operator*(const WAngleTemplate<Type>& a, Type f)
{
  return WAngleTemplate<Type>::MakeFromRadian(a.GetRadian() * f);
}

template <typename Type>
constexpr inline WAngleTemplate<Type> operator*(Type f, const WAngleTemplate<Type>& a)
{
  return WAngleTemplate<Type>::MakeFromRadian(a.GetRadian() * f);
}

template <typename Type>
constexpr inline WAngleTemplate<Type> operator/(const WAngleTemplate<Type>& a, Type f)
{
  return WAngleTemplate<Type>::MakeFromRadian(a.GetRadian() / f);
}

template <typename Type>
constexpr inline Type operator/(const WAngleTemplate<Type>& a, const WAngleTemplate<Type>& b)
{
  return a.GetRadian() / b.GetRadian();
}

template <typename Type>
constexpr inline WAngleTemplate<Type> WAngleTemplate<Type>::AngleBetween(WAngleTemplate<Type> a, WAngleTemplate<Type> b)
{
  // taken from http://gamedev.stackexchange.com/questions/4467/comparing-angles-and-working-out-the-difference

  return WAngleTemplate<Type>(Pi() - WMath::Abs(WMath::Abs(a.GetRadian() - b.GetRadian()) - Pi()));
}



template <typename Type>
inline void WAngleTemplate<Type>::NormalizeRange()
{
  constexpr Type fTwoPi = Type(2.0) * Pi();
  constexpr Type fTwoPiTen = Type(10.0) * Pi();

  if (m_fRadian > fTwoPiTen || m_fRadian < -fTwoPiTen)
  {
    m_fRadian = WMath::Mod(m_fRadian, fTwoPi);
  }

  while (m_fRadian >= fTwoPi)
  {
    m_fRadian -= fTwoPi;
  }

  while (m_fRadian < Type(0.0))
  {
    m_fRadian += fTwoPi;
  }
}
