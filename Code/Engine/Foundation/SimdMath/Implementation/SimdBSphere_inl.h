#pragma once

#include <Foundation/Memory/MemoryUtils.h>

W_ALWAYS_INLINE WSimdBSphere::WSimdBSphere() = default;

W_ALWAYS_INLINE WSimdBSphere::WSimdBSphere(const WSimdVec4f& vCenter, const WSimdFloat& fRadius)
  : m_CenterAndRadius(vCenter)
{
  m_CenterAndRadius.SetW(fRadius);
}

W_ALWAYS_INLINE WSimdBSphere WSimdBSphere::MakeZero()
{
  WSimdBSphere res;
  res.m_CenterAndRadius = WSimdVec4f::MakeZero();
  return res;
}

W_ALWAYS_INLINE WSimdBSphere WSimdBSphere::MakeInvalid(const WSimdVec4f& vCenter /*= WSimdVec4f::MakeZero()*/)
{
  WSimdBSphere res;
  res.m_CenterAndRadius = vCenter;
  res.m_CenterAndRadius.SetW(-WMath::SmallEpsilon<float>());
  return res;
}

W_ALWAYS_INLINE WSimdBSphere WSimdBSphere::MakeFromCenterAndRadius(const WSimdVec4f& vCenter, const WSimdFloat& fRadius)
{
  return WSimdBSphere(vCenter, fRadius);
}

