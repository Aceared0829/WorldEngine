#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/SimdMath/SimdBSphere.h>

W_CREATE_SIMPLE_TEST(SimdMath, SimdBSphere)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "MakeFromCenterAndRadius")
  {
    WSimdBSphere s = WSimdBSphere::MakeFromCenterAndRadius(WSimdVec4f(1, 2, 3), 4);

    W_TEST_BOOL((s.m_CenterAndRadius == WSimdVec4f(1, 2, 3, 4)).AllSet());

    W_TEST_BOOL((s.GetCenter() == WSimdVec4f(1, 2, 3)).AllSet<3>());
    W_TEST_BOOL(s.GetRadius() == 4.0f);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeInvalid / IsValid")
  {
    WSimdBSphere s(WSimdVec4f(1, 2, 3), 4);

    W_TEST_BOOL(s.IsValid());

    s = WSimdBSphere::MakeInvalid();

    W_TEST_BOOL(!s.IsValid());
    W_TEST_BOOL(!s.IsNaN());

    s = WSimdBSphere(WSimdVec4f(1, 2, 3), WMath::NaN<float>());
    W_TEST_BOOL(s.IsNaN());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ExpandToInclude(Point)")
  {
    WSimdBSphere s(WSimdVec4f::MakeZero(), 0.0f);

    s.ExpandToInclude(WSimdVec4f(3, 0, 0));

    W_TEST_BOOL((s.m_CenterAndRadius == WSimdVec4f(0, 0, 0, 3)).AllSet());

    s = WSimdBSphere::MakeInvalid();

    s.ExpandToInclude(WSimdVec4f(0.25, 0, 0));

    W_TEST_BOOL((s.m_CenterAndRadius == WSimdVec4f(0, 0, 0, 0.25)).AllSet());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ExpandToInclude(array)")
  {
    WSimdBSphere s(WSimdVec4f(2, 2, 0), 0.0f);

    WSimdVec4f p[4] = {WSimdVec4f(0, 2, 0), WSimdVec4f(4, 2, 0), WSimdVec4f(2, 0, 0), WSimdVec4f(2, 4, 0)};

    s.ExpandToInclude(p, 4);

    W_TEST_BOOL((s.m_CenterAndRadius == WSimdVec4f(2, 2, 0, 2)).AllSet());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ExpandToInclude (sphere)")
  {
    WSimdBSphere s1(WSimdVec4f(5, 0, 0), 1);
    WSimdBSphere s2(WSimdVec4f(6, 0, 0), 1);
    WSimdBSphere s3(WSimdVec4f(5, 0, 0), 2);

    s1.ExpandToInclude(s2);
    W_TEST_BOOL((s1.m_CenterAndRadius == WSimdVec4f(5, 0, 0, 2)).AllSet());

    s1.ExpandToInclude(s3);
    W_TEST_BOOL((s1.m_CenterAndRadius == WSimdVec4f(5, 0, 0, 2)).AllSet());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Transform")
  {
    WSimdBSphere s(WSimdVec4f(5, 0, 0), 2);

    WSimdTransform t(WSimdVec4f(4, 5, 6));
    t.m_Rotation = WSimdQuat::MakeFromAxisAndAngle(WSimdVec4f(0, 0, 1), WAngle::MakeFromDegree(90));
    t.m_Scale = WSimdVec4f(1, -2, -4);

    s.Transform(t);
    W_TEST_BOOL(s.m_CenterAndRadius.IsEqual(WSimdVec4f(4, 10, 6, 8), WSimdFloat(WMath::SmallEpsilon<float>())).AllSet());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetDistanceTo (point)")
  {
    WSimdBSphere s(WSimdVec4f(5, 0, 0), 2);

    W_TEST_BOOL(s.GetDistanceTo(WSimdVec4f(5, 0, 0)) == -2.0f);
    W_TEST_BOOL(s.GetDistanceTo(WSimdVec4f(7, 0, 0)) == 0.0f);
    W_TEST_BOOL(s.GetDistanceTo(WSimdVec4f(9, 0, 0)) == 2.0f);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetDistanceTo (sphere)")
  {
    WSimdBSphere s1(WSimdVec4f(5, 0, 0), 2);
    WSimdBSphere s2(WSimdVec4f(10, 0, 0), 3);
    WSimdBSphere s3(WSimdVec4f(10, 0, 0), 1);

    W_TEST_BOOL(s1.GetDistanceTo(s2) == 0.0f);
    W_TEST_BOOL(s1.GetDistanceTo(s3) == 2.0f);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Contains (point)")
  {
    WSimdBSphere s(WSimdVec4f(5, 0, 0), 2.0f);

    W_TEST_BOOL(s.Contains(WSimdVec4f(3, 0, 0)));
    W_TEST_BOOL(s.Contains(WSimdVec4f(5, 0, 0)));
    W_TEST_BOOL(s.Contains(WSimdVec4f(6, 0, 0)));
    W_TEST_BOOL(s.Contains(WSimdVec4f(7, 0, 0)));

    W_TEST_BOOL(!s.Contains(WSimdVec4f(2, 0, 0)));
    W_TEST_BOOL(!s.Contains(WSimdVec4f(8, 0, 0)));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Contains (sphere)")
  {
    WSimdBSphere s1(WSimdVec4f(5, 0, 0), 2);
    WSimdBSphere s2(WSimdVec4f(6, 0, 0), 1);
    WSimdBSphere s3(WSimdVec4f(6, 0, 0), 2);

    W_TEST_BOOL(s1.Contains(s1));
    W_TEST_BOOL(s2.Contains(s2));
    W_TEST_BOOL(s3.Contains(s3));

    W_TEST_BOOL(s1.Contains(s2));
    W_TEST_BOOL(!s1.Contains(s3));

    W_TEST_BOOL(!s2.Contains(s3));
    W_TEST_BOOL(s3.Contains(s2));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Overlaps (sphere)")
  {
    WSimdBSphere s1(WSimdVec4f(5, 0, 0), 2);
    WSimdBSphere s2(WSimdVec4f(6, 0, 0), 2);
    WSimdBSphere s3(WSimdVec4f(8, 0, 0), 1);

    W_TEST_BOOL(s1.Overlaps(s1));
    W_TEST_BOOL(s2.Overlaps(s2));
    W_TEST_BOOL(s3.Overlaps(s3));

    W_TEST_BOOL(s1.Overlaps(s2));
    W_TEST_BOOL(!s1.Overlaps(s3));

    W_TEST_BOOL(s2.Overlaps(s3));
    W_TEST_BOOL(s3.Overlaps(s2));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetClampedPoint")
  {
    WSimdBSphere s(WSimdVec4f(1, 2, 3), 2.0f);

    W_TEST_BOOL(s.GetClampedPoint(WSimdVec4f(2, 2, 3)).IsEqual(WSimdVec4f(2, 2, 3), 0.001f).AllSet<3>());
    W_TEST_BOOL(s.GetClampedPoint(WSimdVec4f(5, 2, 3)).IsEqual(WSimdVec4f(3, 2, 3), 0.001f).AllSet<3>());
    W_TEST_BOOL(s.GetClampedPoint(WSimdVec4f(1, 7, 3)).IsEqual(WSimdVec4f(1, 4, 3), 0.001f).AllSet<3>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Comparison")
  {
    WSimdBSphere s1(WSimdVec4f(5, 0, 0), 2);
    WSimdBSphere s2(WSimdVec4f(6, 0, 0), 1);

    W_TEST_BOOL(s1 == WSimdBSphere(WSimdVec4f(5, 0, 0), 2));
    W_TEST_BOOL(s1 != s2);
  }
}
