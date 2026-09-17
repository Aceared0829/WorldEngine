#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Intersection.h>
#include <Foundation/Math/Mat4.h>

W_CREATE_SIMPLE_TEST(Math, Intersection)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "RayPolygonIntersection")
  {
    for (WUInt32 i = 0; i < 100; ++i)
    {
      WMat4 m;
      m = WMat4::MakeAxisRotation(WVec3(i + 1.0f, i * 3.0f, i * 7.0f).GetNormalized(), WAngle::MakeFromDegree((float)i));
      m.SetTranslationVector(WVec3((float)i, i * 2.0f, i * 3.0f));

      WVec3 Vertices[8] = {m.TransformPosition(WVec3(-10, -10, 0)), WVec3(-10, -10, 0), m.TransformPosition(WVec3(10, -10, 0)),
        WVec3(10, -10, 0), m.TransformPosition(WVec3(10, 10, 0)), WVec3(10, 10, 0), m.TransformPosition(WVec3(-10, 10, 0)), WVec3(-10, 10, 0)};

      for (float y = -14.5; y <= 14.5f; y += 2.0f)
      {
        for (float x = -14.5; x <= 14.5f; x += 2.0f)
        {
          const WVec3 vRayDir = m.TransformDirection(WVec3(x, y, -10.0f));
          const WVec3 vRayStart = m.TransformPosition(WVec3(x, y, 0.0f)) - vRayDir * 3.0f;

          const bool bIntersects = (x >= -10.0f && x <= 10.0f && y >= -10.0f && y <= 10.0f);

          float fIntersection;
          WVec3 vIntersection;
          W_TEST_BOOL(WIntersectionUtils::RayPolygonIntersection(vRayStart, vRayDir, Vertices, 4, &fIntersection, &vIntersection, sizeof(WVec3) * 2) == bIntersects);

          if (bIntersects)
          {
            W_TEST_FLOAT(fIntersection, 3.0f, 0.0001f);
            W_TEST_VEC3(vIntersection, m.TransformPosition(WVec3(x, y, 0.0f)), 0.0001f);
          }
        }
      }
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ClosestPoint_PointLineSegment")
  {
    for (WUInt32 i = 0; i < 100; ++i)
    {
      WMat4 m;
      m = WMat4::MakeAxisRotation(WVec3(i + 1.0f, i * 3.0f, i * 7.0f).GetNormalized(), WAngle::MakeFromDegree((float)i));
      m.SetTranslationVector(WVec3((float)i, i * 2.0f, i * 3.0f));

      WVec3 vSegment0 = m.TransformPosition(WVec3(-10, 1, 2));
      WVec3 vSegment1 = m.TransformPosition(WVec3(10, 1, 2));

      for (float f = -20; f <= -10; f += 0.5f)
      {
        const WVec3 vPos = m.TransformPosition(WVec3(f, 10.0f, 20.0f));

        float fFraction = -1.0f;
        const WVec3 vClosest = WIntersectionUtils::ClosestPoint_PointLineSegment(vPos, vSegment0, vSegment1, &fFraction);

        W_TEST_FLOAT(fFraction, 0.0f, 0.0001f);
        W_TEST_VEC3(vClosest, vSegment0, 0.0001f);
      }

      for (float f = -10; f <= 10; f += 0.5f)
      {
        const WVec3 vPos = m.TransformPosition(WVec3(f, 10.0f, 20.0f));

        float fFraction = -1.0f;
        const WVec3 vClosest = WIntersectionUtils::ClosestPoint_PointLineSegment(vPos, vSegment0, vSegment1, &fFraction);

        W_TEST_FLOAT(fFraction, (f + 10.0f) / 20.0f, 0.0001f);
        W_TEST_VEC3(vClosest, m.TransformPosition(WVec3(f, 1, 2)), 0.0001f);
      }

      for (float f = 10; f <= 20; f += 0.5f)
      {
        const WVec3 vPos = m.TransformPosition(WVec3(f, 10.0f, 20.0f));

        float fFraction = -1.0f;
        const WVec3 vClosest = WIntersectionUtils::ClosestPoint_PointLineSegment(vPos, vSegment0, vSegment1, &fFraction);

        W_TEST_FLOAT(fFraction, 1.0f, 0.0001f);
        W_TEST_VEC3(vClosest, vSegment1, 0.0001f);
      }
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Ray2DLine2D")
  {
    for (WUInt32 i = 0; i < 100; ++i)
    {
      WMat4 m;
      m = WMat4::MakeRotationZ(WAngle::MakeFromDegree((float)i));
      m.SetTranslationVector(WVec3((float)i, i * 2.0f, i * 3.0f));

      const WVec2 vSegment0 = m.TransformPosition(WVec3(23, 42, 0)).GetAsVec2();
      const WVec2 vSegmentDir = m.TransformDirection(WVec3(13, 15, 0)).GetAsVec2();

      const WVec2 vSegment1 = vSegment0 + vSegmentDir;

      for (float f = -1.1f; f < 2.0f; f += 0.2f)
      {
        const bool bIntersection = (f >= 0.0f && f <= 1.0f);
        const WVec2 vSegmentPos = vSegment0 + f * vSegmentDir;

        const WVec2 vRayDir = WVec2(2.0f, f);
        const WVec2 vRayStart = vSegmentPos - vRayDir * 5.0f;

        float fIntersection;
        WVec2 vIntersection;
        W_TEST_BOOL(WIntersectionUtils::Ray2DLine2D(vRayStart, vRayDir, vSegment0, vSegment1, &fIntersection, &vIntersection) == bIntersection);

        if (bIntersection)
        {
          W_TEST_FLOAT(fIntersection, 5.0f, 0.0001f);
          W_TEST_VEC2(vIntersection, vSegmentPos, 0.0001f);
        }
      };
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "RayTriangleIntersection")
  {
    for (WUInt32 i = 0; i < 100; ++i)
    {
      WMat4 m;
      m = WMat4::MakeAxisRotation(WVec3(i + 1.0f, i * 3.0f, i * 7.0f).GetNormalized(), WAngle::MakeFromDegree((float)i));
      m.SetTranslationVector(WVec3((float)i, i * 2.0f, i * 3.0f));

      WVec3 Vertices[4] = {m.TransformPosition(WVec3(-10, -10, 0)), m.TransformPosition(WVec3(10, -10, 0)),
        m.TransformPosition(WVec3(10, 10, 0)), m.TransformPosition(WVec3(-10, 10, 0))};

      for (float y = -14.5; y <= 14.5f; y += 2.0f)
      {
        for (float x = -14.5; x <= 14.5f; x += 2.0f)
        {
          const WVec3 vRayDir = m.TransformDirection(WVec3(x, y, -10.0f));
          const WVec3 vRayStart = m.TransformPosition(WVec3(x, y, 0.0f)) - vRayDir * 3.0f;

          const bool bInRect = (x >= -10.0f && x <= 10.0f && y >= -10.0f && y <= 10.0f);

          WVec3 vPlaneIntersection;
          WPlane plane = WPlane::MakeFromPoints(Vertices[0], Vertices[1], Vertices[2]);
          W_TEST_BOOL(plane.GetRayIntersection(vRayStart, vRayDir, nullptr, &vPlaneIntersection));
          const bool bIsOnEdge = WIntersectionUtils::IsPointOnLine(Vertices[0], Vertices[2], vPlaneIntersection);

          float fIntersection1 = 0;
          WVec3 vIntersection1(0.0f);
          float fIntersection2 = 0;
          WVec3 vIntersection2(0.0f);

          const bool bHit1a = WIntersectionUtils::RayPolygonIntersection(vRayStart, vRayDir, Vertices, 3);

          const bool bHit1 = WIntersectionUtils::RayTriangleIntersection(vRayStart, vRayDir, Vertices[0], Vertices[1], Vertices[2], &fIntersection1, &vIntersection1);

          W_TEST_BOOL(bHit1 == bHit1a);

          const bool bHit2 = WIntersectionUtils::RayTriangleIntersection(vRayStart, vRayDir, Vertices[0], Vertices[2], Vertices[3], &fIntersection2, &vIntersection2);

          if (!bInRect)
          {
            // outside rect, neither should hit
            W_TEST_BOOL(bHit1 == false);
            W_TEST_BOOL(bHit2 == false);
            W_TEST_BOOL(bHit1a == false);
          }
          else
          {
            if (!bIsOnEdge) // anything can happen close to the edge
            {
              // inside rect, exactly one triangle should hit
              W_TEST_BOOL(bHit1 || bHit2);
              W_TEST_BOOL(bHit1 != bHit2);
            }

            if (bHit1)
            {
              W_TEST_FLOAT(fIntersection1, 3.0f, 0.0001f);
              W_TEST_VEC3(vIntersection1, m.TransformPosition(WVec3(x, y, 0.0f)), 0.0001f);
            }

            if (bHit2)
            {
              W_TEST_FLOAT(fIntersection2, 3.0f, 0.0001f);
              W_TEST_VEC3(vIntersection2, m.TransformPosition(WVec3(x, y, 0.0f)), 0.0001f);
            }
          }
        }
      }
    }
  }
}
