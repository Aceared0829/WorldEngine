#pragma once

#include <Foundation/Math/Mat4.h>

template <typename Type>
W_ALWAYS_INLINE WBoundingBoxTemplate<Type>::WBoundingBoxTemplate() = default;

template <typename Type>
W_FORCE_INLINE WBoundingBoxTemplate<Type>::WBoundingBoxTemplate(const WVec3Template<Type>& vMin, const WVec3Template<Type>& vMax)
{
  *this = MakeFromMinMax(vMin, vMax);
}

template <typename Type>
W_FORCE_INLINE WBoundingBoxTemplate<Type> WBoundingBoxTemplate<Type>::MakeZero()
{
  WBoundingBoxTemplate<Type> res;
  res.m_vMin = WVec3Template<Type>::MakeZero();
  res.m_vMax = WVec3Template<Type>::MakeZero();
  return res;
}

template <typename Type>
W_FORCE_INLINE WBoundingBoxTemplate<Type> WBoundingBoxTemplate<Type>::MakeInvalid()
{
  WBoundingBoxTemplate<Type> res;
  res.m_vMin.Set(WMath::MaxValue<Type>());
  res.m_vMax.Set(-WMath::MaxValue<Type>());
  return res;
}

template <typename Type>
W_FORCE_INLINE WBoundingBoxTemplate<Type> WBoundingBoxTemplate<Type>::MakeFromCenterAndHalfExtents(const WVec3Template<Type>& vCenter, const WVec3Template<Type>& vHalfExtents)
{
  WBoundingBoxTemplate<Type> res;
  res.m_vMin = vCenter - vHalfExtents;
  res.m_vMax = vCenter + vHalfExtents;
  return res;
}

template <typename Type>
W_FORCE_INLINE WBoundingBoxTemplate<Type> WBoundingBoxTemplate<Type>::MakeFromMinMax(const WVec3Template<Type>& vMin, const WVec3Template<Type>& vMax)
{
  WBoundingBoxTemplate<Type> res;
  res.m_vMin = vMin;
  res.m_vMax = vMax;

  W_ASSERT_DEBUG(res.IsValid(), "The given values don't create a valid bounding box ({0} | {1} | {2} - {3} | {4} | {5})", WArgF(vMin.x, 2), WArgF(vMin.y, 2), WArgF(vMin.z, 2), WArgF(vMax.x, 2), WArgF(vMax.y, 2), WArgF(vMax.z, 2));

  return res;
}

template <typename Type>
W_FORCE_INLINE WBoundingBoxTemplate<Type> WBoundingBoxTemplate<Type>::MakeFromPoints(const WVec3Template<Type>* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride /*= sizeof(WVec3Template<Type>)*/)
{
  WBoundingBoxTemplate<Type> res = MakeInvalid();
  res.ExpandToInclude(pPoints, uiNumPoints, uiStride);
  return res;
}

template <typename Type>
void WBoundingBoxTemplate<Type>::GetCorners(WVec3Template<Type>* out_pCorners) const
{
  W_NAN_ASSERT(this);
  W_ASSERT_DEBUG(out_pCorners != nullptr, "Out Parameter must not be nullptr.");

  out_pCorners[0].Set(m_vMin.x, m_vMin.y, m_vMin.z);
  out_pCorners[1].Set(m_vMin.x, m_vMin.y, m_vMax.z);
  out_pCorners[2].Set(m_vMin.x, m_vMax.y, m_vMin.z);
  out_pCorners[3].Set(m_vMin.x, m_vMax.y, m_vMax.z);
  out_pCorners[4].Set(m_vMax.x, m_vMin.y, m_vMin.z);
  out_pCorners[5].Set(m_vMax.x, m_vMin.y, m_vMax.z);
  out_pCorners[6].Set(m_vMax.x, m_vMax.y, m_vMin.z);
  out_pCorners[7].Set(m_vMax.x, m_vMax.y, m_vMax.z);
}

