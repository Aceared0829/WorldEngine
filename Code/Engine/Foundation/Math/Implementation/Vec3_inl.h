#pragma once

template <typename Type>
W_FORCE_INLINE WVec3Template<Type>::WVec3Template()
{
#if W_ENABLED(W_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const Type TypeNaN = WMath::NaN<Type>();
  x = TypeNaN;
  y = TypeNaN;
  z = TypeNaN;
#endif
}

template <typename Type>
W_ALWAYS_INLINE WVec3Template<Type>::WVec3Template(Type x, Type y, Type z)
  : x(x)
  , y(y)
  , z(z)
{
}

template <typename Type>
W_ALWAYS_INLINE WVec3Template<Type>::WVec3Template(Type v)
  : x(v)
  , y(v)
  , z(v)
{
}

template <typename Type>
W_ALWAYS_INLINE void WVec3Template<Type>::Set(Type xyz)
{
  x = xyz;
  y = xyz;
  z = xyz;
}

template <typename Type>
W_ALWAYS_INLINE void WVec3Template<Type>::Set(Type inX, Type inY, Type inZ)
{
  x = inX;
  y = inY;
  z = inZ;
}

template <typename Type>
W_ALWAYS_INLINE void WVec3Template<Type>::SetZero()
{
  x = y = z = 0;
}

template <typename Type>
W_IMPLEMENT_IF_FLOAT_TYPE W_ALWAYS_INLINE Type WVec3Template<Type>::GetLength() const
{
  return (WMath::Sqrt(GetLengthSquared()));
}

template <typename Type>
W_IMPLEMENT_IF_FLOAT_TYPE W_ALWAYS_INLINE Type WVec3Template<Type>::GetDistanceTo(const WVec3Template<Type>& rhs) const
{
  return (*this - rhs).GetLength();
}

template <typename Type>
W_IMPLEMENT_IF_FLOAT_TYPE W_ALWAYS_INLINE Type WVec3Template<Type>::GetSquaredDistanceTo(const WVec3Template<Type>& rhs) const
{
  return (*this - rhs).GetLengthSquared();
}

template <typename Type>
W_IMPLEMENT_IF_FLOAT_TYPE WResult WVec3Template<Type>::SetLength(Type fNewLength, Type fEpsilon /* = WMath::DefaultEpsilon<Type>() */)
{
  if (NormalizeIfNotZero(WVec3Template<Type>::MakeZero(), fEpsilon) == W_FAILURE)
    return W_FAILURE;

  *this *= fNewLength;
  return W_SUCCESS;
}

template <typename Type>
W_FORCE_INLINE Type WVec3Template<Type>::GetLengthSquared() const
{
  W_NAN_ASSERT(this);

  return (x * x + y * y + z * z);
}

template <typename Type>
W_IMPLEMENT_IF_FLOAT_TYPE W_FORCE_INLINE Type WVec3Template<Type>::GetLengthAndNormalize()
{
  const Type fLength = GetLength();
  *this /= fLength;
  return fLength;
}

template <typename Type>
W_IMPLEMENT_IF_FLOAT_TYPE W_FORCE_INLINE const WVec3Template<Type> WVec3Template<Type>::GetNormalized() const
{
  const Type fLen = GetLength();

  const Type fLengthInv = WMath::Invert(fLen);
  return WVec3Template<Type>(x * fLengthInv, y * fLengthInv, z * fLengthInv);
}

template <typename Type>
W_IMPLEMENT_IF_FLOAT_TYPE W_ALWAYS_INLINE void WVec3Template<Type>::Normalize()
{
  *this /= GetLength();
}

template <typename Type>
W_IMPLEMENT_IF_FLOAT_TYPE WResult WVec3Template<Type>::NormalizeIfNotZero(const WVec3Template<Type>& vFallback, Type fEpsilon)
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
W_IMPLEMENT_IF_FLOAT_TYPE W_FORCE_INLINE bool WVec3Template<Type>::IsNormalized(Type fEpsilon /* = WMath::HugeEpsilon<Type>() */) const
{
  const Type t = GetLength();
  return WMath::IsEqual<Type>(t, (Type)1, fEpsilon);
}

template <typename Type>
W_FORCE_INLINE bool WVec3Template<Type>::IsZero() const
{
  W_NAN_ASSERT(this);

  return ((x == 0.0f) && (y == 0.0f) && (z == 0.0f));
}

template <typename Type>
bool WVec3Template<Type>::IsZero(Type fEpsilon) const
{
  W_NAN_ASSERT(this);

  return (WMath::IsZero(x, fEpsilon) && WMath::IsZero(y, fEpsilon) && WMath::IsZero(z, fEpsilon));
}

