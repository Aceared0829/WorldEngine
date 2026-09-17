#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/SimdMath/SimdBBox.h>
#include <Foundation/SimdMath/SimdConversion.h>

#define W_TEST_SIMD_VECTOR_EQUAL(NUM_COMPONENTS, A, B, EPSILON)                                                                                                        \
  do                                                                                                                                                                    \
  {                                                                                                                                                                     \
    auto _WDiff = B - A;                                                                                                                                               \
    WTestBool((A).IsEqual((B), EPSILON).AllSet<NUM_COMPONENTS>(), "Test failed: " W_PP_STRINGIFY(A) ".IsEqual(" W_PP_STRINGIFY(B) ", " W_PP_STRINGIFY(EPSILON) ")", \
      W_SOURCE_FILE, W_SOURCE_LINE, W_SOURCE_FUNCTION,                                                                                                               \
      "Difference %lf %lf %lf %lf", _WDiff.x(), _WDiff.y(), _WDiff.z(), _WDiff.w());                                                                                \
  } while (false)


W_CREATE_SIMPLE_TEST(SimdMath, SimdBBox)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor")
  {
    WSimdBBox b(WSimdVec4f(-1, -2, -3), WSimdVec4f(1, 2, 3));

    W_TEST_BOOL((b.m_Min == WSimdVec4f(-1, -2, -3)).AllSet<3>());
    W_TEST_BOOL((b.m_Max == WSimdVec4f(1, 2, 3)).AllSet<3>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeInvalid")
  {
    WSimdBBox b = WSimdBBox::MakeInvalid();

    W_TEST_BOOL(!b.IsValid());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsNaN")
  {
    WSimdBBox b = WSimdBBox::MakeInvalid();

    b = WSimdBBox::MakeInvalid();
    W_TEST_BOOL(!b.IsNaN());

    b = WSimdBBox::MakeInvalid();
    b.m_Min.SetX(WMath::NaN<WMathTestType>());
    W_TEST_BOOL(b.IsNaN());

    b = WSimdBBox::MakeInvalid();
    b.m_Min.SetY(WMath::NaN<WMathTestType>());
    W_TEST_BOOL(b.IsNaN());

    b = WSimdBBox::MakeInvalid();
    b.m_Min.SetZ(WMath::NaN<WMathTestType>());
    W_TEST_BOOL(b.IsNaN());

    b = WSimdBBox::MakeInvalid();
    b.m_Max.SetX(WMath::NaN<WMathTestType>());
    W_TEST_BOOL(b.IsNaN());

    b = WSimdBBox::MakeInvalid();
    b.m_Max.SetY(WMath::NaN<WMathTestType>());
    W_TEST_BOOL(b.IsNaN());

    b = WSimdBBox::MakeInvalid();
    b.m_Max.SetZ(WMath::NaN<WMathTestType>());
    W_TEST_BOOL(b.IsNaN());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeFromCenterAndHalfExtents")
  {
    const WSimdBBox b = WSimdBBox::MakeFromCenterAndHalfExtents(WSimdVec4f(1, 2, 3), WSimdVec4f(4, 5, 6));

    W_TEST_BOOL((b.m_Min == WSimdVec4f(-3, -3, -3)).AllSet<3>());
    W_TEST_BOOL((b.m_Max == WSimdVec4f(5, 7, 9)).AllSet<3>());

    W_TEST_BOOL((b.GetCenter() == WSimdVec4f(1, 2, 3)).AllSet<3>());
    W_TEST_BOOL((b.GetExtents() == WSimdVec4f(8, 10, 12)).AllSet<3>());
    W_TEST_BOOL((b.GetHalfExtents() == WSimdVec4f(4, 5, 6)).AllSet<3>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeFromPoints")
  {
    WSimdVec4f p[6] = {
      WSimdVec4f(-4, 0, 0),
      WSimdVec4f(5, 0, 0),
      WSimdVec4f(0, -6, 0),
      WSimdVec4f(0, 7, 0),
      WSimdVec4f(0, 0, -8),
      WSimdVec4f(0, 0, 9),
    };

    const WSimdBBox b = WSimdBBox::MakeFromPoints(p, 6);

    W_TEST_BOOL((b.m_Min == WSimdVec4f(-4, -6, -8)).AllSet<3>());
    W_TEST_BOOL((b.m_Max == WSimdVec4f(5, 7, 9)).AllSet<3>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ExpandToInclude (Point)")
  {
    WSimdBBox b = WSimdBBox::MakeInvalid();
    b.ExpandToInclude(WSimdVec4f(1, 2, 3));

    W_TEST_BOOL((b.m_Min == WSimdVec4f(1, 2, 3)).AllSet<3>());
    W_TEST_BOOL((b.m_Max == WSimdVec4f(1, 2, 3)).AllSet<3>());


    b.ExpandToInclude(WSimdVec4f(2, 3, 4));

    W_TEST_BOOL((b.m_Min == WSimdVec4f(1, 2, 3)).AllSet<3>());
    W_TEST_BOOL((b.m_Max == WSimdVec4f(2, 3, 4)).AllSet<3>());

    b.ExpandToInclude(WSimdVec4f(0, 1, 2));

    W_TEST_BOOL((b.m_Min == WSimdVec4f(0, 1, 2)).AllSet<3>());
    W_TEST_BOOL((b.m_Max == WSimdVec4f(2, 3, 4)).AllSet<3>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ExpandToInclude (array)")
  {
    WSimdVec4f v[4] = {WSimdVec4f(1, 1, 1), WSimdVec4f(-1, -1, -1), WSimdVec4f(2, 2, 2), WSimdVec4f(4, 4, 4)};

    WSimdBBox b = WSimdBBox::MakeInvalid();
    b.ExpandToInclude(v, 2, sizeof(WSimdVec4f) * 2);

    W_TEST_BOOL((b.m_Min == WSimdVec4f(1, 1, 1)).AllSet<3>());
    W_TEST_BOOL((b.m_Max == WSimdVec4f(2, 2, 2)).AllSet<3>());

    b.ExpandToInclude(v, 4);

    W_TEST_BOOL((b.m_Min == WSimdVec4f(-1, -1, -1)).AllSet<3>());
    W_TEST_BOOL((b.m_Max == WSimdVec4f(4, 4, 4)).AllSet<3>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ExpandToInclude (Box)")
  {
    WSimdBBox b1(WSimdVec4f(-1, -2, -3), WSimdVec4f(1, 2, 3));
    WSimdBBox b2(WSimdVec4f(0), WSimdVec4f(4, 5, 6));

    b1.ExpandToInclude(b2);

    W_TEST_BOOL((b1.m_Min == WSimdVec4f(-1, -2, -3)).AllSet<3>());
    W_TEST_BOOL((b1.m_Max == WSimdVec4f(4, 5, 6)).AllSet<3>());

    WSimdBBox b3 = WSimdBBox::MakeInvalid();
    b3.ExpandToInclude(b1);
    W_TEST_BOOL(b3 == b1);

    b2.m_Min = WSimdVec4f(-4, -5, -6);
    b2.m_Max.SetZero();

    b1.ExpandToInclude(b2);

    W_TEST_BOOL((b1.m_Min == WSimdVec4f(-4, -5, -6)).AllSet<3>());
    W_TEST_BOOL((b1.m_Max == WSimdVec4f(4, 5, 6)).AllSet<3>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ExpandToCube")
  {
    WSimdBBox b = WSimdBBox::MakeFromCenterAndHalfExtents(WSimdVec4f(1, 2, 3), WSimdVec4f(4, 5, 6));

    b.ExpandToCube();

    W_TEST_BOOL((b.GetCenter() == WSimdVec4f(1, 2, 3)).AllSet<3>());
    W_TEST_BOOL((b.GetHalfExtents() == WSimdVec4f(6, 6, 6)).AllSet<3>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Contains (Point)")
  {
    WSimdBBox b(WSimdVec4f(0), WSimdVec4f(0));

    W_TEST_BOOL(b.Contains(WSimdVec4f(0)));
    W_TEST_BOOL(!b.Contains(WSimdVec4f(1, 0, 0)));
    W_TEST_BOOL(!b.Contains(WSimdVec4f(-1, 0, 0)));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Contains (Box)")
  {
    WSimdBBox b1(WSimdVec4f(-3), WSimdVec4f(3));
    WSimdBBox b2(WSimdVec4f(-1), WSimdVec4f(1));
    WSimdBBox b3(WSimdVec4f(-1), WSimdVec4f(4));

    W_TEST_BOOL(b1.Contains(b1));
    W_TEST_BOOL(b2.Contains(b2));
    W_TEST_BOOL(b3.Contains(b3));

    W_TEST_BOOL(b1.Contains(b2));
    W_TEST_BOOL(!b1.Contains(b3));

    W_TEST_BOOL(!b2.Contains(b1));
    W_TEST_BOOL(!b2.Contains(b3));

    W_TEST_BOOL(!b3.Contains(b1));
    W_TEST_BOOL(b3.Contains(b2));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Contains (Sphere)")
  {
    WSimdBBox b(WSimdVec4f(1), WSimdVec4f(5));

    W_TEST_BOOL(b.Contains(WSimdBSphere(WSimdVec4f(3), 2)));
    W_TEST_BOOL(!b.Contains(WSimdBSphere(WSimdVec4f(3), 2.1f)));
    W_TEST_BOOL(!b.Contains(WSimdBSphere(WSimdVec4f(8), 2)));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Overlaps (box)")
  {
    WSimdBBox b1(WSimdVec4f(-3), WSimdVec4f(3));
    WSimdBBox b2(WSimdVec4f(-1), WSimdVec4f(1));
    WSimdBBox b3(WSimdVec4f(1), WSimdVec4f(4));
    WSimdBBox b4(WSimdVec4f(-4, 1, 1), WSimdVec4f(4, 2, 2));

    W_TEST_BOOL(b1.Overlaps(b1));
    W_TEST_BOOL(b2.Overlaps(b2));
    W_TEST_BOOL(b3.Overlaps(b3));
    W_TEST_BOOL(b4.Overlaps(b4));

    W_TEST_BOOL(b1.Overlaps(b2));
    W_TEST_BOOL(b1.Overlaps(b3));
    W_TEST_BOOL(b1.Overlaps(b4));

    W_TEST_BOOL(!b2.Overlaps(b3));
    W_TEST_BOOL(!b2.Overlaps(b4));

    W_TEST_BOOL(b3.Overlaps(b4));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Overlaps (Sphere)")
  {
    WSimdBBox b(WSimdVec4f(1), WSimdVec4f(5));

    W_TEST_BOOL(b.Overlaps(WSimdBSphere(WSimdVec4f(3), 2)));
    W_TEST_BOOL(b.Overlaps(WSimdBSphere(WSimdVec4f(3), 2.1f)));
    W_TEST_BOOL(!b.Overlaps(WSimdBSphere(WSimdVec4f(8), 2)));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Grow")
  {
    WSimdBBox b(WSimdVec4f(1, 2, 3), WSimdVec4f(4, 5, 6));
    b.Grow(WSimdVec4f(2, 4, 6));

    W_TEST_BOOL((b.m_Min == WSimdVec4f(-1, -2, -3)).AllSet<3>());
    W_TEST_BOOL((b.m_Max == WSimdVec4f(6, 9, 12)).AllSet<3>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Transform")
  {
    WSimdBBox b(WSimdVec4f(3), WSimdVec4f(5));

    WSimdTransform t(WSimdVec4f(4, 5, 6));
    t.m_Rotation = WSimdQuat::MakeFromAxisAndAngle(WSimdVec4f(0, 0, 1), WAngle::MakeFromDegree(90));
    t.m_Scale = WSimdVec4f(1, -2, -4);

    b.Transform(t);

    W_TEST_SIMD_VECTOR_EQUAL(3, b.m_Min, WSimdVec4f(10, 8, -14), 0.00001f);
    W_TEST_SIMD_VECTOR_EQUAL(3, b.m_Max, WSimdVec4f(14, 10, -6), 0.00001f);

    t.m_Rotation = WSimdQuat::MakeFromAxisAndAngle(WSimdVec4f(0, 0, 1), WAngle::MakeFromDegree(-30));

    b.m_Min = WSimdVec4f(3);
    b.m_Max = WSimdVec4f(5);
    b.Transform(t);

    // reference
    WBoundingBox referenceBox = WBoundingBoxT::MakeFromMinMax(WVec3(3), WVec3(5));
    {
      WQuat q = WQuat::MakeFromAxisAndAngle(WVec3(0, 0, 1), WAngle::MakeFromDegree(-30));

      WTransform referenceTransform(WVec3(4, 5, 6), q, WVec3(1, -2, -4));

      referenceBox.TransformFromOrigin(referenceTransform.GetAsMat4());
    }

    W_TEST_SIMD_VECTOR_EQUAL(3, b.m_Min, WSimdConversion::ToVec3(referenceBox.m_vMin), 0.00001f);
    W_TEST_SIMD_VECTOR_EQUAL(3, b.m_Max, WSimdConversion::ToVec3(referenceBox.m_vMax), 0.00001f);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetClampedPoint")
  {
    WSimdBBox b(WSimdVec4f(-1, -2, -3), WSimdVec4f(1, 2, 3));

    W_TEST_BOOL((b.GetClampedPoint(WSimdVec4f(-2, 0, 0)) == WSimdVec4f(-1, 0, 0)).AllSet<3>());
    W_TEST_BOOL((b.GetClampedPoint(WSimdVec4f(2, 0, 0)) == WSimdVec4f(1, 0, 0)).AllSet<3>());

    W_TEST_BOOL((b.GetClampedPoint(WSimdVec4f(0, -3, 0)) == WSimdVec4f(0, -2, 0)).AllSet<3>());
    W_TEST_BOOL((b.GetClampedPoint(WSimdVec4f(0, 3, 0)) == WSimdVec4f(0, 2, 0)).AllSet<3>());

    W_TEST_BOOL((b.GetClampedPoint(WSimdVec4f(0, 0, -4)) == WSimdVec4f(0, 0, -3)).AllSet<3>());
    W_TEST_BOOL((b.GetClampedPoint(WSimdVec4f(0, 0, 4)) == WSimdVec4f(0, 0, 3)).AllSet<3>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetDistanceSquaredTo (point)")
  {
    WSimdBBox b(WSimdVec4f(-1, -2, -3), WSimdVec4f(1, 2, 3));

    W_TEST_BOOL(b.GetDistanceSquaredTo(WSimdVec4f(-2, 0, 0)) == 1.0f);
    W_TEST_BOOL(b.GetDistanceSquaredTo(WSimdVec4f(2, 0, 0)) == 1.0f);

    W_TEST_BOOL(b.GetDistanceSquaredTo(WSimdVec4f(0, -4, 0)) == 4.0f);
    W_TEST_BOOL(b.GetDistanceSquaredTo(WSimdVec4f(0, 4, 0)) == 4.0f);

    W_TEST_BOOL(b.GetDistanceSquaredTo(WSimdVec4f(0, 0, -6)) == 9.0f);
    W_TEST_BOOL(b.GetDistanceSquaredTo(WSimdVec4f(0, 0, 6)) == 9.0f);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetDistanceTo (point)")
  {
    WSimdBBox b(WSimdVec4f(-1, -2, -3), WSimdVec4f(1, 2, 3));

    W_TEST_BOOL(b.GetDistanceTo(WSimdVec4f(-2, 0, 0)) == 1.0f);
    W_TEST_BOOL(b.GetDistanceTo(WSimdVec4f(2, 0, 0)) == 1.0f);

    W_TEST_BOOL(b.GetDistanceTo(WSimdVec4f(0, -4, 0)) == 2.0f);
    W_TEST_BOOL(b.GetDistanceTo(WSimdVec4f(0, 4, 0)) == 2.0f);

    W_TEST_BOOL(b.GetDistanceTo(WSimdVec4f(0, 0, -6)) == 3.0f);
    W_TEST_BOOL(b.GetDistanceTo(WSimdVec4f(0, 0, 6)) == 3.0f);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Comparison")
  {
    WSimdBBox b1(WSimdVec4f(5, 0, 0), WSimdVec4f(1, 2, 3));
    WSimdBBox b2(WSimdVec4f(6, 0, 0), WSimdVec4f(1, 2, 3));

    W_TEST_BOOL(b1 == WSimdBBox(WSimdVec4f(5, 0, 0), WSimdVec4f(1, 2, 3)));
    W_TEST_BOOL(b1 != b2);
  }
}
