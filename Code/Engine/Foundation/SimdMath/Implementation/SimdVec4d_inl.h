#pragma once



W_ALWAYS_INLINE WSimdVec4d::WSimdVec4d(WInternal::QuadDouble v)
{
  m_v = v;
}

W_ALWAYS_INLINE WSimdVec4d WSimdVec4d::MakeZero()
{
  return WSimdVec4d(WSimdDouble::MakeZero());
}

W_ALWAYS_INLINE WSimdVec4d WSimdVec4d::MakeNaN()
{
  return WSimdVec4d(WSimdDouble::MakeNaN());
}

template <int N>
W_ALWAYS_INLINE WSimdDouble WSimdVec4d::GetLength() const
{
  const WSimdDouble squaredLen = GetLengthSquared<N>();
  return squaredLen.GetSqrt();
}

template <int N>
W_ALWAYS_INLINE WSimdDouble WSimdVec4d::GetInvLength() const
{
  const WSimdDouble squaredLen = GetLengthSquared<N>();
  return squaredLen.GetInvSqrt();
}

template <int N>
W_ALWAYS_INLINE WSimdDouble WSimdVec4d::GetLengthSquared() const
{
  return Dot<N>(*this);
}

template <int N>
W_ALWAYS_INLINE WSimdDouble WSimdVec4d::GetLengthAndNormalize()
{
  const WSimdDouble squaredLen = GetLengthSquared<N>();
  const WSimdDouble reciprocalLen = squaredLen.GetInvSqrt();
  *this = (*this) * reciprocalLen;
  return squaredLen * reciprocalLen;
}

template <int N>
W_ALWAYS_INLINE WSimdVec4d WSimdVec4d::GetNormalized() const
{
  return (*this) * GetInvLength<N>();
}

template <int N>
W_ALWAYS_INLINE void WSimdVec4d::Normalize()
{
  *this = GetNormalized<N>();
}

template <int N>
W_ALWAYS_INLINE void WSimdVec4d::NormalizeIfNotZero(const WSimdVec4d& vFallback, const WSimdDouble& fEpsilon)
{
  WSimdVec4bWide bIsZero(IsZero<N>(fEpsilon));
  *this = Select(bIsZero, vFallback, GetNormalized<N>());
}

template <int N>
W_ALWAYS_INLINE bool WSimdVec4d::IsNormalized(const WSimdDouble& fEpsilon) const
{
  const WSimdDouble sqLength = GetLengthSquared<N>();
  return sqLength.IsEqual(1.0, fEpsilon);
}

inline WSimdDouble WSimdVec4d::GetComponent(int i) const
{
  switch (i)
  {
    case 0:
      return GetComponent<0>();

    case 1:
      return GetComponent<1>();

    case 2:
      return GetComponent<2>();

    default:
      return GetComponent<3>();
  }
}

W_ALWAYS_INLINE WSimdVec4d WSimdVec4d::Fraction() const
{
  return *this - Trunc();
}

// static
W_ALWAYS_INLINE WSimdVec4d WSimdVec4d::Lerp(const WSimdVec4d& a, const WSimdVec4d& b, const WSimdVec4d& t)
{
  return a + t.CompMul(b - a);
}

W_ALWAYS_INLINE WSimdVec4bWide WSimdVec4d::IsEqual(const WSimdVec4d& rhs, const WSimdDouble& fEpsilon) const
{
  WSimdVec4d minusEps = rhs - WSimdVec4d(fEpsilon);
  WSimdVec4d plusEps = rhs + WSimdVec4d(fEpsilon);
  return (*this >= minusEps) && (*this <= plusEps);
}

template <>
W_ALWAYS_INLINE WSimdDouble WSimdVec4d::HorizontalSum<1>() const
{
  return GetComponent<0>();
}

template <>
W_ALWAYS_INLINE WSimdDouble WSimdVec4d::HorizontalMin<1>() const
{
  return GetComponent<0>();
}

template <>
W_ALWAYS_INLINE WSimdDouble WSimdVec4d::HorizontalMax<1>() const
{
  return GetComponent<0>();
}

W_ALWAYS_INLINE WSimdVec4d WSimdVec4d::GetOrthogonalVector() const
{
  const WSimdVec4bWide bIsLessThan = Get<WSwizzle::YYYY>() < WSimdVec4d(0.99);
  return CrossRH(Select(bIsLessThan, WSimdVec4d(0.0, 1.0, 0.0, 0.0), WSimdVec4d(1.0, 0.0, 0.0, 0.0)));
}

W_ALWAYS_INLINE const WSimdVec4d operator*(const WSimdDouble& f, const WSimdVec4d& v)
{
  return v * f;
}
