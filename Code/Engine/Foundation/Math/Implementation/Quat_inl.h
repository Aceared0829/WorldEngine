#pragma once

#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Vec3.h>

template <typename Type>
W_ALWAYS_INLINE WQuatTemplate<Type>::WQuatTemplate()
{
#if W_ENABLED(W_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const Type TypeNaN = WMath::NaN<Type>();
  x = TypeNaN;
  y = TypeNaN;
  z = TypeNaN;
  w = TypeNaN;
#endif
}

template <typename Type>
W_ALWAYS_INLINE WQuatTemplate<Type>::WQuatTemplate(Type inX, Type inY, Type inZ, Type inW)
  : x(inX)
  , y(inY)
  , z(inZ)
  , w(inW)
{
}

template <typename Type>
W_ALWAYS_INLINE const WQuatTemplate<Type> WQuatTemplate<Type>::MakeIdentity()
{
  return WQuatTemplate(0, 0, 0, 1);
}

template <typename Type>
W_ALWAYS_INLINE WQuatTemplate<Type> WQuatTemplate<Type>::MakeFromElements(Type inX, Type inY, Type inZ, Type inW)
{
  return WQuatTemplate<Type>(inX, inY, inZ, inW);
}

template <typename Type>
W_ALWAYS_INLINE void WQuatTemplate<Type>::SetIdentity()
{
  x = (Type)0;
  y = (Type)0;
  z = (Type)0;
  w = (Type)1;
}

template <typename Type>
WQuatTemplate<Type> WQuatTemplate<Type>::MakeFromAxisAndAngle(const WVec3Template<Type>& vRotationAxis, WAngleTemplate<Type> angle)
{
  const WAngleTemplate<Type> halfAngle = angle * Type(0.5);

  WVec3Template<Type> v = static_cast<Type>(WMath::Sin(halfAngle)) * vRotationAxis;
  
  Type w = static_cast<Type>(WMath::Cos(halfAngle));

  return WQuatTemplate<Type>(v.x, v.y, v.z, w);
}

template <typename Type>
void WQuatTemplate<Type>::Normalize()
{
  W_NAN_ASSERT(this);

  Type n = x * x + y * y + z * z + w * w;

  n = WMath::Invert(WMath::Sqrt(n));

  x *= n;
  y *= n;
  z *= n;
  w *= n;
}

template <typename Type>
void WQuatTemplate<Type>::GetRotationAxisAndAngle(WVec3Template<Type>& out_vAxis, WAngleTemplate<Type>& out_angle, Type fEpsilon) const
{
  W_NAN_ASSERT(this);

  out_angle = (Type)2.0 * WMath::ACos<Type>(static_cast<Type>(w));

  const Type s = WMath::Sqrt(1 - w * w);

  if (s < fEpsilon)
  {
    out_vAxis.Set(1, 0, 0);
  }
  else
  {
    const Type ds = (Type)(1.0 / s);
    out_vAxis.x = x * ds;
    out_vAxis.y = y * ds;
    out_vAxis.z = z * ds;
  }
}

template <typename Type>
W_FORCE_INLINE void WQuatTemplate<Type>::Invert()
{
  W_NAN_ASSERT(this);

  *this = GetInverse();
}

template <typename Type>
W_FORCE_INLINE const WQuatTemplate<Type> WQuatTemplate<Type>::GetInverse() const
{
  W_NAN_ASSERT(this);

  return (WQuatTemplate(-x, -y, -z, w));
}

template <typename Type>
W_FORCE_INLINE const WQuatTemplate<Type> WQuatTemplate<Type>::GetNegated() const
{
  W_NAN_ASSERT(this);

  return (WQuatTemplate(-x, -y, -z, -w));
}

template <typename Type>
W_FORCE_INLINE Type WQuatTemplate<Type>::Dot(const WQuatTemplate& rhs) const
{
  W_NAN_ASSERT(this);
  W_NAN_ASSERT(&rhs);

  return GetVectorPart().Dot(rhs.GetVectorPart()) + w * rhs.w;
}