template <typename Type>
bool WVec3Template<Type>::IsNaN() const
{
  if (WMath::IsNaN(x))
    return true;
  if (WMath::IsNaN(y))
    return true;
  if (WMath::IsNaN(z))
    return true;

  return false;
}

template <typename Type>
bool WVec3Template<Type>::IsValid() const
{
  if (!WMath::IsFinite(x))
    return false;
  if (!WMath::IsFinite(y))
    return false;
  if (!WMath::IsFinite(z))
    return false;

  return true;
}

template <typename Type>
W_FORCE_INLINE const WVec3Template<Type> WVec3Template<Type>::operator-() const
{
  W_NAN_ASSERT(this);

  return WVec3Template<Type>(-x, -y, -z);
}

template <typename Type>
W_FORCE_INLINE void WVec3Template<Type>::operator+=(const WVec3Template<Type>& rhs)
{
  x += rhs.x;
  y += rhs.y;
  z += rhs.z;

  W_NAN_ASSERT(this);
}

template <typename Type>
W_FORCE_INLINE void WVec3Template<Type>::operator-=(const WVec3Template<Type>& rhs)
{
  x -= rhs.x;
  y -= rhs.y;
  z -= rhs.z;

  W_NAN_ASSERT(this);
}

template <typename Type>
W_FORCE_INLINE void WVec3Template<Type>::operator*=(const WVec3Template& rhs)
{
  /// \test this is new

  x *= rhs.x;
  y *= rhs.y;
  z *= rhs.z;

  W_NAN_ASSERT(this);
}

template <typename Type>
W_FORCE_INLINE void WVec3Template<Type>::operator/=(const WVec3Template& rhs)
{
  /// \test this is new

  x /= rhs.x;
  y /= rhs.y;
  z /= rhs.z;

  W_NAN_ASSERT(this);
}

template <typename Type>
W_FORCE_INLINE void WVec3Template<Type>::operator*=(Type f)
{
  x *= f;
  y *= f;
  z *= f;

  W_NAN_ASSERT(this);
}

template <typename Type>
W_FORCE_INLINE void WVec3Template<Type>::operator/=(Type f)
{
  if constexpr (std::is_floating_point_v<Type>)
  {
    const Type f_inv = WMath::Invert(f);
    x *= f_inv;
    y *= f_inv;
    z *= f_inv;
  }
  else
  {
    x /= f;
    y /= f;
    z /= f;
  }

  // if this assert fires, you might have tried to normalize a zero-length vector
  W_NAN_ASSERT(this);
}

template <typename Type>
W_IMPLEMENT_IF_FLOAT_TYPE WResult WVec3Template<Type>::CalculateNormal(const WVec3Template<Type>& v1, const WVec3Template<Type>& v2, const WVec3Template<Type>& v3)
{
  *this = (v3 - v2).CrossRH(v1 - v2);
  return NormalizeIfNotZero();
}


template <typename Type>
W_IMPLEMENT_IF_FLOAT_TYPE WVec3Template<Type> WVec3Template<Type>::MakeOrthogonalVector(const WVec3Template<Type>& vDirection, const WVec3Template<Type>& vBasis1, const WVec3Template<Type>& vBasis2)
{
  W_ASSERT_DEBUG(vDirection.IsNormalized() && vBasis1.IsNormalized() && vBasis2.IsNormalized(), "All input vectors must be normalized.");

  // do the cross product with the basis that is less similar to the direction
  if (WMath::Abs(vDirection.Dot(vBasis1)) < WMath::Abs(vDirection.Dot(vBasis2)))
  {
    return vDirection.CrossRH(vBasis1);
  }
  else
  {
    return vDirection.CrossRH(vBasis2);
  }
}

template <typename Type>
W_IMPLEMENT_IF_FLOAT_TYPE void WVec3Template<Type>::MakeOrthogonalTo(const WVec3Template<Type>& vNormal)
{
  W_ASSERT_DEBUG(vNormal.IsNormalized(), "The vector to make this vector orthogonal to, must be normalized. It's length is {0}", WArgF(vNormal.GetLength(), 3));

  WVec3Template<Type> vOrtho = vNormal.CrossRH(*this);
  *this = vOrtho.CrossRH(vNormal);
}

