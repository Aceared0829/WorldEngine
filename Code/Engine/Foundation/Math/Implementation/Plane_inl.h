#pragma once

#include <Foundation/Math/Mat4.h>

template <typename Type>
W_FORCE_INLINE WPlaneTemplate<Type>::WPlaneTemplate()
{
#if W_ENABLED(W_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const Type TypeNaN = WMath::NaN<Type>();
  m_vNormal.Set(TypeNaN);
  m_fNegDistance = TypeNaN;
#endif
}

template <typename Type>
WPlaneTemplate<Type> WPlaneTemplate<Type>::MakeInvalid()
{
  WPlaneTemplate<Type> res;
  res.m_vNormal.Set(0);
  res.m_fNegDistance = 0;
  return res;
}

template <typename Type>
WPlaneTemplate<Type> WPlaneTemplate<Type>::MakeFromNormalAndPoint(const WVec3Template<Type>& vNormal, const WVec3Template<Type>& vPointOnPlane)
{
  W_ASSERT_DEV(vNormal.IsNormalized(), "Normal must be normalized.");

  WPlaneTemplate<Type> res;
  res.m_vNormal = vNormal;
  res.m_fNegDistance = -vNormal.Dot(vPointOnPlane);
  return res;
}

template <typename Type>
WPlaneTemplate<Type> WPlaneTemplate<Type>::MakeFromPoints(const WVec3Template<Type>& v1, const WVec3Template<Type>& v2, const WVec3Template<Type>& v3)
{
  WPlaneTemplate<Type> res;
  W_VERIFY(res.m_vNormal.CalculateNormal(v1, v2, v3).Succeeded(), "The 3 provided points do not form a plane");

  res.m_fNegDistance = -res.m_vNormal.Dot(v1);
  return res;
}

template <typename Type>
WVec4Template<Type> WPlaneTemplate<Type>::GetAsVec4() const
{
  return WVec4(m_vNormal.x, m_vNormal.y, m_vNormal.z, m_fNegDistance);
}

template <typename Type>
WResult WPlaneTemplate<Type>::SetFromPoints(const WVec3Template<Type>& v1, const WVec3Template<Type>& v2, const WVec3Template<Type>& v3)
{
  if (m_vNormal.CalculateNormal(v1, v2, v3) == W_FAILURE)
    return W_FAILURE;

  m_fNegDistance = -m_vNormal.Dot(v1);
  return W_SUCCESS;
}

template <typename Type>
WResult WPlaneTemplate<Type>::SetFromPoints(const WVec3Template<Type>* const pVertices)
{
  if (m_vNormal.CalculateNormal(pVertices[0], pVertices[1], pVertices[2]) == W_FAILURE)
    return W_FAILURE;

  m_fNegDistance = -m_vNormal.Dot(pVertices[0]);
  return W_SUCCESS;
}

template <typename Type>
WResult WPlaneTemplate<Type>::SetFromDirections(const WVec3Template<Type>& vTangent1, const WVec3Template<Type>& vTangent2, const WVec3Template<Type>& vPointOnPlane)
{
  WVec3Template<Type> vNormal = vTangent1.CrossRH(vTangent2);
  WResult res = vNormal.NormalizeIfNotZero();

  m_vNormal = vNormal;
  m_fNegDistance = -vNormal.Dot(vPointOnPlane);
  return res;
}

template <typename Type>
void WPlaneTemplate<Type>::Transform(const WMat3Template<Type>& m)
{
  WVec3Template<Type> vPointOnPlane = m_vNormal * -m_fNegDistance;

  // Transform the normal
  WVec3Template<Type> vTransformedNormal = m.TransformDirection(m_vNormal);

  // Normalize the normal vector
  const bool normalizeSucceeded = vTransformedNormal.NormalizeIfNotZero().Succeeded();
  W_ASSERT_DEBUG(normalizeSucceeded, "");
  W_IGNORE_UNUSED(normalizeSucceeded);

  // If the plane's distance is already infinite, there won't be any meaningful change
  // to it as a result of the transformation.
  if (!WMath::IsFinite(m_fNegDistance))
  {
    m_vNormal = vTransformedNormal;
  }
  else
  {
    *this = WPlaneTemplate<Type>::MakeFromNormalAndPoint(vTransformedNormal, m * vPointOnPlane);
  }
}

template <typename Type>
void WPlaneTemplate<Type>::Transform(const WMat4Template<Type>& m)
{
  WVec3Template<Type> vPointOnPlane = m_vNormal * -m_fNegDistance;

  // Transform the normal
  WVec3Template<Type> vTransformedNormal = m.TransformDirection(m_vNormal);

  // Normalize the normal vector
  const bool normalizeSucceeded = vTransformedNormal.NormalizeIfNotZero().Succeeded();
  W_ASSERT_DEBUG(normalizeSucceeded, "");
  W_IGNORE_UNUSED(normalizeSucceeded);

  // If the plane's distance is already infinite, there won't be any meaningful change
  // to it as a result of the transformation.
  if (!WMath::IsFinite(m_fNegDistance))
  {
    m_vNormal = vTransformedNormal;
  }
  else
  {
    *this = WPlaneTemplate<Type>::MakeFromNormalAndPoint(vTransformedNormal, m * vPointOnPlane);
  }
}

