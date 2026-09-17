#pragma once

#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>

// *** Vec2 and Vec3 Code ***
// Cannot put this into the Vec3_inl.h file, that would result in circular dependencies

template <typename Type>
W_FORCE_INLINE const WVec3Template<Type> WVec2Template<Type>::GetAsVec3(Type z) const
{
  W_NAN_ASSERT(this);

  return WVec3Template<Type>(x, y, z);
}

template <typename Type>
W_FORCE_INLINE const WVec4Template<Type> WVec2Template<Type>::GetAsVec4(Type z, Type w) const
{
  W_NAN_ASSERT(this);

  return WVec4Template<Type>(x, y, z, w);
}

template <typename Type>
W_FORCE_INLINE const WVec2Template<Type> WVec3Template<Type>::GetAsVec2() const
{
  // don't assert here, as the 3rd and 4th component may be NaN when this is fine, e.g. during interop with the SIMD classes
  // W_NAN_ASSERT(this);

  return WVec2Template<Type>(x, y);
}

template <typename Type>
W_FORCE_INLINE const WVec4Template<Type> WVec3Template<Type>::GetAsVec4(Type w) const
{
  W_NAN_ASSERT(this);

  return WVec4Template<Type>(x, y, z, w);
}

template <typename Type>
W_FORCE_INLINE const WVec4Template<Type> WVec3Template<Type>::GetAsPositionVec4() const
{
  // don't assert here, as the 4th component may be NaN when this is fine, e.g. during interop with the SIMD classes
  // W_NAN_ASSERT(this);

  return WVec4Template<Type>(x, y, z, 1);
}

template <typename Type>
W_FORCE_INLINE const WVec4Template<Type> WVec3Template<Type>::GetAsDirectionVec4() const
{
  // don't assert here, as the 4th component may be NaN when this is fine, e.g. during interop with the SIMD classes
  // W_NAN_ASSERT(this);

  return WVec4Template<Type>(x, y, z, 0);
}

// *****************

template <typename Type>
W_ALWAYS_INLINE WVec4Template<Type>::WVec4Template()
{
#if W_ENABLED(W_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const Type TypeNaN = WMath::NaN<Type>();
  x = TypeNaN;
  y = TypeNaN;
  z = 0;
  w = 0;
#endif
}

template <typename Type>
W_ALWAYS_INLINE WVec4Template<Type>::WVec4Template(Type x, Type y, Type z, Type w)
  : x(x)
  , y(y)
  , z(z)
  , w(w)
{
}

template <typename Type>
W_ALWAYS_INLINE WVec4Template<Type>::WVec4Template(WVec3Template<Type> vXyz, Type w)
  : x(vXyz.x)
  , y(vXyz.y)
  , z(vXyz.z)
  , w(w)
{
}

template <typename Type>
W_ALWAYS_INLINE WVec4Template<Type>::WVec4Template(Type v)
  : x(v)
  , y(v)
  , z(v)
  , w(v)
{
}

template <typename Type>
W_FORCE_INLINE const WVec2Template<Type> WVec4Template<Type>::GetAsVec2() const
{
  // don't assert here, as the 4th component may be NaN when this is fine, e.g. during interop with the SIMD classes
  // W_NAN_ASSERT(this);

  return WVec2Template<Type>(x, y);
}

template <typename Type>
W_FORCE_INLINE const WVec3Template<Type> WVec4Template<Type>::GetAsVec3() const
{
  // don't assert here, as the 4th component may be NaN when this is fine, e.g. during interop with the SIMD classes
  // W_NAN_ASSERT(this);

  return WVec3Template<Type>(x, y, z);
}

template <typename Type>
W_ALWAYS_INLINE void WVec4Template<Type>::Set(Type xyzw)
{
  x = xyzw;
  y = xyzw;
  z = xyzw;
  w = xyzw;
}

template <typename Type>
W_ALWAYS_INLINE void WVec4Template<Type>::Set(Type inX, Type inY, Type inZ, Type inW)
{
  x = inX;
  y = inY;
  z = inZ;
  w = inW;
}

template <typename Type>
inline void WVec4Template<Type>::SetZero()
{
  x = y = z = w = 0;
}

template <typename Type>
W_IMPLEMENT_IF_FLOAT_TYPE W_ALWAYS_INLINE Type WVec4Template<Type>::GetLength() const
{
  return (WMath::Sqrt(GetLengthSquared()));
}

template <typename Type>
W_FORCE_INLINE Type WVec4Template<Type>::GetLengthSquared() const
{
  W_NAN_ASSERT(this);

  return (x * x + y * y + z * z + w * w);
}

template <typename Type>
W_IMPLEMENT_IF_FLOAT_TYPE W_FORCE_INLINE Type WVec4Template<Type>::GetLengthAndNormalize()
{
  const Type fLength = GetLength();
  *this /= fLength;
  return fLength;
}

