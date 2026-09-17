#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Intersection.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Math/Plane.h>

bool WIntersectionUtils::RayTriangleIntersection(const WVec3& vRayStartPos, const WVec3& vRayDir, const WVec3& vVertex0, const WVec3& vVertex1, const WVec3& vVertex2, float* out_pIntersectionTime /*= nullptr*/, WVec3* out_pIntersectionPoint /*= nullptr*/)
{
  const WPlane plane = WPlane::MakeFromPoints(vVertex0, vVertex1, vVertex2);

  WVec3 vIntersection;

  if (!plane.GetRayIntersection(vRayStartPos, vRayDir, out_pIntersectionTime, &vIntersection))
    return false;

  if (out_pIntersectionPoint)
    *out_pIntersectionPoint = vIntersection;

  {
    const WVec3 edge = vVertex1 - vVertex0;
    const WVec3 vp = vIntersection - vVertex0;
    if (plane.m_vNormal.Dot(edge.CrossRH(vp)) < 0)
    {
      return false;
    }
  }

  {
    const WVec3 edge = vVertex2 - vVertex1;
    const WVec3 vp = vIntersection - vVertex1;
    if (plane.m_vNormal.Dot(edge.CrossRH(vp)) < 0)
    {
      return false;
    }
  }

  {
    const WVec3 edge = vVertex0 - vVertex2;
    const WVec3 vp = vIntersection - vVertex2;
    if (plane.m_vNormal.Dot(edge.CrossRH(vp)) < 0)
    {
      return false;
    }
  }

  return true;
}

bool WIntersectionUtils::RayTriangleIntersection(const WVec3& vRayOrigin, const WVec3& vRayDir, const WVec3& vVertex0, const WVec3& vVertex1, const WVec3& vVertex2, WVec3& out_vBarycentricCoords, float* out_pIntersectionTime, WVec3* out_pIntersectionPoint)
{
  // source: https://www.graphics.cornell.edu/pubs/1997/MT97.pdf

  constexpr float fEpsilon = 0.00001f;

  WVec3 edge1 = vVertex1 - vVertex0;
  WVec3 edge2 = vVertex2 - vVertex0;

  WVec3 pvec = vRayDir.CrossRH(edge2);

  float det = edge1.Dot(pvec);
  if (det > -fEpsilon && det < fEpsilon)
    return false;

  float inv_det = 1.0f / det;

  WVec3 tvec = vRayOrigin - vVertex0;

  float u = tvec.Dot(pvec) * inv_det;
  if (u < 0.0f || u > 1.0f)
    return false;

  WVec3 qvec = tvec.CrossRH(edge1);

  float v = vRayDir.Dot(qvec) * inv_det;
  if (v < 0.0f || v > 1.0f)
    return false;

  out_vBarycentricCoords = WVec3(1.0f - u - v, u, v);

  if (out_pIntersectionTime)
    *out_pIntersectionTime = edge2.Dot(qvec) * inv_det;

  if (out_pIntersectionPoint)
    *out_pIntersectionPoint = vVertex0 * (1.0f - u - v) + vVertex1 * u + vVertex2 * v;

  return true;
}

bool WIntersectionUtils::RayTriangleIntersectionCullBackface(const WVec3& vRayOrigin, const WVec3& vRayDir, const WVec3& vVertex0, const WVec3& vVertex1, const WVec3& vVertex2, WVec3& out_vBarycentricCoords, float* out_pIntersectionTime, WVec3* out_pIntersectionPoint)
{
  // source: https://www.graphics.cornell.edu/pubs/1997/MT97.pdf

  constexpr float fEpsilon = 0.00001f;

  WVec3 edge1 = vVertex1 - vVertex0;
  WVec3 edge2 = vVertex2 - vVertex0;

  WVec3 pvec = vRayDir.CrossRH(edge2);

  float det = edge1.Dot(pvec);
  if (det < fEpsilon)
    return false;

  WVec3 tvec = vRayOrigin - vVertex0;

  float u = tvec.Dot(pvec);
  if (u < 0 || u > det)
    return false;

  WVec3 qvec = tvec.CrossRH(edge1);

  float v = qvec.Dot(vRayDir);
  if (v < 0 || u + v > det)
    return false;

  float inv_det = 1.0f / det;
  u *= inv_det;
  v *= inv_det;

  out_vBarycentricCoords = WVec3(1.0f - u - v, u, v);

  if (out_pIntersectionTime)
    *out_pIntersectionTime = edge2.Dot(qvec) * inv_det;

  if (out_pIntersectionPoint)
    *out_pIntersectionPoint = vVertex0 * (1.0f - u - v) + vVertex1 * u + vVertex2 * v;

  return true;
}

