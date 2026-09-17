#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Vec3.h>

namespace WIntersectionUtils
{
  /// Checks whether a ray intersects with a triangle.
  ///
  /// The vertex winding order does not matter, triangles will be hit from both sides.
  ///
  /// \param vRayStartPos
  ///   The start position of the ray.
  /// \param vRayDir
  ///   The direction of the ray. This does not need to be normalized. Depending on its length, out_fIntersectionTime will be scaled differently.
  /// \param vVertex0, vVertex1, vVertex2
  ///   The three vertices forming the triangle.
  /// \param out_fIntersectionTime
  ///   The 'time' at which the ray intersects the triangle. If \a vRayDir is normalized, this is the exact distance.
  ///   out_fIntersectionPoint == vRayStartPos + vRayDir * out_fIntersectionTime
  ///   This parameter is optional and may be set to nullptr.
  /// \param out_fIntersectionPoint
  ///   The point where the ray intersects the triangle.
  ///   out_fIntersectionPoint == vRayStartPos + vRayDir * out_fIntersectionTime
  ///   This parameter is optional and may be set to nullptr.
  ///
  /// \return
  ///   True, if the ray intersects the triangle, false otherwise.
  W_FOUNDATION_DLL bool RayTriangleIntersection(const WVec3& vRayStartPos, const WVec3& vRayDir, const WVec3& vVertex0, const WVec3& vVertex1, const WVec3& vVertex2, float* out_pIntersectionTime = nullptr, WVec3* out_pIntersectionPoint = nullptr); // [tested]

  /// Checks whether a ray intersects with a triangle.
  ///
  /// The vertex winding order does not matter, triangles will be hit from both sides.
  ///
  /// \param vRayOrigin
  ///   The start position of the ray.
  /// \param vRayDir
  ///   The direction of the ray. This does not need to be normalized. Depending on its length, out_fIntersectionTime will be scaled differently.
  /// \param vVertex0, vVertex1, vVertex2
  ///   The three vertices forming the triangle.
  /// \param out_fIntersectionTime
  ///   The 'time' at which the ray intersects the triangle. If \a vRayDir is normalized, this is the exact distance.
  ///   IntersectionPoint == vRayOrigin + vRayDir * out_fIntersectionTime
  ///   This parameter is optional and may be set to nullptr.
  /// \param out_fIntersectionPoint
  ///   The point where the ray intersects the triangle.
  ///   out_fIntersectionPoint == vRayStartPos + vRayDir * out_fIntersectionTime
  ///   This parameter is optional and may be set to nullptr.
  /// \param out_pBarycentricCoords
  ///   The barycentric coordinates of the point where the ray intersects the triangle.
  ///   This parameter is optional and may be set to nullptr.
  ///
  /// \return
  ///   True, if the ray intersects the triangle, false otherwise.
  W_FOUNDATION_DLL bool RayTriangleIntersection(const WVec3& vRayOrigin, const WVec3& vRayDir, const WVec3& vVertex0, const WVec3& vVertex1, const WVec3& vVertex2, WVec3& out_vBarycentricCoords, float* out_pIntersectionTime = nullptr, WVec3* out_pIntersectionPoint = nullptr);

