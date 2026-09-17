#pragma once

#include <Foundation/Memory/MemoryUtils.h>

W_ALWAYS_INLINE WSimdBBox::WSimdBBox() = default;

W_ALWAYS_INLINE WSimdBBox::WSimdBBox(const WSimdVec4f& vMin, const WSimdVec4f& vMax)
  : m_Min(vMin)
  , m_Max(vMax)
{
}

W_ALWAYS_INLINE WSimdBBox WSimdBBox::MakeZero()
{
  return WSimdBBox(WSimdVec4f::MakeZero(), WSimdVec4f::MakeZero());
}

W_ALWAYS_INLINE WSimdBBox WSimdBBox::MakeInvalid()
{
  return WSimdBBox(WSimdVec4f(WMath::MaxValue<float>()), WSimdVec4f(-WMath::MaxValue<float>()));
}

W_ALWAYS_INLINE WSimdBBox WSimdBBox::MakeFromCenterAndHalfExtents(const WSimdVec4f& vCenter, const WSimdVec4f& vHalfExtents)
{
  return WSimdBBox(vCenter - vHalfExtents, vCenter + vHalfExtents);
}

W_ALWAYS_INLINE WSimdBBox WSimdBBox::MakeFromMinMax(const WSimdVec4f& vMin, const WSimdVec4f& vMax)
{
  return WSimdBBox(vMin, vMax);
}

W_ALWAYS_INLINE WSimdBBox WSimdBBox::MakeFromPoints(const WSimdVec4f* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride /*= sizeof(WSimdVec4f)*/)
{
  WSimdBBox box = WSimdBBox::MakeInvalid();
  box.ExpandToInclude(pPoints, uiNumPoints, uiStride);
  return box;
}

W_ALWAYS_INLINE void WSimdBBox::SetInvalid()
{
  m_Min.Set(WMath::MaxValue<float>());
  m_Max.Set(-WMath::MaxValue<float>());
}

W_ALWAYS_INLINE void WSimdBBox::SetCenterAndHalfExtents(const WSimdVec4f& vCenter, const WSimdVec4f& vHalfExtents)
{
  m_Min = vCenter - vHalfExtents;
  m_Max = vCenter + vHalfExtents;
}

W_ALWAYS_INLINE void WSimdBBox::SetFromPoints(const WSimdVec4f* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride)
{
  *this = MakeInvalid();
  ExpandToInclude(pPoints, uiNumPoints, uiStride);
}

W_ALWAYS_INLINE bool WSimdBBox::IsValid() const
{
  return m_Min.IsValid<3>() && m_Max.IsValid<3>() && (m_Min <= m_Max).AllSet<3>();
}

W_ALWAYS_INLINE bool WSimdBBox::IsNaN() const
{
  return m_Min.IsNaN<3>() || m_Max.IsNaN<3>();
}

W_ALWAYS_INLINE WSimdVec4f WSimdBBox::GetCenter() const
{
  return (m_Min + m_Max) * WSimdFloat(0.5f);
}

W_ALWAYS_INLINE WSimdVec4f WSimdBBox::GetExtents() const
{
  return m_Max - m_Min;
}

W_ALWAYS_INLINE WSimdVec4f WSimdBBox::GetHalfExtents() const
{
  return (m_Max - m_Min) * WSimdFloat(0.5f);
}

W_ALWAYS_INLINE void WSimdBBox::ExpandToInclude(const WSimdVec4f& vPoint)
{
  m_Min = m_Min.CompMin(vPoint);
  m_Max = m_Max.CompMax(vPoint);
}

inline void WSimdBBox::ExpandToInclude(const WSimdVec4f* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride)
{
  W_ASSERT_DEBUG(pPoints != nullptr, "Array may not be nullptr.");
  W_ASSERT_DEBUG(uiStride >= sizeof(WSimdVec4f), "Data may not overlap.");

  const WSimdVec4f* pCur = pPoints;

  for (WUInt32 i = 0; i < uiNumPoints; ++i)
  {
    ExpandToInclude(*pCur);

    pCur = WMemoryUtils::AddByteOffset(pCur, uiStride);
  }
}

