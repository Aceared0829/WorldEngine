#pragma once

template <typename Type>
W_ALWAYS_INLINE WVec2Template<Type>::WVec2Template()
{
#if W_ENABLED(W_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const Type TypeNaN = WMath::NaN<Type>();
  x = TypeNaN;
  y = TypeNaN;
#endif
}

template <typename Type>
W_ALWAYS_INLINE WVec2Template<Type>::WVec2Template(Type x, Type y)
  : x(x)
  , y(y)
{
}

template <typename Type>
W_ALWAYS_INLINE WVec2Template<Type>::WVec2Template(Type v)
  : x(v)
  , y(v)
{
}

template <typename Type>
W_ALWAYS_INLINE void WVec2Template<Type>::Set(Type xy)
{
  x = xy;
  y = xy;
}

template <typename Type>
W_ALWAYS_INLINE void WVec2Template<Type>::Set(Type inX, Type inY)
{
  x = inX;
  y = inY;
}

template <typename Type>
W_ALWAYS_INLINE void WVec2Template<Type>::SetZero()
{
  x = y = 0;
}

template <typename Type>
W_IMPLEMENT_IF_FLOAT_TYPE W_ALWAYS_INLINE Type WVec2Template<Type>::GetLength() const
{
  return (WMath::Sqrt(GetLengthSquared()));
}

template <typename Type>
W_IMPLEMENT_IF_FLOAT_TYPE W_ALWAYS_INLINE Type WVec2Template<Type>::GetDistanceTo(const WVec2Template<Type>& rhs) const
{
  return (*this - rhs).GetLength();
}

template <typename Type>
W_IMPLEMENT_IF_FLOAT_TYPE W_ALWAYS_INLINE Type WVec2Template<Type>::GetSquaredDistanceTo(const WVec2Template<Type>& rhs) const
{
  return (*this - rhs).GetLengthSquared();
}

template <typename Type>
W_IMPLEMENT_IF_FLOAT_TYPE WResult WVec2Template<Type>::SetLength(Type fNewLength, Type fEpsilon /* = WMath::DefaultEpsilon<Type>() */)
{
  if (NormalizeIfNotZero(WVec2Template<Type>::MakeZero(), fEpsilon) == W_FAILURE)
    return W_FAILURE;

  *this *= fNewLength;
  return W_SUCCESS;
}

template <typename Type>
W_ALWAYS_INLINE Type WVec2Template<Type>::GetLengthSquared() const
{
  return (x * x + y * y);
}

template <typename Type>
W_IMPLEMENT_IF_FLOAT_TYPE W_FORCE_INLINE Type WVec2Template<Type>::GetLengthAndNormalize()
{
  const Type fLength = GetLength();
  *this /= fLength;
  return fLength;
}

template <typename Type>
W_IMPLEMENT_IF_FLOAT_TYPE W_FORCE_INLINE const WVec2Template<Type> WVec2Template<Type>::GetNormalized() const
{
  const Type fLen = GetLength();

  const Type fLengthInv = WMath::Invert(fLen);
  return WVec2Template<Type>(x * fLengthInv, y * fLengthInv);
}

template <typename Type>
W_IMPLEMENT_IF_FLOAT_TYPE W_ALWAYS_INLINE void WVec2Template<Type>::Normalize()
{
  *this /= GetLength();
}

template <typename Type>
W_IMPLEMENT_IF_FLOAT_TYPE inline WResult WVec2Template<Type>::NormalizeIfNotZero(const WVec2Template<Type>& vFallback, Type fEpsilon)
{
  W_NAN_ASSERT(&vFallback);

  const Type fLength = GetLength();

  if (!WMath::IsFinite(fLength) || WMath::IsZero(fLength, fEpsilon))
  {
    *this = vFallback;
    return W_FAILURE;
  }

  *this /= fLength;
  return W_SUCCESS;
}

/*! \note Normalization, especially with SSE is not very precise. So this function checks whether the (squared)
  length is between a lower and upper limit.
*/
template <typename Type>
W_IMPLEMENT_IF_FLOAT_TYPE inline bool WVec2Template<Type>::IsNormalized(Type fEpsilon /* = WMath::HugeEpsilon<Type>() */) const
{
  const Type t = GetLengthSquared();
  return WMath::IsEqual(t, (Type)(1), fEpsilon);
}

template <typename Type>
inline bool WVec2Template<Type>::IsZero() const
{
  return (x == 0 && y == 0);
}