  /// Checks whether a ray intersects with a triangle.
  ///
  /// The vertex winding order DOES matter, triangles will not be hit from the back side.
  ///
  /// \param vRayOrigin
  ///   The start position of the ray.
  /// \param vRayDir
  ///   The direction of the ray. This does not need to be normalized. Depending on its length, out_fIntersectionTime will be scaled differently.
  /// \param vVertex0, vVertex1, vVertex2
  ///   The three vertices forming the triangle.
  /// \param out_fIntersectionTime
  ///   The 'time' at which the ray intersects the triangle. If \a vRayDir is normalized, this is the exact distance.
  ///   IntersectionPoint == vRayOrigin + vRayDir * out_fIntersectionTime
  ///   This parameter is optional and may be set to nullptr.
  /// \param out_fIntersectionPoint
  ///   The point where the ray intersects the triangle.
  ///   out_fIntersectionPoint == vRayStartPos + vRayDir * out_fIntersectionTime
  ///   This parameter is optional and may be set to nullptr.
  /// \param out_pBarycentricCoords
  ///   The barycentric coordinates of the point where the ray intersects the triangle.
  ///   This parameter is optional and may be set to nullptr.
  ///
  /// \return
  ///   True, if the ray intersects the triangle, false otherwise.
  W_FOUNDATION_DLL bool RayTriangleIntersectionCullBackface(const WVec3& vRayOrigin, const WVec3& vRayDir, const WVec3& vVertex0, const WVec3& vVertex1, const WVec3& vVertex2, WVec3& out_vBarycentricCoords, float* out_pIntersectionTime = nullptr, WVec3* out_pIntersectionPoint = nullptr);

  /// Checks whether a ray intersects with a polygon.
  ///
  /// The vertex winding order does not matter, polygons will be hit from both sides.
  ///
  /// \param vRayStartPos
  ///   The start position of the ray.
  /// \param vRayDir
  ///   The direction of the ray. This does not need to be normalized. Depending on its length, out_fIntersectionTime will be scaled differently.
  /// \param pPolygonVertices
  ///   Pointer to the first vertex of the polygon.
  /// \param uiNumVertices
  ///   The number of vertices in the polygon.
  /// \param out_fIntersectionTime
  ///   The 'time' at which the ray intersects the polygon. If \a vRayDir is normalized, this is the exact distance.
  ///   out_fIntersectionPoint == vRayStartPos + vRayDir * out_fIntersectionTime
  ///   This parameter is optional and may be set to nullptr.
  /// \param out_fIntersectionPoint
  ///   The point where the ray intersects the polygon.
  ///   out_fIntersectionPoint == vRayStartPos + vRayDir * out_fIntersectionTime
  ///   This parameter is optional and may be set to nullptr.
  /// \param uiVertexStride
  ///   The stride in bytes between each vertex in the pPolygonVertices array. If the array is tightly packed, this will equal sizeof(WVec3), but it
  ///   can be larger, if the vertices are interleaved with other data.
  /// \return
  ///   True, if the ray intersects the polygon, false otherwise.
  W_FOUNDATION_DLL bool RayPolygonIntersection(const WVec3& vRayStartPos, const WVec3& vRayDir, const WVec3* pPolygonVertices,
    WUInt32 uiNumVertices, float* out_pIntersectionTime = nullptr, WVec3* out_pIntersectionPoint = nullptr,
    WUInt32 uiVertexStride = sizeof(WVec3)); // [tested]


  /// Returns point on the line segment that is closest to \a vStartPoint. Optionally also returns the fraction along the segment, where that
  /// point is located.
  W_FOUNDATION_DLL WVec3 ClosestPoint_PointLineSegment(const WVec3& vStartPoint, const WVec3& vLineSegmentPos0, const WVec3& vLineSegmentPos1,
    float* out_pFractionAlongSegment = nullptr); // [tested]

  /// Computes the intersection point and time of the 2D ray with the 2D line segment. Returns true, if there is an intersection.
  W_FOUNDATION_DLL bool Ray2DLine2D(const WVec2& vRayStartPos, const WVec2& vRayDir, const WVec2& vLineSegmentPos0,
    const WVec2& vLineSegmentPos1, float* out_pIntersectionTime = nullptr, WVec2* out_pIntersectionPoint = nullptr); // [tested]

  /// Tests whether a point is located on a line
  W_FOUNDATION_DLL bool IsPointOnLine(const WVec3& vLineStart, const WVec3& vLineEnd, const WVec3& vPoint, float fMaxDist = 0.01f);

} // namespace WIntersectionUtils