template <typename Type>
W_IMPLEMENT_IF_FLOAT_TYPE W_FORCE_INLINE const WVec4Template<Type> WVec4Template<Type>::GetNormalized() const
{
  const Type fLen = GetLength();

  const Type fLengthInv = WMath::Invert(fLen);
  return WVec4Template<Type>(x * fLengthInv, y * fLengthInv, z * fLengthInv, w * fLengthInv);
}

template <typename Type>
W_IMPLEMENT_IF_FLOAT_TYPE W_ALWAYS_INLINE void WVec4Template<Type>::Normalize()
{
  *this /= GetLength();
}

template <typename Type>
W_IMPLEMENT_IF_FLOAT_TYPE inline WResult WVec4Template<Type>::NormalizeIfNotZero(const WVec4Template<Type>& vFallback, Type fEpsilon)
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
W_IMPLEMENT_IF_FLOAT_TYPE inline bool WVec4Template<Type>::IsNormalized(Type fEpsilon /* = WMath::HugeEpsilon<Type>() */) const
{
  const Type t = GetLengthSquared();
  return WMath::IsEqual(t, (Type)1, fEpsilon);
}

template <typename Type>
inline bool WVec4Template<Type>::IsZero() const
{
  W_NAN_ASSERT(this);

  return ((x == 0.0f) && (y == 0.0f) && (z == 0.0f) && (w == 0.0f));
}

template <typename Type>
inline bool WVec4Template<Type>::IsZero(Type fEpsilon) const
{
  W_NAN_ASSERT(this);

  return (WMath::IsZero(x, fEpsilon) && WMath::IsZero(y, fEpsilon) && WMath::IsZero(z, fEpsilon) && WMath::IsZero(w, fEpsilon));
}

template <typename Type>
inline bool WVec4Template<Type>::IsNaN() const
{
  if (WMath::IsNaN(x))
    return true;
  if (WMath::IsNaN(y))
    return true;
  if (WMath::IsNaN(z))
    return true;
  if (WMath::IsNaN(w))
    return true;

  return false;
}

template <typename Type>
inline bool WVec4Template<Type>::IsValid() const
{
  if (!WMath::IsFinite(x))
    return false;
  if (!WMath::IsFinite(y))
    return false;
  if (!WMath::IsFinite(z))
    return false;
  if (!WMath::IsFinite(w))
    return false;

  return true;
}

template <typename Type>
W_FORCE_INLINE const WVec4Template<Type> WVec4Template<Type>::operator-() const
{
  W_NAN_ASSERT(this);

  return WVec4Template<Type>(-x, -y, -z, -w);
}

template <typename Type>
W_FORCE_INLINE void WVec4Template<Type>::operator+=(const WVec4Template<Type>& vCc)
{
  x += vCc.x;
  y += vCc.y;
  z += vCc.z;
  w += vCc.w;

  W_NAN_ASSERT(this);
}

template <typename Type>
W_FORCE_INLINE void WVec4Template<Type>::operator-=(const WVec4Template<Type>& vCc)
{
  x -= vCc.x;
  y -= vCc.y;
  z -= vCc.z;
  w -= vCc.w;

  W_NAN_ASSERT(this);
}

template <typename Type>
W_FORCE_INLINE void WVec4Template<Type>::operator*=(Type f)
{
  x *= f;
  y *= f;
  z *= f;
  w *= f;

  W_NAN_ASSERT(this);
}

template <typename Type>
W_FORCE_INLINE void WVec4Template<Type>::operator/=(Type f)
{
  if constexpr (std::is_floating_point_v<Type>)
  {
    const Type f_inv = WMath::Invert(f);
    x *= f_inv;
    y *= f_inv;
    z *= f_inv;
    w *= f_inv;
  }
  else
  {
    x /= f;
    y /= f;
    z /= f;
    w /= f;
  }

  W_NAN_ASSERT(this);
}

template <typename Type>
W_FORCE_INLINE Type WVec4Template<Type>::Dot(const WVec4Template<Type>& rhs) const
{
  W_NAN_ASSERT(this);
  W_NAN_ASSERT(&rhs);

  return ((x * rhs.x) + (y * rhs.y) + (z * rhs.z) + (w * rhs.w));
}

template <typename Type>
inline const WVec4Template<Type> WVec4Template<Type>::CompMin(const WVec4Template<Type>& rhs) const
{
  W_NAN_ASSERT(this);
  W_NAN_ASSERT(&rhs);

  return WVec4Template<Type>(WMath::Min(x, rhs.x), WMath::Min(y, rhs.y), WMath::Min(z, rhs.z), WMath::Min(w, rhs.w));
}

template <typename Type>
inline const WVec4Template<Type> WVec4Template<Type>::CompMax(const WVec4Template<Type>& rhs) const
{
  W_NAN_ASSERT(this);
  W_NAN_ASSERT(&rhs);

  return WVec4Template<Type>(WMath::Max(x, rhs.x), WMath::Max(y, rhs.y), WMath::Max(z, rhs.z), WMath::Max(w, rhs.w));
}

