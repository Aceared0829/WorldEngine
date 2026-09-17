#pragma once

W_ALWAYS_INLINE WSimdTransformd::WSimdTransformd() = default;

W_ALWAYS_INLINE WSimdTransformd::WSimdTransformd(const WSimdVec4d& vPosition, const WSimdQuatd& qRotation, const WSimdVec4d& vScale)
  : m_Position(vPosition)
  , m_Rotation(qRotation)
  , m_Scale(vScale)
{
}

W_ALWAYS_INLINE WSimdTransformd::WSimdTransformd(const WSimdQuatd& qRotation)
  : m_Rotation(qRotation)
{
  m_Position.SetZero();
  m_Scale.Set(1.0f);
}

inline WSimdTransformd WSimdTransformd::Make(const WSimdVec4d& vPosition, const WSimdQuatd& qRotation /*= WSimdQuatd::IdentityQuaternion()*/, const WSimdVec4d& vScale /*= WSimdVec4d(1.0f)*/)
{
  WSimdTransformd res;
  res.m_Position = vPosition;
  res.m_Rotation = qRotation;
  res.m_Scale = vScale;
  return res;
}

W_ALWAYS_INLINE WSimdTransformd WSimdTransformd::MakeIdentity()
{
  WSimdTransformd res;
  res.m_Position.SetZero();
  res.m_Rotation = WSimdQuatd::MakeIdentity();
  res.m_Scale.Set(1.0f);
  return res;
}

inline WSimdTransformd WSimdTransformd::MakeLocalTransform(const WSimdTransformd& globalTransformParent, const WSimdTransformd& globalTransformChild)
{
  const WSimdQuatd invRot = -globalTransformParent.m_Rotation;
  const WSimdVec4d invScale = globalTransformParent.m_Scale.GetReciprocal();

  WSimdTransformd res;
  res.m_Position = (invRot * (globalTransformChild.m_Position - globalTransformParent.m_Position)).CompMul(invScale);
  res.m_Rotation = invRot * globalTransformChild.m_Rotation;
  res.m_Scale = invScale.CompMul(globalTransformChild.m_Scale);
  return res;
}

W_ALWAYS_INLINE WSimdTransformd WSimdTransformd::MakeGlobalTransform(const WSimdTransformd& globalTransformParent, const WSimdTransformd& localTransformChild)
{
  return globalTransformParent * localTransformChild;
}

W_ALWAYS_INLINE WSimdDouble WSimdTransformd::GetMaxScale() const
{
  return m_Scale.Abs().HorizontalMax<3>();
}

W_ALWAYS_INLINE bool WSimdTransformd::HasMirrorScaling() const
{
  return (m_Scale.x() * m_Scale.y() * m_Scale.z()) < WSimdDouble::MakeZero();
}

W_ALWAYS_INLINE bool WSimdTransformd::HasOnlyUniformScaling() const
{
  const WSimdDouble fEpsilon = WMath::DefaultEpsilon<float>();
  return m_Scale.x().IsEqual(m_Scale.y(), fEpsilon) && m_Scale.x().IsEqual(m_Scale.z(), fEpsilon);
}

W_ALWAYS_INLINE bool WSimdTransformd::IsEqual(const WSimdTransformd& rhs, const WSimdDouble& fEpsilon) const
{
  return m_Position.IsEqual(rhs.m_Position, fEpsilon).AllSet<3>() && m_Rotation.IsEqualRotation(rhs.m_Rotation, fEpsilon) &&
         m_Scale.IsEqual(rhs.m_Scale, fEpsilon).AllSet<3>();
}

W_ALWAYS_INLINE void WSimdTransformd::Invert()
{
  (*this) = GetInverse();
}

W_ALWAYS_INLINE WSimdTransformd WSimdTransformd::GetInverse() const
{
  WSimdQuatd invRot = -m_Rotation;
  WSimdVec4d invScale = m_Scale.GetReciprocal();
  WSimdVec4d invPos = invRot * (invScale.CompMul(-m_Position));

  return WSimdTransformd(invPos, invRot, invScale);
}

