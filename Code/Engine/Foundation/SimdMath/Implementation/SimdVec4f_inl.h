#pragma once

W_ALWAYS_INLINE WSimdVec4f::WSimdVec4f(WInternal::QuadFloat v)
{
  m_v = v;
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::MakeZero()
{
  return WSimdVec4f(WSimdFloat::MakeZero());
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::MakeNaN()
{
  return WSimdVec4f(WSimdFloat::MakeNaN());
}

template <int N, WMathAcc::Enum acc>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::GetLength() const
{
  const WSimdFloat squaredLen = GetLengthSquared<N>();
  return squaredLen.GetSqrt<acc>();
}

template <int N, WMathAcc::Enum acc>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::GetInvLength() const
{
  const WSimdFloat squaredLen = GetLengthSquared<N>();
  return squaredLen.GetInvSqrt<acc>();
}

template <int N>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::GetLengthSquared() const
{
  return Dot<N>(*this);
}

template <int N, WMathAcc::Enum acc>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::GetLengthAndNormalize()
{
  const WSimdFloat squaredLen = GetLengthSquared<N>();
  const WSimdFloat reciprocalLen = squaredLen.GetInvSqrt<acc>();
  *this = (*this) * reciprocalLen;
  return squaredLen * reciprocalLen;
}

template <int N, WMathAcc::Enum acc>
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::GetNormalized() const
{
  return (*this) * GetInvLength<N, acc>();
}

template <int N, WMathAcc::Enum acc>
W_ALWAYS_INLINE void WSimdVec4f::Normalize()
{
  *this = GetNormalized<N, acc>();
}

template <int N, WMathAcc::Enum acc>
W_ALWAYS_INLINE void WSimdVec4f::NormalizeIfNotZero(const WSimdVec4f& vFallback, const WSimdFloat& fEpsilon)
{
  WSimdVec4b bIsZero = IsZero<N>(fEpsilon);
  *this = Select(bIsZero, vFallback, GetNormalized<N, acc>());
}

template <int N>
W_ALWAYS_INLINE bool WSimdVec4f::IsNormalized(const WSimdFloat& fEpsilon) const
{
  const WSimdFloat sqLength = GetLengthSquared<N>();
  return sqLength.IsEqual(1.0f, fEpsilon);
}

inline WSimdFloat WSimdVec4f::GetComponent(int i) const
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

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::Fraction() const
{
  return *this - Trunc();
}

// static
W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::Lerp(const WSimdVec4f& a, const WSimdVec4f& b, const WSimdVec4f& t)
{
  return a + t.CompMul(b - a);
}

W_ALWAYS_INLINE WSimdVec4b WSimdVec4f::IsEqual(const WSimdVec4f& rhs, const WSimdFloat& fEpsilon) const
{
  WSimdVec4f minusEps = rhs - WSimdVec4f(fEpsilon);
  WSimdVec4f plusEps = rhs + WSimdVec4f(fEpsilon);
  return (*this >= minusEps) && (*this <= plusEps);
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::HorizontalSum<1>() const
{
  return GetComponent<0>();
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::HorizontalMin<1>() const
{
  return GetComponent<0>();
}

template <>
W_ALWAYS_INLINE WSimdFloat WSimdVec4f::HorizontalMax<1>() const
{
  return GetComponent<0>();
}

W_ALWAYS_INLINE WSimdVec4f WSimdVec4f::GetOrthogonalVector() const
{
  const WSimdVec4b bIsLessThan = Get<WSwizzle::YYYY>() < WSimdVec4f(0.99f);
  return CrossRH(Select(bIsLessThan, WSimdVec4f(0, 1, 0, 0), WSimdVec4f(1, 0, 0, 0)));
}

W_ALWAYS_INLINE const WSimdVec4f operator*(const WSimdFloat& f, const WSimdVec4f& v)
{
  return v * f;
}