inline WSimdBSphere WSimdBSphere::MakeFromPoints(const WSimdVec4f* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride /*= sizeof(WSimdVec4f)*/)
{
  W_ASSERT_DEBUG(pPoints != nullptr, "The array must not be empty.");
  W_ASSERT_DEBUG(uiStride >= sizeof(WSimdVec4f), "The data must not overlap.");
  W_ASSERT_DEBUG(uiNumPoints > 0, "The array must contain at least one point.");

  WSimdBSphere res;

  const WSimdVec4f* pCur = pPoints;

  WSimdVec4f vCenter = WSimdVec4f::MakeZero();
  for (WUInt32 i = 0; i < uiNumPoints; ++i)
  {
    vCenter += *pCur;
    pCur = WMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  res.m_CenterAndRadius = vCenter / WSimdFloat(uiNumPoints);

  pCur = pPoints;

  WSimdFloat fMaxDistSquare = WSimdFloat::MakeZero();
  for (WUInt32 i = 0; i < uiNumPoints; ++i)
  {
    const WSimdFloat fDistSQR = (*pCur - res.m_CenterAndRadius).GetLengthSquared<3>();
    fMaxDistSquare = fMaxDistSquare.Max(fDistSQR);

    pCur = WMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  res.m_CenterAndRadius.SetW(fMaxDistSquare.GetSqrt());

  return res;
}

W_ALWAYS_INLINE void WSimdBSphere::SetInvalid()
{
  m_CenterAndRadius.Set(0.0f, 0.0f, 0.0f, -WMath::SmallEpsilon<float>());
}

W_ALWAYS_INLINE bool WSimdBSphere::IsValid() const
{
  return m_CenterAndRadius.IsValid<4>() && GetRadius() >= WSimdFloat::MakeZero();
}

W_ALWAYS_INLINE bool WSimdBSphere::IsNaN() const
{
  return m_CenterAndRadius.IsNaN<4>();
}

W_ALWAYS_INLINE WSimdVec4f WSimdBSphere::GetCenter() const
{
  return m_CenterAndRadius;
}

W_ALWAYS_INLINE WSimdFloat WSimdBSphere::GetRadius() const
{
  return m_CenterAndRadius.w();
}

W_ALWAYS_INLINE void WSimdBSphere::SetFromPoints(const WSimdVec4f* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride)
{
  *this = MakeFromPoints(pPoints, uiNumPoints, uiStride);
}

W_ALWAYS_INLINE void WSimdBSphere::ExpandToInclude(const WSimdVec4f& vPoint)
{
  const WSimdFloat fDist = (vPoint - m_CenterAndRadius).GetLength<3>();

  m_CenterAndRadius.SetW(fDist.Max(GetRadius()));
}

inline void WSimdBSphere::ExpandToInclude(const WSimdVec4f* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride)
{
  W_ASSERT_DEBUG(pPoints != nullptr, "The array must not be empty.");
  W_ASSERT_DEBUG(uiStride >= sizeof(WSimdVec4f), "The data must not overlap.");

  const WSimdVec4f* pCur = pPoints;

  WSimdFloat fMaxDistSquare = WSimdFloat::MakeZero();

  for (WUInt32 i = 0; i < uiNumPoints; ++i)
  {
    const WSimdFloat fDistSQR = (*pCur - m_CenterAndRadius).GetLengthSquared<3>();
    fMaxDistSquare = fMaxDistSquare.Max(fDistSQR);

    pCur = WMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  m_CenterAndRadius.SetW(fMaxDistSquare.GetSqrt().Max(GetRadius()));
}

W_ALWAYS_INLINE void WSimdBSphere::ExpandToInclude(const WSimdBSphere& rhs)
{
  const WSimdFloat fReqRadius = (rhs.m_CenterAndRadius - m_CenterAndRadius).GetLength<3>() + rhs.GetRadius();

  m_CenterAndRadius.SetW(fReqRadius.Max(GetRadius()));
}

inline void WSimdBSphere::Transform(const WSimdTransform& t)
{
  WSimdVec4f newCenterAndRadius = t.TransformPosition(m_CenterAndRadius);
  newCenterAndRadius.SetW(t.GetMaxScale() * GetRadius());

  m_CenterAndRadius = newCenterAndRadius;
}

inline void WSimdBSphere::Transform(const WSimdMat4f& mMat)
{
  WSimdFloat radius = m_CenterAndRadius.w();
  m_CenterAndRadius = mMat.TransformPosition(m_CenterAndRadius);

  WSimdFloat maxRadius = mMat.m_col0.Dot<3>(mMat.m_col0);
  maxRadius = maxRadius.Max(mMat.m_col1.Dot<3>(mMat.m_col1));
  maxRadius = maxRadius.Max(mMat.m_col2.Dot<3>(mMat.m_col2));
  radius *= maxRadius.GetSqrt();

  m_CenterAndRadius.SetW(radius);
}

W_ALWAYS_INLINE WSimdFloat WSimdBSphere::GetDistanceTo(const WSimdVec4f& vPoint) const
{
  return (vPoint - m_CenterAndRadius).GetLength<3>() - GetRadius();
}

W_ALWAYS_INLINE WSimdFloat WSimdBSphere::GetDistanceTo(const WSimdBSphere& rhs) const
{
  return (rhs.m_CenterAndRadius - m_CenterAndRadius).GetLength<3>() - GetRadius() - rhs.GetRadius();
}

W_ALWAYS_INLINE bool WSimdBSphere::Contains(const WSimdVec4f& vPoint) const
{
  WSimdFloat radius = GetRadius();
  return (vPoint - m_CenterAndRadius).GetLengthSquared<3>() <= (radius * radius);
}

W_ALWAYS_INLINE bool WSimdBSphere::Contains(const WSimdBSphere& rhs) const
{
  return (rhs.m_CenterAndRadius - m_CenterAndRadius).GetLength<3>() + rhs.GetRadius() <= GetRadius();
}

W_ALWAYS_INLINE bool WSimdBSphere::Overlaps(const WSimdBSphere& rhs) const
{
  WSimdFloat radius = (rhs.m_CenterAndRadius + m_CenterAndRadius).w();
  return (rhs.m_CenterAndRadius - m_CenterAndRadius).GetLengthSquared<3>() < (radius * radius);
}

inline WSimdVec4f WSimdBSphere::GetClampedPoint(const WSimdVec4f& vPoint)
{
  WSimdVec4f vDir = vPoint - m_CenterAndRadius;
  WSimdFloat fDist = vDir.GetLengthAndNormalize<3>().Min(GetRadius());

  return m_CenterAndRadius + (vDir * fDist);
}

W_ALWAYS_INLINE bool WSimdBSphere::operator==(const WSimdBSphere& rhs) const
{
  return (m_CenterAndRadius == rhs.m_CenterAndRadius).AllSet();
}

W_ALWAYS_INLINE bool WSimdBSphere::operator!=(const WSimdBSphere& rhs) const
{
  return (m_CenterAndRadius != rhs.m_CenterAndRadius).AnySet();
}