template <typename Type>
W_FORCE_INLINE void WPlaneTemplate<Type>::Flip()
{
  m_fNegDistance = -m_fNegDistance;
  m_vNormal = -m_vNormal;
}

template <typename Type>
W_FORCE_INLINE Type WPlaneTemplate<Type>::GetDistanceTo(const WVec3Template<Type>& vPoint) const
{
  return (m_vNormal.Dot(vPoint) + m_fNegDistance);
}

template <typename Type>
W_FORCE_INLINE WPositionOnPlane::Enum WPlaneTemplate<Type>::GetPointPosition(const WVec3Template<Type>& vPoint) const
{
  return (m_vNormal.Dot(vPoint) < -m_fNegDistance ? WPositionOnPlane::Back : WPositionOnPlane::Front);
}

template <typename Type>
WPositionOnPlane::Enum WPlaneTemplate<Type>::GetPointPosition(const WVec3Template<Type>& vPoint, Type fPlaneHalfWidth) const
{
  const Type f = m_vNormal.Dot(vPoint);

  if (f + fPlaneHalfWidth < -m_fNegDistance)
    return WPositionOnPlane::Back;

  if (f - fPlaneHalfWidth > -m_fNegDistance)
    return WPositionOnPlane::Front;

  return WPositionOnPlane::OnPlane;
}

template <typename Type>
W_FORCE_INLINE const WVec3Template<Type> WPlaneTemplate<Type>::ProjectOntoPlane(const WVec3Template<Type>& vPoint) const
{
  return vPoint - m_vNormal * (m_vNormal.Dot(vPoint) + m_fNegDistance);
}

template <typename Type>
W_FORCE_INLINE const WVec3Template<Type> WPlaneTemplate<Type>::Mirror(const WVec3Template<Type>& vPoint) const
{
  return vPoint - (Type)2 * GetDistanceTo(vPoint) * m_vNormal;
}

template <typename Type>
const WVec3Template<Type> WPlaneTemplate<Type>::GetCoplanarDirection(const WVec3Template<Type>& vDirection) const
{
  WVec3Template<Type> res = vDirection;
  res.MakeOrthogonalTo(m_vNormal);
  return res;
}

template <typename Type>
bool WPlaneTemplate<Type>::IsIdentical(const WPlaneTemplate& rhs) const
{
  return m_vNormal.IsIdentical(rhs.m_vNormal) && m_fNegDistance == rhs.m_fNegDistance;
}

template <typename Type>
bool WPlaneTemplate<Type>::IsEqual(const WPlaneTemplate& rhs, Type fEpsilon) const
{
  return m_vNormal.IsEqual(rhs.m_vNormal, fEpsilon) && WMath::IsEqual(m_fNegDistance, rhs.m_fNegDistance, fEpsilon);
}

template <typename Type>
W_ALWAYS_INLINE bool operator==(const WPlaneTemplate<Type>& lhs, const WPlaneTemplate<Type>& rhs)
{
  return lhs.IsIdentical(rhs);
}

template <typename Type>
W_ALWAYS_INLINE bool operator!=(const WPlaneTemplate<Type>& lhs, const WPlaneTemplate<Type>& rhs)
{
  return !lhs.IsIdentical(rhs);
}

template <typename Type>
bool WPlaneTemplate<Type>::FlipIfNecessary(const WVec3Template<Type>& vPoint, bool bPlaneShouldFacePoint)
{
  if ((GetPointPosition(vPoint) == WPositionOnPlane::Front) != bPlaneShouldFacePoint)
  {
    Flip();
    return true;
  }

  return false;
}

template <typename Type>
bool WPlaneTemplate<Type>::IsValid() const
{
  return !IsNaN() && m_vNormal.IsNormalized(WMath::DefaultEpsilon<Type>());
}

template <typename Type>
bool WPlaneTemplate<Type>::IsNaN() const
{
  return WMath::IsNaN(m_fNegDistance) || m_vNormal.IsNaN();
}

template <typename Type>
bool WPlaneTemplate<Type>::IsFinite() const
{
  return m_vNormal.IsValid() && WMath::IsFinite(m_fNegDistance);
}

/*! The given vertices can be partially equal or lie on the same line. The algorithm will try to find 3 vertices, that
  form a plane, and deduce the normal from them. This algorithm is much slower, than all the other methods, so only
  use it, when you know, that your data can contain such configurations. */
