#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/BoundingSphere.h>

template <typename Type>
void TestBoundingSphere()
{
  using WBoundingSphereType = WBoundingSphereTemplate<Type>;
  using WBoundingBoxType = WBoundingBoxTemplate<Type>;
  using WVec3Type = WVec3Template<Type>;
  using WMat4Type = WMat4Template<Type>;

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor")
  {
    WBoundingSphereType s = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(1, 2, 3), 4);

    W_TEST_BOOL(s.m_vCenter == WVec3Type(1, 2, 3));
    W_TEST_BOOL(s.m_fRadius == 4);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetInvalid / IsValid")
  {
    WBoundingSphereType s = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(1, 2, 3), 4);

    W_TEST_BOOL(s.IsValid());

    s = WBoundingSphereType::MakeInvalid();

    W_TEST_BOOL(!s.IsValid());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeZero / IsZero")
  {
    WBoundingSphereType s = WBoundingSphereType::MakeZero();

    W_TEST_BOOL(s.IsValid());
    W_TEST_BOOL(s.m_vCenter.IsZero());
    W_TEST_BOOL(s.m_fRadius == 0);
    W_TEST_BOOL(s.IsZero());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetElements")
  {
    WBoundingSphereType s = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(1, 2, 3), 4);

    W_TEST_BOOL(s.m_vCenter == WVec3Type(1, 2, 3));
    W_TEST_BOOL(s.m_fRadius == 4);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetFromPoints")
  {


    WVec3Type p[4] = {WVec3Type(2, 6, 0), WVec3Type(4, 2, 0), WVec3Type(2, 0, 0), WVec3Type(0, 4, 0)};

    WBoundingSphereType s = WBoundingSphereType::MakeFromPoints(p, 4);

    W_TEST_BOOL(s.m_vCenter == WVec3Type(2, 3, 0));
    W_TEST_BOOL(s.m_fRadius == 3);

    for (int i = 0; i < W_ARRAY_SIZE(p); ++i)
    {
      W_TEST_BOOL(s.Contains(p[i]));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ExpandToInclude(Point)")
  {
    WBoundingSphereType s = WBoundingSphereType::MakeZero();

    s.ExpandToInclude(WVec3Type(3, 0, 0));

    W_TEST_BOOL(s.m_vCenter == WVec3Type(0, 0, 0));
    W_TEST_BOOL(s.m_fRadius == 3);

    s = WBoundingSphereType::MakeInvalid();

    s.ExpandToInclude(WVec3Type(0.25, 0.0, 0.0));

    W_TEST_BOOL(s.m_vCenter == WVec3Type(0, 0, 0));
    W_TEST_BOOL(s.m_fRadius == Type(0.25));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ExpandToInclude(array)")
  {
    WBoundingSphereType s = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(2, 2, 0), 0.0f);

    WVec3Type p[4] = {WVec3Type(0, 2, 0), WVec3Type(4, 2, 0), WVec3Type(2, 0, 0), WVec3Type(2, 4, 0)};

    s.ExpandToInclude(p, 4);

    W_TEST_BOOL(s.m_vCenter == WVec3Type(2, 2, 0));
    W_TEST_BOOL(s.m_fRadius == 2);

    for (int i = 0; i < W_ARRAY_SIZE(p); ++i)
    {
      W_TEST_BOOL(s.Contains(p[i]));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ExpandToInclude (sphere)")
  {
    WBoundingSphereType s1, s2, s3;
    s1 = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(5, 0, 0), 1);
    s2 = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(6, 0, 0), 1);
    s3 = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(5, 0, 0), 2);

    s1.ExpandToInclude(s2);
    W_TEST_BOOL(s1.m_vCenter == WVec3Type(5, 0, 0));
    W_TEST_BOOL(s1.m_fRadius == 2);

    s1.ExpandToInclude(s3);
    W_TEST_BOOL(s1.m_vCenter == WVec3Type(5, 0, 0));
    W_TEST_BOOL(s1.m_fRadius == 2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ExpandToInclude (box)")
  {
    WBoundingSphereType s = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(1, 2, 3), 1);

    WBoundingBoxType b = WBoundingBoxType::MakeFromCenterAndHalfExtents(WVec3Type(1, 2, 3), WVec3Type(2.0f));

    s.ExpandToInclude(b);

    W_TEST_BOOL(s.m_vCenter == WVec3Type(1, 2, 3));
    W_TEST_FLOAT(s.m_fRadius, WMath::Sqrt(Type(12)), Type(0.000001));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Grow")
  {
    WBoundingSphereType s = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(1, 2, 3), 4);

    s.Grow(5);

    W_TEST_BOOL(s.m_vCenter == WVec3Type(1, 2, 3));
    W_TEST_BOOL(s.m_fRadius == 9);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsIdentical, ==, !=")
  {
    WBoundingSphereType s1, s2, s3;

    s1 = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(1, 2, 3), 4);
    s2 = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(1, 2, 3), 4);
    s3 = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(1.001f, 2.001f, 3.001f), 4.001f);

    W_TEST_BOOL(s1 == s1);
    W_TEST_BOOL(s2 == s2);
    W_TEST_BOOL(s3 == s3);

    W_TEST_BOOL(s1 == s2);
    W_TEST_BOOL(s2 == s1);

    W_TEST_BOOL(s1 != s3);
    W_TEST_BOOL(s2 != s3);
    W_TEST_BOOL(s3 != s1);
    W_TEST_BOOL(s3 != s2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsEqual")
  {
    WBoundingSphereType s1, s2, s3;

    s1 = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(1, 2, 3), 4);
    s2 = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(1, 2, 3), 4);
    s3 = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(1.001f, 2.001f, 3.001f), 4.001f);

    W_TEST_BOOL(s1.IsEqual(s1));
    W_TEST_BOOL(s2.IsEqual(s2));
    W_TEST_BOOL(s3.IsEqual(s3));

    W_TEST_BOOL(s1.IsEqual(s2));
    W_TEST_BOOL(s2.IsEqual(s1));

    W_TEST_BOOL(!s1.IsEqual(s3, Type(0.0001)));
    W_TEST_BOOL(!s2.IsEqual(s3, Type(0.0001)));
    W_TEST_BOOL(!s3.IsEqual(s1, Type(0.0001)));
    W_TEST_BOOL(!s3.IsEqual(s2, Type(0.0001)));

    W_TEST_BOOL(s1.IsEqual(s3, Type(0.002)));
    W_TEST_BOOL(s2.IsEqual(s3, Type(0.002)));
    W_TEST_BOOL(s3.IsEqual(s1, Type(0.002)));
    W_TEST_BOOL(s3.IsEqual(s2, Type(0.002)));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Translate")
  {
    WBoundingSphereType s = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(1, 2, 3), 4);

    s.Translate(WVec3Type(4, 5, 6));

    W_TEST_BOOL(s.m_vCenter == WVec3Type(5, 7, 9));
    W_TEST_BOOL(s.m_fRadius == 4);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ScaleFromCenter")
  {
    WBoundingSphereType s = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(1, 2, 3), 4);

    s.ScaleFromCenter(5.0f);

    W_TEST_BOOL(s.m_vCenter == WVec3Type(1, 2, 3));
    W_TEST_BOOL(s.m_fRadius == 20);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ScaleFromOrigin")
  {
    WBoundingSphereType s = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(1, 2, 3), 4);

    s.ScaleFromOrigin(WVec3Type(2, 3, 4));

    W_TEST_BOOL(s.m_vCenter == WVec3Type(2, 6, 12));
    W_TEST_BOOL(s.m_fRadius == 16);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetDistanceTo (point)")
  {
    WBoundingSphereType s = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(5, 0, 0), 2);

    W_TEST_BOOL(s.GetDistanceTo(WVec3Type(5, 0, 0)) == -2);
    W_TEST_BOOL(s.GetDistanceTo(WVec3Type(7, 0, 0)) == 0);
    W_TEST_BOOL(s.GetDistanceTo(WVec3Type(9, 0, 0)) == 2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetDistanceTo (sphere)")
  {
    WBoundingSphereType s1, s2, s3;
    s1 = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(5, 0, 0), 2);
    s2 = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(10, 0, 0), 3);
    s3 = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(10, 0, 0), 1);

    W_TEST_BOOL(s1.GetDistanceTo(s2) == 0);
    W_TEST_BOOL(s1.GetDistanceTo(s3) == 2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetDistanceTo (array)")
  {
    WBoundingSphereType s = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(0.0f), 0.0f);

    WVec3Type p[4] = {
      WVec3Type(5),
      WVec3Type(10),
      WVec3Type(15),
      WVec3Type(7),
    };

    W_TEST_FLOAT(s.GetDistanceTo(p, 4), WVec3Type(5).GetLength(), Type(0.001));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Contains (point)")
  {
    WBoundingSphereType s = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(5, 0, 0), 2.0f);

    W_TEST_BOOL(s.Contains(WVec3Type(3, 0, 0)));
    W_TEST_BOOL(s.Contains(WVec3Type(5, 0, 0)));
    W_TEST_BOOL(s.Contains(WVec3Type(6, 0, 0)));
    W_TEST_BOOL(s.Contains(WVec3Type(7, 0, 0)));

    W_TEST_BOOL(!s.Contains(WVec3Type(2, 0, 0)));
    W_TEST_BOOL(!s.Contains(WVec3Type(8, 0, 0)));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Contains (array)")
  {
    WBoundingSphereType s = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(0.0f), 6.0f);

    WVec3Type p[4] = {
      WVec3Type(3),
      WVec3Type(10),
      WVec3Type(2),
      WVec3Type(7),
    };

    W_TEST_BOOL(s.Contains(p, 2, sizeof(WVec3Type) * 2));
    W_TEST_BOOL(!s.Contains(p + 1, 2, sizeof(WVec3Type) * 2));
    W_TEST_BOOL(!s.Contains(p, 4, sizeof(WVec3Type)));
  }

  // Disabled because MSVC 2017 has code generation issues in Release builds
  W_TEST_BLOCK(WTestBlock::Disabled, "Contains (sphere)")
  {
    WBoundingSphereType s1, s2, s3;
    s1 = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(5, 0, 0), 2);
    s2 = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(6, 0, 0), 1);
    s3 = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(6, 0, 0), 2);

    W_TEST_BOOL(s1.Contains(s1));
    W_TEST_BOOL(s2.Contains(s2));
    W_TEST_BOOL(s3.Contains(s3));

    W_TEST_BOOL(s1.Contains(s2));
    W_TEST_BOOL(!s1.Contains(s3));

    W_TEST_BOOL(!s2.Contains(s3));
    W_TEST_BOOL(s3.Contains(s2));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Contains (box)")
  {
    WBoundingSphereType s = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(1, 2, 3), 4);
    WBoundingBoxType b1 = WBoundingBoxType::MakeFromMinMax(WVec3Type(1, 2, 3) - WVec3Type(1), WVec3Type(1, 2, 3) + WVec3Type(1));
    WBoundingBoxType b2 = WBoundingBoxType::MakeFromMinMax(WVec3Type(1, 2, 3) - WVec3Type(1), WVec3Type(1, 2, 3) + WVec3Type(3));

    W_TEST_BOOL(s.Contains(b1));
    W_TEST_BOOL(!s.Contains(b2));

    WVec3Type vDir(1, 1, 1);
    vDir.SetLength(3.99f).IgnoreResult();
    WBoundingBoxType b3 = WBoundingBoxType::MakeFromMinMax(WVec3Type(1, 2, 3) - WVec3Type(1), WVec3Type(1, 2, 3) + vDir);

    W_TEST_BOOL(s.Contains(b3));

    vDir.SetLength(4.01f).IgnoreResult();
    WBoundingBoxType b4 = WBoundingBoxType::MakeFromMinMax(WVec3Type(1, 2, 3) - WVec3Type(1), WVec3Type(1, 2, 3) + vDir);

    W_TEST_BOOL(!s.Contains(b4));
  }



  W_TEST_BLOCK(WTestBlock::Enabled, "Overlaps (array)")
  {
    WBoundingSphereType s = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(0.0f), 6.0f);

    WVec3Type p[4] = {
      WVec3Type(3),
      WVec3Type(10),
      WVec3Type(2),
      WVec3Type(7),
    };

    W_TEST_BOOL(s.Overlaps(p, 2, sizeof(WVec3Type) * 2));
    W_TEST_BOOL(!s.Overlaps(p + 1, 2, sizeof(WVec3Type) * 2));
    W_TEST_BOOL(s.Overlaps(p, 4, sizeof(WVec3Type)));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Overlaps (sphere)")
  {
    WBoundingSphereType s1, s2, s3;
    s1 = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(5, 0, 0), 2);
    s2 = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(6, 0, 0), 2);
    s3 = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(8, 0, 0), 1);

    W_TEST_BOOL(s1.Overlaps(s1));
    W_TEST_BOOL(s2.Overlaps(s2));
    W_TEST_BOOL(s3.Overlaps(s3));

    W_TEST_BOOL(s1.Overlaps(s2));
    W_TEST_BOOL(!s1.Overlaps(s3));

    W_TEST_BOOL(s2.Overlaps(s3));
    W_TEST_BOOL(s3.Overlaps(s2));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Overlaps (box)")
  {
    WBoundingSphereType s = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(1, 2, 3), 2);
    WBoundingBoxType b1 = WBoundingBoxType::MakeFromMinMax(WVec3Type(1, 2, 3), WVec3Type(1, 2, 3) + WVec3Type(2));
    WBoundingBoxType b2 = WBoundingBoxType::MakeFromMinMax(WVec3Type(1, 2, 3) + WVec3Type(2), WVec3Type(1, 2, 3) + WVec3Type(3));

    W_TEST_BOOL(s.Overlaps(b1));
    W_TEST_BOOL(!s.Overlaps(b2));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetBoundingBox")
  {
    WBoundingSphereType s = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(1, 2, 3), 2.0f);

    WBoundingBoxType b = s.GetBoundingBox();

    W_TEST_BOOL(b.m_vMin == WVec3Type(-1, 0, 1));
    W_TEST_BOOL(b.m_vMax == WVec3Type(3, 4, 5));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetClampedPoint")
  {
    WBoundingSphereType s = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(1, 2, 3), 2.0f);

    W_TEST_VEC3(s.GetClampedPoint(WVec3Type(2, 2, 3)), WVec3Type(2, 2, 3), Type(0.001));
    W_TEST_VEC3(s.GetClampedPoint(WVec3Type(5, 2, 3)), WVec3Type(3, 2, 3), Type(0.001));
    W_TEST_VEC3(s.GetClampedPoint(WVec3Type(1, 7, 3)), WVec3Type(1, 4, 3), Type(0.001));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetRayIntersection")
  {
    WBoundingSphereType s = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(1, 2, 3), 4);

    for (WUInt32 i = 0; i < 10000; ++i)
    {
      const WVec3Type vDir =
        WVec3Type(WMath::Sin(WAngleTemplate<Type>::MakeFromDegree(i * 1.0f)), WMath::Cos(WAngleTemplate<Type>::MakeFromDegree(i * 3.0f)), WMath::Cos(WAngleTemplate<Type>::MakeFromDegree(i * 1.0f)))
          .GetNormalized();
      const WVec3Type vTarget = vDir * s.m_fRadius + s.m_vCenter;
      const WVec3Type vSource = vTarget + vDir * Type(5);

      W_TEST_FLOAT((vSource - vTarget).GetLength(), 5.0f, Type(0.001));

      Type fIntersection;
      WVec3Type vIntersection;
      W_TEST_BOOL(s.GetRayIntersection(vSource, -vDir, &fIntersection, &vIntersection) == true);
      W_TEST_FLOAT(fIntersection, (vSource - vTarget).GetLength(), Type(0.0001));
      W_TEST_BOOL(vIntersection.IsEqual(vTarget, Type(0.0001)));

      W_TEST_BOOL(s.GetRayIntersection(vSource, vDir, &fIntersection, &vIntersection) == false);

      W_TEST_BOOL(s.GetRayIntersection(vTarget - vDir, vDir, &fIntersection, &vIntersection) == true);
      W_TEST_FLOAT(fIntersection, 1, Type(0.0001));
      W_TEST_BOOL(vIntersection.IsEqual(vTarget, Type(0.0001)));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetLineSegmentIntersection")
  {
    WBoundingSphereType s = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(1, 2, 3), 4);

    for (WUInt32 i = 0; i < 10000; ++i)
    {
      const WVec3Type vDir = WVec3Type(WMath::Sin(WAngleTemplate<Type>::MakeFromDegree(i * Type(1))), WMath::Cos(WAngleTemplate<Type>::MakeFromDegree(i * Type(3))),
        WMath::Cos(WAngleTemplate<Type>::MakeFromDegree(i * Type(1))))
                             .GetNormalized();
      const WVec3Type vTarget = vDir * s.m_fRadius + s.m_vCenter - vDir;
      const WVec3Type vSource = vTarget + vDir * Type(5);

      Type fIntersection;
      WVec3Type vIntersection;
      W_TEST_BOOL(s.GetLineSegmentIntersection(vSource, vTarget, &fIntersection, &vIntersection) == true);
      W_TEST_FLOAT(fIntersection, 4.0f / 5.0f, Type(0.0001));
      W_TEST_BOOL(vIntersection.IsEqual(vTarget + vDir, Type(0.0001)));

      W_TEST_BOOL(s.GetLineSegmentIntersection(vTarget, vSource, &fIntersection, &vIntersection) == true);
      W_TEST_FLOAT(fIntersection, 1.0f / 5.0f, Type(0.0001));
      W_TEST_BOOL(vIntersection.IsEqual(vTarget + vDir, Type(0.0001)));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "TransformFromOrigin")
  {
    WBoundingSphereType s = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(1, 2, 3), 4);
    WMat4Type mTransform;

    mTransform = WMat4Type::MakeTranslation(WVec3Type(5, 6, 7));
    mTransform.SetScalingFactors(WVec3Type(4, 3, 2)).IgnoreResult();

    s.TransformFromOrigin(mTransform);

    W_TEST_BOOL(s.m_vCenter == WVec3Type(9, 12, 13));
    W_TEST_BOOL(s.m_fRadius == 16);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "TransformFromCenter")
  {
    WBoundingSphereType s = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(1, 2, 3), 4);
    WMat4Type mTransform;

    mTransform = WMat4Type::MakeTranslation(WVec3Type(5, 6, 7));
    mTransform.SetScalingFactors(WVec3Type(4, 3, 2)).IgnoreResult();

    s.TransformFromCenter(mTransform);

    W_TEST_BOOL(s.m_vCenter == WVec3Type(6, 8, 10));
    W_TEST_BOOL(s.m_fRadius == 16);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsNaN")
  {
    if (WMath::SupportsNaN<Type>())
    {
      WBoundingSphereType s;

      s = WBoundingSphereType::MakeInvalid();
      W_TEST_BOOL(!s.IsNaN());

      s = WBoundingSphereType::MakeInvalid();
      s.m_fRadius = WMath::NaN<Type>();
      W_TEST_BOOL(s.IsNaN());

      s = WBoundingSphereType::MakeInvalid();
      s.m_vCenter.x = WMath::NaN<Type>();
      W_TEST_BOOL(s.IsNaN());

      s = WBoundingSphereType::MakeInvalid();
      s.m_vCenter.y = WMath::NaN<Type>();
      W_TEST_BOOL(s.IsNaN());

      s = WBoundingSphereType::MakeInvalid();
      s.m_vCenter.z = WMath::NaN<Type>();
      W_TEST_BOOL(s.IsNaN());
    }
  }
}

W_CREATE_SIMPLE_TEST(Math, BoundingSpheref)
{
  TestBoundingSphere<float>();
}

W_CREATE_SIMPLE_TEST(Math, BoundingSphered)
{
  TestBoundingSphere<double>();
}
