#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/BoundingBox.h>

template <typename Type>
void TestBoundingBox()
{
  using WBoundingBoxType = WBoundingBoxTemplate<Type>;
  using WBoundingSphereType = WBoundingSphereTemplate<Type>;
  using WVec3Type = WVec3Template<Type>;
  using WMat4Type = WMat4Template<Type>;

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeFromMinMax")
  {
    WBoundingBoxType b = WBoundingBoxType::MakeFromMinMax(WVec3Type(-1, -2, -3), WVec3Type(1, 2, 3));

    W_TEST_BOOL(b.m_vMin == WVec3Type(-1, -2, -3));
    W_TEST_BOOL(b.m_vMax == WVec3Type(1, 2, 3));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeFromMinMax")
  {
    WBoundingBoxType b = WBoundingBoxType::MakeFromMinMax(WVec3Type(-1, -2, -3), WVec3Type(1, 2, 3));

    W_TEST_BOOL(b.m_vMin == WVec3Type(-1, -2, -3));
    W_TEST_BOOL(b.m_vMax == WVec3Type(1, 2, 3));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeFromPoints")
  {
    WVec3Type p[6] = {
      WVec3Type(-4, 0, 0),
      WVec3Type(5, 0, 0),
      WVec3Type(0, -6, 0),
      WVec3Type(0, 7, 0),
      WVec3Type(0, 0, -8),
      WVec3Type(0, 0, 9),
    };

    WBoundingBoxType b = WBoundingBoxType::MakeFromPoints(p, 6);

    W_TEST_BOOL(b.m_vMin == WVec3Type(-4, -6, -8));
    W_TEST_BOOL(b.m_vMax == WVec3Type(5, 7, 9));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeInvalid")
  {
    WBoundingBoxType b;
    b = WBoundingBoxType::MakeInvalid();

    W_TEST_BOOL(!b.IsValid());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeFromCenterAndHalfExtents")
  {
    WBoundingBoxType b = WBoundingBoxType::MakeFromCenterAndHalfExtents(WVec3Type(1, 2, 3), WVec3Type(4, 5, 6));

    W_TEST_BOOL(b.m_vMin == WVec3Type(-3, -3, -3));
    W_TEST_BOOL(b.m_vMax == WVec3Type(5, 7, 9));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetCorners")
  {
    WBoundingBoxType b = WBoundingBoxType::MakeFromMinMax(WVec3Type(-1, -2, -3), WVec3Type(1, 2, 3));

    WVec3Type c[8];
    b.GetCorners(c);

    W_TEST_BOOL(c[0] == WVec3Type(-1, -2, -3));
    W_TEST_BOOL(c[1] == WVec3Type(-1, -2, 3));
    W_TEST_BOOL(c[2] == WVec3Type(-1, 2, -3));
    W_TEST_BOOL(c[3] == WVec3Type(-1, 2, 3));
    W_TEST_BOOL(c[4] == WVec3Type(1, -2, -3));
    W_TEST_BOOL(c[5] == WVec3Type(1, -2, 3));
    W_TEST_BOOL(c[6] == WVec3Type(1, 2, -3));
    W_TEST_BOOL(c[7] == WVec3Type(1, 2, 3));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ExpandToInclue (Point)")
  {
    WBoundingBoxType b;
    b = WBoundingBoxType::MakeInvalid();
    b.ExpandToInclude(WVec3Type(1, 2, 3));

    W_TEST_BOOL(b.m_vMin == WVec3Type(1, 2, 3));
    W_TEST_BOOL(b.m_vMax == WVec3Type(1, 2, 3));


    b.ExpandToInclude(WVec3Type(2, 3, 4));

    W_TEST_BOOL(b.m_vMin == WVec3Type(1, 2, 3));
    W_TEST_BOOL(b.m_vMax == WVec3Type(2, 3, 4));

    b.ExpandToInclude(WVec3Type(0, 1, 2));

    W_TEST_BOOL(b.m_vMin == WVec3Type(0, 1, 2));
    W_TEST_BOOL(b.m_vMax == WVec3Type(2, 3, 4));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ExpandToInclude (Box)")
  {
    WBoundingBoxType b1, b2;

    b1 = WBoundingBoxType::MakeFromMinMax(WVec3Type(-1, -2, -3), WVec3Type(1, 2, 3));
    b2 = WBoundingBoxType::MakeFromMinMax(WVec3Type(0), WVec3Type(4, 5, 6));

    b1.ExpandToInclude(b2);

    W_TEST_BOOL(b1.m_vMin == WVec3Type(-1, -2, -3));
    W_TEST_BOOL(b1.m_vMax == WVec3Type(4, 5, 6));

    b2 = WBoundingBoxType::MakeFromMinMax(WVec3Type(-4, -5, -6), WVec3Type(0));

    b1.ExpandToInclude(b2);

    W_TEST_BOOL(b1.m_vMin == WVec3Type(-4, -5, -6));
    W_TEST_BOOL(b1.m_vMax == WVec3Type(4, 5, 6));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ExpandToInclude (array)")
  {
    WVec3Type v[4] = {WVec3Type(1, 1, 1), WVec3Type(-1, -1, -1), WVec3Type(2, 2, 2), WVec3Type(4, 4, 4)};

    WBoundingBoxType b;
    b = WBoundingBoxType::MakeInvalid();
    b.ExpandToInclude(v, 2, sizeof(WVec3Type) * 2);

    W_TEST_BOOL(b.m_vMin == WVec3Type(1, 1, 1));
    W_TEST_BOOL(b.m_vMax == WVec3Type(2, 2, 2));

    b.ExpandToInclude(v, 4, sizeof(WVec3Type));

    W_TEST_BOOL(b.m_vMin == WVec3Type(-1, -1, -1));
    W_TEST_BOOL(b.m_vMax == WVec3Type(4, 4, 4));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ExpandToCube")
  {
    WBoundingBoxType b = WBoundingBoxType::MakeFromCenterAndHalfExtents(WVec3Type(1, 2, 3), WVec3Type(4, 5, 6));

    b.ExpandToCube();

    W_TEST_VEC3(b.GetCenter(), WVec3Type(1, 2, 3), WMath::DefaultEpsilon<Type>());
    W_TEST_VEC3(b.GetHalfExtents(), WVec3Type(6, 6, 6), WMath::DefaultEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Grow")
  {
    WBoundingBoxType b = WBoundingBoxType::MakeFromMinMax(WVec3Type(1, 2, 3), WVec3Type(4, 5, 6));
    b.Grow(WVec3Type(2, 4, 6));

    W_TEST_BOOL(b.m_vMin == WVec3Type(-1, -2, -3));
    W_TEST_BOOL(b.m_vMax == WVec3Type(6, 9, 12));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Contains (Point)")
  {
    WBoundingBoxType b = WBoundingBoxType::MakeFromMinMax(WVec3Type(0), WVec3Type(0));

    W_TEST_BOOL(b.Contains(WVec3Type(0)));
    W_TEST_BOOL(!b.Contains(WVec3Type(1, 0, 0)));
    W_TEST_BOOL(!b.Contains(WVec3Type(-1, 0, 0)));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Contains (Box)")
  {
    WBoundingBoxType b1 = WBoundingBoxType::MakeFromMinMax(WVec3Type(-3), WVec3Type(3));
    WBoundingBoxType b2 = WBoundingBoxType::MakeFromMinMax(WVec3Type(-1), WVec3Type(1));
    WBoundingBoxType b3 = WBoundingBoxType::MakeFromMinMax(WVec3Type(-1), WVec3Type(4));

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

  W_TEST_BLOCK(WTestBlock::Enabled, "Contains (Array)")
  {
    WBoundingBoxType b = WBoundingBoxType::MakeFromMinMax(WVec3Type(1), WVec3Type(5));

    WVec3Type v[4] = {WVec3Type(0), WVec3Type(1), WVec3Type(5), WVec3Type(6)};

    W_TEST_BOOL(!b.Contains(&v[0], 4, sizeof(WVec3Type)));
    W_TEST_BOOL(b.Contains(&v[1], 2, sizeof(WVec3Type)));
    W_TEST_BOOL(b.Contains(&v[2], 1, sizeof(WVec3Type)));

    W_TEST_BOOL(!b.Contains(&v[1], 2, sizeof(WVec3Type) * 2));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Contains (Sphere)")
  {
    WBoundingBoxType b = WBoundingBoxType::MakeFromMinMax(WVec3Type(1), WVec3Type(5));

    W_TEST_BOOL(b.Contains(WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(3), 2)));
    W_TEST_BOOL(!b.Contains(WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(3), Type(2.1))));
    W_TEST_BOOL(!b.Contains(WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(8), 2)));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Overlaps (box)")
  {
    WBoundingBoxType b1 = WBoundingBoxType::MakeFromMinMax(WVec3Type(-3), WVec3Type(3));
    WBoundingBoxType b2 = WBoundingBoxType::MakeFromMinMax(WVec3Type(-1), WVec3Type(1));
    WBoundingBoxType b3 = WBoundingBoxType::MakeFromMinMax(WVec3Type(1), WVec3Type(4));
    WBoundingBoxType b4 = WBoundingBoxType::MakeFromMinMax(WVec3Type(-4, 1, 1), WVec3Type(4, 2, 2));

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

  W_TEST_BLOCK(WTestBlock::Enabled, "Overlaps (Array)")
  {
    WBoundingBoxType b = WBoundingBoxType::MakeFromMinMax(WVec3Type(1), WVec3Type(5));

    WVec3Type v[4] = {WVec3Type(0), WVec3Type(1), WVec3Type(5), WVec3Type(6)};

    W_TEST_BOOL(!b.Overlaps(&v[0], 1, sizeof(WVec3Type)));
    W_TEST_BOOL(!b.Overlaps(&v[3], 1, sizeof(WVec3Type)));

    W_TEST_BOOL(b.Overlaps(&v[0], 4, sizeof(WVec3Type)));
    W_TEST_BOOL(b.Overlaps(&v[1], 2, sizeof(WVec3Type)));
    W_TEST_BOOL(b.Overlaps(&v[2], 1, sizeof(WVec3Type)));

    W_TEST_BOOL(b.Overlaps(&v[1], 2, sizeof(WVec3Type) * 2));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Overlaps (Sphere)")
  {
    WBoundingBoxType b = WBoundingBoxType::MakeFromMinMax(WVec3Type(1), WVec3Type(5));

    W_TEST_BOOL(b.Overlaps(WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(3), 2)));
    W_TEST_BOOL(b.Overlaps(WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(3), Type(2.1))));
    W_TEST_BOOL(!b.Overlaps(WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(8), 2)));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsIdentical, ==, !=")
  {
    WBoundingBoxType b1, b2, b3;

    b1 = WBoundingBoxType::MakeFromMinMax(WVec3Type(1), WVec3Type(2));
    b2 = WBoundingBoxType::MakeFromMinMax(WVec3Type(1), WVec3Type(2));
    b3 = WBoundingBoxType::MakeFromMinMax(WVec3Type(1), WVec3Type(Type(2.01)));

    W_TEST_BOOL(b1.IsIdentical(b1));
    W_TEST_BOOL(b2.IsIdentical(b2));
    W_TEST_BOOL(b3.IsIdentical(b3));

    W_TEST_BOOL(b1 == b1);
    W_TEST_BOOL(b2 == b2);
    W_TEST_BOOL(b3 == b3);

    W_TEST_BOOL(b1.IsIdentical(b2));
    W_TEST_BOOL(b2.IsIdentical(b1));

    W_TEST_BOOL(!b1.IsIdentical(b3));
    W_TEST_BOOL(!b2.IsIdentical(b3));
    W_TEST_BOOL(!b3.IsIdentical(b1));
    W_TEST_BOOL(!b3.IsIdentical(b1));

    W_TEST_BOOL(b1 == b2);
    W_TEST_BOOL(b2 == b1);

    W_TEST_BOOL(b1 != b3);
    W_TEST_BOOL(b2 != b3);
    W_TEST_BOOL(b3 != b1);
    W_TEST_BOOL(b3 != b1);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsEqual")
  {
    WBoundingBoxType b1, b2;
    b1 = WBoundingBoxType::MakeFromMinMax(WVec3Type(-1), WVec3Type(1));
    b2 = WBoundingBoxType::MakeFromMinMax(WVec3Type(-1), WVec3Type(2));

    W_TEST_BOOL(!b1.IsEqual(b2));
    W_TEST_BOOL(!b1.IsEqual(b2, Type(0.5)));
    W_TEST_BOOL(b1.IsEqual(b2, 1));
    W_TEST_BOOL(b1.IsEqual(b2, 2));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetCenter")
  {
    WBoundingBoxType b = WBoundingBoxType::MakeFromMinMax(WVec3Type(3), WVec3Type(7));

    W_TEST_BOOL(b.GetCenter() == WVec3Type(5));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetExtents")
  {
    WBoundingBoxType b = WBoundingBoxType::MakeFromMinMax(WVec3Type(3), WVec3Type(7));

    W_TEST_BOOL(b.GetExtents() == WVec3Type(4));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetHalfExtents")
  {
    WBoundingBoxType b = WBoundingBoxType::MakeFromMinMax(WVec3Type(3), WVec3Type(7));

    W_TEST_BOOL(b.GetHalfExtents() == WVec3Type(2));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Translate")
  {
    WBoundingBoxType b = WBoundingBoxType::MakeFromMinMax(WVec3Type(3), WVec3Type(5));

    b.Translate(WVec3Type(1, 2, 3));

    W_TEST_BOOL(b.m_vMin == WVec3Type(4, 5, 6));
    W_TEST_BOOL(b.m_vMax == WVec3Type(6, 7, 8));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ScaleFromCenter")
  {
    {
      WBoundingBoxType b = WBoundingBoxType::MakeFromMinMax(WVec3Type(3), WVec3Type(5));

      b.ScaleFromCenter(WVec3Type(1, 2, 3));

      W_TEST_BOOL(b.m_vMin == WVec3Type(3, 2, 1));
      W_TEST_BOOL(b.m_vMax == WVec3Type(5, 6, 7));
    }
    {
      WBoundingBoxType b = WBoundingBoxType::MakeFromMinMax(WVec3Type(3), WVec3Type(5));

      b.ScaleFromCenter(WVec3Type(-1, -2, -3));

      W_TEST_BOOL(b.m_vMin == WVec3Type(3, 2, 1));
      W_TEST_BOOL(b.m_vMax == WVec3Type(5, 6, 7));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ScaleFromOrigin")
  {
    {
      WBoundingBoxType b = WBoundingBoxType::MakeFromMinMax(WVec3Type(3), WVec3Type(5));

      b.ScaleFromOrigin(WVec3Type(1, 2, 3));

      W_TEST_BOOL(b.m_vMin == WVec3Type(3, 6, 9));
      W_TEST_BOOL(b.m_vMax == WVec3Type(5, 10, 15));
    }
    {
      WBoundingBoxType b = WBoundingBoxType::MakeFromMinMax(WVec3Type(3), WVec3Type(5));

      b.ScaleFromOrigin(WVec3Type(-1, -2, -3));

      W_TEST_BOOL(b.m_vMin == WVec3Type(-5, -10, -15));
      W_TEST_BOOL(b.m_vMax == WVec3Type(-3, -6, -9));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "TransformFromOrigin")
  {
    WBoundingBoxType b = WBoundingBoxType::MakeFromMinMax(WVec3Type(3), WVec3Type(5));

    WMat4Type m = WMat4Type::MakeScaling(WVec3Type(2));

    b.TransformFromOrigin(m);

    W_TEST_BOOL(b.m_vMin == WVec3Type(6, 6, 6));
    W_TEST_BOOL(b.m_vMax == WVec3Type(10, 10, 10));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "TransformFromCenter")
  {
    WBoundingBoxType b = WBoundingBoxType::MakeFromMinMax(WVec3Type(3), WVec3Type(5));

    WMat4Type m = WMat4Type::MakeScaling(WVec3Type(2));

    b.TransformFromCenter(m);

    W_TEST_BOOL(b.m_vMin == WVec3Type(2, 2, 2));
    W_TEST_BOOL(b.m_vMax == WVec3Type(6, 6, 6));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetClampedPoint")
  {
    WBoundingBoxType b = WBoundingBoxType::MakeFromMinMax(WVec3Type(-1, -2, -3), WVec3Type(1, 2, 3));

    W_TEST_BOOL(b.GetClampedPoint(WVec3Type(-2, 0, 0)) == WVec3Type(-1, 0, 0));
    W_TEST_BOOL(b.GetClampedPoint(WVec3Type(2, 0, 0)) == WVec3Type(1, 0, 0));

    W_TEST_BOOL(b.GetClampedPoint(WVec3Type(0, -3, 0)) == WVec3Type(0, -2, 0));
    W_TEST_BOOL(b.GetClampedPoint(WVec3Type(0, 3, 0)) == WVec3Type(0, 2, 0));

    W_TEST_BOOL(b.GetClampedPoint(WVec3Type(0, 0, -4)) == WVec3Type(0, 0, -3));
    W_TEST_BOOL(b.GetClampedPoint(WVec3Type(0, 0, 4)) == WVec3Type(0, 0, 3));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetDistanceTo (point)")
  {
    WBoundingBoxType b = WBoundingBoxType::MakeFromMinMax(WVec3Type(-1, -2, -3), WVec3Type(1, 2, 3));

    W_TEST_BOOL(b.GetDistanceTo(WVec3Type(-2, 0, 0)) == 1);
    W_TEST_BOOL(b.GetDistanceTo(WVec3Type(2, 0, 0)) == 1);

    W_TEST_BOOL(b.GetDistanceTo(WVec3Type(0, -4, 0)) == 2);
    W_TEST_BOOL(b.GetDistanceTo(WVec3Type(0, 4, 0)) == 2);

    W_TEST_BOOL(b.GetDistanceTo(WVec3Type(0, 0, -6)) == 3);
    W_TEST_BOOL(b.GetDistanceTo(WVec3Type(0, 0, 6)) == 3);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetDistanceTo (Sphere)")
  {
    WBoundingBoxType b = WBoundingBoxType::MakeFromMinMax(WVec3Type(1), WVec3Type(5));

    W_TEST_BOOL(b.GetDistanceTo(WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(3), 2)) < 0);
    W_TEST_BOOL(b.GetDistanceTo(WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(5), 1)) < 0);
    W_TEST_FLOAT(b.GetDistanceTo(WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(8, 2, 2), 2)), 1, Type(0.001));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetDistanceTo (box)")
  {
    WBoundingBoxType b = WBoundingBoxType::MakeFromMinMax(WVec3Type(1), WVec3Type(5));

    WBoundingBoxType b1, b2, b3;
    b1 = WBoundingBoxType::MakeFromCenterAndHalfExtents(WVec3Type(3), WVec3Type(2));
    b2 = WBoundingBoxType::MakeFromCenterAndHalfExtents(WVec3Type(5), WVec3Type(1));
    b3 = WBoundingBoxType::MakeFromCenterAndHalfExtents(WVec3Type(9, 2, 2), WVec3Type(2));

    auto test1 = b.GetDistanceTo(b1);
    auto test2 = b.GetDistanceTo(b2);
    W_TEST_BOOL(b.GetDistanceTo(b1) <= 0);
    W_TEST_BOOL(b.GetDistanceTo(b2) <= 0);
    W_TEST_FLOAT(b.GetDistanceTo(b3), 2, Type(0.001));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetDistanceSquaredTo (point)")
  {
    WBoundingBoxType b = WBoundingBoxType::MakeFromMinMax(WVec3Type(-1, -2, -3), WVec3Type(1, 2, 3));

    W_TEST_BOOL(b.GetDistanceSquaredTo(WVec3Type(-2, 0, 0)) == 1);
    W_TEST_BOOL(b.GetDistanceSquaredTo(WVec3Type(2, 0, 0)) == 1);

    W_TEST_BOOL(b.GetDistanceSquaredTo(WVec3Type(0, -4, 0)) == 4);
    W_TEST_BOOL(b.GetDistanceSquaredTo(WVec3Type(0, 4, 0)) == 4);

    W_TEST_BOOL(b.GetDistanceSquaredTo(WVec3Type(0, 0, -6)) == 9);
    W_TEST_BOOL(b.GetDistanceSquaredTo(WVec3Type(0, 0, 6)) == 9);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetDistanceSquaredTo (box)")
  {
    WBoundingBoxType b = WBoundingBoxType::MakeFromMinMax(WVec3Type(1), WVec3Type(5));

    WBoundingBoxType b1, b2, b3;
    b1 = WBoundingBoxType::MakeFromCenterAndHalfExtents(WVec3Type(3), WVec3Type(2));
    b2 = WBoundingBoxType::MakeFromCenterAndHalfExtents(WVec3Type(5), WVec3Type(1));
    b3 = WBoundingBoxType::MakeFromCenterAndHalfExtents(WVec3Type(9, 2, 2), WVec3Type(2));

    W_TEST_BOOL(b.GetDistanceSquaredTo(b1) <= 0);
    W_TEST_BOOL(b.GetDistanceSquaredTo(b2) <= 0);
    W_TEST_FLOAT(b.GetDistanceSquaredTo(b3), 4, Type(0.001));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetBoundingSphere")
  {
    WBoundingBoxType b;
    b = WBoundingBoxType::MakeFromCenterAndHalfExtents(WVec3Type(5, 4, 2), WVec3Type(3));

    WBoundingSphereType s = b.GetBoundingSphere();

    W_TEST_BOOL(s.m_vCenter == WVec3Type(5, 4, 2));
    W_TEST_FLOAT(s.m_fRadius, WVec3Type(3).GetLength(), Type(0.001));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetRayIntersection")
  {
    if (WMath::SupportsInfinity<Type>())
    {
      const WVec3Type c = WVec3Type(10);

      WBoundingBoxType b;
      b = WBoundingBoxType::MakeFromCenterAndHalfExtents(c, WVec3Type(2, 4, 8));

      for (Type x = b.m_vMin.x - Type(1); x < b.m_vMax.x + Type(1); x += Type(0.2))
      {
        for (Type y = b.m_vMin.y - Type(1); y < b.m_vMax.y + Type(1); y += Type(0.2))
        {
          for (Type z = b.m_vMin.z - Type(1); z < b.m_vMax.z + Type(1); z += Type(0.2))
          {
            const WVec3Type v(x, y, z);

            if (b.Contains(v))
              continue;

            const WVec3Type vTarget = b.GetClampedPoint(v);

            const WVec3Type vDir = (vTarget - c).GetNormalized();

            const WVec3Type vSource = vTarget + vDir * Type(3);

            Type f;
            WVec3Type vi;
            W_TEST_BOOL(b.GetRayIntersection(vSource, -vDir, &f, &vi) == true);
            W_TEST_FLOAT(f, 3, Type(0.001));
            W_TEST_BOOL(vi.IsEqual(vTarget, Type(0.0001)));

            W_TEST_BOOL(b.GetRayIntersection(vSource, vDir, &f, &vi) == false);
            W_TEST_BOOL(b.GetRayIntersection(vTarget, vDir, &f, &vi) == false);
          }
        }
      }
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetLineSegmentIntersection")
  {
    if (WMath::SupportsInfinity<Type>())
    {
      const WVec3Type c = WVec3Type(10);

      WBoundingBoxType b;
      b = WBoundingBoxType::MakeFromCenterAndHalfExtents(c, WVec3Type(2, 4, 8));

      for (Type x = b.m_vMin.x - Type(1); x < b.m_vMax.x + Type(1); x += Type(0.2))
      {
        for (Type y = b.m_vMin.y - Type(1); y < b.m_vMax.y + Type(1); y += Type(0.2))
        {
          for (Type z = b.m_vMin.z - Type(1); z < b.m_vMax.z + Type(1); z += Type(0.2))
          {
            const WVec3Type v(x, y, z);

            if (b.Contains(v))
              continue;

            const WVec3Type vTarget0 = b.GetClampedPoint(v);

            const WVec3Type vDir = (vTarget0 - c).GetNormalized();

            const WVec3Type vTarget = vTarget0 - vDir * Type(1);
            const WVec3Type vSource = vTarget0 + vDir * Type(3);

            Type f;
            WVec3Type vi;
            W_TEST_BOOL(b.GetLineSegmentIntersection(vSource, vTarget, &f, &vi) == true);
            W_TEST_FLOAT(f, Type(0.75), Type(0.001));
            W_TEST_BOOL(vi.IsEqual(vTarget0, Type(0.0001)));
          }
        }
      }
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsNaN")
  {
    if (WMath::SupportsNaN<Type>())
    {
      WBoundingBoxType b;

      b = WBoundingBoxType::MakeInvalid();
      W_TEST_BOOL(!b.IsNaN());

      b = WBoundingBoxType::MakeInvalid();
      b.m_vMin.x = WMath::NaN<Type>();
      W_TEST_BOOL(b.IsNaN());

      b = WBoundingBoxType::MakeInvalid();
      b.m_vMin.y = WMath::NaN<Type>();
      W_TEST_BOOL(b.IsNaN());

      b = WBoundingBoxType::MakeInvalid();
      b.m_vMin.z = WMath::NaN<Type>();
      W_TEST_BOOL(b.IsNaN());

      b = WBoundingBoxType::MakeInvalid();
      b.m_vMax.x = WMath::NaN<Type>();
      W_TEST_BOOL(b.IsNaN());

      b = WBoundingBoxType::MakeInvalid();
      b.m_vMax.y = WMath::NaN<Type>();
      W_TEST_BOOL(b.IsNaN());

      b = WBoundingBoxType::MakeInvalid();
      b.m_vMax.z = WMath::NaN<Type>();
      W_TEST_BOOL(b.IsNaN());
    }
  }
}

W_CREATE_SIMPLE_TEST(Math, BoundingBoxf)
{
  TestBoundingBox<float>();
}

W_CREATE_SIMPLE_TEST(Math, BoundingBoxd)
{
  TestBoundingBox<double>();
}
