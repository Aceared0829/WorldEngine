#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Frustum.h>
#include <Foundation/SimdMath/SimdBBox.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <Foundation/Utilities/GraphicsUtils.h>

WFrustum::WFrustum() = default;
WFrustum::~WFrustum() = default;

WFrustum WFrustum::MakeInvalid()
{
  WFrustum frustum;
  for (WUInt32 i = 0; i < PLANE_COUNT; ++i)
  {
    frustum.m_Planes[i] = WPlane::MakeInvalid();
  }

  return frustum;
}

const WPlane& WFrustum::GetPlane(WUInt8 uiPlane) const
{
  W_ASSERT_DEBUG(uiPlane < PLANE_COUNT, "Invalid plane index.");

  return m_Planes[uiPlane];
}

WPlane& WFrustum::AccessPlane(WUInt8 uiPlane)
{
  W_ASSERT_DEBUG(uiPlane < PLANE_COUNT, "Invalid plane index.");

  return m_Planes[uiPlane];
}

bool WFrustum::IsValid() const
{
  // For frustums with infinite farplanes we test a finite frustum slice for validity, as the
  // computations below don't work when 4 of the corner points are at infinity.
  if (WMath::Abs(m_Planes[FarPlane].m_fNegDistance) == WMath::Infinity<float>())
  {
    WFrustum finiteSlice = *this;
    finiteSlice.m_Planes[FarPlane].m_fNegDistance = -2.f * WMath::Abs(m_Planes[NearPlane].m_fNegDistance);
    return finiteSlice.IsValid();
  }

  for (WUInt32 i = 0; i < PLANE_COUNT; ++i)
  {
    if (!m_Planes[i].IsValid() || (i != FarPlane && !WMath::IsFinite(m_Planes[i].m_fNegDistance)))
      return false;
  }

  WVec3 corners[8];
  if (ComputeCornerPoints(corners).Failed())
    return false;

  WVec3 center = WVec3::MakeZero();
  for (WUInt32 i = 0; i < 8; ++i)
  {
    center += corners[i];
  }
  center /= 8.0f;

  if (GetObjectPosition(&center, 1) != WVolumePosition::Inside)
    return false;

  return true;
}

WFrustum WFrustum::MakeFromPlanes(const WPlane* pPlanes)
{
  WFrustum frustum;
  const WResult res = TryMakeFromPlanes(frustum, pPlanes);
  W_ASSERT_DEV(res.Succeeded() && frustum.IsValid(), "Frustum is not valid after construction.");
  W_IGNORE_UNUSED(res);
  return frustum;
}

WResult WFrustum::TryMakeFromPlanes(WFrustum& out_frustum, const WPlane* pPlanes)
{
  WFrustum f;

  for (WUInt32 i = 0; i < PLANE_COUNT; ++i)
    f.m_Planes[i] = pPlanes[i];

  if (f.IsValid())
  {
    out_frustum = std::move(f);
    return W_SUCCESS;
  }

  return W_FAILURE;
}

void WFrustum::TransformFrustum(const WMat4& mTransform)
{
  for (WUInt32 i = 0; i < PLANE_COUNT; ++i)
  {
    m_Planes[i].Transform(mTransform);
  }
}

WFrustum WFrustum::GetTransformedFrustum(const WMat4& mTransform) const
{
  WFrustum result = *this;
  result.TransformFrustum(mTransform);
  return result;
}

WVolumePosition::Enum WFrustum::GetObjectPosition(const WVec3* pVertices, WUInt32 uiNumVertices) const
{
  /// \test Not yet tested

  bool bOnSomePlane = false;

  for (WUInt32 i = 0; i < PLANE_COUNT; ++i)
  {
    const WPositionOnPlane::Enum pos = m_Planes[i].GetObjectPosition(pVertices, uiNumVertices);

    if (pos == WPositionOnPlane::Back)
      continue;

    if (pos == WPositionOnPlane::Front)
      return WVolumePosition::Outside;

    bOnSomePlane = true;
  }

  if (bOnSomePlane)
    return WVolumePosition::Intersecting;

  return WVolumePosition::Inside;
}