bool WIntersectionUtils::RayPolygonIntersection(const WVec3& vRayStartPos, const WVec3& vRayDir, const WVec3* pPolygonVertices,
  WUInt32 uiNumVertices, float* out_pIntersectionTime, WVec3* out_pIntersectionPoint, WUInt32 uiVertexStride)
{
  W_ASSERT_DEBUG(uiNumVertices >= 3, "A polygon must have at least three vertices.");
  W_ASSERT_DEBUG(uiVertexStride >= sizeof(WVec3), "The vertex stride is invalid.");

  WPlane plane = WPlane::MakeFromPoints(*pPolygonVertices, *WMemoryUtils::AddByteOffset(pPolygonVertices, uiVertexStride), *WMemoryUtils::AddByteOffset(pPolygonVertices, uiVertexStride * 2));

  W_ASSERT_DEBUG(plane.IsValid(), "The given polygon's plane is invalid (computed from the first three vertices only).");

  WVec3 vIntersection;

  if (!plane.GetRayIntersection(vRayStartPos, vRayDir, out_pIntersectionTime, &vIntersection))
    return false;

  if (out_pIntersectionPoint)
    *out_pIntersectionPoint = vIntersection;

  // start with the last point as the 'wrap around' position
  WVec3 vPrevPoint = *WMemoryUtils::AddByteOffset(pPolygonVertices, WMath::SafeMultiply32(uiVertexStride, (uiNumVertices - 1)));

  // for each polygon edge
  for (WUInt32 i = 0; i < uiNumVertices; ++i)
  {
    const WVec3 vThisPoint = *WMemoryUtils::AddByteOffset(pPolygonVertices, WMath::SafeMultiply32(uiVertexStride, i));

    const WVec3 edge = vThisPoint - vPrevPoint;
    const WVec3 vp = vIntersection - vPrevPoint;
    if (plane.m_vNormal.Dot(edge.CrossRH(vp)) < 0)
    {
      return false;
    }

    vPrevPoint = vThisPoint;
  }

  // inside all edge planes -> inside the polygon -> there is a proper intersection
  return true;
}

WVec3 WIntersectionUtils::ClosestPoint_PointLineSegment(
  const WVec3& vStartPoint, const WVec3& vLineSegmentPos0, const WVec3& vLineSegmentPos1, float* out_pFractionAlongSegment)
{
  const WVec3 vLineDir = vLineSegmentPos1 - vLineSegmentPos0;
  const WVec3 vToStartPoint = vStartPoint - vLineSegmentPos0;

  const float fProjected = vToStartPoint.Dot(vLineDir);

  float fPosAlongSegment;

  // clamp t to [0; 1] range, and only do the division etc. when necessary
  if (fProjected <= 0.0f)
  {
    fPosAlongSegment = 0.0f;
  }
  else
  {
    const float fSquaredDirLen = vLineDir.GetLengthSquared();

    if (fProjected >= fSquaredDirLen)
    {
      fPosAlongSegment = 1.0f;
    }
    else
    {
      fPosAlongSegment = fProjected / fSquaredDirLen;
    }
  }

  if (out_pFractionAlongSegment)
    *out_pFractionAlongSegment = fPosAlongSegment;

  return vLineSegmentPos0 + fPosAlongSegment * vLineDir;
}

bool WIntersectionUtils::Ray2DLine2D(const WVec2& vRayStartPos, const WVec2& vRayDir, const WVec2& vLineSegmentPos0,
  const WVec2& vLineSegmentPos1, float* out_pIntersectionTime, WVec2* out_pIntersectionPoint)
{
  const WVec2 vLineDir = vLineSegmentPos1 - vLineSegmentPos0;

  // 2D Plane
  const WVec2 vPlaneNormal = vLineDir.GetOrthogonalVector();
  const float fPlaneNegDist = -vPlaneNormal.Dot(vLineSegmentPos0);

  WVec2 vIntersection;
  float fIntersectionTime;

  // 2D Plane ray intersection test
  {
    const float fPlaneSide = vPlaneNormal.Dot(vRayStartPos) + fPlaneNegDist;
    const float fCosAlpha = vPlaneNormal.Dot(vRayDir);

    if (fCosAlpha == 0)                                      // ray is orthogonal to plane
      return false;

    if (WMath::Sign(fPlaneSide) == WMath::Sign(fCosAlpha)) // ray points away from the plane
      return false;

    fIntersectionTime = -fPlaneSide / fCosAlpha;

    vIntersection = vRayStartPos + fIntersectionTime * vRayDir;
  }

  const WVec2 vToIntersection = vIntersection - vLineSegmentPos0;

  const float fProjected = vLineDir.Dot(vToIntersection);

  if (fProjected < 0.0f)
    return false;

  if (fProjected > vLineDir.GetLengthSquared())
    return false;

  if (out_pIntersectionTime)
    *out_pIntersectionTime = fIntersectionTime;

  if (out_pIntersectionPoint)
    *out_pIntersectionPoint = vIntersection;

  return true;
}

bool WIntersectionUtils::IsPointOnLine(const WVec3& vLineStart, const WVec3& vLineEnd, const WVec3& vPoint, float fMaxDist /*= 0.01f*/)
{
  const WVec3 vClosest = ClosestPoint_PointLineSegment(vPoint, vLineStart, vLineEnd);
  const float fClosestDistSqr = (vClosest - vPoint).GetLengthSquared();

  return (fClosestDistSqr <= fMaxDist * fMaxDist);
}