template <typename Type>
W_ALWAYS_INLINE WVec3Template<Type> WQuatTemplate<Type>::Rotate(const WVec3Template<Type>& v) const
{
  return *this * v;
}

template <typename Type>
W_ALWAYS_INLINE const WVec3Template<Type> operator*(const WQuatTemplate<Type>& q, const WVec3Template<Type>& v)
{
  WVec3Template<Type> t = q.GetVectorPart().CrossRH(v) * (Type)2;
  return v + q.w * t + q.GetVectorPart().CrossRH(t);
}

template <typename Type>
W_ALWAYS_INLINE const WQuatTemplate<Type> operator*(const WQuatTemplate<Type>& q1, const WQuatTemplate<Type>& q2)
{
  WQuatTemplate<Type> q;

  q.w = q1.w * q2.w - (q1.x * q2.x + q1.y * q2.y + q1.z * q2.z);

  const WVec3Template<Type> v1 = q1.GetVectorPart();
  const WVec3Template<Type> v2 = q2.GetVectorPart();

  const WVec3Template<Type> vr = q1.w * v2 + q2.w * v1 + v1.CrossRH(v2);
  q.x = vr.x;
  q.y = vr.y;
  q.z = vr.z;

  return q;
}

template <typename Type>
bool WQuatTemplate<Type>::IsValid(Type fEpsilon) const
{
  if (!GetVectorPart().IsValid())
    return false;
  if (!WMath::IsFinite(w))
    return false;

  Type n = x * x + y * y + z * z + w * w;

  return WMath::IsEqual(n, (Type)1, fEpsilon);
}

template <typename Type>
bool WQuatTemplate<Type>::IsNaN() const
{
  return WMath::IsNaN(x) || WMath::IsNaN(y) || WMath::IsNaN(z) || WMath::IsNaN(w);
}

template <typename Type>
bool WQuatTemplate<Type>::IsEqualRotation(const WQuatTemplate<Type>& qOther, Type fEpsilon) const
{
  if (GetVectorPart().IsEqual(qOther.GetVectorPart(), fEpsilon) && WMath::IsEqual(w, qOther.w, fEpsilon))
  {
    return true;
  }

  WVec3Template<Type> vA1, vA2;
  WAngleTemplate<Type> A1, A2;

  GetRotationAxisAndAngle(vA1, A1);
  qOther.GetRotationAxisAndAngle(vA2, A2);

  if ((A1.IsEqualSimple(A2, WAngleTemplate<Type>::MakeFromDegree(static_cast<Type>((Type)360.0 * fEpsilon)))) && (vA1.IsEqual(vA2, fEpsilon)))
    return true;

  if ((A1.IsEqualSimple(-A2, WAngleTemplate<Type>::MakeFromDegree(static_cast<Type>((Type)360.0 * fEpsilon)))) && (vA1.IsEqual(-vA2, fEpsilon)))
    return true;

  return false;
}

template <typename Type>
const WMat3Template<Type> WQuatTemplate<Type>::GetAsMat3() const
{
  W_NAN_ASSERT(this);

  WMat3Template<Type> m;

  const Type fTx = x + x;
  const Type fTy = y + y;
  const Type fTz = z + z;
  const Type fTwx = fTx * w;
  const Type fTwy = fTy * w;
  const Type fTwz = fTz * w;
  const Type fTxx = fTx * x;
  const Type fTxy = fTy * x;
  const Type fTxz = fTz * x;
  const Type fTyy = fTy * y;
  const Type fTyz = fTz * y;
  const Type fTzz = fTz * z;

  m.Element(0, 0) = (Type)1 - (fTyy + fTzz);
  m.Element(1, 0) = fTxy - fTwz;
  m.Element(2, 0) = fTxz + fTwy;
  m.Element(0, 1) = fTxy + fTwz;
  m.Element(1, 1) = (Type)1 - (fTxx + fTzz);
  m.Element(2, 1) = fTyz - fTwx;
  m.Element(0, 2) = fTxz - fTwy;
  m.Element(1, 2) = fTyz + fTwx;
  m.Element(2, 2) = (Type)1 - (fTxx + fTyy);
  return m;
}