template <typename Type>
WResult WPlaneTemplate<Type>::SetFromPoints(const WVec3Template<Type>* const pVertices, WUInt32 uiMaxVertices)
{
  WInt32 iPoints[3];

  if (FindSupportPoints(pVertices, uiMaxVertices, iPoints[0], iPoints[1], iPoints[2]) == W_FAILURE)
  {
    SetFromPoints(pVertices).IgnoreResult();
    return W_FAILURE;
  }

  SetFromPoints(pVertices[iPoints[0]], pVertices[iPoints[1]], pVertices[iPoints[2]]).IgnoreResult();
  return W_SUCCESS;
}

template <typename Type>
WResult WPlaneTemplate<Type>::FindSupportPoints(const WVec3Template<Type>* const pVertices, int iMaxVertices, int& out_i1, int& out_i2, int& out_i3)
{
  const WVec3Template<Type> v1 = pVertices[0];

  bool bFoundSecond = false;

  int i = 1;
  while (i < iMaxVertices)
  {
    if (pVertices[i].IsEqual(v1, 0.001f) == false)
    {
      bFoundSecond = true;
      break;
    }

    ++i;
  }

  if (!bFoundSecond)
    return W_FAILURE;

  const WVec3Template<Type> v2 = pVertices[i];

  const WVec3Template<Type> vDir1 = (v1 - v2).GetNormalized();

  out_i1 = 0;
  out_i2 = i;

  ++i;

  while (i < iMaxVertices)
  {
    // check for inequality, then for non-collinearity
    if ((pVertices[i].IsEqual(v2, 0.001f) == false) && (WMath::Abs((pVertices[i] - v2).GetNormalized().Dot(vDir1)) < (Type)0.999))
    {
      out_i3 = i;
      return W_SUCCESS;
    }

    ++i;
  }

  return W_FAILURE;
}

template <typename Type>
WPositionOnPlane::Enum WPlaneTemplate<Type>::GetObjectPosition(const WVec3Template<Type>* const pPoints, WUInt32 uiVertices) const
{
  bool bFront = false;
  bool bBack = false;

  for (WUInt32 i = 0; i < uiVertices; ++i)
  {
    switch (GetPointPosition(pPoints[i]))
    {
      case WPositionOnPlane::Front:
        if (bBack)
          return (WPositionOnPlane::Spanning);
        bFront = true;
        break;
      case WPositionOnPlane::Back:
        if (bFront)
          return (WPositionOnPlane::Spanning);
        bBack = true;
        break;

      default:
        break;
    }
  }

  return (bFront ? WPositionOnPlane::Front : WPositionOnPlane::Back);
}

template <typename Type>
WPositionOnPlane::Enum WPlaneTemplate<Type>::GetObjectPosition(const WVec3Template<Type>* const pPoints, WUInt32 uiVertices, Type fPlaneHalfWidth) const
{
  bool bFront = false;
  bool bBack = false;

  for (WUInt32 i = 0; i < uiVertices; ++i)
  {
    switch (GetPointPosition(pPoints[i], fPlaneHalfWidth))
    {
      case WPositionOnPlane::Front:
        if (bBack)
          return (WPositionOnPlane::Spanning);
        bFront = true;
        break;
      case WPositionOnPlane::Back:
        if (bFront)
          return (WPositionOnPlane::Spanning);
        bBack = true;
        break;

      default:
        break;
    }
  }

  if (bFront)
    return (WPositionOnPlane::Front);
  if (bBack)
    return (WPositionOnPlane::Back);

  return (WPositionOnPlane::OnPlane);
}

template <typename Type>
bool WPlaneTemplate<Type>::GetRayIntersection(const WVec3Template<Type>& vRayStartPos, const WVec3Template<Type>& vRayDir, Type* out_pIntersectionDistance, WVec3Template<Type>* out_pIntersection) const
{
  W_ASSERT_DEBUG(vRayStartPos.IsValid(), "Ray start position must be valid.");
  W_ASSERT_DEBUG(vRayDir.IsValid(), "Ray direction must be valid.");

  const Type fPlaneSide = GetDistanceTo(vRayStartPos);
  const Type fCosAlpha = m_vNormal.Dot(vRayDir);

  if (WMath::IsZero(fCosAlpha, (Type)0.00001))                 // ray is orthogonal to plane
    return false;

  if (WMath::Sign(fPlaneSide) == WMath::Sign(fCosAlpha)) // ray points away from the plane
    return false;

  const Type fTime = -fPlaneSide / fCosAlpha;

  if (out_pIntersectionDistance)
    *out_pIntersectionDistance = fTime;

  if (out_pIntersection)
    *out_pIntersection = vRayStartPos + fTime * vRayDir;

  return true;
}

