#pragma once

#include <Foundation/Math/Transform.h>

template <typename Type>
inline WTransformTemplate<Type>::WTransformTemplate(const WVec3Template<Type>& vPosition,
  const WQuatTemplate<Type>& qRotation, const WVec3Template<Type>& vScale)
  : m_vPosition(vPosition)
  , m_qRotation(qRotation)
  , m_vScale(vScale)
{
}

template <typename Type>
inline WTransformTemplate<Type> WTransformTemplate<Type>::Make(const WVec3Template<Type>& vPosition, const WQuatTemplate<Type>& qRotation /*= WQuatTemplate<Type>::IdentityQuaternion()*/, const WVec3Template<Type>& vScale /*= WVec3Template<Type>(1)*/)
{
  WTransformTemplate<Type> res;
  res.m_vPosition = vPosition;
  res.m_qRotation = qRotation;
  res.m_vScale = vScale;
  return res;
}

template <typename Type>
inline WTransformTemplate<Type> WTransformTemplate<Type>::MakeIdentity()
{
  WTransformTemplate<Type> res;
  res.m_vPosition.SetZero();
  res.m_qRotation = WQuatTemplate<Type>::MakeIdentity();
  res.m_vScale.Set(1.0f);
  return res;
}

template <typename Type>
WTransformTemplate<Type> WTransformTemplate<Type>::MakeFromMat4(const WMat4Template<Type>& mMat)
{
  WMat3Template<Type> mRot = mMat.GetRotationalPart();

  WTransformTemplate<Type> res;
  res.m_vPosition = mMat.GetTranslationVector();
  res.m_vScale = mRot.GetScalingFactors();
  mRot.SetScalingFactors(WVec3Template<Type>(1)).IgnoreResult();
  res.m_qRotation = WQuatTemplate<Type>::MakeFromMat3(mRot);
  return res;
}

template <typename Type>
WTransformTemplate<Type> WTransformTemplate<Type>::MakeLocalTransform(const WTransformTemplate& globalTransformParent, const WTransformTemplate& globalTransformChild)
{
  const auto invRot = globalTransformParent.m_qRotation.GetInverse();
  const auto invScale = WVec3Template<Type>(1).CompDiv(globalTransformParent.m_vScale);

  WTransformTemplate<Type> res;
  res.m_vPosition = (invRot * (globalTransformChild.m_vPosition - globalTransformParent.m_vPosition)).CompMul(invScale);
  res.m_qRotation = invRot * globalTransformChild.m_qRotation;
  res.m_vScale = invScale.CompMul(globalTransformChild.m_vScale);
  return res;
}

template <typename Type>
W_ALWAYS_INLINE WTransformTemplate<Type> WTransformTemplate<Type>::MakeGlobalTransform(const WTransformTemplate& globalTransformParent, const WTransformTemplate& localTransformChild)
{
  return globalTransformParent * localTransformChild;
}

template <typename Type>
W_ALWAYS_INLINE void WTransformTemplate<Type>::SetIdentity()
{
  *this = MakeIdentity();
}

template <typename Type>
W_ALWAYS_INLINE Type WTransformTemplate<Type>::GetMaxScale() const
{
  auto absScale = m_vScale.Abs();
  return WMath::Max(absScale.x, WMath::Max(absScale.y, absScale.z));
}

template <typename Type>
W_ALWAYS_INLINE bool WTransformTemplate<Type>::HasMirrorScaling() const
{
  return (m_vScale.x * m_vScale.y * m_vScale.z) < 0.0f;
}

template <typename Type>
W_ALWAYS_INLINE bool WTransformTemplate<Type>::HasOnlyUniformScaling() const
{
  const Type fEpsilon = WMath::DefaultEpsilon<Type>();
  return WMath::IsEqual(m_vScale.x, m_vScale.y, fEpsilon) && WMath::IsEqual(m_vScale.x, m_vScale.z, fEpsilon);
}

template <typename Type>
inline bool WTransformTemplate<Type>::IsIdentical(const WTransformTemplate<Type>& rhs) const
{
  return m_vPosition.IsIdentical(rhs.m_vPosition) && (m_qRotation == rhs.m_qRotation) && m_vScale.IsIdentical(rhs.m_vScale);
}

template <typename Type>
inline bool WTransformTemplate<Type>::IsEqual(const WTransformTemplate<Type>& rhs, Type fEpsilon) const
{
  return m_vPosition.IsEqual(rhs.m_vPosition, fEpsilon) && m_qRotation.IsEqualRotation(rhs.m_qRotation, fEpsilon) && m_vScale.IsEqual(rhs.m_vScale, fEpsilon);
}

template <typename Type>
inline bool WTransformTemplate<Type>::IsValid() const
{
  return m_vPosition.IsValid() && m_qRotation.IsValid(0.005f) && m_vScale.IsValid();
}