template <typename Type>
const WMat4Template<Type> WQuatTemplate<Type>::GetAsMat4() const
{
  W_NAN_ASSERT(this);

  WMat4Template<Type> m;

  const Type fTx = x + x;
  const Type fTy = y + y;
  const Type fTz = z + z;
  const Type fTwx = fTx * w;
  const Type fTwy = fTy * w;
  const Type fTwz = fTz * w;
  const Type fTxx = fTx * x;
  const Type fTxy = fTy * x;
  const Type fTxz = fTz * x;
  const Type fTyy = fTy * y;
  const Type fTyz = fTz * y;
  const Type fTzz = fTz * z;

  m.Element(0, 0) = (Type)1 - (fTyy + fTzz);
  m.Element(1, 0) = fTxy - fTwz;
  m.Element(2, 0) = fTxz + fTwy;
  m.Element(3, 0) = (Type)0;
  m.Element(0, 1) = fTxy + fTwz;
  m.Element(1, 1) = (Type)1 - (fTxx + fTzz);
  m.Element(2, 1) = fTyz - fTwx;
  m.Element(3, 1) = (Type)0;
  m.Element(0, 2) = fTxz - fTwy;
  m.Element(1, 2) = fTyz + fTwx;
  m.Element(2, 2) = (Type)1 - (fTxx + fTyy);
  m.Element(3, 2) = (Type)0;
  m.Element(0, 3) = (Type)0;
  m.Element(1, 3) = (Type)0;
  m.Element(2, 3) = (Type)0;
  m.Element(3, 3) = (Type)1;
  return m;
}

template <typename Type>
WQuatTemplate<Type> WQuatTemplate<Type>::MakeFromMat3(const WMat3Template<Type>& m)
{
  W_NAN_ASSERT(&m);

  const Type trace = m.Element(0, 0) + m.Element(1, 1) + m.Element(2, 2);
  const Type half = (Type)0.5;

  Type val[4];

  if (trace > (Type)0)
  {
    Type s = WMath::Sqrt(trace + (Type)1);
    Type t = half / s;

    val[0] = (m.Element(1, 2) - m.Element(2, 1)) * t;
    val[1] = (m.Element(2, 0) - m.Element(0, 2)) * t;
    val[2] = (m.Element(0, 1) - m.Element(1, 0)) * t;

    val[3] = half * s;
  }
  else
  {
    const WInt32 next[] = {1, 2, 0};
    WInt32 i = 0;

    if (m.Element(1, 1) > m.Element(0, 0))
      i = 1;

    if (m.Element(2, 2) > m.Element(i, i))
      i = 2;

    WInt32 j = next[i];
    WInt32 k = next[j];

    Type s = WMath::Sqrt(m.Element(i, i) - (m.Element(j, j) + m.Element(k, k)) + (Type)1);
    Type t = half / s;

    val[i] = half * s;
    val[3] = (m.Element(j, k) - m.Element(k, j)) * t;
    val[j] = (m.Element(i, j) + m.Element(j, i)) * t;
    val[k] = (m.Element(i, k) + m.Element(k, i)) * t;
  }

  WQuatTemplate<Type> q;
  q.x = val[0];
  q.y = val[1];
  q.z = val[2];
  q.w = val[3];
  return q;
}

template <typename Type>
void WQuatTemplate<Type>::ReconstructFromMat3(const WMat3Template<Type>& mMat)
{
  const WVec3 x = (mMat * WVec3(1, 0, 0)).GetNormalized();
  const WVec3 y = (mMat * WVec3(0, 1, 0)).GetNormalized();
  const WVec3 z = x.CrossRH(y);

  WMat3 m;
  m.SetColumn(0, x);
  m.SetColumn(1, y);
  m.SetColumn(2, z);

  *this = WQuat::MakeFromMat3(m);
}

template <typename Type>
void WQuatTemplate<Type>::ReconstructFromMat4(const WMat4Template<Type>& mMat)
{
  const WVec3 x = mMat.TransformDirection(WVec3(1, 0, 0)).GetNormalized();
  const WVec3 y = mMat.TransformDirection(WVec3(0, 1, 0)).GetNormalized();
  const WVec3 z = x.CrossRH(y);

  WMat3 m;
  m.SetColumn(0, x);
  m.SetColumn(1, y);
  m.SetColumn(2, z);

  *this = WQuat::MakeFromMat3(m);
}

