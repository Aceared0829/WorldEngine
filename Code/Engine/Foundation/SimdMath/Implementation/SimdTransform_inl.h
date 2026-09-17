#pragma once

W_ALWAYS_INLINE WSimdTransform::WSimdTransform() = default;

W_ALWAYS_INLINE WSimdTransform::WSimdTransform(const WSimdVec4f& vPosition, const WSimdQuat& qRotation, const WSimdVec4f& vScale)
  : m_Position(vPosition)
  , m_Rotation(qRotation)
  , m_Scale(vScale)
{
}

W_ALWAYS_INLINE WSimdTransform::WSimdTransform(const WSimdQuat& qRotation)
  : m_Rotation(qRotation)
{
  m_Position.SetZero();
  m_Scale.Set(1.0f);
}

inline WSimdTransform WSimdTransform::Make(const WSimdVec4f& vPosition, const WSimdQuat& qRotation /*= WSimdQuat::IdentityQuaternion()*/, const WSimdVec4f& vScale /*= WSimdVec4f(1.0f)*/)
{
  WSimdTransform res;
  res.m_Position = vPosition;
  res.m_Rotation = qRotation;
  res.m_Scale = vScale;
  return res;
}

W_ALWAYS_INLINE WSimdTransform WSimdTransform::MakeIdentity()
{
  WSimdTransform res;
  res.m_Position.SetZero();
  res.m_Rotation = WSimdQuat::MakeIdentity();
  res.m_Scale.Set(1.0f);
  return res;
}

inline WSimdTransform WSimdTransform::MakeLocalTransform(const WSimdTransform& globalTransformParent, const WSimdTransform& globalTransformChild)
{
  const WSimdQuat invRot = -globalTransformParent.m_Rotation;
  const WSimdVec4f invScale = globalTransformParent.m_Scale.GetReciprocal();

  WSimdTransform res;
  res.m_Position = (invRot * (globalTransformChild.m_Position - globalTransformParent.m_Position)).CompMul(invScale);
  res.m_Rotation = invRot * globalTransformChild.m_Rotation;
  res.m_Scale = invScale.CompMul(globalTransformChild.m_Scale);
  return res;
}

W_ALWAYS_INLINE WSimdTransform WSimdTransform::MakeGlobalTransform(const WSimdTransform& globalTransformParent, const WSimdTransform& localTransformChild)
{
  return globalTransformParent * localTransformChild;
}

W_ALWAYS_INLINE WSimdFloat WSimdTransform::GetMaxScale() const
{
  return m_Scale.Abs().HorizontalMax<3>();
}

W_ALWAYS_INLINE bool WSimdTransform::HasMirrorScaling() const
{
  return (m_Scale.x() * m_Scale.y() * m_Scale.z()) < WSimdFloat::MakeZero();
}

W_ALWAYS_INLINE bool WSimdTransform::HasOnlyUniformScaling() const
{
  const WSimdFloat fEpsilon = WMath::DefaultEpsilon<float>();
  return m_Scale.x().IsEqual(m_Scale.y(), fEpsilon) && m_Scale.x().IsEqual(m_Scale.z(), fEpsilon);
}

W_ALWAYS_INLINE bool WSimdTransform::IsEqual(const WSimdTransform& rhs, const WSimdFloat& fEpsilon) const
{
  return m_Position.IsEqual(rhs.m_Position, fEpsilon).AllSet<3>() && m_Rotation.IsEqualRotation(rhs.m_Rotation, fEpsilon) &&
         m_Scale.IsEqual(rhs.m_Scale, fEpsilon).AllSet<3>();
}

W_ALWAYS_INLINE void WSimdTransform::Invert()
{
  (*this) = GetInverse();
}

W_ALWAYS_INLINE WSimdTransform WSimdTransform::GetInverse() const
{
  WSimdQuat invRot = -m_Rotation;
  WSimdVec4f invScale = m_Scale.GetReciprocal();
  WSimdVec4f invPos = invRot * (invScale.CompMul(-m_Position));

  return WSimdTransform(invPos, invRot, invScale);
}

