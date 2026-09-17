#pragma once

W_ALWAYS_INLINE WSimdBBoxSphere::WSimdBBoxSphere() = default;

W_ALWAYS_INLINE WSimdBBoxSphere::WSimdBBoxSphere(const WSimdVec4f& vCenter, const WSimdVec4f& vBoxHalfExtents, const WSimdFloat& fSphereRadius)
  : m_CenterAndRadius(vCenter)
  , m_BoxHalfExtents(vBoxHalfExtents)
{
  m_CenterAndRadius.SetW(fSphereRadius);
}

inline WSimdBBoxSphere::WSimdBBoxSphere(const WSimdBBox& box, const WSimdBSphere& sphere)
{
  *this = MakeFromBoxAndSphere(box, sphere);
}

inline WSimdBBoxSphere::WSimdBBoxSphere(const WSimdBBox& box)
  : m_CenterAndRadius(box.GetCenter())
  , m_BoxHalfExtents(m_CenterAndRadius - box.m_Min)
{
  m_CenterAndRadius.SetW(m_BoxHalfExtents.GetLength<3>());
}

W_ALWAYS_INLINE WSimdBBoxSphere::WSimdBBoxSphere(const WSimdBSphere& sphere)
  : m_CenterAndRadius(sphere.m_CenterAndRadius)
  , m_BoxHalfExtents(WSimdVec4f(sphere.GetRadius()))
{
}

W_ALWAYS_INLINE WSimdBBoxSphere WSimdBBoxSphere::MakeZero()
{
  WSimdBBoxSphere res;
  res.m_CenterAndRadius = WSimdVec4f::MakeZero();
  res.m_BoxHalfExtents = WSimdVec4f::MakeZero();
  return res;
}

W_ALWAYS_INLINE WSimdBBoxSphere WSimdBBoxSphere::MakeInvalid()
{
  WSimdBBoxSphere res;
  res.m_CenterAndRadius.Set(0.0f, 0.0f, 0.0f, -WMath::SmallEpsilon<float>());
  res.m_BoxHalfExtents.Set(-WMath::MaxValue<float>());
  return res;
}

W_ALWAYS_INLINE WSimdBBoxSphere WSimdBBoxSphere::MakeFromCenterExtents(const WSimdVec4f& vCenter, const WSimdVec4f& vBoxHalfExtents, const WSimdFloat& fSphereRadius)
{
  WSimdBBoxSphere res;
  res.m_CenterAndRadius = vCenter;
  res.m_BoxHalfExtents = vBoxHalfExtents;
  res.m_CenterAndRadius.SetW(fSphereRadius);
  return res;
}

inline WSimdBBoxSphere WSimdBBoxSphere::MakeFromPoints(const WSimdVec4f* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride /*= sizeof(WSimdVec4f)*/)
{
  const WSimdBBox box = WSimdBBox::MakeFromPoints(pPoints, uiNumPoints, uiStride);

  WSimdBBoxSphere res;

  res.m_CenterAndRadius = box.GetCenter();
  res.m_BoxHalfExtents = res.m_CenterAndRadius - box.m_Min;

  WSimdBSphere sphere(res.m_CenterAndRadius, WSimdFloat::MakeZero());
  sphere.ExpandToInclude(pPoints, uiNumPoints, uiStride);

  res.m_CenterAndRadius.SetW(sphere.GetRadius());

  return res;
}

W_ALWAYS_INLINE WSimdBBoxSphere WSimdBBoxSphere::MakeFromBox(const WSimdBBox& box)
{
  return WSimdBBoxSphere(box);
}

W_ALWAYS_INLINE WSimdBBoxSphere WSimdBBoxSphere::MakeFromSphere(const WSimdBSphere& sphere)
{
  return WSimdBBoxSphere(sphere);
}

W_ALWAYS_INLINE WSimdBBoxSphere WSimdBBoxSphere::MakeFromBoxAndSphere(const WSimdBBox& box, const WSimdBSphere& sphere)
{
  WSimdBBoxSphere res;
  res.m_CenterAndRadius = box.GetCenter();
  res.m_BoxHalfExtents = res.m_CenterAndRadius - box.m_Min;
  res.m_CenterAndRadius.SetW(res.m_BoxHalfExtents.GetLength<3>().Min((sphere.GetCenter() - res.m_CenterAndRadius).GetLength<3>() + sphere.GetRadius()));
  return res;
}