template <typename Type>
W_FORCE_INLINE const WVec3Template<Type> WBoundingBoxTemplate<Type>::GetCenter() const
{
  return m_vMin + GetHalfExtents();
}

template <typename Type>
W_ALWAYS_INLINE const WVec3Template<Type> WBoundingBoxTemplate<Type>::GetExtents() const
{
  return m_vMax - m_vMin;
}

template <typename Type>
W_FORCE_INLINE const WVec3Template<Type> WBoundingBoxTemplate<Type>::GetHalfExtents() const
{
  return (m_vMax - m_vMin) / (Type)2;
}

template <typename Type>
bool WBoundingBoxTemplate<Type>::IsValid() const
{
  return (m_vMin.IsValid() && m_vMax.IsValid() && m_vMin.x <= m_vMax.x && m_vMin.y <= m_vMax.y && m_vMin.z <= m_vMax.z);
}

template <typename Type>
bool WBoundingBoxTemplate<Type>::IsNaN() const
{
  return m_vMin.IsNaN() || m_vMax.IsNaN();
}

template <typename Type>
W_FORCE_INLINE void WBoundingBoxTemplate<Type>::ExpandToInclude(const WVec3Template<Type>& vPoint)
{
  m_vMin = m_vMin.CompMin(vPoint);
  m_vMax = m_vMax.CompMax(vPoint);
}

template <typename Type>
W_FORCE_INLINE void WBoundingBoxTemplate<Type>::ExpandToInclude(const WBoundingBoxTemplate<Type>& rhs)
{
  W_ASSERT_DEBUG(rhs.IsValid(), "rhs must be a valid AABB.");
  m_vMin = m_vMin.CompMin(rhs.m_vMin);
  m_vMax = m_vMax.CompMax(rhs.m_vMax);
}

template <typename Type>
void WBoundingBoxTemplate<Type>::ExpandToInclude(const WVec3Template<Type>* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride)
{
  W_ASSERT_DEBUG(pPoints != nullptr, "Array may not be nullptr.");
  W_ASSERT_DEBUG(uiStride >= sizeof(WVec3Template<Type>), "Data may not overlap.");

  const WVec3Template<Type>* pCur = &pPoints[0];

  for (WUInt32 i = 0; i < uiNumPoints; ++i)
  {
    ExpandToInclude(*pCur);

    pCur = WMemoryUtils::AddByteOffset(pCur, uiStride);
  }
}

template <typename Type>
void WBoundingBoxTemplate<Type>::ExpandToCube()
{
  WVec3Template<Type> vHalfExtents = GetHalfExtents();
  const WVec3Template<Type> vCenter = m_vMin + vHalfExtents;

  const Type f = WMath::Max(vHalfExtents.x, vHalfExtents.y, vHalfExtents.z);

  m_vMin = vCenter - WVec3Template<Type>(f);
  m_vMax = vCenter + WVec3Template<Type>(f);
}

template <typename Type>
W_FORCE_INLINE void WBoundingBoxTemplate<Type>::Grow(const WVec3Template<Type>& vDiff)
{
  W_ASSERT_DEBUG(IsValid(), "Cannot grow a box that is invalid.");

  m_vMax += vDiff;
  m_vMin -= vDiff;

  W_ASSERT_DEBUG(IsValid(), "The grown box has become invalid.");
}

template <typename Type>
W_FORCE_INLINE bool WBoundingBoxTemplate<Type>::Contains(const WVec3Template<Type>& vPoint) const
{
  W_NAN_ASSERT(this);
  W_NAN_ASSERT(&vPoint);

  return (WMath::IsInRange(vPoint.x, m_vMin.x, m_vMax.x) && WMath::IsInRange(vPoint.y, m_vMin.y, m_vMax.y) &&
          WMath::IsInRange(vPoint.z, m_vMin.z, m_vMax.z));
}

template <typename Type>
W_FORCE_INLINE bool WBoundingBoxTemplate<Type>::Contains(const WBoundingBoxTemplate<Type>& rhs) const
{
  return Contains(rhs.m_vMin) && Contains(rhs.m_vMax);
}