template <typename Type>
bool WPlaneTemplate<Type>::GetRayIntersectionBiDirectional(const WVec3Template<Type>& vRayStartPos, const WVec3Template<Type>& vRayDir, Type* out_pIntersectionDistance, WVec3Template<Type>* out_pIntersection) const
{
  W_ASSERT_DEBUG(vRayStartPos.IsValid(), "Ray start position must be valid.");
  W_ASSERT_DEBUG(vRayDir.IsValid(), "Ray direction must be valid.");

  const Type fPlaneSide = GetDistanceTo(vRayStartPos);
  const Type fCosAlpha = m_vNormal.Dot(vRayDir);

  if (WMath::IsZero(fCosAlpha, (Type)0.00001)) // ray is orthogonal to plane
    return false;

  const Type fTime = -fPlaneSide / fCosAlpha;

  if (out_pIntersectionDistance)
    *out_pIntersectionDistance = fTime;

  if (out_pIntersection)
    *out_pIntersection = vRayStartPos + fTime * vRayDir;

  return true;
}

template <typename Type>
bool WPlaneTemplate<Type>::GetLineSegmentIntersection(const WVec3Template<Type>& vLineStartPos, const WVec3Template<Type>& vLineEndPos, Type* out_pHitFraction, WVec3Template<Type>* out_pIntersection) const
{
  Type fTime = 0;

  if (!GetRayIntersection(vLineStartPos, vLineEndPos - vLineStartPos, &fTime, out_pIntersection))
    return false;

  if (out_pHitFraction)
    *out_pHitFraction = fTime;

  return (fTime <= 1);
}

template <typename Type>
Type WPlaneTemplate<Type>::GetMinimumDistanceTo(const WVec3Template<Type>* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride /* = sizeof (WVec3Template<Type>) */) const
{
  W_ASSERT_DEBUG(pPoints != nullptr, "Array may not be nullptr.");
  W_ASSERT_DEBUG(uiStride >= sizeof(WVec3Template<Type>), "Stride must be at least sizeof(WVec3Template) to not have overlapping data.");
  W_ASSERT_DEBUG(uiNumPoints >= 1, "Array must contain at least one point.");

  Type fMinDist = WMath::MaxValue<Type>();

  const WVec3Template<Type>* pCurPoint = pPoints;

  for (WUInt32 i = 0; i < uiNumPoints; ++i)
  {
    fMinDist = WMath::Min(m_vNormal.Dot(*pCurPoint), fMinDist);

    pCurPoint = WMemoryUtils::AddByteOffset(pCurPoint, uiStride);
  }

  return fMinDist + m_fNegDistance;
}

template <typename Type>
void WPlaneTemplate<Type>::GetMinMaxDistanceTo(Type& out_fMin, Type& out_fMax, const WVec3Template<Type>* pPoints, WUInt32 uiNumPoints, WUInt32 uiStride /* = sizeof (WVec3Template<Type>) */) const
{
  W_ASSERT_DEBUG(pPoints != nullptr, "Array may not be nullptr.");
  W_ASSERT_DEBUG(uiStride >= sizeof(WVec3Template<Type>), "Stride must be at least sizeof(WVec3Template) to not have overlapping data.");
  W_ASSERT_DEBUG(uiNumPoints >= 1, "Array must contain at least one point.");

  out_fMin = WMath::MaxValue<Type>();
  out_fMax = -WMath::MaxValue<Type>();

  const WVec3Template<Type>* pCurPoint = pPoints;

  for (WUInt32 i = 0; i < uiNumPoints; ++i)
  {
    const Type f = m_vNormal.Dot(*pCurPoint);

    out_fMin = WMath::Min(f, out_fMin);
    out_fMax = WMath::Max(f, out_fMax);

    pCurPoint = WMemoryUtils::AddByteOffset(pCurPoint, uiStride);
  }

  out_fMin += m_fNegDistance;
  out_fMax += m_fNegDistance;
}

template <typename Type>
WResult WPlaneTemplate<Type>::GetPlanesIntersectionPoint(const WPlaneTemplate& p0, const WPlaneTemplate& p1, const WPlaneTemplate& p2, WVec3Template<Type>& out_vResult)
{
  const WVec3Template<Type> n1(p0.m_vNormal);
  const WVec3Template<Type> n2(p1.m_vNormal);
  const WVec3Template<Type> n3(p2.m_vNormal);

  const Type det = n1.Dot(n2.CrossRH(n3));

  if (WMath::IsZero<Type>(det, WMath::LargeEpsilon<Type>()))
    return W_FAILURE;

  out_vResult = (-p0.m_fNegDistance * n2.CrossRH(n3) + -p1.m_fNegDistance * n3.CrossRH(n1) + -p2.m_fNegDistance * n1.CrossRH(n2)) / det;

  return W_SUCCESS;
}

#include <Foundation/Math/Implementation/AllClasses_inl.h>
