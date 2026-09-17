#pragma once

#include <Foundation/Math/Mat4.h>

template <typename Type>
W_FORCE_INLINE WBoundingSphereTemplate<Type>::WBoundingSphereTemplate()
{
#if W_ENABLED(W_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  // m_vCenter is already initialized to NaN by its own constructor.
  const Type TypeNaN = WMath::NaN<Type>();
  m_fRadius = TypeNaN;
#endif
}

template <typename Type>
W_FORCE_INLINE WBoundingSphereTemplate<Type> WBoundingSphereTemplate<Type>::MakeZero()
{
  WBoundingSphereTemplate<Type> res;
  res.m_vCenter.SetZero();
  res.m_fRadius = 0.0f;
  return res;
}

template <typename Type>
W_FORCE_INLINE WBoundingSphereTemplate<Type> WBoundingSphereTemplate<Type>::MakeInvalid(const WVec3Template<Type>& vCenter)
{
  WBoundingSphereTemplate<Type> res;
  res.m_vCenter = vCenter;
  res.m_fRadius = -WMath::SmallEpsilon<Type>(); // has to be very small for ExpandToInclude to work
  return res;
}

template <typename Type>
W_FORCE_INLINE WBoundingSphereTemplate<Type> WBoundingSphereTemplate<Type>::MakeFromCenterAndRadius(const WVec3Template<Type>& vCenter, Type fRadius)
{
  WBoundingSphereTemplate<Type> res;
  res.m_vCenter = vCenter;
  res.m_fRadius = fRadius;
  W_ASSERT_DEBUG(res.IsValid(), "The sphere was created with invalid values.");
  return res;
}