template <typename Type>
W_ALWAYS_INLINE const WMat4Template<Type> WTransformTemplate<Type>::GetAsMat4() const
{
  auto result = m_qRotation.GetAsMat4();

  result.m_fElementsCM[0] *= m_vScale.x;
  result.m_fElementsCM[1] *= m_vScale.x;
  result.m_fElementsCM[2] *= m_vScale.x;

  result.m_fElementsCM[4] *= m_vScale.y;
  result.m_fElementsCM[5] *= m_vScale.y;
  result.m_fElementsCM[6] *= m_vScale.y;

  result.m_fElementsCM[8] *= m_vScale.z;
  result.m_fElementsCM[9] *= m_vScale.z;
  result.m_fElementsCM[10] *= m_vScale.z;

  result.m_fElementsCM[12] = m_vPosition.x;
  result.m_fElementsCM[13] = m_vPosition.y;
  result.m_fElementsCM[14] = m_vPosition.z;

  return result;
}


template <typename Type>
W_ALWAYS_INLINE void WTransformTemplate<Type>::operator+=(const WVec3Template<Type>& v)
{
  m_vPosition += v;
}

template <typename Type>
W_ALWAYS_INLINE void WTransformTemplate<Type>::operator-=(const WVec3Template<Type>& v)
{
  m_vPosition -= v;
}

template <typename Type>
W_ALWAYS_INLINE WVec3Template<Type> WTransformTemplate<Type>::TransformPosition(const WVec3Template<Type>& v) const
{
  const auto scaled = m_vScale.CompMul(v);
  const auto rotated = m_qRotation * scaled;
  return m_vPosition + rotated;
}

template <typename Type>
W_ALWAYS_INLINE WVec3Template<Type> WTransformTemplate<Type>::TransformDirection(const WVec3Template<Type>& v) const
{
  const auto scaled = m_vScale.CompMul(v);
  const auto rotated = m_qRotation * scaled;
  return rotated;
}

template <typename Type>
W_ALWAYS_INLINE const WTransformTemplate<Type> operator*(const WQuatTemplate<Type>& q, const WTransformTemplate<Type>& t)
{
  WTransformTemplate<Type> r;

  r.m_vPosition = t.m_vPosition;
  r.m_qRotation = q * t.m_qRotation;
  r.m_vScale = t.m_vScale;

  return r;
}

template <typename Type>
W_ALWAYS_INLINE const WTransformTemplate<Type> operator*(const WTransformTemplate<Type>& t, const WQuatTemplate<Type>& q)
{
  WTransformTemplate<Type> r;

  r.m_vPosition = t.m_vPosition;
  r.m_qRotation = t.m_qRotation * q;
  r.m_vScale = t.m_vScale;

  return r;
}

template <typename Type>
W_ALWAYS_INLINE const WTransformTemplate<Type> operator+(const WTransformTemplate<Type>& t, const WVec3Template<Type>& v)
{
  return WTransformTemplate<Type>(t.m_vPosition + v, t.m_qRotation, t.m_vScale);
}

template <typename Type>
W_ALWAYS_INLINE const WTransformTemplate<Type> operator-(const WTransformTemplate<Type>& t, const WVec3Template<Type>& v)
{
  return WTransformTemplate<Type>(t.m_vPosition - v, t.m_qRotation, t.m_vScale);
}

template <typename Type>
W_ALWAYS_INLINE const WVec3Template<Type> operator*(const WTransformTemplate<Type>& t, const WVec3Template<Type>& v)
{
  return t.TransformPosition(v);
}

template <typename Type>
inline const WTransformTemplate<Type> operator*(const WTransformTemplate<Type>& t1, const WTransformTemplate<Type>& t2)
{
  WTransformTemplate<Type> t;

  t.m_vPosition = (t1.m_qRotation * t2.m_vPosition.CompMul(t1.m_vScale)) + t1.m_vPosition;
  t.m_qRotation = t1.m_qRotation * t2.m_qRotation;
  t.m_vScale = t1.m_vScale.CompMul(t2.m_vScale);

  return t;
}

template <typename Type>
W_ALWAYS_INLINE bool operator==(const WTransformTemplate<Type>& t1, const WTransformTemplate<Type>& t2)
{
  return t1.IsIdentical(t2);
}

template <typename Type>
W_ALWAYS_INLINE bool operator!=(const WTransformTemplate<Type>& t1, const WTransformTemplate<Type>& t2)
{
  return !t1.IsIdentical(t2);
}

template <typename Type>
W_ALWAYS_INLINE void WTransformTemplate<Type>::Invert()
{
  (*this) = GetInverse();
}

template <typename Type>
inline const WTransformTemplate<Type> WTransformTemplate<Type>::GetInverse() const
{
  const auto invRot = m_qRotation.GetInverse();
  const auto invScale = WVec3Template<Type>(1).CompDiv(m_vScale);
  const auto invPos = invRot * (invScale.CompMul(-m_vPosition));

  return WTransformTemplate<Type>(invPos, invRot, invScale);
}
