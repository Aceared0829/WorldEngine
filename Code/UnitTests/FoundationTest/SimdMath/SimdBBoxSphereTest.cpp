#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/SimdMath/SimdBBoxSphere.h>
#include <Foundation/SimdMath/SimdConversion.h>

W_CREATE_SIMPLE_TEST(SimdMath, SimdBBoxSphere)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Make Functions")
  {
    WSimdBBoxSphere b = WSimdBBoxSphere::MakeFromCenterExtents(WSimdVec4f(-1, -2, -3), WSimdVec4f(1, 2, 3), 2);

    W_TEST_BOOL((b.m_CenterAndRadius == WSimdVec4f(-1, -2, -3, 2)).AllSet<4>());
    W_TEST_BOOL((b.m_BoxHalfExtents == WSimdVec4f(1, 2, 3)).AllSet<3>());

    WSimdBBox box(WSimdVec4f(1, 1, 1), WSimdVec4f(3, 3, 3));
    WSimdBSphere sphere(WSimdVec4f(2, 2, 2), 1);

    b = WSimdBBoxSphere::MakeFromBoxAndSphere(box, sphere);

    W_TEST_BOOL((b.m_CenterAndRadius == WSimdVec4f(2, 2, 2, 1)).AllSet<4>());
    W_TEST_BOOL((b.m_BoxHalfExtents == WSimdVec4f(1, 1, 1)).AllSet<3>());
    W_TEST_BOOL(b.GetBox() == box);
    W_TEST_BOOL(b.GetSphere() == sphere);

    b = WSimdBBoxSphere::MakeFromBox(box);

    W_TEST_BOOL(b.m_CenterAndRadius.IsEqual(WSimdVec4f(2, 2, 2, WMath::Sqrt(3.0f)), 0.00001f).AllSet<4>());
    W_TEST_BOOL((b.m_BoxHalfExtents == WSimdVec4f(1, 1, 1)).AllSet<3>());
    W_TEST_BOOL(b.GetBox() == box);

    b = WSimdBBoxSphere::MakeFromSphere(sphere);

    W_TEST_BOOL((b.m_CenterAndRadius == WSimdVec4f(2, 2, 2, 1)).AllSet<4>());
    W_TEST_BOOL((b.m_BoxHalfExtents == WSimdVec4f(1, 1, 1)).AllSet<3>());
    W_TEST_BOOL(b.GetSphere() == sphere);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeInvalid")
  {
    WSimdBBoxSphere b = WSimdBBoxSphere::MakeInvalid();

    W_TEST_BOOL(!b.IsValid());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsNaN")
  {
    if (WMath::SupportsNaN<float>())
    {
      WSimdBBoxSphere b = WSimdBBoxSphere::MakeInvalid();

      b = WSimdBBoxSphere::MakeInvalid();
      W_TEST_BOOL(!b.IsNaN());

      b = WSimdBBoxSphere::MakeInvalid();
      b.m_CenterAndRadius.SetX(WMath::NaN<float>());
      W_TEST_BOOL(b.IsNaN());

      b = WSimdBBoxSphere::MakeInvalid();
      b.m_CenterAndRadius.SetY(WMath::NaN<float>());
      W_TEST_BOOL(b.IsNaN());

      b = WSimdBBoxSphere::MakeInvalid();
      b.m_CenterAndRadius.SetZ(WMath::NaN<float>());
      W_TEST_BOOL(b.IsNaN());

      b = WSimdBBoxSphere::MakeInvalid();
      b.m_CenterAndRadius.SetW(WMath::NaN<float>());
      W_TEST_BOOL(b.IsNaN());

      b = WSimdBBoxSphere::MakeInvalid();
      b.m_BoxHalfExtents.SetX(WMath::NaN<float>());
      W_TEST_BOOL(b.IsNaN());

      b = WSimdBBoxSphere::MakeInvalid();
      b.m_BoxHalfExtents.SetY(WMath::NaN<float>());
      W_TEST_BOOL(b.IsNaN());

      b = WSimdBBoxSphere::MakeInvalid();
      b.m_BoxHalfExtents.SetZ(WMath::NaN<float>());
      W_TEST_BOOL(b.IsNaN());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetFromPoints")
  {
    WSimdVec4f p[6] = {
      WSimdVec4f(-4, 0, 0),
      WSimdVec4f(5, 0, 0),
      WSimdVec4f(0, -6, 0),
      WSimdVec4f(0, 7, 0),
      WSimdVec4f(0, 0, -8),
      WSimdVec4f(0, 0, 9),
    };

    const WSimdBBoxSphere b = WSimdBBoxSphere::MakeFromPoints(p, 6);

    W_TEST_BOOL((b.m_CenterAndRadius == WSimdVec4f(0.5, 0.5, 0.5)).AllSet<3>());
    W_TEST_BOOL((b.m_BoxHalfExtents == WSimdVec4f(4.5, 6.5, 8.5)).AllSet<3>());
    W_TEST_BOOL(b.m_CenterAndRadius.w().IsEqual(WSimdVec4f(0.5, 0.5, 8.5).GetLength<3>(), 0.00001f));
    W_TEST_BOOL(b.m_CenterAndRadius.w() <= b.m_BoxHalfExtents.GetLength<3>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ExpandToInclude")
  {
    WSimdBBoxSphere b1 = WSimdBBoxSphere::MakeInvalid();
    WSimdBBoxSphere b2(WSimdBBox(WSimdVec4f(2, 2, 2), WSimdVec4f(4, 4, 4)));

    b1.ExpandToInclude(b2);
    W_TEST_BOOL(b1 == b2);

    WSimdBSphere sphere(WSimdVec4f(2, 2, 2), 2);
    b2 = WSimdBBoxSphere(sphere);

    b1.ExpandToInclude(b2);
    W_TEST_BOOL(b1 != b2);

    W_TEST_BOOL((b1.m_CenterAndRadius == WSimdVec4f(2, 2, 2)).AllSet<3>());
    W_TEST_BOOL((b1.m_BoxHalfExtents == WSimdVec4f(2, 2, 2)).AllSet<3>());
    W_TEST_FLOAT(b1.m_CenterAndRadius.w(), WMath::Sqrt(3.0f) * 2.0f, 0.00001f);
    W_TEST_BOOL(b1.m_CenterAndRadius.w() <= b1.m_BoxHalfExtents.GetLength<3>());

    b1 = WSimdBBoxSphere::MakeInvalid();
    b2 = WSimdBBox(WSimdVec4f(0.25, 0.25, 0.25), WSimdVec4f(0.5, 0.5, 0.5));

    b1.ExpandToInclude(b2);
    W_TEST_BOOL(b1 == b2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Transform")
  {
    WSimdBBoxSphere b = WSimdBBoxSphere::MakeFromCenterExtents(WSimdVec4f(1), WSimdVec4f(5), 5);

    WSimdTransform t(WSimdVec4f(1, 1, 1), WSimdQuat::MakeIdentity(), WSimdVec4f(2, 3, -2));

    b.Transform(t);

    W_TEST_BOOL((b.m_CenterAndRadius == WSimdVec4f(3, 4, -1, 15)).AllSet<4>());
    W_TEST_BOOL((b.m_BoxHalfExtents == WSimdVec4f(10, 15, 10)).AllSet<3>());

    // verification
    WRandom rnd;
    rnd.Initialize(0x736454);

    WDynamicArray<WSimdVec4f, WAlignedAllocatorWrapper> points;
    points.SetCountUninitialized(10);
    float fSize = 10;

    for (WUInt32 i = 0; i < points.GetCount(); ++i)
    {
      float x = (float)rnd.DoubleMinMax(-fSize, fSize);
      float y = (float)rnd.DoubleMinMax(-fSize, fSize);
      float z = (float)rnd.DoubleMinMax(-fSize, fSize);
      points[i] = WSimdVec4f(x, y, z);
    }

    b = WSimdBBoxSphere::MakeFromPoints(points.GetData(), points.GetCount());

    t.m_Rotation = WSimdQuat::MakeFromAxisAndAngle(WSimdVec4f(0, 0, 1), WAngle::MakeFromDegree(-30));
    b.Transform(t);

    for (WUInt32 i = 0; i < points.GetCount(); ++i)
    {
      WSimdVec4f tp = t.TransformPosition(points[i]);

      WSimdFloat boxDist = b.GetBox().GetDistanceTo(tp);
      W_TEST_BOOL(boxDist < WMath::DefaultEpsilon<float>());

      WSimdFloat sphereDist = b.GetSphere().GetDistanceTo(tp);
      W_TEST_BOOL(sphereDist < WMath::DefaultEpsilon<float>());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Comparison")
  {
    WSimdBBoxSphere b1(WSimdBBox(WSimdVec4f(5, 0, 0), WSimdVec4f(1, 2, 3)));
    WSimdBBoxSphere b2(WSimdBBox(WSimdVec4f(6, 0, 0), WSimdVec4f(1, 2, 3)));

    W_TEST_BOOL(b1 == WSimdBBoxSphere(WSimdBBox(WSimdVec4f(5, 0, 0), WSimdVec4f(1, 2, 3))));
    W_TEST_BOOL(b1 != b2);
  }
}