W_FORCE_INLINE WSimdMat4f WSimdTransform::GetAsMat4() const
{
  WSimdMat4f result = m_Rotation.GetAsMat4();

  result.m_col0 *= m_Scale.x();
  result.m_col1 *= m_Scale.y();
  result.m_col2 *= m_Scale.z();
  result.m_col3 = m_Position;
  result.m_col3.SetW(1.0f);

  return result;
}

W_ALWAYS_INLINE WSimdVec4f WSimdTransform::TransformPosition(const WSimdVec4f& v) const
{
  const WSimdVec4f scaled = m_Scale.CompMul(v);
  const WSimdVec4f rotated = m_Rotation * scaled;
  return m_Position + rotated;
}

W_ALWAYS_INLINE WSimdVec4f WSimdTransform::TransformDirection(const WSimdVec4f& v) const
{
  const WSimdVec4f scaled = m_Scale.CompMul(v);
  return m_Rotation * scaled;
}

W_ALWAYS_INLINE const WSimdVec4f operator*(const WSimdTransform& t, const WSimdVec4f& v)
{
  return t.TransformPosition(v);
}

inline const WSimdTransform operator*(const WSimdTransform& lhs, const WSimdTransform& rhs)
{
  WSimdTransform t;

  t.m_Position = (lhs.m_Rotation * rhs.m_Position.CompMul(lhs.m_Scale)) + lhs.m_Position;
  t.m_Rotation = lhs.m_Rotation * rhs.m_Rotation;
  t.m_Scale = lhs.m_Scale.CompMul(rhs.m_Scale);

  return t;
}

W_ALWAYS_INLINE void WSimdTransform::operator*=(const WSimdTransform& other)
{
  (*this) = (*this) * other;
}

W_ALWAYS_INLINE const WSimdTransform operator*(const WSimdTransform& lhs, const WSimdQuat& q)
{
  WSimdTransform t;
  t.m_Position = lhs.m_Position;
  t.m_Rotation = lhs.m_Rotation * q;
  t.m_Scale = lhs.m_Scale;
  return t;
}

W_ALWAYS_INLINE const WSimdTransform operator*(const WSimdQuat& q, const WSimdTransform& rhs)
{
  WSimdTransform t;
  t.m_Position = rhs.m_Position;
  t.m_Rotation = q * rhs.m_Rotation;
  t.m_Scale = rhs.m_Scale;
  return t;
}

W_ALWAYS_INLINE void WSimdTransform::operator*=(const WSimdQuat& q)
{
  m_Rotation = m_Rotation * q;
}

W_ALWAYS_INLINE const WSimdTransform operator+(const WSimdTransform& lhs, const WSimdVec4f& v)
{
  WSimdTransform t;

  t.m_Position = lhs.m_Position + v;
  t.m_Rotation = lhs.m_Rotation;
  t.m_Scale = lhs.m_Scale;

  return t;
}

W_ALWAYS_INLINE const WSimdTransform operator-(const WSimdTransform& lhs, const WSimdVec4f& v)
{
  WSimdTransform t;

  t.m_Position = lhs.m_Position - v;
  t.m_Rotation = lhs.m_Rotation;
  t.m_Scale = lhs.m_Scale;

  return t;
}

W_ALWAYS_INLINE void WSimdTransform::operator+=(const WSimdVec4f& v)
{
  m_Position += v;
}

W_ALWAYS_INLINE void WSimdTransform::operator-=(const WSimdVec4f& v)
{
  m_Position -= v;
}

W_ALWAYS_INLINE bool operator==(const WSimdTransform& lhs, const WSimdTransform& rhs)
{
  return (lhs.m_Position == rhs.m_Position).AllSet<3>() && lhs.m_Rotation == rhs.m_Rotation && (lhs.m_Scale == rhs.m_Scale).AllSet<3>();
}

W_ALWAYS_INLINE bool operator!=(const WSimdTransform& lhs, const WSimdTransform& rhs)
{
  return !(lhs == rhs);
}