static WPositionOnPlane::Enum GetPlaneObjectPosition(const WPlane& p, const WVec3* const pPoints, WUInt32 uiVertices, const WMat4& mTransform)
{
  bool bFront = false;
  bool bBack = false;

  for (WUInt32 i = 0; i < uiVertices; ++i)
  {
    switch (p.GetPointPosition(mTransform * pPoints[i]))
    {
      case WPositionOnPlane::Front:
      {
        if (bBack)
          return WPositionOnPlane::Spanning;

        bFront = true;
      }
      break;

      case WPositionOnPlane::Back:
      {
        if (bFront)
          return (WPositionOnPlane::Spanning);

        bBack = true;
      }
      break;

      default:
        break;
    }
  }

  return (bFront ? WPositionOnPlane::Front : WPositionOnPlane::Back);
}


WVolumePosition::Enum WFrustum::GetObjectPosition(const WVec3* pVertices, WUInt32 uiNumVertices, const WMat4& mObjectTransform) const
{
  /// \test Not yet tested

  bool bOnSomePlane = false;

  for (WUInt32 i = 0; i < PLANE_COUNT; ++i)
  {
    const WPositionOnPlane::Enum pos = GetPlaneObjectPosition(m_Planes[i], pVertices, uiNumVertices, mObjectTransform);

    if (pos == WPositionOnPlane::Back)
      continue;

    if (pos == WPositionOnPlane::Front)
      return WVolumePosition::Outside;

    bOnSomePlane = true;
  }

  if (bOnSomePlane)
    return WVolumePosition::Intersecting;

  return WVolumePosition::Inside;
}

WVolumePosition::Enum WFrustum::GetObjectPosition(const WBoundingSphere& sphere) const
{
  /// \test Not yet tested

  bool bOnSomePlane = false;

  for (WUInt32 i = 0; i < PLANE_COUNT; ++i)
  {
    const WPositionOnPlane::Enum pos = m_Planes[i].GetObjectPosition(sphere);

    if (pos == WPositionOnPlane::Back)
      continue;

    if (pos == WPositionOnPlane::Front)
      return WVolumePosition::Outside;

    bOnSomePlane = true;
  }

  if (bOnSomePlane)
    return WVolumePosition::Intersecting;

  return WVolumePosition::Inside;
}

WVolumePosition::Enum WFrustum::GetObjectPosition(const WBoundingBox& box) const
{
  /// \test Not yet tested

  bool bOnSomePlane = false;

  for (WUInt32 i = 0; i < PLANE_COUNT; ++i)
  {
    const WPositionOnPlane::Enum pos = m_Planes[i].GetObjectPosition(box);

    if (pos == WPositionOnPlane::Back)
      continue;

    if (pos == WPositionOnPlane::Front)
      return WVolumePosition::Outside;

    bOnSomePlane = true;
  }

  if (bOnSomePlane)
    return WVolumePosition::Intersecting;

  return WVolumePosition::Inside;
}

void WFrustum::InvertFrustum()
{
  for (WUInt32 i = 0; i < PLANE_COUNT; ++i)
    m_Planes[i].Flip();
}