W_ALWAYS_INLINE void WSimdBBoxSphere::SetInvalid()
{
  m_CenterAndRadius.Set(0.0f, 0.0f, 0.0f, -WMath::SmallEpsilon<float>());
  m_BoxHalfExtents.Set(-WMath::MaxValue<float>());
}

W_ALWAYS_INLINE bool WSimdBBoxSphere::IsValid() const
{
  return m_CenterAndRadius.IsValid<4>() && m_CenterAndRadius.w() >= WSimdFloat::MakeZero() && m_BoxHalfExtents.IsValid<3>() &&
         (m_BoxHalfExtents >= WSimdVec4f::MakeZero()).AllSet<3>();
}

inline bool WSimdBBoxSphere::IsNaN() const
{
  return m_CenterAndRadius.IsNaN<4>() || m_BoxHalfExtents.IsNaN<3>();
}

W_ALWAYS_INLINE void WSimdBBoxSphere::SetFromPoints(const WSimdVec4f* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride)
{
  *this = MakeFromPoints(pPoints, uiNumPoints, uiStride);
}

W_ALWAYS_INLINE WSimdBBox WSimdBBoxSphere::GetBox() const
{
  return WSimdBBox::MakeFromCenterAndHalfExtents(m_CenterAndRadius, m_BoxHalfExtents);
}

W_ALWAYS_INLINE WSimdBSphere WSimdBBoxSphere::GetSphere() const
{
  WSimdBSphere sphere;
  sphere.m_CenterAndRadius = m_CenterAndRadius;
  return sphere;
}

inline void WSimdBBoxSphere::ExpandToInclude(const WSimdBBoxSphere& rhs)
{
  WSimdBBox box = GetBox();
  box.ExpandToInclude(rhs.GetBox());

  WSimdVec4f center = box.GetCenter();
  WSimdVec4f boxHalfExtents = center - box.m_Min;
  WSimdFloat tmpRadius = boxHalfExtents.GetLength<3>();

  const WSimdFloat fSphereRadiusA = (m_CenterAndRadius - center).GetLength<3>() + m_CenterAndRadius.w();
  const WSimdFloat fSphereRadiusB = (rhs.m_CenterAndRadius - center).GetLength<3>() + rhs.m_CenterAndRadius.w();

  m_CenterAndRadius = center;
  m_CenterAndRadius.SetW(tmpRadius.Min(fSphereRadiusA.Max(fSphereRadiusB)));
  m_BoxHalfExtents = boxHalfExtents;
}

W_ALWAYS_INLINE void WSimdBBoxSphere::Transform(const WSimdTransform& t)
{
  Transform(t.GetAsMat4());
}

W_ALWAYS_INLINE void WSimdBBoxSphere::Transform(const WSimdMat4f& mMat)
{
  WSimdFloat radius = m_CenterAndRadius.w();
  m_CenterAndRadius = mMat.TransformPosition(m_CenterAndRadius);

  WSimdFloat maxRadius = mMat.m_col0.Dot<3>(mMat.m_col0);
  maxRadius = maxRadius.Max(mMat.m_col1.Dot<3>(mMat.m_col1));
  maxRadius = maxRadius.Max(mMat.m_col2.Dot<3>(mMat.m_col2));
  radius *= maxRadius.GetSqrt();

  m_CenterAndRadius.SetW(radius);

  WSimdVec4f newHalfExtents = mMat.m_col0.Abs() * m_BoxHalfExtents.x();
  newHalfExtents += mMat.m_col1.Abs() * m_BoxHalfExtents.y();
  newHalfExtents += mMat.m_col2.Abs() * m_BoxHalfExtents.z();

  m_BoxHalfExtents = newHalfExtents.CompMin(WSimdVec4f(radius));
}

W_ALWAYS_INLINE bool WSimdBBoxSphere::operator==(const WSimdBBoxSphere& rhs) const
{
  return (m_CenterAndRadius == rhs.m_CenterAndRadius).AllSet<4>() && (m_BoxHalfExtents == rhs.m_BoxHalfExtents).AllSet<3>();
}

W_ALWAYS_INLINE bool WSimdBBoxSphere::operator!=(const WSimdBBoxSphere& rhs) const
{
  return !(*this == rhs);
}