/*! \note This function will ALWAYS return a quaternion that rotates from one direction to another.
  If both directions are identical, it is the unit rotation (none). If they are exactly opposing, this will be
  ANY 180.0 degree rotation. That means the vectors will align perfectly, but there is no determine rotation for other points
  that might be rotated with this quaternion. If a main / fallback axis is needed to rotate points, you need to calculate
  such a rotation with other means.
*/
template <typename Type>
WQuatTemplate<Type> WQuatTemplate<Type>::MakeShortestRotation(const WVec3Template<Type>& vDirFrom, const WVec3Template<Type>& vDirTo)
{
  const WVec3Template<Type> v0 = vDirFrom.GetNormalized();
  const WVec3Template<Type> v1 = vDirTo.GetNormalized();

  const Type fDot = v0.Dot(v1);

  // if both vectors are identical -> no rotation needed
  if (WMath::IsEqual(fDot, (Type)1, (Type)0.0000001))
  {
    return MakeIdentity();
  }
  else if (WMath::IsEqual(fDot, (Type)-1, (Type)0.0000001)) // if both vectors are opposing
  {
    // find an axis, that is not identical and not opposing, WVec3Template::Cross-product to find perpendicular vector, rotate around that
    if (WMath::Abs(v0.Dot(WVec3Template<Type>(1, 0, 0))) < (Type)0.8)
      return MakeFromAxisAndAngle(v0.CrossRH(WVec3Template<Type>(1, 0, 0)).GetNormalized(), WAngleTemplate<Type>::MakeFromRadian(WMath::Pi<Type>()));
    else
      return MakeFromAxisAndAngle(v0.CrossRH(WVec3Template<Type>(0, 1, 0)).GetNormalized(), WAngleTemplate<Type>::MakeFromRadian(WMath::Pi<Type>()));
  }

  const WVec3Template<Type> c = v0.CrossRH(v1);
  const Type d = v0.Dot(v1);
  const Type s = WMath::Sqrt(((Type)1 + d) * (Type)2);

  W_ASSERT_DEBUG(c.IsValid(), "SetShortestRotation failed.");

  const Type fOneDivS = 1.0f / s;

  WQuatTemplate<Type> q;
  q.x = c.x * fOneDivS;
  q.y = c.y * fOneDivS;
  q.z = c.z * fOneDivS;
  q.w = s / (Type)2;
  q.Normalize();

  return q;
}

template <typename Type>
WQuatTemplate<Type> WQuatTemplate<Type>::MakeSlerp(const WQuatTemplate<Type>& qFrom, const WQuatTemplate<Type>& qTo, Type t)
{
  W_ASSERT_DEBUG((t >= (Type)0) && (t <= (Type)1), "Invalid lerp factor.");

  const Type one = 1;
  const Type qdelta = (Type)1 - (Type)0.001;

  const Type fDot = (qFrom.x * qTo.x + qFrom.y * qTo.y + qFrom.z * qTo.z + qFrom.w * qTo.w);

  Type cosTheta = fDot;

  bool bFlipSign = false;
  if (cosTheta < (Type)0)
  {
    bFlipSign = true;
    cosTheta = -cosTheta;
  }

  Type t0, t1;

  if (cosTheta < qdelta)
  {
    WAngleTemplate<Type> theta = WMath::ACos((Type)cosTheta);

    // use sqrtInv(1+c^2) instead of 1.0/sin(theta)
    const Type iSinTheta = (Type)1 / WMath::Sqrt(one - (cosTheta * cosTheta));
    const WAngleTemplate<Type> tTheta = static_cast<Type>(t) * theta;

    Type s0 = WMath::Sin(theta - tTheta);
    Type s1 = WMath::Sin(tTheta);

    t0 = s0 * iSinTheta;
    t1 = s1 * iSinTheta;
  }
  else
  {
    // If q0 is nearly the same as q1 we just linearly interpolate
    t0 = one - t;
    t1 = t;
  }

  if (bFlipSign)
    t1 = -t1;

  WQuatTemplate<Type> q;

  q.x = t0 * qFrom.x;
  q.y = t0 * qFrom.y;
  q.z = t0 * qFrom.z;
  q.w = t0 * qFrom.w;

  q.x += t1 * qTo.x;
  q.y += t1 * qTo.y;
  q.z += t1 * qTo.z;
  q.w += t1 * qTo.w;

  q.Normalize();

  return q;
}

