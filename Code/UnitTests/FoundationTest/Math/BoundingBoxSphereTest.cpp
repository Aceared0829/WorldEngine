#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/BoundingBoxSphere.h>

template <typename Type>
void TestBoundingBoxSphere()
{
  using WBoundingBoxSphereType = WBoundingBoxSphereTemplate<Type>;
  using WBoundingBoxType = WBoundingBoxTemplate<Type>;
  using WBoundingSphereType = WBoundingSphereTemplate<Type>;
  using WVec3Type = WVec3Template<Type>;
  using WMat4Type = WMat4Template<Type>;

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor")
  {
    WBoundingBoxSphereType b = WBoundingBoxSphereType::MakeFromCenterExtents(WVec3Type(-1, -2, -3), WVec3Type(1, 2, 3), 2);

    W_TEST_BOOL(b.m_vCenter == WVec3Type(-1, -2, -3));
    W_TEST_BOOL(b.m_vBoxHalfExtents == WVec3Type(1, 2, 3));
    W_TEST_BOOL(b.m_fSphereRadius == 2);

    WBoundingBoxType box = WBoundingBoxType::MakeFromMinMax(WVec3Type(1, 1, 1), WVec3Type(3, 3, 3));
    WBoundingSphereType sphere = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(2, 2, 2), 1);

    b = WBoundingBoxSphereType::MakeFromBoxAndSphere(box, sphere);

    W_TEST_BOOL(b.m_vCenter == WVec3Type(2, 2, 2));
    W_TEST_BOOL(b.m_vBoxHalfExtents == WVec3Type(1, 1, 1));
    W_TEST_BOOL(b.m_fSphereRadius == 1);
    W_TEST_BOOL(b.GetBox() == box);
    W_TEST_BOOL(b.GetSphere() == sphere);

    b = WBoundingBoxSphereType::MakeFromBox(box);

    W_TEST_BOOL(b.m_vCenter == WVec3Type(2, 2, 2));
    W_TEST_BOOL(b.m_vBoxHalfExtents == WVec3Type(1, 1, 1));
    W_TEST_FLOAT(b.m_fSphereRadius, WMath::Sqrt(Type(3)), WMath::DefaultEpsilon<Type>());
    W_TEST_BOOL(b.GetBox() == box);

    b = WBoundingBoxSphereType::MakeFromSphere(sphere);

    W_TEST_BOOL(b.m_vCenter == WVec3Type(2, 2, 2));
    W_TEST_BOOL(b.m_vBoxHalfExtents == WVec3Type(1, 1, 1));
    W_TEST_BOOL(b.m_fSphereRadius == 1);
    W_TEST_BOOL(b.GetSphere() == sphere);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetFromPoints")
  {
    WVec3Type p[6] = {
      WVec3Type(-4, 0, 0),
      WVec3Type(5, 0, 0),
      WVec3Type(0, -6, 0),
      WVec3Type(0, 7, 0),
      WVec3Type(0, 0, -8),
      WVec3Type(0, 0, 9),
    };

    WBoundingBoxSphereType b = WBoundingBoxSphereType::MakeFromPoints(p, 6);

    W_TEST_BOOL(b.m_vCenter == WVec3Type(0.5, 0.5, 0.5));
    W_TEST_BOOL(b.m_vBoxHalfExtents == WVec3Type(4.5, 6.5, 8.5));
    W_TEST_FLOAT(b.m_fSphereRadius, WVec3Type(0.5, 0.5, 8.5).GetLength(), WMath::DefaultEpsilon<Type>());
    W_TEST_BOOL(b.m_fSphereRadius <= b.m_vBoxHalfExtents.GetLength());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetInvalid")
  {
    WBoundingBoxSphereType b = WBoundingBoxSphereType::MakeInvalid();

    W_TEST_BOOL(!b.IsValid());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ExpandToInclude")
  {
    WBoundingBoxSphereType b1 = WBoundingBoxSphereType::MakeInvalid();
    WBoundingBoxSphereType b2 = WBoundingBoxSphereType::MakeFromBox(WBoundingBoxType::MakeFromMinMax(WVec3Type(2, 2, 2), WVec3Type(4, 4, 4)));

    b1.ExpandToInclude(b2);
    W_TEST_BOOL(b1 == b2);

    WBoundingSphereType sphere = WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(2, 2, 2), 2);
    b2 = WBoundingBoxSphereType::MakeFromSphere(sphere);

    b1.ExpandToInclude(b2);
    W_TEST_BOOL(b1 != b2);

    W_TEST_BOOL(b1.m_vCenter == WVec3Type(2, 2, 2));
    W_TEST_BOOL(b1.m_vBoxHalfExtents == WVec3Type(2, 2, 2));
    W_TEST_FLOAT(b1.m_fSphereRadius, WMath::Sqrt(Type(3)) * 2, WMath::DefaultEpsilon<Type>());
    W_TEST_BOOL(b1.m_fSphereRadius <= b1.m_vBoxHalfExtents.GetLength());

    b1 = WBoundingBoxSphereType::MakeInvalid();
    b2 = WBoundingBoxSphereType::MakeFromBox(WBoundingBoxType::MakeFromMinMax(WVec3Type(0.25, 0.25, 0.25), WVec3Type(0.5, 0.5, 0.5)));

    b1.ExpandToInclude(b2);
    W_TEST_BOOL(b1 == b2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Transform")
  {
    WBoundingBoxSphereType b = WBoundingBoxSphereType::MakeFromCenterExtents(WVec3Type(1), WVec3Type(5), 5);

    WMat4Type m;
    m = WMat4Type::MakeScaling(WVec3Type(-2, -3, -2));
    m.SetTranslationVector(WVec3Type(1, 1, 1));

    b.Transform(m);

    W_TEST_BOOL(b.m_vCenter == WVec3Type(-1, -2, -1));
    W_TEST_BOOL(b.m_vBoxHalfExtents == WVec3Type(10, 15, 10));
    W_TEST_BOOL(b.m_fSphereRadius == 15);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsNaN")
  {
    if (WMath::SupportsNaN<Type>())
    {
      WBoundingBoxSphereType b;

      b = WBoundingBoxSphereType::MakeInvalid();
      W_TEST_BOOL(!b.IsNaN());

      b = WBoundingBoxSphereType::MakeInvalid();
      b.m_vCenter.x = WMath::NaN<Type>();
      W_TEST_BOOL(b.IsNaN());

      b = WBoundingBoxSphereType::MakeInvalid();
      b.m_vCenter.y = WMath::NaN<Type>();
      W_TEST_BOOL(b.IsNaN());

      b = WBoundingBoxSphereType::MakeInvalid();
      b.m_vCenter.z = WMath::NaN<Type>();
      W_TEST_BOOL(b.IsNaN());

      b = WBoundingBoxSphereType::MakeInvalid();
      b.m_vBoxHalfExtents.x = WMath::NaN<Type>();
      W_TEST_BOOL(b.IsNaN());

      b = WBoundingBoxSphereType::MakeInvalid();
      b.m_vBoxHalfExtents.y = WMath::NaN<Type>();
      W_TEST_BOOL(b.IsNaN());

      b = WBoundingBoxSphereType::MakeInvalid();
      b.m_vBoxHalfExtents.z = WMath::NaN<Type>();
      W_TEST_BOOL(b.IsNaN());

      b = WBoundingBoxSphereType::MakeInvalid();
      b.m_fSphereRadius = WMath::NaN<Type>();
      W_TEST_BOOL(b.IsNaN());
    }
  }
}

W_CREATE_SIMPLE_TEST(Math, BoundingBoxSpheref)
{
  TestBoundingBoxSphere<float>();
}

W_CREATE_SIMPLE_TEST(Math, BoundingBoxSphered)
{
  TestBoundingBoxSphere<double>();
}