template <typename Type>
W_FORCE_INLINE WBoundingSphereTemplate<Type> WBoundingSphereTemplate<Type>::MakeFromPoints(const WVec3Template<Type>* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride /*= sizeof(WVec3Template<Type>)*/)
{
  W_ASSERT_DEBUG(pPoints != nullptr, "The array must not be empty.");
  W_ASSERT_DEBUG(uiStride >= sizeof(WVec3Template<Type>), "The data must not overlap.");
  W_ASSERT_DEBUG(uiNumPoints > 0, "The array must contain at least one point.");

  const WVec3Template<Type>* pCur = &pPoints[0];

  WVec3Template<Type> vCenter(0.0f);

  for (WUInt32 i = 0; i < uiNumPoints; ++i)
  {
    vCenter += *pCur;
    pCur = WMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  vCenter /= (Type)uiNumPoints;

  Type fMaxDistSQR = 0.0f;

  pCur = &pPoints[0];
  for (WUInt32 i = 0; i < uiNumPoints; ++i)
  {
    const Type fDistSQR = (*pCur - vCenter).GetLengthSquared();
    fMaxDistSQR = WMath::Max(fMaxDistSQR, fDistSQR);

    pCur = WMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  WBoundingSphereTemplate<Type> res;
  res.m_vCenter = vCenter;
  res.m_fRadius = WMath::Sqrt(fMaxDistSQR);

  W_ASSERT_DEBUG(res.IsValid(), "The point cloud contained corrupted data.");

  return res;
}

template <typename Type>
bool WBoundingSphereTemplate<Type>::IsZero(Type fEpsilon /* = WMath::DefaultEpsilon<Type>() */) const
{
  return m_vCenter.IsZero(fEpsilon) && WMath::IsZero(m_fRadius, fEpsilon);
}

template <typename Type>
bool WBoundingSphereTemplate<Type>::IsValid() const
{
  return (m_vCenter.IsValid() && m_fRadius >= 0.0f);
}

template <typename Type>
bool WBoundingSphereTemplate<Type>::IsNaN() const
{
  return (m_vCenter.IsNaN() || WMath::IsNaN(m_fRadius));
}

template <typename Type>
void WBoundingSphereTemplate<Type>::ExpandToInclude(const WVec3Template<Type>& vPoint)
{
  const Type fDistSQR = (vPoint - m_vCenter).GetLengthSquared();

  if (WMath::Square(m_fRadius) < fDistSQR)
    m_fRadius = WMath::Sqrt(fDistSQR);
}

template <typename Type>
void WBoundingSphereTemplate<Type>::ExpandToInclude(const WBoundingSphereTemplate<Type>& rhs)
{
  const Type fReqRadius = (rhs.m_vCenter - m_vCenter).GetLength() + rhs.m_fRadius;

  m_fRadius = WMath::Max(m_fRadius, fReqRadius);
}

template <typename Type>
W_FORCE_INLINE void WBoundingSphereTemplate<Type>::Grow(Type fDiff)
{
  W_ASSERT_DEBUG(IsValid(), "Cannot grow a sphere that is invalid.");

  m_fRadius += fDiff;

  W_ASSERT_DEBUG(IsValid(), "The grown sphere has become invalid.");
}

template <typename Type>
bool WBoundingSphereTemplate<Type>::IsIdentical(const WBoundingSphereTemplate<Type>& rhs) const
{
  return (m_vCenter.IsIdentical(rhs.m_vCenter) && m_fRadius == rhs.m_fRadius);
}

template <typename Type>
bool WBoundingSphereTemplate<Type>::IsEqual(const WBoundingSphereTemplate<Type>& rhs, Type fEpsilon) const
{
  return (m_vCenter.IsEqual(rhs.m_vCenter, fEpsilon) && WMath::IsEqual(m_fRadius, rhs.m_fRadius, fEpsilon));
}

template <typename Type>
W_ALWAYS_INLINE bool operator==(const WBoundingSphereTemplate<Type>& lhs, const WBoundingSphereTemplate<Type>& rhs)
{
  return lhs.IsIdentical(rhs);
}

template <typename Type>
W_ALWAYS_INLINE bool operator!=(const WBoundingSphereTemplate<Type>& lhs, const WBoundingSphereTemplate<Type>& rhs)
{
  return !lhs.IsIdentical(rhs);
}

template <typename Type>
W_ALWAYS_INLINE void WBoundingSphereTemplate<Type>::Translate(const WVec3Template<Type>& vTranslation)
{
  m_vCenter += vTranslation;
}

template <typename Type>
W_FORCE_INLINE void WBoundingSphereTemplate<Type>::ScaleFromCenter(Type fScale)
{
  W_ASSERT_DEBUG(fScale >= 0.0f, "Cannot invert the sphere.");

  m_fRadius *= fScale;

  W_NAN_ASSERT(this);
}

template <typename Type>
void WBoundingSphereTemplate<Type>::ScaleFromOrigin(const WVec3Template<Type>& vScale)
{
  W_ASSERT_DEBUG(vScale.x >= 0.0f, "Cannot invert the sphere.");
  W_ASSERT_DEBUG(vScale.y >= 0.0f, "Cannot invert the sphere.");
  W_ASSERT_DEBUG(vScale.z >= 0.0f, "Cannot invert the sphere.");

  m_vCenter = m_vCenter.CompMul(vScale);

  // scale the radius by the maximum scaling factor (the sphere cannot become an ellipsoid,
  // so to be a 'bounding' sphere, it should be as large as possible
  m_fRadius *= WMath::Max(vScale.x, vScale.y, vScale.z);
}

template <typename Type>
void WBoundingSphereTemplate<Type>::TransformFromOrigin(const WMat4Template<Type>& mTransform)
{
  m_vCenter = mTransform.TransformPosition(m_vCenter);

  const WVec3Template<Type> Scale = mTransform.GetScalingFactors();
  m_fRadius *= WMath::Max(Scale.x, Scale.y, Scale.z);
}

template <typename Type>
void WBoundingSphereTemplate<Type>::TransformFromCenter(const WMat4Template<Type>& mTransform)
{
  m_vCenter += mTransform.GetTranslationVector();

  const WVec3Template<Type> Scale = mTransform.GetScalingFactors();
  m_fRadius *= WMath::Max(Scale.x, Scale.y, Scale.z);
}

template <typename Type>
Type WBoundingSphereTemplate<Type>::GetDistanceTo(const WVec3Template<Type>& vPoint) const
{
  return (vPoint - m_vCenter).GetLength() - m_fRadius;
}

template <typename Type>
Type WBoundingSphereTemplate<Type>::GetDistanceTo(const WBoundingSphereTemplate<Type>& rhs) const
{
  return (rhs.m_vCenter - m_vCenter).GetLength() - m_fRadius - rhs.m_fRadius;
}

template <typename Type>
bool WBoundingSphereTemplate<Type>::Contains(const WVec3Template<Type>& vPoint) const
{
  return (vPoint - m_vCenter).GetLengthSquared() <= WMath::Square(m_fRadius);
}

template <typename Type>
bool WBoundingSphereTemplate<Type>::Contains(const WBoundingSphereTemplate<Type>& rhs) const
{
  return (rhs.m_vCenter - m_vCenter).GetLength() + rhs.m_fRadius <= m_fRadius;
}

template <typename Type>
bool WBoundingSphereTemplate<Type>::Overlaps(const WBoundingSphereTemplate<Type>& rhs) const
{
  return (rhs.m_vCenter - m_vCenter).GetLengthSquared() < WMath::Square(rhs.m_fRadius + m_fRadius);
}

template <typename Type>
const WVec3Template<Type> WBoundingSphereTemplate<Type>::GetClampedPoint(const WVec3Template<Type>& vPoint)
{
  const WVec3Template<Type> vDir = vPoint - m_vCenter;
  const Type fDistSQR = vDir.GetLengthSquared();

  // return the point, if it is already inside the sphere
  if (fDistSQR <= WMath::Square(m_fRadius))
    return vPoint;

  // otherwise return a point on the surface of the sphere

  const Type fLength = WMath::Sqrt(fDistSQR);

  return m_vCenter + m_fRadius * (vDir / fLength);
}

template <typename Type>
bool WBoundingSphereTemplate<Type>::Contains(const WVec3Template<Type>* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride /* = sizeof(WVec3Template) */) const
{
  W_ASSERT_DEBUG(pPoints != nullptr, "The array must not be empty.");
  W_ASSERT_DEBUG(uiNumPoints > 0, "The array must contain at least one point.");
  W_ASSERT_DEBUG(uiStride >= sizeof(WVec3Template<Type>), "The data must not overlap.");

  const Type fRadiusSQR = WMath::Square(m_fRadius);

  const WVec3Template<Type>* pCur = &pPoints[0];

  for (WUInt32 i = 0; i < uiNumPoints; ++i)
  {
    if ((*pCur - m_vCenter).GetLengthSquared() > fRadiusSQR)
      return false;

    pCur = WMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  return true;
}

template <typename Type>
bool WBoundingSphereTemplate<Type>::Overlaps(const WVec3Template<Type>* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride /* = sizeof(WVec3Template) */) const
{
  W_ASSERT_DEBUG(pPoints != nullptr, "The array must not be empty.");
  W_ASSERT_DEBUG(uiNumPoints > 0, "The array must contain at least one point.");
  W_ASSERT_DEBUG(uiStride >= sizeof(WVec3Template<Type>), "The data must not overlap.");

  const Type fRadiusSQR = WMath::Square(m_fRadius);

  const WVec3Template<Type>* pCur = &pPoints[0];

  for (WUInt32 i = 0; i < uiNumPoints; ++i)
  {
    if ((*pCur - m_vCenter).GetLengthSquared() <= fRadiusSQR)
      return true;

    pCur = WMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  return false;
}

template <typename Type>
void WBoundingSphereTemplate<Type>::ExpandToInclude(const WVec3Template<Type>* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride /* = sizeof(WVec3Template) */)
{
  W_ASSERT_DEBUG(pPoints != nullptr, "The array must not be empty.");
  W_ASSERT_DEBUG(uiStride >= sizeof(WVec3Template<Type>), "The data must not overlap.");

  const WVec3Template<Type>* pCur = &pPoints[0];

  Type fMaxDistSQR = 0.0f;

  for (WUInt32 i = 0; i < uiNumPoints; ++i)
  {
    const Type fDistSQR = (*pCur - m_vCenter).GetLengthSquared();
    fMaxDistSQR = WMath::Max(fMaxDistSQR, fDistSQR);

    pCur = WMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  if (WMath::Square(m_fRadius) < fMaxDistSQR)
    m_fRadius = WMath::Sqrt(fMaxDistSQR);
}

template <typename Type>
Type WBoundingSphereTemplate<Type>::GetDistanceTo(const WVec3Template<Type>* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride /* = sizeof(WVec3Template) */) const
{
  W_ASSERT_DEBUG(pPoints != nullptr, "The array must not be empty.");
  W_ASSERT_DEBUG(uiNumPoints > 0, "The array must contain at least one point.");
  W_ASSERT_DEBUG(uiStride >= sizeof(WVec3Template<Type>), "The data must not overlap.");

  const WVec3Template<Type>* pCur = &pPoints[0];

  Type fMinDistSQR = WMath::MaxValue<Type>();

  for (WUInt32 i = 0; i < uiNumPoints; ++i)
  {
    const Type fDistSQR = (*pCur - m_vCenter).GetLengthSquared();

    fMinDistSQR = WMath::Min(fMinDistSQR, fDistSQR);

    pCur = WMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  return WMath::Sqrt(fMinDistSQR);
}

template <typename Type>
bool WBoundingSphereTemplate<Type>::GetRayIntersection(const WVec3Template<Type>& vRayStartPos, const WVec3Template<Type>& vRayDirNormalized,
  Type* out_pIntersectionDistance /* = nullptr */, WVec3Template<Type>* out_pIntersection /* = nullptr */) const
{
  W_ASSERT_DEBUG(vRayDirNormalized.IsNormalized(), "The ray direction must be normalized.");

  // Ugly Code taken from 'Real Time Rendering First Edition' Page 299

  const Type fRadiusSQR = WMath::Square(m_fRadius);
  const WVec3Template<Type> vRelPos = m_vCenter - vRayStartPos;

  const Type d = vRelPos.Dot(vRayDirNormalized);
  const Type fRelPosLenSQR = vRelPos.GetLengthSquared();

  if (d < 0.0f && fRelPosLenSQR > fRadiusSQR)
    return false;

  const Type m2 = fRelPosLenSQR - WMath::Square(d);

  if (m2 > fRadiusSQR)
    return false;

  const Type q = WMath::Sqrt(fRadiusSQR - m2);

  Type fIntersectionTime;

  if (fRelPosLenSQR > fRadiusSQR)
    fIntersectionTime = d - q;
  else
    fIntersectionTime = d + q;

  if (out_pIntersectionDistance)
    *out_pIntersectionDistance = fIntersectionTime;
  if (out_pIntersection)
    *out_pIntersection = vRayStartPos + vRayDirNormalized * fIntersectionTime;

  return true;
}

template <typename Type>
bool WBoundingSphereTemplate<Type>::GetLineSegmentIntersection(const WVec3Template<Type>& vLineStartPos, const WVec3Template<Type>& vLineEndPos,
  Type* out_pHitFraction /* = nullptr */, WVec3Template<Type>* out_pIntersection /* = nullptr */) const
{
  Type fIntersection = 0.0f;

  const WVec3Template<Type> vDir = vLineEndPos - vLineStartPos;
  WVec3Template<Type> vDirNorm = vDir;
  const Type fLen = vDirNorm.GetLengthAndNormalize();

  if (!GetRayIntersection(vLineStartPos, vDirNorm, &fIntersection))
    return false;

  if (fIntersection > fLen)
    return false;

  if (out_pHitFraction)
    *out_pHitFraction = fIntersection / fLen;

  if (out_pIntersection)
    *out_pIntersection = vLineStartPos + vDirNorm * fIntersection;

  return true;
}

#include <Foundation/Math/Implementation/AllClasses_inl.h>