template <typename Type>
inline const WVec4Template<Type> WVec4Template<Type>::CompClamp(const WVec4Template& vLow, const WVec4Template& vHigh) const
{
  W_NAN_ASSERT(this);
  W_NAN_ASSERT(&vLow);
  W_NAN_ASSERT(&vHigh);

  return WVec4Template<Type>(WMath::Clamp(x, vLow.x, vHigh.x), WMath::Clamp(y, vLow.y, vHigh.y), WMath::Clamp(z, vLow.z, vHigh.z), WMath::Clamp(w, vLow.w, vHigh.w));
}

template <typename Type>
inline const WVec4Template<Type> WVec4Template<Type>::CompMul(const WVec4Template<Type>& rhs) const
{
  W_NAN_ASSERT(this);
  W_NAN_ASSERT(&rhs);

  return WVec4Template<Type>(x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w);
}

W_MSVC_ANALYSIS_WARNING_PUSH
W_MSVC_ANALYSIS_WARNING_DISABLE(4723)
template <typename Type>
inline const WVec4Template<Type> WVec4Template<Type>::CompDiv(const WVec4Template<Type>& rhs) const
{
  W_NAN_ASSERT(this);
  W_NAN_ASSERT(&rhs);

  return WVec4Template<Type>(x / rhs.x, y / rhs.y, z / rhs.z, w / rhs.w);
}
W_MSVC_ANALYSIS_WARNING_POP

template <typename Type>
inline const WVec4Template<Type> WVec4Template<Type>::Abs() const
{
  W_NAN_ASSERT(this);

  return WVec4Template<Type>(WMath::Abs(x), WMath::Abs(y), WMath::Abs(z), WMath::Abs(w));
}

template <typename Type>
W_FORCE_INLINE const WVec4Template<Type> operator+(const WVec4Template<Type>& v1, const WVec4Template<Type>& v2)
{
  W_NAN_ASSERT(&v1);
  W_NAN_ASSERT(&v2);

  return WVec4Template<Type>(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w);
}

template <typename Type>
W_FORCE_INLINE const WVec4Template<Type> operator-(const WVec4Template<Type>& v1, const WVec4Template<Type>& v2)
{
  W_NAN_ASSERT(&v1);
  W_NAN_ASSERT(&v2);

  return WVec4Template<Type>(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w);
}

template <typename Type>
W_FORCE_INLINE const WVec4Template<Type> operator*(Type f, const WVec4Template<Type>& v)
{
  W_NAN_ASSERT(&v);

  return WVec4Template<Type>(v.x * f, v.y * f, v.z * f, v.w * f);
}

template <typename Type>
W_FORCE_INLINE const WVec4Template<Type> operator*(const WVec4Template<Type>& v, Type f)
{
  W_NAN_ASSERT(&v);

  return WVec4Template<Type>(v.x * f, v.y * f, v.z * f, v.w * f);
}

template <typename Type>
W_FORCE_INLINE const WVec4Template<Type> operator/(const WVec4Template<Type>& v, Type f)
{
  W_NAN_ASSERT(&v);

  if constexpr (std::is_floating_point_v<Type>)
  {
    // multiplication is much faster than division
    const Type f_inv = WMath::Invert(f);
    return WVec4Template<Type>(v.x * f_inv, v.y * f_inv, v.z * f_inv, v.w * f_inv);
  }
  else
  {
    return WVec4Template<Type>(v.x / f, v.y / f, v.z / f, v.w / f);
  }
}

template <typename Type>
inline bool WVec4Template<Type>::IsIdentical(const WVec4Template<Type>& rhs) const
{
  W_NAN_ASSERT(this);
  W_NAN_ASSERT(&rhs);

  return ((x == rhs.x) && (y == rhs.y) && (z == rhs.z) && (w == rhs.w));
}

template <typename Type>
inline bool WVec4Template<Type>::IsEqual(const WVec4Template<Type>& rhs, Type fEpsilon) const
{
  W_NAN_ASSERT(this);
  W_NAN_ASSERT(&rhs);

  return (WMath::IsEqual(x, rhs.x, fEpsilon) && WMath::IsEqual(y, rhs.y, fEpsilon) && WMath::IsEqual(z, rhs.z, fEpsilon) && WMath::IsEqual(w, rhs.w, fEpsilon));
}

template <typename Type>
W_ALWAYS_INLINE bool operator==(const WVec4Template<Type>& v1, const WVec4Template<Type>& v2)
{
  return v1.IsIdentical(v2);
}

template <typename Type>
W_ALWAYS_INLINE bool operator!=(const WVec4Template<Type>& v1, const WVec4Template<Type>& v2)
{
  return !v1.IsIdentical(v2);
}

template <typename Type>
W_FORCE_INLINE bool operator<(const WVec4Template<Type>& v1, const WVec4Template<Type>& v2)
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
  if (v1.z < v2.z)
    return true;
  if (v1.z > v2.z)
    return false;

  return (v1.w < v2.w);
}