template <typename Type>
bool WBoundingBoxTemplate<Type>::Contains(const WVec3Template<Type>* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride /* = sizeof(WVec3Template<Type>) */) const
{
  W_ASSERT_DEBUG(pPoints != nullptr, "Array must not be NuLL.");
  W_ASSERT_DEBUG(uiStride >= sizeof(WVec3Template<Type>), "Data must not overlap.");

  const WVec3Template<Type>* pCur = &pPoints[0];

  for (WUInt32 i = 0; i < uiNumPoints; ++i)
  {
    if (!Contains(*pCur))
      return false;

    pCur = WMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  return true;
}

template <typename Type>
bool WBoundingBoxTemplate<Type>::Overlaps(const WBoundingBoxTemplate<Type>& rhs) const
{
  W_NAN_ASSERT(this);
  W_NAN_ASSERT(&rhs);

  if (rhs.m_vMin.x >= m_vMax.x)
    return false;
  if (rhs.m_vMin.y >= m_vMax.y)
    return false;
  if (rhs.m_vMin.z >= m_vMax.z)
    return false;

  if (m_vMin.x >= rhs.m_vMax.x)
    return false;
  if (m_vMin.y >= rhs.m_vMax.y)
    return false;
  if (m_vMin.z >= rhs.m_vMax.z)
    return false;

  return true;
}

template <typename Type>
bool WBoundingBoxTemplate<Type>::Overlaps(const WVec3Template<Type>* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride /* = sizeof(WVec3Template<Type>) */) const
{
  W_ASSERT_DEBUG(pPoints != nullptr, "Array must not be NuLL.");
  W_ASSERT_DEBUG(uiStride >= sizeof(WVec3Template<Type>), "Data must not overlap.");

  const WVec3Template<Type>* pCur = &pPoints[0];

  for (WUInt32 i = 0; i < uiNumPoints; ++i)
  {
    if (Contains(*pCur))
      return true;

    pCur = WMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  return false;
}

template <typename Type>
W_ALWAYS_INLINE bool WBoundingBoxTemplate<Type>::IsIdentical(const WBoundingBoxTemplate<Type>& rhs) const
{
  return (m_vMin == rhs.m_vMin && m_vMax == rhs.m_vMax);
}

template <typename Type>
bool WBoundingBoxTemplate<Type>::IsEqual(const WBoundingBoxTemplate<Type>& rhs, Type fEpsilon) const
{
  return (m_vMin.IsEqual(rhs.m_vMin, fEpsilon) && m_vMax.IsEqual(rhs.m_vMax, fEpsilon));
}

template <typename Type>
W_ALWAYS_INLINE bool operator==(const WBoundingBoxTemplate<Type>& lhs, const WBoundingBoxTemplate<Type>& rhs)
{
  return lhs.IsIdentical(rhs);
}

template <typename Type>
W_ALWAYS_INLINE bool operator!=(const WBoundingBoxTemplate<Type>& lhs, const WBoundingBoxTemplate<Type>& rhs)
{
  return !lhs.IsIdentical(rhs);
}

template <typename Type>
W_FORCE_INLINE void WBoundingBoxTemplate<Type>::Translate(const WVec3Template<Type>& vDiff)
{
  m_vMin += vDiff;
  m_vMax += vDiff;
}

template <typename Type>
void WBoundingBoxTemplate<Type>::ScaleFromCenter(const WVec3Template<Type>& vScale)
{
  const WVec3Template<Type> vCenter = GetCenter();
  const WVec3Template<Type> vNewMin = vCenter + (m_vMin - vCenter).CompMul(vScale);
  const WVec3Template<Type> vNewMax = vCenter + (m_vMax - vCenter).CompMul(vScale);

  // this is necessary for negative scalings to work as expected
  m_vMin = vNewMin.CompMin(vNewMax);
  m_vMax = vNewMin.CompMax(vNewMax);
}

template <typename Type>
W_FORCE_INLINE void WBoundingBoxTemplate<Type>::ScaleFromOrigin(const WVec3Template<Type>& vScale)
{
  const WVec3Template<Type> vNewMin = m_vMin.CompMul(vScale);
  const WVec3Template<Type> vNewMax = m_vMax.CompMul(vScale);

  // this is necessary for negative scalings to work as expected
  m_vMin = vNewMin.CompMin(vNewMax);
  m_vMax = vNewMin.CompMax(vNewMax);
}

template <typename Type>
void WBoundingBoxTemplate<Type>::TransformFromCenter(const WMat4Template<Type>& mTransform)
{
  WVec3Template<Type> vCorners[8];
  GetCorners(vCorners);

  const WVec3Template<Type> vCenter = GetCenter();
  *this = MakeInvalid();

  for (WUInt32 i = 0; i < 8; ++i)
    ExpandToInclude(vCenter + mTransform.TransformPosition(vCorners[i] - vCenter));
}

template <typename Type>
void WBoundingBoxTemplate<Type>::TransformFromOrigin(const WMat4Template<Type>& mTransform)
{
  WVec3Template<Type> vCorners[8];
  GetCorners(vCorners);

  mTransform.TransformPosition(vCorners, 8);

  *this = MakeInvalid();
  ExpandToInclude(vCorners, 8);
}

template <typename Type>
W_FORCE_INLINE const WVec3Template<Type> WBoundingBoxTemplate<Type>::GetClampedPoint(const WVec3Template<Type>& vPoint) const
{
  return vPoint.CompMin(m_vMax).CompMax(m_vMin);
}

template <typename Type>
Type WBoundingBoxTemplate<Type>::GetDistanceTo(const WVec3Template<Type>& vPoint) const
{
  const WVec3Template<Type> vClamped = GetClampedPoint(vPoint);

  return (vPoint - vClamped).GetLength();
}

template <typename Type>
Type WBoundingBoxTemplate<Type>::GetDistanceSquaredTo(const WVec3Template<Type>& vPoint) const
{
  const WVec3Template<Type> vClamped = GetClampedPoint(vPoint);

  return (vPoint - vClamped).GetLengthSquared();
}

template <typename Type>
Type WBoundingBoxTemplate<Type>::GetDistanceSquaredTo(const WBoundingBoxTemplate<Type>& rhs) const
{
  // This will return zero for overlapping boxes

  W_NAN_ASSERT(this);
  W_NAN_ASSERT(&rhs);

  Type fDistSQR = (Type)0.0;

  {
    if (rhs.m_vMin.x > m_vMax.x)
    {
      fDistSQR += WMath::Square(rhs.m_vMin.x - m_vMax.x);
    }
    else if (rhs.m_vMax.x < m_vMin.x)
    {
      fDistSQR += WMath::Square(m_vMin.x - rhs.m_vMax.x);
    }
  }

  {
    if (rhs.m_vMin.y > m_vMax.y)
    {
      fDistSQR += WMath::Square(rhs.m_vMin.y - m_vMax.y);
    }
    else if (rhs.m_vMax.y < m_vMin.y)
    {
      fDistSQR += WMath::Square(m_vMin.y - rhs.m_vMax.y);
    }
  }

  {
    if (rhs.m_vMin.z > m_vMax.z)
    {
      fDistSQR += WMath::Square(rhs.m_vMin.z - m_vMax.z);
    }
    else if (rhs.m_vMax.z < m_vMin.z)
    {
      fDistSQR += WMath::Square(m_vMin.z - rhs.m_vMax.z);
    }
  }

  return fDistSQR;
}

template <typename Type>
Type WBoundingBoxTemplate<Type>::GetDistanceTo(const WBoundingBoxTemplate<Type>& rhs) const
{
  return WMath::Sqrt(GetDistanceSquaredTo(rhs));
}

template <typename Type>
bool WBoundingBoxTemplate<Type>::GetRayIntersection(const WVec3Template<Type>& vStartPos, const WVec3Template<Type>& vRayDir, Type* out_pIntersectionDistance, WVec3Template<Type>* out_pIntersection) const
{
  // This code was taken from: http://people.csail.mit.edu/amy/papers/box-jgt.pdf
  // "An Efficient and Robust Ray-Box Intersection Algorithm"
  // Contrary to previous implementation, this one actually works with ray/box configurations
  // that produce division by zero and multiplication with infinity (which can produce NaNs).

  W_ASSERT_DEBUG(WMath::SupportsInfinity<Type>(), "This type does not support infinite values, which is required for this algorithm.");
  W_ASSERT_DEBUG(vStartPos.IsValid(), "Ray start position must be valid.");
  W_ASSERT_DEBUG(vRayDir.IsValid(), "Ray direction must be valid.");

  W_NAN_ASSERT(this);

  Type tMin, tMax;

  // Compare along X and Z axis, find intersection point
  {
    Type tMinY, tMaxY;

    const Type fDivX = (Type)1.0 / vRayDir.x;
    const Type fDivY = (Type)1.0 / vRayDir.y;

    if (vRayDir.x >= (Type)0.0)
    {
      tMin = (m_vMin.x - vStartPos.x) * fDivX;
      tMax = (m_vMax.x - vStartPos.x) * fDivX;
    }
    else
    {
      tMin = (m_vMax.x - vStartPos.x) * fDivX;
      tMax = (m_vMin.x - vStartPos.x) * fDivX;
    }

    if (vRayDir.y >= (Type)0.0)
    {
      tMinY = (m_vMin.y - vStartPos.y) * fDivY;
      tMaxY = (m_vMax.y - vStartPos.y) * fDivY;
    }
    else
    {
      tMinY = (m_vMax.y - vStartPos.y) * fDivY;
      tMaxY = (m_vMin.y - vStartPos.y) * fDivY;
    }

    if (tMin > tMaxY || tMinY > tMax)
      return false;

    if (tMinY > tMin)
      tMin = tMinY;
    if (tMaxY < tMax)
      tMax = tMaxY;
  }

  // Compare along Z axis and previous result, find intersection point
  {
    Type tMinZ, tMaxZ;

    const Type fDivZ = (Type)1.0 / vRayDir.z;

    if (vRayDir.z >= (Type)0.0)
    {
      tMinZ = (m_vMin.z - vStartPos.z) * fDivZ;
      tMaxZ = (m_vMax.z - vStartPos.z) * fDivZ;
    }
    else
    {
      tMinZ = (m_vMax.z - vStartPos.z) * fDivZ;
      tMaxZ = (m_vMin.z - vStartPos.z) * fDivZ;
    }

    if (tMin > tMaxZ || tMinZ > tMax)
      return false;

    if (tMinZ > tMin)
      tMin = tMinZ;
    if (tMaxZ < tMax)
      tMax = tMaxZ;
  }

  // rays that start inside the box are considered as not hitting the box
  if (tMax <= (Type)0.0)
    return false;

  if (out_pIntersectionDistance)
    *out_pIntersectionDistance = tMin;

  if (out_pIntersection)
    *out_pIntersection = vStartPos + tMin * vRayDir;

  return true;
}

template <typename Type>
bool WBoundingBoxTemplate<Type>::GetLineSegmentIntersection(const WVec3Template<Type>& vStartPos, const WVec3Template<Type>& vEndPos, Type* out_pLineFraction, WVec3Template<Type>* out_pIntersection) const
{
  const WVec3Template<Type> vRayDir = vEndPos - vStartPos;

  Type fIntersection = (Type)0.0;
  if (!GetRayIntersection(vStartPos, vRayDir, &fIntersection, out_pIntersection))
    return false;

  if (out_pLineFraction)
    *out_pLineFraction = fIntersection;

  return fIntersection <= (Type)1.0;
}



#include <Foundation/Math/Implementation/AllClasses_inl.h>