template <typename Type>
W_IMPLEMENT_IF_FLOAT_TYPE const WVec3Template<Type> WVec3Template<Type>::GetOrthogonalVector() const
{
  W_ASSERT_DEBUG(!IsZero(WMath::SmallEpsilon<Type>()), "The vector must not be zero to be able to compute an orthogonal vector.");

  Type fDot = WMath::Abs(this->Dot(WVec3Template<Type>(0, 1, 0)));
  if (fDot < 0.999f)
    return this->CrossRH(WVec3Template<Type>(0, 1, 0));

  return this->CrossRH(WVec3Template<Type>(1, 0, 0));
}

template <typename Type>
W_IMPLEMENT_IF_FLOAT_TYPE const WVec3Template<Type> WVec3Template<Type>::GetReflectedVector(const WVec3Template<Type>& vNormal) const
{
  W_ASSERT_DEBUG(vNormal.IsNormalized(), "vNormal must be normalized.");

  return ((*this) - ((Type)2 * this->Dot(vNormal) * vNormal));
}

template <typename Type>
W_FORCE_INLINE Type WVec3Template<Type>::Dot(const WVec3Template<Type>& rhs) const
{
  W_NAN_ASSERT(this);
  W_NAN_ASSERT(&rhs);

  return ((x * rhs.x) + (y * rhs.y) + (z * rhs.z));
}

template <typename Type>
const WVec3Template<Type> WVec3Template<Type>::CrossRH(const WVec3Template<Type>& rhs) const
{
  W_NAN_ASSERT(this);
  W_NAN_ASSERT(&rhs);

  return WVec3Template<Type>(y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z, x * rhs.y - y * rhs.x);
}

template <typename Type>
WAngleTemplate<Type> WVec3Template<Type>::GetAngleBetween(const WVec3Template<Type>& rhs) const
{
  W_ASSERT_DEBUG(this->IsNormalized(), "This vector must be normalized. Length is: {}", this->GetLength());
  W_ASSERT_DEBUG(rhs.IsNormalized(), "The other vector must be normalized. Length is: {}", rhs.GetLength());

  return WMath::ACos<Type>(static_cast<Type>(WMath::Clamp(this->Dot(rhs), (Type)-1, (Type)1)));
}

template <typename Type>
WAngleTemplate<Type> WVec3Template<Type>::GetAngleBetween(const WVec3Template<Type>& vForward, const WVec3Template<Type>& vUp) const
{
  W_ASSERT_DEBUG(this->IsNormalized(), "This vector must be normalized. Length is: {}", this->GetLength());
  W_ASSERT_DEBUG(vForward.IsNormalized(), "The other vector must be normalized. Length is: {}", vForward.GetLength());
  W_ASSERT_DEBUG(vUp.IsNormalized(), "The other vector must be normalized. Length is: {}", vUp.GetLength());

  const WVec3Template<Type> vRight = vForward.CrossRH(vUp).GetNormalized();
  const WAngleTemplate<Type> shortAngle = GetAngleBetween(vForward);

  if (this->Dot(vRight) < 0) // more than 90 degrees away from it
  {
    return -shortAngle;
  }

  return shortAngle;
}

template <typename Type>
W_FORCE_INLINE const WVec3Template<Type> WVec3Template<Type>::CompMin(const WVec3Template<Type>& rhs) const
{
  W_NAN_ASSERT(this);
  W_NAN_ASSERT(&rhs);

  return WVec3Template<Type>(WMath::Min(x, rhs.x), WMath::Min(y, rhs.y), WMath::Min(z, rhs.z));
}

template <typename Type>
W_FORCE_INLINE const WVec3Template<Type> WVec3Template<Type>::CompMax(const WVec3Template<Type>& rhs) const
{
  W_NAN_ASSERT(this);
  W_NAN_ASSERT(&rhs);

  return WVec3Template<Type>(WMath::Max(x, rhs.x), WMath::Max(y, rhs.y), WMath::Max(z, rhs.z));
}

template <typename Type>
W_FORCE_INLINE const WVec3Template<Type> WVec3Template<Type>::CompClamp(const WVec3Template& vLow, const WVec3Template& vHigh) const
{
  W_NAN_ASSERT(this);
  W_NAN_ASSERT(&vLow);
  W_NAN_ASSERT(&vHigh);

  return WVec3Template<Type>(WMath::Clamp(x, vLow.x, vHigh.x), WMath::Clamp(y, vLow.y, vHigh.y), WMath::Clamp(z, vLow.z, vHigh.z));
}

template <typename Type>
W_FORCE_INLINE const WVec3Template<Type> WVec3Template<Type>::CompMul(const WVec3Template<Type>& rhs) const
{
  W_NAN_ASSERT(this);
  W_NAN_ASSERT(&rhs);

  return WVec3Template<Type>(x * rhs.x, y * rhs.y, z * rhs.z);
}