W_FORCE_INLINE WSimdMat4d WSimdTransformd::GetAsMat4() const
{
  WSimdMat4d result = m_Rotation.GetAsMat4();

  result.m_col0 *= m_Scale.x();
  result.m_col1 *= m_Scale.y();
  result.m_col2 *= m_Scale.z();
  result.m_col3 = m_Position;
  result.m_col3.SetW(1.0f);

  return result;
}

W_ALWAYS_INLINE WSimdVec4d WSimdTransformd::TransformPosition(const WSimdVec4d& v) const
{
  const WSimdVec4d scaled = m_Scale.CompMul(v);
  const WSimdVec4d rotated = m_Rotation * scaled;
  return m_Position + rotated;
}

W_ALWAYS_INLINE WSimdVec4d WSimdTransformd::TransformDirection(const WSimdVec4d& v) const
{
  const WSimdVec4d scaled = m_Scale.CompMul(v);
  return m_Rotation * scaled;
}

W_ALWAYS_INLINE const WSimdVec4d operator*(const WSimdTransformd& t, const WSimdVec4d& v)
{
  return t.TransformPosition(v);
}

inline const WSimdTransformd operator*(const WSimdTransformd& lhs, const WSimdTransformd& rhs)
{
  WSimdTransformd t;

  t.m_Position = (lhs.m_Rotation * rhs.m_Position.CompMul(lhs.m_Scale)) + lhs.m_Position;
  t.m_Rotation = lhs.m_Rotation * rhs.m_Rotation;
  t.m_Scale = lhs.m_Scale.CompMul(rhs.m_Scale);

  return t;
}

W_ALWAYS_INLINE void WSimdTransformd::operator*=(const WSimdTransformd& other)
{
  (*this) = (*this) * other;
}

W_ALWAYS_INLINE const WSimdTransformd operator*(const WSimdTransformd& lhs, const WSimdQuatd& q)
{
  WSimdTransformd t;
  t.m_Position = lhs.m_Position;
  t.m_Rotation = lhs.m_Rotation * q;
  t.m_Scale = lhs.m_Scale;
  return t;
}

W_ALWAYS_INLINE const WSimdTransformd operator*(const WSimdQuatd& q, const WSimdTransformd& rhs)
{
  WSimdTransformd t;
  t.m_Position = rhs.m_Position;
  t.m_Rotation = q * rhs.m_Rotation;
  t.m_Scale = rhs.m_Scale;
  return t;
}

W_ALWAYS_INLINE void WSimdTransformd::operator*=(const WSimdQuatd& q)
{
  m_Rotation = m_Rotation * q;
}

W_ALWAYS_INLINE const WSimdTransformd operator+(const WSimdTransformd& lhs, const WSimdVec4d& v)
{
  WSimdTransformd t;

  t.m_Position = lhs.m_Position + v;
  t.m_Rotation = lhs.m_Rotation;
  t.m_Scale = lhs.m_Scale;

  return t;
}

W_ALWAYS_INLINE const WSimdTransformd operator-(const WSimdTransformd& lhs, const WSimdVec4d& v)
{
  WSimdTransformd t;

  t.m_Position = lhs.m_Position - v;
  t.m_Rotation = lhs.m_Rotation;
  t.m_Scale = lhs.m_Scale;

  return t;
}

W_ALWAYS_INLINE void WSimdTransformd::operator+=(const WSimdVec4d& v)
{
  m_Position += v;
}

W_ALWAYS_INLINE void WSimdTransformd::operator-=(const WSimdVec4d& v)
{
  m_Position -= v;
}

W_ALWAYS_INLINE bool operator==(const WSimdTransformd& lhs, const WSimdTransformd& rhs)
{
  return (lhs.m_Position == rhs.m_Position).AllSet<3>() && lhs.m_Rotation == rhs.m_Rotation && (lhs.m_Scale == rhs.m_Scale).AllSet<3>();
}

W_ALWAYS_INLINE bool operator!=(const WSimdTransformd& lhs, const WSimdTransformd& rhs)
{
  return !(lhs == rhs);
}