WResult WFrustum::ComputeCornerPoints(WVec3 out_pPoints[FrustumCorner::CORNER_COUNT]) const
{
  W_SUCCEED_OR_RETURN(WPlane::GetPlanesIntersectionPoint(m_Planes[NearPlane], m_Planes[TopPlane], m_Planes[LeftPlane], out_pPoints[FrustumCorner::NearTopLeft]));
  W_SUCCEED_OR_RETURN(WPlane::GetPlanesIntersectionPoint(m_Planes[NearPlane], m_Planes[TopPlane], m_Planes[RightPlane], out_pPoints[FrustumCorner::NearTopRight]));
  W_SUCCEED_OR_RETURN(WPlane::GetPlanesIntersectionPoint(m_Planes[NearPlane], m_Planes[BottomPlane], m_Planes[LeftPlane], out_pPoints[FrustumCorner::NearBottomLeft]));
  W_SUCCEED_OR_RETURN(WPlane::GetPlanesIntersectionPoint(m_Planes[NearPlane], m_Planes[BottomPlane], m_Planes[RightPlane], out_pPoints[FrustumCorner::NearBottomRight]));

  W_SUCCEED_OR_RETURN(WPlane::GetPlanesIntersectionPoint(m_Planes[FarPlane], m_Planes[TopPlane], m_Planes[LeftPlane], out_pPoints[FrustumCorner::FarTopLeft]));
  W_SUCCEED_OR_RETURN(WPlane::GetPlanesIntersectionPoint(m_Planes[FarPlane], m_Planes[TopPlane], m_Planes[RightPlane], out_pPoints[FrustumCorner::FarTopRight]));
  W_SUCCEED_OR_RETURN(WPlane::GetPlanesIntersectionPoint(m_Planes[FarPlane], m_Planes[BottomPlane], m_Planes[LeftPlane], out_pPoints[FrustumCorner::FarBottomLeft]));
  W_SUCCEED_OR_RETURN(WPlane::GetPlanesIntersectionPoint(m_Planes[FarPlane], m_Planes[BottomPlane], m_Planes[RightPlane], out_pPoints[FrustumCorner::FarBottomRight]));

  return W_SUCCESS;
}

WFrustum WFrustum::MakeFromMVP(const WMat4& mModelViewProjection0, WClipSpaceDepthRange::Enum depthRange, WHandedness::Enum handedness)
{
  WFrustum frustum;
  const WResult res = TryMakeFromMVP(frustum, mModelViewProjection0, depthRange, handedness);
  W_ASSERT_DEV(res.Succeeded() && frustum.IsValid(), "Frustum is not valid after construction.");
  W_IGNORE_UNUSED(res);
  return frustum;
}

WResult WFrustum::TryMakeFromMVP(WFrustum& out_frustum, const WMat4& mModelViewProjection0, WClipSpaceDepthRange::Enum depthRange, WHandedness::Enum handedness)
{
  WMat4 ModelViewProjection = mModelViewProjection0;
  WGraphicsUtils::ConvertProjectionMatrixDepthRange(ModelViewProjection, depthRange, WClipSpaceDepthRange::MinusOneToOne);

  WVec4 planes[6];

  if (handedness == WHandedness::LeftHanded)
  {
    ModelViewProjection.SetRow(0, -ModelViewProjection.GetRow(0));
  }

  planes[LeftPlane] = -ModelViewProjection.GetRow(3) - ModelViewProjection.GetRow(0);
  planes[RightPlane] = -ModelViewProjection.GetRow(3) + ModelViewProjection.GetRow(0);
  planes[BottomPlane] = -ModelViewProjection.GetRow(3) - ModelViewProjection.GetRow(1);
  planes[TopPlane] = -ModelViewProjection.GetRow(3) + ModelViewProjection.GetRow(1);
  planes[NearPlane] = -ModelViewProjection.GetRow(3) - ModelViewProjection.GetRow(2);
  planes[FarPlane] = -ModelViewProjection.GetRow(3) + ModelViewProjection.GetRow(2);

  // Normalize planes
  for (int p = 0; p < 6; ++p)
  {
    const float len = planes[p].GetAsVec3().GetLength();
    // doing the division here manually since we want to accept the case where length is 0 (infinite plane)
    const float invLen = 1.f / len;
    planes[p].x *= WMath::IsFinite(invLen) ? invLen : 0.f;
    planes[p].y *= WMath::IsFinite(invLen) ? invLen : 0.f;
    planes[p].z *= WMath::IsFinite(invLen) ? invLen : 0.f;
    planes[p].w *= invLen;
  }

  // The last matrix row is giving the camera's plane, which means its normal is
  // also the camera's viewing direction.
  const WVec3 cameraViewDirection = ModelViewProjection.GetRow(3).GetAsVec3();

  // Making sure the near/far plane is always closest/farthest. The way we derive the
  // planes always yields the closer plane pointing towards the camera and the farther
  // plane pointing away from the camera, so flip when that relationship inverts.
  if (planes[FarPlane].GetAsVec3().Dot(cameraViewDirection) < 0)
  {
    W_ASSERT_DEBUG(planes[NearPlane].GetAsVec3().Dot(cameraViewDirection) >= 0, "");
    WMath::Swap(planes[NearPlane], planes[FarPlane]);
  }

  // In case we have an infinity far plane projection, the normal is invalid.
  // We'll just take the mirrored normal from the near plane.
  W_ASSERT_DEBUG(planes[NearPlane].IsValid(), "Near plane is expected to be non-nan and finite at this point!");
  if (WMath::Abs(planes[FarPlane].w) == WMath::Infinity<float>())
  {
    planes[FarPlane] = (-planes[NearPlane].GetAsVec3()).GetAsVec4(planes[FarPlane].w);
  }

  static_assert(sizeof(WFrustum) == sizeof(planes));
  if (reinterpret_cast<WFrustum*>(planes)->IsValid())
  {
    static_assert(offsetof(WPlane, m_vNormal) == offsetof(WVec4, x) && offsetof(WPlane, m_fNegDistance) == offsetof(WVec4, w));
    WMemoryUtils::Copy(out_frustum.m_Planes, (WPlane*)planes, 6);
    return W_SUCCESS;
  }

  return W_FAILURE;
}