template <typename Type>
W_FORCE_INLINE const WVec3Template<Type> WVec3Template<Type>::CompDiv(const WVec3Template<Type>& rhs) const
{
  W_NAN_ASSERT(this);
  W_NAN_ASSERT(&rhs);

  return WVec3Template<Type>(x / rhs.x, y / rhs.y, z / rhs.z);
}

template <typename Type>
inline const WVec3Template<Type> WVec3Template<Type>::Abs() const
{
  W_NAN_ASSERT(this);

  return WVec3Template<Type>(WMath::Abs(x), WMath::Abs(y), WMath::Abs(z));
}

template <typename Type>
W_FORCE_INLINE const WVec3Template<Type> operator+(const WVec3Template<Type>& v1, const WVec3Template<Type>& v2)
{
  W_NAN_ASSERT(&v1);
  W_NAN_ASSERT(&v2);

  return WVec3Template<Type>(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
}

template <typename Type>
W_FORCE_INLINE const WVec3Template<Type> operator-(const WVec3Template<Type>& v1, const WVec3Template<Type>& v2)
{
  W_NAN_ASSERT(&v1);
  W_NAN_ASSERT(&v2);

  return WVec3Template<Type>(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
}

template <typename Type>
W_FORCE_INLINE const WVec3Template<Type> operator*(Type f, const WVec3Template<Type>& v)
{
  W_NAN_ASSERT(&v);

  return WVec3Template<Type>(v.x * f, v.y * f, v.z * f);
}

template <typename Type>
W_FORCE_INLINE const WVec3Template<Type> operator*(const WVec3Template<Type>& v, Type f)
{
  W_NAN_ASSERT(&v);

  return WVec3Template<Type>(v.x * f, v.y * f, v.z * f);
}

template <typename Type>
W_FORCE_INLINE const WVec3Template<Type> operator/(const WVec3Template<Type>& v, Type f)
{
  W_NAN_ASSERT(&v);

  if constexpr (std::is_floating_point_v<Type>)
  {
    // multiplication is much faster than division
    const Type f_inv = WMath::Invert(f);
    return WVec3Template<Type>(v.x * f_inv, v.y * f_inv, v.z * f_inv);
  }
  else
  {
    return WVec3Template<Type>(v.x / f, v.y / f, v.z / f);
  }
}

template <typename Type>
bool WVec3Template<Type>::IsIdentical(const WVec3Template<Type>& rhs) const
{
  W_NAN_ASSERT(this);
  W_NAN_ASSERT(&rhs);

  return ((x == rhs.x) && (y == rhs.y) && (z == rhs.z));
}

template <typename Type>
bool WVec3Template<Type>::IsEqual(const WVec3Template<Type>& rhs, Type fEpsilon) const
{
  W_NAN_ASSERT(this);
  W_NAN_ASSERT(&rhs);

  return (WMath::IsEqual(x, rhs.x, fEpsilon) && WMath::IsEqual(y, rhs.y, fEpsilon) && WMath::IsEqual(z, rhs.z, fEpsilon));
}

template <typename Type>
W_ALWAYS_INLINE bool operator==(const WVec3Template<Type>& v1, const WVec3Template<Type>& v2)
{
  return v1.IsIdentical(v2);
}

template <typename Type>
W_ALWAYS_INLINE bool operator!=(const WVec3Template<Type>& v1, const WVec3Template<Type>& v2)
{
  return !v1.IsIdentical(v2);
}

template <typename Type>
W_FORCE_INLINE bool operator<(const WVec3Template<Type>& v1, const WVec3Template<Type>& v2)
{
  W_NAN_ASSERT(&v1);
  W_NAN_ASSERT(&v2);

  if (v1.x < v2.x)
    return true;
  if (v1.x > v2.x)
    return false;
  if (v1.y < v2.y)
    return true;
  if (v1.y > v2.y)
    return false;

  return (v1.z < v2.z);
}

template <typename Type>
W_IMPLEMENT_IF_FLOAT_TYPE const WVec3Template<Type> WVec3Template<Type>::GetRefractedVector(const WVec3Template<Type>& vNormal, Type fRefIndex1, Type fRefIndex2) const
{
  W_ASSERT_DEBUG(vNormal.IsNormalized(), "vNormal must be normalized.");

  const Type n = fRefIndex1 / fRefIndex2;
  const Type cosI = this->Dot(vNormal);
  const Type sinT2 = n * n * (1.0f - (cosI * cosI));

  // invalid refraction
  if (sinT2 > 1.0f)
    return (*this);

  return ((n * (*this)) - (n + WMath::Sqrt(1.0f - sinT2)) * vNormal);
}