template <typename Type>
inline bool WVec2Template<Type>::IsZero(Type fEpsilon) const
{
  W_NAN_ASSERT(this);

  return (WMath::IsZero(x, fEpsilon) && WMath::IsZero(y, fEpsilon));
}

template <typename Type>
inline bool WVec2Template<Type>::IsNaN() const
{
  if (WMath::IsNaN(x))
    return true;
  if (WMath::IsNaN(y))
    return true;

  return false;
}

template <typename Type>
inline bool WVec2Template<Type>::IsValid() const
{
  if (!WMath::IsFinite(x))
    return false;
  if (!WMath::IsFinite(y))
    return false;

  return true;
}

template <typename Type>
W_FORCE_INLINE const WVec2Template<Type> WVec2Template<Type>::operator-() const
{
  W_NAN_ASSERT(this);

  return WVec2Template<Type>(-x, -y);
}

template <typename Type>
W_FORCE_INLINE void WVec2Template<Type>::operator+=(const WVec2Template<Type>& rhs)
{
  x += rhs.x;
  y += rhs.y;

  W_NAN_ASSERT(this);
}

template <typename Type>
W_FORCE_INLINE void WVec2Template<Type>::operator-=(const WVec2Template<Type>& rhs)
{
  x -= rhs.x;
  y -= rhs.y;

  W_NAN_ASSERT(this);
}

template <typename Type>
W_FORCE_INLINE void WVec2Template<Type>::operator*=(Type f)
{
  x *= f;
  y *= f;

  W_NAN_ASSERT(this);
}

template <typename Type>
W_FORCE_INLINE void WVec2Template<Type>::operator/=(Type f)
{
  if constexpr (std::is_floating_point_v<Type>)
  {
    const Type f_inv = WMath::Invert(f);
    x *= f_inv;
    y *= f_inv;
  }
  else
  {
    x /= f;
    y /= f;
  }

  W_NAN_ASSERT(this);
}

template <typename Type>
W_IMPLEMENT_IF_FLOAT_TYPE inline void WVec2Template<Type>::MakeOrthogonalTo(const WVec2Template<Type>& vNormal)
{
  W_ASSERT_DEBUG(vNormal.IsNormalized(), "The normal must be normalized.");

  const Type fDot = this->Dot(vNormal);
  *this -= fDot * vNormal;
}

template <typename Type>
W_FORCE_INLINE const WVec2Template<Type> WVec2Template<Type>::GetOrthogonalVector() const
{
  W_NAN_ASSERT(this);
  W_ASSERT_DEBUG(!IsZero(WMath::SmallEpsilon<Type>()), "The vector must not be zero to be able to compute an orthogonal vector.");

  return WVec2Template<Type>(-y, x);
}

template <typename Type>
W_IMPLEMENT_IF_FLOAT_TYPE inline const WVec2Template<Type> WVec2Template<Type>::GetReflectedVector(const WVec2Template<Type>& vNormal) const
{
  W_ASSERT_DEBUG(vNormal.IsNormalized(), "vNormal must be normalized.");

  return ((*this) - (2 * this->Dot(vNormal) * vNormal));
}

template <typename Type>
W_FORCE_INLINE Type WVec2Template<Type>::Dot(const WVec2Template<Type>& rhs) const
{
  W_NAN_ASSERT(this);
  W_NAN_ASSERT(&rhs);

  return ((x * rhs.x) + (y * rhs.y));
}

template <typename Type>
inline WAngleTemplate<Type> WVec2Template<Type>::GetAngleBetween(const WVec2Template<Type>& rhs) const
{
  W_ASSERT_DEBUG(this->IsNormalized(), "This vector must be normalized.");
  W_ASSERT_DEBUG(rhs.IsNormalized(), "The other vector must be normalized.");

  return WMath::ACos(static_cast<Type>(WMath::Clamp<Type>(this->Dot(rhs), (Type)-1, (Type)1)));
}

template <typename Type>
W_FORCE_INLINE const WVec2Template<Type> WVec2Template<Type>::CompMin(const WVec2Template<Type>& rhs) const
{
  W_NAN_ASSERT(this);
  W_NAN_ASSERT(&rhs);

  return WVec2Template<Type>(WMath::Min(x, rhs.x), WMath::Min(y, rhs.y));
}

template <typename Type>
W_FORCE_INLINE const WVec2Template<Type> WVec2Template<Type>::CompMax(const WVec2Template<Type>& rhs) const
{
  W_NAN_ASSERT(this);
  W_NAN_ASSERT(&rhs);

  return WVec2Template<Type>(WMath::Max(x, rhs.x), WMath::Max(y, rhs.y));
}