template <typename Type>
W_ALWAYS_INLINE bool operator==(const WQuatTemplate<Type>& q1, const WQuatTemplate<Type>& q2)
{
  return q1.x == q2.x && q1.y == q2.y && q1.z == q2.z && q1.w == q2.w;
}

template <typename Type>
W_ALWAYS_INLINE bool operator!=(const WQuatTemplate<Type>& q1, const WQuatTemplate<Type>& q2)
{
  return !(q1 == q2);
}

template <typename Type>
void WQuatTemplate<Type>::GetAsEulerAngles(WAngleTemplate<Type>& out_x, WAngleTemplate<Type>& out_y, WAngleTemplate<Type>& out_z) const
{
  // Taken from https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
  // and http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToEuler/
  // adapted to our convention (yaw->pitch->roll, ZYX order or 3-2-1 order)

  auto& yaw = out_z;
  auto& pitch = out_y;
  auto& roll = out_x;

  const double fSingularityTest = w * y - z * x;
  const double fSingularityThreshold = 0.4999995;

  if (fSingularityTest > fSingularityThreshold) // singularity at north pole
  {
    yaw = (Type)-2.0 * WMath::ATan2<Type>(x, w);
    pitch = WAngleTemplate<Type>::MakeFromDegree(90.0f);
    roll = WAngleTemplate<Type>::MakeFromDegree(0.0f);
  }
  else if (fSingularityTest < -fSingularityThreshold) // singularity at south pole
  {
    yaw = (Type)2.0 * WMath::ATan2<Type>(x, w);
    pitch = WAngleTemplate<Type>::MakeFromDegree(-90.0f);
    roll = WAngleTemplate<Type>::MakeFromDegree(0.0f);
  }
  else
  {
    // yaw (z-axis rotation)
    const double siny = 2.0 * (w * z + x * y);
    const double cosy = 1.0 - 2.0 * (y * y + z * z);
    yaw = WMath::ATan2<Type>((Type)siny, (Type)cosy);

    // pitch (y-axis rotation)
    pitch = WMath::ASin<Type>(2.0f * (Type)fSingularityTest);

    // roll (x-axis rotation)
    const double sinr = 2.0 * (w * x + y * z);
    const double cosr = 1.0 - 2.0 * (x * x + y * y);
    roll = WMath::ATan2<Type>((Type)sinr, (Type)cosr);
  }
}

template <typename Type>
WQuatTemplate<Type> WQuatTemplate<Type>::MakeFromEulerAngles(const WAngleTemplate<Type>& x, const WAngleTemplate<Type>& y, const WAngleTemplate<Type>& z)
{
  /// Taken from here (yaw->pitch->roll, ZYX order or 3-2-1 order):
  /// https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
  const auto& yaw = z;
  const auto& pitch = y;
  const auto& roll = x;
  const double cy = WMath::Cos(yaw * Type(0.5));
  const double sy = WMath::Sin(yaw * Type(0.5));
  const double cp = WMath::Cos(pitch * Type(0.5));
  const double sp = WMath::Sin(pitch * Type(0.5));
  const double cr = WMath::Cos(roll * Type(0.5));
  const double sr = WMath::Sin(roll * Type(0.5));

  WQuatTemplate<Type> q;
  q.w = (Type)(cy * cp * cr + sy * sp * sr);
  q.x = (Type)(cy * cp * sr - sy * sp * cr);
  q.y = (Type)(cy * sp * cr + sy * cp * sr);
  q.z = (Type)(sy * cp * cr - cy * sp * sr);
  return q;
}