WFrustum WFrustum::MakeFromFOV(const WVec3& vPosition, const WVec3& vForwards, const WVec3& vUp, WAngle fovX, WAngle fovY, float fNearPlane, float fFarPlane)
{
  WFrustum frustum;
  const WResult res = TryMakeFromFOV(frustum, vPosition, vForwards, vUp, fovX, fovY, fNearPlane, fFarPlane);
  W_ASSERT_DEV(res.Succeeded() && frustum.IsValid(), "Frustum is not valid after construction.");
  W_IGNORE_UNUSED(res);
  return frustum;
}

WResult WFrustum::TryMakeFromFOV(WFrustum& out_frustum, const WVec3& vPosition, const WVec3& vForwards, const WVec3& vUp, WAngle fovX, WAngle fovY, float fNearPlane, float fFarPlane)
{
  W_ASSERT_DEBUG(WMath::Abs(vForwards.GetNormalized().Dot(vUp.GetNormalized())) < 0.999f, "Up dir must be different from forward direction");

  const WVec3 vForwardsNorm = vForwards.GetNormalized();
  const WVec3 vRightNorm = vForwards.CrossRH(vUp).GetNormalized();
  const WVec3 vUpNorm = vRightNorm.CrossRH(vForwards).GetNormalized();

  WFrustum res;

  // Near Plane
  res.m_Planes[NearPlane] = WPlane::MakeFromNormalAndPoint(-vForwardsNorm, vPosition + fNearPlane * vForwardsNorm);

  // Far Plane
  res.m_Planes[FarPlane] = WPlane::MakeFromNormalAndPoint(vForwardsNorm, vPosition + fFarPlane * vForwardsNorm);

  // Making sure the near/far plane is always closest/farthest.
  if (fNearPlane > fFarPlane)
  {
    WMath::Swap(res.m_Planes[NearPlane], res.m_Planes[FarPlane]);
  }

  WMat3 mLocalFrame;
  mLocalFrame.SetColumn(0, vRightNorm);
  mLocalFrame.SetColumn(1, vUpNorm);
  mLocalFrame.SetColumn(2, -vForwardsNorm);

  const float fCosFovX = WMath::Cos(fovX * 0.5f);
  const float fSinFovX = WMath::Sin(fovX * 0.5f);

  const float fCosFovY = WMath::Cos(fovY * 0.5f);
  const float fSinFovY = WMath::Sin(fovY * 0.5f);

  // Left Plane
  {
    WVec3 vPlaneNormal = mLocalFrame * WVec3(-fCosFovX, 0, fSinFovX);
    vPlaneNormal.Normalize();

    res.m_Planes[LeftPlane] = WPlane::MakeFromNormalAndPoint(vPlaneNormal, vPosition);
  }

  // Right Plane
  {
    WVec3 vPlaneNormal = mLocalFrame * WVec3(fCosFovX, 0, fSinFovX);
    vPlaneNormal.Normalize();

    res.m_Planes[RightPlane] = WPlane::MakeFromNormalAndPoint(vPlaneNormal, vPosition);
  }

  // Bottom Plane
  {
    WVec3 vPlaneNormal = mLocalFrame * WVec3(0, -fCosFovY, fSinFovY);
    vPlaneNormal.Normalize();

    res.m_Planes[BottomPlane] = WPlane::MakeFromNormalAndPoint(vPlaneNormal, vPosition);
  }

  // Top Plane
  {
    WVec3 vPlaneNormal = mLocalFrame * WVec3(0, fCosFovY, fSinFovY);
    vPlaneNormal.Normalize();

    res.m_Planes[TopPlane] = WPlane::MakeFromNormalAndPoint(vPlaneNormal, vPosition);
  }

  if (res.IsValid())
  {
    out_frustum = std::move(res);
    return W_SUCCESS;
  }

  return W_FAILURE;
}