template <typename Type>
W_FORCE_INLINE const WVec2Template<Type> WVec2Template<Type>::CompClamp(const WVec2Template<Type>& vLow, const WVec2Template<Type>& vHigh) const
{
  W_NAN_ASSERT(this);
  W_NAN_ASSERT(&vLow);
  W_NAN_ASSERT(&vHigh);

  return WVec2Template<Type>(WMath::Clamp(x, vLow.x, vHigh.x), WMath::Clamp(y, vLow.y, vHigh.y));
}

template <typename Type>
W_FORCE_INLINE const WVec2Template<Type> WVec2Template<Type>::CompMul(const WVec2Template<Type>& rhs) const
{
  W_NAN_ASSERT(this);
  W_NAN_ASSERT(&rhs);

  return WVec2Template<Type>(x * rhs.x, y * rhs.y);
}

template <typename Type>
W_FORCE_INLINE const WVec2Template<Type> WVec2Template<Type>::CompDiv(const WVec2Template<Type>& rhs) const
{
  W_NAN_ASSERT(this);
  W_NAN_ASSERT(&rhs);

  return WVec2Template<Type>(x / rhs.x, y / rhs.y);
}

template <typename Type>
inline const WVec2Template<Type> WVec2Template<Type>::Abs() const
{
  W_NAN_ASSERT(this);

  return WVec2Template<Type>(WMath::Abs(x), WMath::Abs(y));
}

template <typename Type>
W_FORCE_INLINE const WVec2Template<Type> operator+(const WVec2Template<Type>& v1, const WVec2Template<Type>& v2)
{
  W_NAN_ASSERT(&v1);
  W_NAN_ASSERT(&v2);

  return WVec2Template<Type>(v1.x + v2.x, v1.y + v2.y);
}

template <typename Type>
W_FORCE_INLINE const WVec2Template<Type> operator-(const WVec2Template<Type>& v1, const WVec2Template<Type>& v2)
{
  W_NAN_ASSERT(&v1);
  W_NAN_ASSERT(&v2);

  return WVec2Template<Type>(v1.x - v2.x, v1.y - v2.y);
}

template <typename Type>
W_FORCE_INLINE const WVec2Template<Type> operator*(Type f, const WVec2Template<Type>& v)
{
  W_NAN_ASSERT(&v);

  return WVec2Template<Type>(v.x * f, v.y * f);
}

template <typename Type>
W_FORCE_INLINE const WVec2Template<Type> operator*(const WVec2Template<Type>& v, Type f)
{
  W_NAN_ASSERT(&v);

  return WVec2Template<Type>(v.x * f, v.y * f);
}

template <typename Type>
W_FORCE_INLINE const WVec2Template<Type> operator/(const WVec2Template<Type>& v, Type f)
{
  W_NAN_ASSERT(&v);

  if constexpr (std::is_floating_point_v<Type>)
  {
    // multiplication is much faster than division
    const Type f_inv = WMath::Invert(f);
    return WVec2Template<Type>(v.x * f_inv, v.y * f_inv);
  }
  else
  {
    return WVec2Template<Type>(v.x / f, v.y / f);
  }
}

template <typename Type>
inline bool WVec2Template<Type>::IsIdentical(const WVec2Template<Type>& rhs) const
{
  W_NAN_ASSERT(this);
  W_NAN_ASSERT(&rhs);

  return ((x == rhs.x) && (y == rhs.y));
}

template <typename Type>
inline bool WVec2Template<Type>::IsEqual(const WVec2Template<Type>& rhs, Type fEpsilon) const
{
  W_NAN_ASSERT(this);
  W_NAN_ASSERT(&rhs);

  return (WMath::IsEqual(x, rhs.x, fEpsilon) && WMath::IsEqual(y, rhs.y, fEpsilon));
}

template <typename Type>
W_FORCE_INLINE bool operator==(const WVec2Template<Type>& v1, const WVec2Template<Type>& v2)
{
  return v1.IsIdentical(v2);
}

template <typename Type>
W_FORCE_INLINE bool operator!=(const WVec2Template<Type>& v1, const WVec2Template<Type>& v2)
{
  return !v1.IsIdentical(v2);
}

template <typename Type>
W_FORCE_INLINE bool operator<(const WVec2Template<Type>& v1, const WVec2Template<Type>& v2)
{
  W_NAN_ASSERT(&v1);
  W_NAN_ASSERT(&v2);

  if (v1.x < v2.x)
    return true;
  if (v1.x > v2.x)
    return false;

  return (v1.y < v2.y);
}