W_ALWAYS_INLINE void WSimdBBox::ExpandToInclude(const WSimdBBox& rhs)
{
  m_Min = m_Min.CompMin(rhs.m_Min);
  m_Max = m_Max.CompMax(rhs.m_Max);
}

inline void WSimdBBox::ExpandToCube()
{
  const WSimdVec4f center = GetCenter();
  const WSimdVec4f halfExtents = center - m_Min;

  *this = WSimdBBox::MakeFromCenterAndHalfExtents(center, WSimdVec4f(halfExtents.HorizontalMax<3>()));
}

W_ALWAYS_INLINE bool WSimdBBox::Contains(const WSimdVec4f& vPoint) const
{
  return ((vPoint >= m_Min) && (vPoint <= m_Max)).AllSet<3>();
}

W_ALWAYS_INLINE bool WSimdBBox::Contains(const WSimdBBox& rhs) const
{
  return Contains(rhs.m_Min) && Contains(rhs.m_Max);
}

inline bool WSimdBBox::Contains(const WSimdBSphere& rhs) const
{
  const WSimdBBox otherBox = WSimdBBox::MakeFromCenterAndHalfExtents(rhs.GetCenter(), WSimdVec4f(rhs.GetRadius()));

  return Contains(otherBox);
}

W_ALWAYS_INLINE bool WSimdBBox::Overlaps(const WSimdBBox& rhs) const
{
  return ((m_Max > rhs.m_Min) && (m_Min < rhs.m_Max)).AllSet<3>();
}

inline bool WSimdBBox::Overlaps(const WSimdBSphere& rhs) const
{
  // check whether the closest point between box and sphere is inside the sphere (it is definitely inside the box)
  return rhs.Contains(GetClampedPoint(rhs.GetCenter()));
}

W_ALWAYS_INLINE void WSimdBBox::Grow(const WSimdVec4f& vDiff)
{
  m_Max += vDiff;
  m_Min -= vDiff;
}

W_ALWAYS_INLINE void WSimdBBox::Translate(const WSimdVec4f& vDiff)
{
  m_Min += vDiff;
  m_Max += vDiff;
}

W_ALWAYS_INLINE void WSimdBBox::Transform(const WSimdTransform& t)
{
  Transform(t.GetAsMat4());
}

W_ALWAYS_INLINE void WSimdBBox::Transform(const WSimdMat4f& mMat)
{
  const WSimdVec4f center = GetCenter();
  const WSimdVec4f halfExtents = center - m_Min;

  const WSimdVec4f newCenter = mMat.TransformPosition(center);

  WSimdVec4f newHalfExtents = mMat.m_col0.Abs() * halfExtents.x();
  newHalfExtents += mMat.m_col1.Abs() * halfExtents.y();
  newHalfExtents += mMat.m_col2.Abs() * halfExtents.z();

  *this = WSimdBBox::MakeFromCenterAndHalfExtents(newCenter, newHalfExtents);
}

W_ALWAYS_INLINE WSimdVec4f WSimdBBox::GetClampedPoint(const WSimdVec4f& vPoint) const
{
  return vPoint.CompMin(m_Max).CompMax(m_Min);
}

inline WSimdFloat WSimdBBox::GetDistanceSquaredTo(const WSimdVec4f& vPoint) const
{
  const WSimdVec4f vClamped = GetClampedPoint(vPoint);

  return (vPoint - vClamped).GetLengthSquared<3>();
}

inline WSimdFloat WSimdBBox::GetDistanceTo(const WSimdVec4f& vPoint) const
{
  const WSimdVec4f vClamped = GetClampedPoint(vPoint);

  return (vPoint - vClamped).GetLength<3>();
}

W_ALWAYS_INLINE bool WSimdBBox::operator==(const WSimdBBox& rhs) const
{
  return ((m_Min == rhs.m_Min) && (m_Max == rhs.m_Max)).AllSet<3>();
}

W_ALWAYS_INLINE bool WSimdBBox::operator!=(const WSimdBBox& rhs) const
{
  return ((m_Min != rhs.m_Min) || (m_Max != rhs.m_Max)).AnySet<3>();
}