WFrustum WFrustum::MakeFromCorners(const WVec3 pCorners[FrustumCorner::CORNER_COUNT])
{
  WFrustum frustum;
  const WResult res = TryMakeFromCorners(frustum, pCorners);
  W_ASSERT_DEV(res.Succeeded() && frustum.IsValid(), "Frustum is not valid after construction.");
  W_IGNORE_UNUSED(res);
  return frustum;
}

WResult WFrustum::TryMakeFromCorners(WFrustum& out_frustum, const WVec3 pCorners[FrustumCorner::CORNER_COUNT])
{
  WFrustum res;

  res.m_Planes[PlaneType::LeftPlane] = WPlane::MakeFromPoints(pCorners[FrustumCorner::FarTopLeft], pCorners[FrustumCorner::NearBottomLeft], pCorners[FrustumCorner::NearTopLeft]);

  res.m_Planes[PlaneType::RightPlane] = WPlane::MakeFromPoints(pCorners[FrustumCorner::NearTopRight], pCorners[FrustumCorner::FarBottomRight], pCorners[FrustumCorner::FarTopRight]);

  res.m_Planes[PlaneType::BottomPlane] = WPlane::MakeFromPoints(pCorners[FrustumCorner::NearBottomLeft], pCorners[FrustumCorner::FarBottomRight], pCorners[FrustumCorner::NearBottomRight]);

  res.m_Planes[PlaneType::TopPlane] = WPlane::MakeFromPoints(pCorners[FrustumCorner::FarTopLeft], pCorners[FrustumCorner::NearTopRight], pCorners[FrustumCorner::FarTopRight]);

  res.m_Planes[PlaneType::FarPlane] = WPlane::MakeFromPoints(pCorners[FrustumCorner::FarTopLeft], pCorners[FrustumCorner::FarBottomRight], pCorners[FrustumCorner::FarBottomLeft]);

  res.m_Planes[PlaneType::NearPlane] = WPlane::MakeFromPoints(pCorners[FrustumCorner::NearTopLeft], pCorners[FrustumCorner::NearBottomRight], pCorners[FrustumCorner::NearTopRight]);

  if (res.IsValid())
  {
    out_frustum = std::move(res);
    return W_SUCCESS;
  }

  return W_FAILURE;
}
