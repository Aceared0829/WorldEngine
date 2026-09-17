#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Frustum.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/Utilities/GraphicsUtils.h>

W_CREATE_SIMPLE_TEST(Math, Frustum)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "MakeFromPlanes")
  {
    WFrustum f;

    WPlane p[6];
    p[WFrustum::PlaneType::LeftPlane] = WPlane::MakeFromNormalAndPoint(WVec3(-1, 0, 0), WVec3(-2, 0, 0));
    p[WFrustum::PlaneType::RightPlane] = WPlane::MakeFromNormalAndPoint(WVec3(+1, 0, 0), WVec3(+2, 0, 0));
    p[WFrustum::PlaneType::BottomPlane] = WPlane::MakeFromNormalAndPoint(WVec3(0, -1, 0), WVec3(0, -2, 0));
    p[WFrustum::PlaneType::TopPlane] = WPlane::MakeFromNormalAndPoint(WVec3(0, +1, 0), WVec3(0, +2, 0));
    p[WFrustum::PlaneType::NearPlane] = WPlane::MakeFromNormalAndPoint(WVec3(0, 0, -1), WVec3(0, 0, 0));
    p[WFrustum::PlaneType::FarPlane] = WPlane::MakeFromNormalAndPoint(WVec3(0, 0, 1), WVec3(0, 0, 100));

    f = WFrustum::MakeFromPlanes(p);

    W_TEST_BOOL(f.GetPlane(0) == p[0]);
    W_TEST_BOOL(f.GetPlane(1) == p[1]);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "TransformFrustum/GetTransformedFrustum")
  {
    WFrustum f;

    WPlane p[6];
    p[WFrustum::PlaneType::LeftPlane] = WPlane::MakeFromNormalAndPoint(WVec3(-1, 0, 0), WVec3(-2, 0, 0));
    p[WFrustum::PlaneType::RightPlane] = WPlane::MakeFromNormalAndPoint(WVec3(+1, 0, 0), WVec3(+2, 0, 0));
    p[WFrustum::PlaneType::BottomPlane] = WPlane::MakeFromNormalAndPoint(WVec3(0, -1, 0), WVec3(0, -2, 0));
    p[WFrustum::PlaneType::TopPlane] = WPlane::MakeFromNormalAndPoint(WVec3(0, +1, 0), WVec3(0, +2, 0));
    p[WFrustum::PlaneType::NearPlane] = WPlane::MakeFromNormalAndPoint(WVec3(0, 0, -1), WVec3(0, 0, 0));
    p[WFrustum::PlaneType::FarPlane] = WPlane::MakeFromNormalAndPoint(WVec3(0, 0, 1), WVec3(0, 0, 100));

    f = WFrustum::MakeFromPlanes(p);

    WMat4 mTransform;
    mTransform = WMat4::MakeRotationY(WAngle::MakeFromDegree(90.0f));
    mTransform.SetTranslationVector(WVec3(2, 3, 4));

    WFrustum tf = f;
    tf.TransformFrustum(mTransform);

    p[0].Transform(mTransform);
    p[1].Transform(mTransform);

    for (int planeIndex = 0; planeIndex < 6; ++planeIndex)
    {
      W_TEST_BOOL(f.GetTransformedFrustum(mTransform).GetPlane(planeIndex) == tf.GetPlane(planeIndex));
    }

    W_TEST_BOOL(tf.GetPlane(0).IsEqual(p[0], WMath::LargeEpsilon<float>()));
    W_TEST_BOOL(tf.GetPlane(1).IsEqual(p[1], WMath::LargeEpsilon<float>()));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "InvertFrustum")
  {
    WFrustum f;

    WPlane p[6];
    p[WFrustum::PlaneType::LeftPlane] = WPlane::MakeFromNormalAndPoint(WVec3(-1, 0, 0), WVec3(-2, 0, 0));
    p[WFrustum::PlaneType::RightPlane] = WPlane::MakeFromNormalAndPoint(WVec3(+1, 0, 0), WVec3(+2, 0, 0));
    p[WFrustum::PlaneType::BottomPlane] = WPlane::MakeFromNormalAndPoint(WVec3(0, -1, 0), WVec3(0, -2, 0));
    p[WFrustum::PlaneType::TopPlane] = WPlane::MakeFromNormalAndPoint(WVec3(0, +1, 0), WVec3(0, +2, 0));
    p[WFrustum::PlaneType::NearPlane] = WPlane::MakeFromNormalAndPoint(WVec3(0, 0, -1), WVec3(0, 0, 0));
    p[WFrustum::PlaneType::FarPlane] = WPlane::MakeFromNormalAndPoint(WVec3(0, 0, 1), WVec3(0, 0, 100));

    f = WFrustum::MakeFromPlanes(p);

    f.InvertFrustum();

    p[0].Flip();
    p[1].Flip();

    W_TEST_BOOL(f.GetPlane(0) == p[0]);
    W_TEST_BOOL(f.GetPlane(1) == p[1]);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetFrustum")
  {
    // check that the extracted frustum planes are always the same, no matter the handedness or depth-range

    // test the different depth ranges
    for (int r = 0; r < 2; ++r)
    {
      const WClipSpaceDepthRange::Enum range = (r == 0) ? WClipSpaceDepthRange::MinusOneToOne : WClipSpaceDepthRange::ZeroToOne;

      // test rotated model-view matrices
      for (int rot = 0; rot < 360; rot += 45)
      {
        WVec3 vLookDir;
        vLookDir.Set(WMath::Sin(WAngle::MakeFromDegree((float)rot)), 0, -WMath::Cos(WAngle::MakeFromDegree((float)rot)));

        WVec3 vRightDir;
        vRightDir.Set(WMath::Sin(WAngle::MakeFromDegree(rot + 90.0f)), 0, -WMath::Cos(WAngle::MakeFromDegree(rot + 90.0f)));

        const WVec3 vCamPos(rot * 1.0f, rot * 0.5f, rot * -0.3f);

        // const WMat4 mViewLH = WGraphicsUtils::CreateViewMatrix(vCamPos, vLookDir, -vRightDir, WVec3(0, 1, 0), WHandedness::LeftHanded);
        // const WMat4 mViewRH = WGraphicsUtils::CreateViewMatrix(vCamPos, vLookDir, vRightDir, WVec3(0, 1, 0), WHandedness::RightHanded);
        const WMat4 mViewLH = WGraphicsUtils::CreateLookAtViewMatrix(vCamPos, vCamPos + vLookDir, WVec3(0, 1, 0), WHandedness::LeftHanded);
        const WMat4 mViewRH = WGraphicsUtils::CreateLookAtViewMatrix(vCamPos, vCamPos + vLookDir, WVec3(0, 1, 0), WHandedness::RightHanded);

        const WMat4 mProjLH = WGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(
          WAngle::MakeFromDegree(90), 1.0f, 1.0f, 100.0f, range, WClipSpaceYMode::Regular, WHandedness::LeftHanded);
        const WMat4 mProjRH = WGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(
          WAngle::MakeFromDegree(90), 1.0f, 1.0f, 100.0f, range, WClipSpaceYMode::Regular, WHandedness::RightHanded);

        const WMat4 mViewProjLH = mProjLH * mViewLH;
        const WMat4 mViewProjRH = mProjRH * mViewRH;

        WFrustum fB;
        const WFrustum fLH = WFrustum::MakeFromMVP(mViewProjLH, range, WHandedness::LeftHanded);
        const WFrustum fRH = WFrustum::MakeFromMVP(mViewProjRH, range, WHandedness::RightHanded);

        fB = WFrustum::MakeFromFOV(vCamPos, vLookDir, WVec3(0, 1, 0), WAngle::MakeFromDegree(90), WAngle::MakeFromDegree(90), 1.0f, 100.0f);

        W_TEST_BOOL(fRH.GetPlane(WFrustum::NearPlane).IsEqual(fB.GetPlane(WFrustum::NearPlane), 0.1f));
        W_TEST_BOOL(fRH.GetPlane(WFrustum::LeftPlane).IsEqual(fB.GetPlane(WFrustum::LeftPlane), 0.1f));
        W_TEST_BOOL(fRH.GetPlane(WFrustum::RightPlane).IsEqual(fB.GetPlane(WFrustum::RightPlane), 0.1f));
        W_TEST_BOOL(fRH.GetPlane(WFrustum::FarPlane).IsEqual(fB.GetPlane(WFrustum::FarPlane), 0.1f));
        W_TEST_BOOL(fRH.GetPlane(WFrustum::BottomPlane).IsEqual(fB.GetPlane(WFrustum::BottomPlane), 0.1f));
        W_TEST_BOOL(fRH.GetPlane(WFrustum::TopPlane).IsEqual(fB.GetPlane(WFrustum::TopPlane), 0.1f));

        W_TEST_BOOL(fLH.GetPlane(WFrustum::NearPlane).IsEqual(fB.GetPlane(WFrustum::NearPlane), 0.1f));
        W_TEST_BOOL(fLH.GetPlane(WFrustum::LeftPlane).IsEqual(fB.GetPlane(WFrustum::LeftPlane), 0.1f));
        W_TEST_BOOL(fLH.GetPlane(WFrustum::RightPlane).IsEqual(fB.GetPlane(WFrustum::RightPlane), 0.1f));
        W_TEST_BOOL(fLH.GetPlane(WFrustum::FarPlane).IsEqual(fB.GetPlane(WFrustum::FarPlane), 0.1f));
        W_TEST_BOOL(fLH.GetPlane(WFrustum::BottomPlane).IsEqual(fB.GetPlane(WFrustum::BottomPlane), 0.1f));
        W_TEST_BOOL(fLH.GetPlane(WFrustum::TopPlane).IsEqual(fB.GetPlane(WFrustum::TopPlane), 0.1f));
      }
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Culling")
  {
    const WVec3 offsetPos(23, 17, -9);
    const WVec3 camDir[6] = {WVec3(-1, 0, 0), WVec3(1, 0, 0), WVec3(0, -1, 0), WVec3(0, 1, 0), WVec3(0, 0, -1), WVec3(0, 0, 1)};
    const WVec3 objPos[6] = {WVec3(-9, 0, 0), WVec3(9, 0, 0), WVec3(0, -9, 0), WVec3(0, 9, 0), WVec3(0, 0, -9), WVec3(0, 0, 9)};

    for (WUInt32 dir = 0; dir < 6; ++dir)
    {
      WFrustum fDir;
      fDir = WFrustum::MakeFromFOV(offsetPos, camDir[dir], camDir[dir].GetOrthogonalVector() /*arbitrary*/, WAngle::MakeFromDegree(90), WAngle::MakeFromDegree(90), 1.0f, 100.0f);

      for (WUInt32 obj = 0; obj < 6; ++obj)
      {
        // box
        {
          WBoundingBox boundingObj;
          boundingObj = WBoundingBox::MakeFromCenterAndHalfExtents(offsetPos + objPos[obj], WVec3(1.0f));

          const WVolumePosition::Enum res = fDir.GetObjectPosition(boundingObj);

          if (obj == dir)
            W_TEST_BOOL(res == WVolumePosition::Inside);
          else
            W_TEST_BOOL(res == WVolumePosition::Outside);
        }

        // sphere
        {
          WBoundingSphere boundingObj = WBoundingSphere::MakeFromCenterAndRadius(offsetPos + objPos[obj], 0.93f);

          const WVolumePosition::Enum res = fDir.GetObjectPosition(boundingObj);

          if (obj == dir)
            W_TEST_BOOL(res == WVolumePosition::Inside);
          else
            W_TEST_BOOL(res == WVolumePosition::Outside);
        }

        // vertices
        {
          WBoundingBox boundingObj;
          boundingObj = WBoundingBox::MakeFromCenterAndHalfExtents(offsetPos + objPos[obj], WVec3(1.0f));

          WVec3 vertices[8];
          boundingObj.GetCorners(vertices);

          const WVolumePosition::Enum res = fDir.GetObjectPosition(vertices, 8);

          if (obj == dir)
            W_TEST_BOOL(res == WVolumePosition::Inside);
          else
            W_TEST_BOOL(res == WVolumePosition::Outside);
        }

        // vertices + transform
        {
          WBoundingBox boundingObj;
          boundingObj = WBoundingBox::MakeFromCenterAndHalfExtents(objPos[obj], WVec3(1.0f));

          WVec3 vertices[8];
          boundingObj.GetCorners(vertices);

          WMat4 transform = WMat4::MakeTranslation(offsetPos);

          const WVolumePosition::Enum res = fDir.GetObjectPosition(vertices, 8, transform);

          if (obj == dir)
            W_TEST_BOOL(res == WVolumePosition::Inside);
          else
            W_TEST_BOOL(res == WVolumePosition::Outside);
        }

        // SIMD box
        {
          WBoundingBox boundingObj;
          boundingObj = WBoundingBox::MakeFromCenterAndHalfExtents(offsetPos + objPos[obj], WVec3(1.0f));

          const bool res = fDir.Overlaps(WSimdConversion::ToBBox(boundingObj));

          if (obj == dir)
            W_TEST_BOOL(res == true);
          else
            W_TEST_BOOL(res == false);
        }

        // SIMD sphere
        {
          WBoundingSphere boundingObj = WBoundingSphere::MakeFromCenterAndRadius(offsetPos + objPos[obj], 0.93f);

          const bool res = fDir.Overlaps(WSimdConversion::ToBSphere(boundingObj));

          if (obj == dir)
            W_TEST_BOOL(res == true);
          else
            W_TEST_BOOL(res == false);
        }
      }
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ComputeCornerPoints")
  {
    const WMat4 mProj = WGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(
      WAngle::MakeFromDegree(90), 1.0f, 1.0f, 10.0f, WClipSpaceDepthRange::MinusOneToOne, WClipSpaceYMode::Regular, WHandedness::RightHanded);

    WFrustum frustum[2];
    frustum[0] = WFrustum::MakeFromMVP(mProj, WClipSpaceDepthRange::MinusOneToOne, WHandedness::RightHanded);
    frustum[1] = WFrustum::MakeFromFOV(WVec3::MakeZero(), WVec3(0, 0, -1), WVec3(0, 1, 0), WAngle::MakeFromDegree(90), WAngle::MakeFromDegree(90), 1.0f, 10.0f);

    for (int f = 0; f < 2; ++f)
    {
      WVec3 corner[8];
      frustum[f].ComputeCornerPoints(corner).AssertSuccess();

      WPositionOnPlane::Enum results[8][6];

      for (int c = 0; c < 8; ++c)
      {
        for (int p = 0; p < 6; ++p)
        {
          results[c][p] = WPositionOnPlane::Back;
        }
      }

      results[WFrustum::FrustumCorner::NearTopLeft][WFrustum::PlaneType::NearPlane] = WPositionOnPlane::OnPlane;
      results[WFrustum::FrustumCorner::NearTopLeft][WFrustum::PlaneType::TopPlane] = WPositionOnPlane::OnPlane;
      results[WFrustum::FrustumCorner::NearTopLeft][WFrustum::PlaneType::LeftPlane] = WPositionOnPlane::OnPlane;

      results[WFrustum::FrustumCorner::NearTopRight][WFrustum::PlaneType::NearPlane] = WPositionOnPlane::OnPlane;
      results[WFrustum::FrustumCorner::NearTopRight][WFrustum::PlaneType::TopPlane] = WPositionOnPlane::OnPlane;
      results[WFrustum::FrustumCorner::NearTopRight][WFrustum::PlaneType::RightPlane] = WPositionOnPlane::OnPlane;

      results[WFrustum::FrustumCorner::NearBottomLeft][WFrustum::PlaneType::NearPlane] = WPositionOnPlane::OnPlane;
      results[WFrustum::FrustumCorner::NearBottomLeft][WFrustum::PlaneType::BottomPlane] = WPositionOnPlane::OnPlane;
      results[WFrustum::FrustumCorner::NearBottomLeft][WFrustum::PlaneType::LeftPlane] = WPositionOnPlane::OnPlane;

      results[WFrustum::FrustumCorner::NearBottomRight][WFrustum::PlaneType::NearPlane] = WPositionOnPlane::OnPlane;
      results[WFrustum::FrustumCorner::NearBottomRight][WFrustum::PlaneType::BottomPlane] = WPositionOnPlane::OnPlane;
      results[WFrustum::FrustumCorner::NearBottomRight][WFrustum::PlaneType::RightPlane] = WPositionOnPlane::OnPlane;

      results[WFrustum::FrustumCorner::FarTopLeft][WFrustum::PlaneType::FarPlane] = WPositionOnPlane::OnPlane;
      results[WFrustum::FrustumCorner::FarTopLeft][WFrustum::PlaneType::TopPlane] = WPositionOnPlane::OnPlane;
      results[WFrustum::FrustumCorner::FarTopLeft][WFrustum::PlaneType::LeftPlane] = WPositionOnPlane::OnPlane;

      results[WFrustum::FrustumCorner::FarTopRight][WFrustum::PlaneType::FarPlane] = WPositionOnPlane::OnPlane;
      results[WFrustum::FrustumCorner::FarTopRight][WFrustum::PlaneType::TopPlane] = WPositionOnPlane::OnPlane;
      results[WFrustum::FrustumCorner::FarTopRight][WFrustum::PlaneType::RightPlane] = WPositionOnPlane::OnPlane;

      results[WFrustum::FrustumCorner::FarBottomLeft][WFrustum::PlaneType::FarPlane] = WPositionOnPlane::OnPlane;
      results[WFrustum::FrustumCorner::FarBottomLeft][WFrustum::PlaneType::BottomPlane] = WPositionOnPlane::OnPlane;
      results[WFrustum::FrustumCorner::FarBottomLeft][WFrustum::PlaneType::LeftPlane] = WPositionOnPlane::OnPlane;

      results[WFrustum::FrustumCorner::FarBottomRight][WFrustum::PlaneType::FarPlane] = WPositionOnPlane::OnPlane;
      results[WFrustum::FrustumCorner::FarBottomRight][WFrustum::PlaneType::BottomPlane] = WPositionOnPlane::OnPlane;
      results[WFrustum::FrustumCorner::FarBottomRight][WFrustum::PlaneType::RightPlane] = WPositionOnPlane::OnPlane;

      for (int c = 0; c < 8; ++c)
      {
        WFrustum::FrustumCorner cornerName = (WFrustum::FrustumCorner)c;

        for (int p = 0; p < 6; ++p)
        {
          WFrustum::PlaneType planeName = (WFrustum::PlaneType)p;

          WPlane plane = frustum[f].GetPlane(planeName);
          WPositionOnPlane::Enum expected = results[cornerName][planeName];
          WPositionOnPlane::Enum result = plane.GetPointPosition(corner[cornerName], 0.1f);
          // float fDistToPlane = plane.GetDistanceTo(corner[cornerName]);
          W_TEST_BOOL(result == expected);
        }
      }
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeFromCorners")
  {
    const WFrustum fOrg = WFrustum::MakeFromFOV(WVec3(1, 2, 3), WVec3(1, 1, 0).GetNormalized(), WVec3(0, 0, 1).GetNormalized(), WAngle::MakeFromDegree(110), WAngle::MakeFromDegree(70), 0.1f, 100.0f);

    WVec3 corners[8];
    fOrg.ComputeCornerPoints(corners).AssertSuccess();

    const WFrustum fNew = WFrustum::MakeFromCorners(corners);

    for (WUInt32 i = 0; i < 6; ++i)
    {
      WPlane p1 = fOrg.GetPlane(i);
      WPlane p2 = fNew.GetPlane(i);

      W_TEST_BOOL(p1.IsEqual(p2, WMath::LargeEpsilon<float>()));
    }

    WVec3 corners2[8];
    fNew.ComputeCornerPoints(corners2).AssertSuccess();

    // On ARM64, MakeFromCorners uses cross products on nearly-parallel edge vectors (~171 units long, differing by ~0.2),
    // causing catastrophic cancellation that amplifies small input differences into larger plane normal errors.
    // The reconstructed far-plane corners end up with ~0.005 error vs ~0.001 on x86.
#if W_ENABLED(W_PLATFORM_ARCH_ARM)
    const float fCornerEpsilon = WMath::VeryHugeEpsilon<float>();
#else
    const float fCornerEpsilon = WMath::HugeEpsilon<float>();
#endif

    for (WUInt32 i = 0; i < 8; ++i)
    {
      W_TEST_BOOL(corners[i].IsEqual(corners2[i], fCornerEpsilon));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeFromMVPInfiniteFarPlane")
  {
    WMat4 perspective = WGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(WAngle::MakeFromDegree(90), 1.0f, WMath::Infinity<float>(), 100.0f, WClipSpaceDepthRange::ZeroToOne, WClipSpaceYMode::Regular, WHandedness::RightHanded);

    auto frustum = WFrustum::MakeFromMVP(perspective);
    W_TEST_BOOL(frustum.IsValid());
  }
}
