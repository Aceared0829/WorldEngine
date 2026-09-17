#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Utilities/GraphicsUtils.h>

W_CREATE_SIMPLE_TEST(Utility, GraphicsUtils)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Perspective (-1/1): ConvertWorldPosToScreenPos / ConvertScreenPosToWorldPos")
  {
    WMat4 mProj, mProjInv;

    mProj = WGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(WAngle::MakeFromDegree(85.0f), 2.0f, 1.0f, 1000.0f, WClipSpaceDepthRange::MinusOneToOne, WClipSpaceYMode::Regular, WHandedness::LeftHanded);
    mProjInv = mProj.GetInverse();

    for (WUInt32 y = 0; y < 25; ++y)
    {
      for (WUInt32 x = 0; x < 50; ++x)
      {
        WVec3 vPoint, vDir;
        W_TEST_BOOL(WGraphicsUtils::ConvertScreenPosToWorldPos(mProjInv, 0, 0, 50, 25, WVec3((float)x, (float)y, 0.5f), vPoint, &vDir, WClipSpaceDepthRange::MinusOneToOne).Succeeded());

        W_TEST_VEC3(vDir, vPoint.GetNormalized(), 0.01f);

        WVec3 vScreen;
        W_TEST_BOOL(WGraphicsUtils::ConvertWorldPosToScreenPos(mProj, 0, 0, 50, 25, vPoint, vScreen, WClipSpaceDepthRange::MinusOneToOne).Succeeded());

        W_TEST_VEC3(vScreen, WVec3((float)x, (float)y, 0.5f), 0.01f);
      }
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Perspective (0/1): ConvertWorldPosToScreenPos / ConvertScreenPosToWorldPos")
  {
    WMat4 mProj, mProjInv;
    mProj = WGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(WAngle::MakeFromDegree(85.0f), 2.0f, 1.0f, 1000.0f, WClipSpaceDepthRange::ZeroToOne, WClipSpaceYMode::Regular, WHandedness::LeftHanded);
    mProjInv = mProj.GetInverse();

    for (WUInt32 y = 0; y < 25; ++y)
    {
      for (WUInt32 x = 0; x < 50; ++x)
      {
        WVec3 vPoint, vDir;
        W_TEST_BOOL(WGraphicsUtils::ConvertScreenPosToWorldPos(mProjInv, 0, 0, 50, 25, WVec3((float)x, (float)y, 0.5f), vPoint, &vDir, WClipSpaceDepthRange::ZeroToOne).Succeeded());

        W_TEST_VEC3(vDir, vPoint.GetNormalized(), 0.01f);

        WVec3 vScreen;
        W_TEST_BOOL(WGraphicsUtils::ConvertWorldPosToScreenPos(mProj, 0, 0, 50, 25, vPoint, vScreen, WClipSpaceDepthRange::ZeroToOne).Succeeded());

        W_TEST_VEC3(vScreen, WVec3((float)x, (float)y, 0.5f), 0.01f);
      }
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Ortho (-1/1): ConvertWorldPosToScreenPos / ConvertScreenPosToWorldPos")
  {
    WMat4 mProj, mProjInv;
    mProj = WGraphicsUtils::CreateOrthographicProjectionMatrix(50, 25, 1.0f, 1000.0f, WClipSpaceDepthRange::MinusOneToOne, WClipSpaceYMode::Regular, WHandedness::LeftHanded);

    mProjInv = mProj.GetInverse();

    for (WUInt32 y = 0; y < 25; ++y)
    {
      for (WUInt32 x = 0; x < 50; ++x)
      {
        WVec3 vPoint, vDir;
        W_TEST_BOOL(WGraphicsUtils::ConvertScreenPosToWorldPos(mProjInv, 0, 0, 50, 25, WVec3((float)x, (float)y, 0.5f), vPoint, &vDir, WClipSpaceDepthRange::MinusOneToOne).Succeeded());

        W_TEST_VEC3(vDir, WVec3(0, 0, 1.0f), 0.01f);

        WVec3 vScreen;
        W_TEST_BOOL(
          WGraphicsUtils::ConvertWorldPosToScreenPos(mProj, 0, 0, 50, 25, vPoint, vScreen, WClipSpaceDepthRange::MinusOneToOne).Succeeded());

        W_TEST_VEC3(vScreen, WVec3((float)x, (float)y, 0.5f), 0.01f);
      }
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Ortho (0/1): ConvertWorldPosToScreenPos / ConvertScreenPosToWorldPos")
  {
    WMat4 mProj, mProjInv;
    mProj = WGraphicsUtils::CreateOrthographicProjectionMatrix(50, 25, 1.0f, 1000.0f, WClipSpaceDepthRange::ZeroToOne, WClipSpaceYMode::Regular, WHandedness::LeftHanded);
    mProjInv = mProj.GetInverse();

    for (WUInt32 y = 0; y < 25; ++y)
    {
      for (WUInt32 x = 0; x < 50; ++x)
      {
        WVec3 vPoint, vDir;
        W_TEST_BOOL(WGraphicsUtils::ConvertScreenPosToWorldPos(mProjInv, 0, 0, 50, 25, WVec3((float)x, (float)y, 0.5f), vPoint, &vDir, WClipSpaceDepthRange::ZeroToOne).Succeeded());

        W_TEST_VEC3(vDir, WVec3(0, 0, 1.0f), 0.01f);

        WVec3 vScreen;
        W_TEST_BOOL(WGraphicsUtils::ConvertWorldPosToScreenPos(mProj, 0, 0, 50, 25, vPoint, vScreen, WClipSpaceDepthRange::ZeroToOne).Succeeded());

        W_TEST_VEC3(vScreen, WVec3((float)x, (float)y, 0.5f), 0.01f);
      }
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ConvertProjectionMatrixDepthRange")
  {
    WMat4 mProj1, mProj2;
    mProj1 = WGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(
      WAngle::MakeFromDegree(85.0f), 2.0f, 1.0f, 1000.0f, WClipSpaceDepthRange::ZeroToOne, WClipSpaceYMode::Regular, WHandedness::LeftHanded);
    mProj2 = WGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(
      WAngle::MakeFromDegree(85.0f), 2.0f, 1.0f, 1000.0f, WClipSpaceDepthRange::MinusOneToOne, WClipSpaceYMode::Regular, WHandedness::LeftHanded);

    WMat4 mProj1b = mProj1;
    WMat4 mProj2b = mProj2;
    WGraphicsUtils::ConvertProjectionMatrixDepthRange(mProj1b, WClipSpaceDepthRange::ZeroToOne, WClipSpaceDepthRange::MinusOneToOne);
    WGraphicsUtils::ConvertProjectionMatrixDepthRange(mProj2b, WClipSpaceDepthRange::MinusOneToOne, WClipSpaceDepthRange::ZeroToOne);

    W_TEST_BOOL(mProj1.IsEqual(mProj2b, 0.001f));
    W_TEST_BOOL(mProj2.IsEqual(mProj1b, 0.001f));
  }

  struct DepthRange
  {
    float fNear = 0.0f;
    float fFar = 0.0f;
  };

  W_TEST_BLOCK(WTestBlock::Enabled, "ExtractPerspectiveMatrixFieldOfView")
  {
    DepthRange depthRanges[] = {{1.0f, 1000.0f}, {1000.0f, 1.0f}, {0.5f, 20.0f}, {20.0f, 0.5f}};
    WClipSpaceDepthRange::Enum clipRanges[] = {WClipSpaceDepthRange::ZeroToOne, WClipSpaceDepthRange::MinusOneToOne};
    WHandedness::Enum handednesses[] = {WHandedness::LeftHanded, WHandedness::RightHanded};
    WClipSpaceYMode::Enum clipSpaceYModes[] = {WClipSpaceYMode::Regular, WClipSpaceYMode::Flipped};

    for (auto clipSpaceYMode : clipSpaceYModes)
    {
      for (auto handedness : handednesses)
      {
        for (auto depthRange : depthRanges)
        {
          for (auto clipRange : clipRanges)
          {
            for (WUInt32 angle = 10; angle < 180; angle += 10)
            {
              {
                WMat4 mProj;
                mProj = WGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(
                  WAngle::MakeFromDegree((float)angle), 2.0f, depthRange.fNear, depthRange.fFar, clipRange, clipSpaceYMode, handedness);

                WAngle fovx, fovy;
                WGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(mProj, fovx, fovy);

                W_TEST_FLOAT(fovx.GetDegree(), (float)angle, 0.5f);
              }

              {
                WMat4 mProj;
                mProj = WGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(
                  WAngle::MakeFromDegree((float)angle), 1.0f / 3.0f, depthRange.fNear, depthRange.fFar, clipRange, clipSpaceYMode, handedness);

                WAngle fovx, fovy;
                WGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(mProj, fovx, fovy);

                W_TEST_FLOAT(fovy.GetDegree(), (float)angle, 0.5f);
              }

              {
                const float fMinDepth = WMath::Min(depthRange.fNear, depthRange.fFar);
                const WAngle right = WAngle::MakeFromDegree((float)angle) / 2.0f;
                const WAngle top = WAngle::MakeFromDegree((float)angle) / 2.0f;
                const float fLeft = WMath::Tan(-right) * fMinDepth;
                const float fRight = WMath::Tan(right) * fMinDepth * 0.8f;
                const float fBottom = WMath::Tan(-top) * fMinDepth;
                const float fTop = WMath::Tan(top) * fMinDepth * 0.7f;

                WMat4 mProj;
                mProj = WGraphicsUtils::CreatePerspectiveProjectionMatrix(fLeft, fRight, fBottom, fTop, depthRange.fNear, depthRange.fFar, clipRange, clipSpaceYMode, handedness);

                float fNearOut, fFarOut;
                W_TEST_BOOL(WGraphicsUtils::ExtractNearAndFarClipPlaneDistances(fNearOut, fFarOut, mProj, clipRange).Succeeded());
                W_TEST_FLOAT(depthRange.fNear, fNearOut, 0.1f);
                W_TEST_FLOAT(depthRange.fFar, fFarOut, 0.1f);

                float fLeftOut, fRightOut, fBottomOut, fTopOut;
                W_TEST_BOOL(WGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(mProj, fLeftOut, fRightOut, fBottomOut, fTopOut, clipRange, clipSpaceYMode).Succeeded());
                W_TEST_FLOAT(fLeft, fLeftOut, WMath::LargeEpsilon<float>());
                W_TEST_FLOAT(fRight, fRightOut, WMath::LargeEpsilon<float>());
                W_TEST_FLOAT(fBottom, fBottomOut, WMath::LargeEpsilon<float>());
                W_TEST_FLOAT(fTop, fTopOut, WMath::LargeEpsilon<float>());

                WAngle fFovLeft;
                WAngle fFovRight;
                WAngle fFovBottom;
                WAngle fFovTop;
                WGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(mProj, fFovLeft, fFovRight, fFovBottom, fFovTop, clipSpaceYMode);

                W_TEST_FLOAT(fLeft, WMath::Tan(fFovLeft) * fMinDepth, WMath::LargeEpsilon<float>());
                W_TEST_FLOAT(fRight, WMath::Tan(fFovRight) * fMinDepth, WMath::LargeEpsilon<float>());
                W_TEST_FLOAT(fBottom, WMath::Tan(fFovBottom) * fMinDepth, WMath::LargeEpsilon<float>());
                W_TEST_FLOAT(fTop, WMath::Tan(fFovTop) * fMinDepth, WMath::LargeEpsilon<float>());
              }
            }
          }
        }
      }
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ExtractNearAndFarClipPlaneDistances")
  {
    DepthRange depthRanges[] = {{0.001f, 100.0f}, {0.01f, 10.0f}, {10.0f, 0.01f}, {1.01f, 110.0f}, {110.0f, 1.01f}};
    WClipSpaceDepthRange::Enum clipRanges[] = {WClipSpaceDepthRange::ZeroToOne, WClipSpaceDepthRange::MinusOneToOne};
    WHandedness::Enum handednesses[] = {WHandedness::LeftHanded, WHandedness::RightHanded};
    WClipSpaceYMode::Enum clipSpaceYModes[] = {WClipSpaceYMode::Regular, WClipSpaceYMode::Flipped};
    WAngle fovs[] = {WAngle::MakeFromDegree(10.0f), WAngle::MakeFromDegree(70.0f)};

    for (auto clipSpaceYMode : clipSpaceYModes)
    {
      for (auto handedness : handednesses)
      {
        for (auto depthRange : depthRanges)
        {
          for (auto clipRange : clipRanges)
          {
            for (auto fov : fovs)
            {
              WMat4 mProj;
              mProj = WGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(
                fov, 0.7f, depthRange.fNear, depthRange.fFar, clipRange, clipSpaceYMode, handedness);

              float fNearOut, fFarOut;
              W_TEST_BOOL(WGraphicsUtils::ExtractNearAndFarClipPlaneDistances(fNearOut, fFarOut, mProj, clipRange).Succeeded());

              W_TEST_FLOAT(depthRange.fNear, fNearOut, 0.1f);
              W_TEST_FLOAT(depthRange.fFar, fFarOut, 0.2f);
            }
          }
        }
      }
    }

    { // Test failure on broken projection matrix
      // This matrix has a 0 in the w-component of the third column (invalid perspective divide)
      float vals[] = {0.770734549f, 0.000000000f, 0.000000000f, 0.000000000f, 0.000000000f, 1.73205078f, 0.000000000f, 0.000000000f, 0.000000000f, 0.000000000f, -1.00000000f, 0.00000000f, 0.000000000, 0.000000000f, -0.100000001f, 0.000000000f};
      WMat4 mProj;
      memcpy(mProj.m_fElementsCM, vals, 16 * sizeof(float));
      float fNearOut = 0.f, fFarOut = 0.f;
      W_TEST_BOOL(WGraphicsUtils::ExtractNearAndFarClipPlaneDistances(fNearOut, fFarOut, mProj, WClipSpaceDepthRange::MinusOneToOne).Failed());
      W_TEST_BOOL(fNearOut == 0.0f);
      W_TEST_BOOL(fFarOut == 0.0f);
    }

    { // Test failure on broken projection matrix
      // This matrix has a 0 in the z-component of the fourth column (one or both projection planes are zero)
      float vals[] = {0.770734549f, 0.000000000f, 0.000000000f, 0.000000000f, 0.000000000f, 1.73205078f, 0.000000000f, 0.000000000f, 0.000000000f, 0.000000000f, -1.00000000f, -1.00000000f, 0.000000000f, 0.000000000f, 0.000000000f, 0.000000000f};
      WMat4 mProj;
      memcpy(mProj.m_fElementsCM, vals, 16 * sizeof(float));
      float fNearOut = 0.f, fFarOut = 0.f;
      W_TEST_BOOL(WGraphicsUtils::ExtractNearAndFarClipPlaneDistances(fNearOut, fFarOut, mProj, WClipSpaceDepthRange::MinusOneToOne).Failed());
      W_TEST_BOOL(fNearOut == 0.0f);
      W_TEST_BOOL(fFarOut == 0.0f);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ComputeInterpolatedFrustumPlane")
  {
    for (WUInt32 i = 0; i <= 10; ++i)
    {
      float nearPlane = 1.0f;
      float farPlane = 1000.0f;

      WMat4 mProj;
      mProj = WGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(
        WAngle::MakeFromDegree(90.0f), 1.0f, nearPlane, farPlane, WClipSpaceDepthRange::ZeroToOne, WClipSpaceYMode::Regular, WHandedness::LeftHanded);

      const WPlane horz = WGraphicsUtils::ComputeInterpolatedFrustumPlane(
        WGraphicsUtils::FrustumPlaneInterpolation::LeftToRight, i * 0.1f, mProj, WClipSpaceDepthRange::ZeroToOne);
      const WPlane vert = WGraphicsUtils::ComputeInterpolatedFrustumPlane(
        WGraphicsUtils::FrustumPlaneInterpolation::BottomToTop, i * 0.1f, mProj, WClipSpaceDepthRange::ZeroToOne);
      const WPlane forw = WGraphicsUtils::ComputeInterpolatedFrustumPlane(
        WGraphicsUtils::FrustumPlaneInterpolation::NearToFar, i * 0.1f, mProj, WClipSpaceDepthRange::ZeroToOne);

      // Generate clip space point at intersection of the 3 planes and project to worldspace
      WVec4 clipSpacePoint = WVec4(0.1f * i * 2 - 1, 0.1f * i * 2 - 1, 0.1f * i, 1);

      WVec4 worldSpacePoint = mProj.GetInverse() * clipSpacePoint;
      worldSpacePoint /= worldSpacePoint.w;

      W_TEST_FLOAT(horz.GetDistanceTo(WVec3::MakeZero()), 0.0f, 0.01f);
      W_TEST_FLOAT(vert.GetDistanceTo(WVec3::MakeZero()), 0.0f, 0.01f);

      if (i == 0)
      {
        W_TEST_FLOAT(forw.GetDistanceTo(WVec3::MakeZero()), -nearPlane, 0.01f);
      }
      else if (i == 10)
      {
        W_TEST_FLOAT(forw.GetDistanceTo(WVec3::MakeZero()), -farPlane, 0.01f);
      }

      W_TEST_FLOAT(horz.GetDistanceTo(worldSpacePoint.GetAsVec3()), 0.0f, 0.02f);
      W_TEST_FLOAT(vert.GetDistanceTo(worldSpacePoint.GetAsVec3()), 0.0f, 0.02f);
      W_TEST_FLOAT(forw.GetDistanceTo(worldSpacePoint.GetAsVec3()), 0.0f, 0.02f);

      // this isn't interpolated linearly across the angle (rotated), so the epsilon has to be very large (just an approx test)
      W_TEST_FLOAT(horz.m_vNormal.GetAngleBetween(WVec3(1, 0, 0)).GetDegree(), WMath::Abs(-45.0f + 90.0f * i * 0.1f), 4.0f);
      W_TEST_FLOAT(vert.m_vNormal.GetAngleBetween(WVec3(0, 1, 0)).GetDegree(), WMath::Abs(-45.0f + 90.0f * i * 0.1f), 4.0f);
      W_TEST_VEC3(forw.m_vNormal, WVec3(0, 0, 1), 0.01f);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CreateLookAtViewMatrix / CreateInverseLookAtViewMatrix")
  {
    for (int h = 0; h < 2; ++h)
    {
      const WHandedness::Enum handedness = (h == 0) ? WHandedness::LeftHanded : WHandedness::RightHanded;

      {
        WMat3 mLook3 = WGraphicsUtils::CreateLookAtViewMatrix(WVec3(1, 0, 0), WVec3(0, 0, 1), handedness);
        WMat3 mLookInv3 = WGraphicsUtils::CreateInverseLookAtViewMatrix(WVec3(1, 0, 0), WVec3(0, 0, 1), handedness);

        W_TEST_BOOL((mLook3 * mLookInv3).IsIdentity(0.01f));

        WMat4 mLook4 = WGraphicsUtils::CreateLookAtViewMatrix(WVec3(0), WVec3(1, 0, 0), WVec3(0, 0, 1), handedness);
        WMat4 mLookInv4 = WGraphicsUtils::CreateInverseLookAtViewMatrix(WVec3(0), WVec3(1, 0, 0), WVec3(0, 0, 1), handedness);

        W_TEST_BOOL((mLook4 * mLookInv4).IsIdentity(0.01f));

        W_TEST_BOOL(mLook3.IsEqual(mLook4.GetRotationalPart(), 0.01f));
        W_TEST_BOOL(mLookInv3.IsEqual(mLookInv4.GetRotationalPart(), 0.01f));
      }

      {
        WMat4 mLook4 = WGraphicsUtils::CreateLookAtViewMatrix(WVec3(1, 2, 0), WVec3(4, 5, 0), WVec3(0, 0, 1), handedness);
        WMat4 mLookInv4 = WGraphicsUtils::CreateInverseLookAtViewMatrix(WVec3(1, 2, 0), WVec3(4, 5, 0), WVec3(0, 0, 1), handedness);

        W_TEST_BOOL((mLook4 * mLookInv4).IsIdentity(0.01f));
      }
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CreateViewMatrix / DecomposeViewMatrix / CreateInverseViewMatrix")
  {
    for (int h = 0; h < 2; ++h)
    {
      const WHandedness::Enum handedness = (h == 0) ? WHandedness::LeftHanded : WHandedness::RightHanded;

      const WVec3 vEye(0);
      const WVec3 vTarget(0, 0, 1);
      const WVec3 vUp0(0, 1, 0);
      const WVec3 vFwd = (vTarget - vEye).GetNormalized();
      WVec3 vRight = vUp0.CrossRH(vFwd).GetNormalized();
      const WVec3 vUp = vFwd.CrossRH(vRight).GetNormalized();

      if (handedness == WHandedness::RightHanded)
        vRight = -vRight;

      const WMat4 mLookAt = WGraphicsUtils::CreateLookAtViewMatrix(vEye, vTarget, vUp0, handedness);

      WVec3 decFwd, decRight, decUp, decPos;
      WGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, mLookAt, handedness);

      W_TEST_VEC3(decPos, vEye, 0.01f);
      W_TEST_VEC3(decFwd, vFwd, 0.01f);
      W_TEST_VEC3(decUp, vUp, 0.01f);
      W_TEST_VEC3(decRight, vRight, 0.01f);

      const WMat4 mView = WGraphicsUtils::CreateViewMatrix(decPos, decFwd, decRight, decUp, handedness);
      const WMat4 mViewInv = WGraphicsUtils::CreateInverseViewMatrix(decPos, decFwd, decRight, decUp, handedness);

      W_TEST_BOOL(mLookAt.IsEqual(mView, 0.01f));

      W_TEST_BOOL((mLookAt * mViewInv).IsIdentity());
    }
  }
}
