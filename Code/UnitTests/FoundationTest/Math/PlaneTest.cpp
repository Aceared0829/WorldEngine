#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/BoundingSphere.h>
#include <Foundation/Math/Plane.h>
#include <Foundation/Math/Random.h>

template <typename Type>
void TestPlane()
{
  using WPlaneType = WPlaneTemplate<Type>;
  using WVec3Type = WVec3Template<Type>;
  using WMat3Type = WMat3Template<Type>;
  using WMat4Type = WMat4Template<Type>;
  using WBoundingBoxType = WBoundingBoxTemplate<Type>;
  using WBoundingSphereType = WBoundingSphereTemplate<Type>;

  W_TEST_BLOCK(WTestBlock::Enabled, "Default Constructor")
  {
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
    if (WMath::SupportsNaN<Type>())
    {
      // In debug the default constructor initializes everything with NaN.
      WPlaneType p;
      W_TEST_BOOL(WMath::IsNaN(p.m_vNormal.x) && WMath::IsNaN(p.m_vNormal.y) && WMath::IsNaN(p.m_vNormal.z) && WMath::IsNaN(p.m_fNegDistance));
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    Type testBlock[4] = {(Type)1, (Type)2, (Type)3, (Type)4};
    WPlaneType* p = ::new ((void*)&testBlock[0]) WPlaneType;
    W_TEST_BOOL(p->m_vNormal.x == (Type)1 && p->m_vNormal.y == (Type)2 && p->m_vNormal.z == (Type)3 && p->m_fNegDistance == (Type)4);
#endif
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor(Normal, Point)")
  {
    WPlaneType p = WPlaneType::MakeFromNormalAndPoint(WVec3Type(1, 0, 0), WVec3Type(5, 3, 1));

    W_TEST_BOOL(p.m_vNormal == WVec3Type(1, 0, 0));
    W_TEST_FLOAT(p.m_fNegDistance, (Type)-5.0, (Type)0.0001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor(Point, Point, Point)")
  {
    WPlaneType p = WPlaneType::MakeFromPoints(WVec3Type(-1, 5, 1), WVec3Type(1, 5, 1), WVec3Type(0, 5, -5));

    W_TEST_VEC3(p.m_vNormal, WVec3Type(0, 1, 0), (Type)0.0001);
    W_TEST_FLOAT(p.m_fNegDistance, (Type)-5.0, (Type)0.0001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor(Points)")
  {
    WVec3Type v[3] = {WVec3Type(-1, 5, 1), WVec3Type(1, 5, 1), WVec3Type(0, 5, -5)};

    WPlaneType p;
    p.SetFromPoints(v).AssertSuccess();

    W_TEST_VEC3(p.m_vNormal, WVec3Type(0, 1, 0), (Type)0.0001);
    W_TEST_FLOAT(p.m_fNegDistance, (Type)-5.0, (Type)0.0001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor(Points, numpoints)")
  {
    WVec3Type v[6] = {WVec3Type(-1, 5, 1), WVec3Type(-1, 5, 1), WVec3Type(1, 5, 1), WVec3Type(1, 5, 1), WVec3Type(0, 5, -5), WVec3Type(0, 5, -5)};

    WPlaneType p;
    p.SetFromPoints(v, 6).AssertSuccess();

    W_TEST_VEC3(p.m_vNormal, WVec3Type(0, 1, 0), (Type)0.0001);
    W_TEST_FLOAT(p.m_fNegDistance, (Type)-5.0, (Type)0.0001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetFromNormalAndPoint")
  {
    WPlaneType p;
    p = WPlaneType::MakeFromNormalAndPoint(WVec3Type(1, 0, 0), WVec3Type(5, 3, 1));

    W_TEST_BOOL(p.m_vNormal == WVec3Type(1, 0, 0));
    W_TEST_FLOAT(p.m_fNegDistance, (Type)-5.0, (Type)0.0001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetFromPoints")
  {
    WPlaneType p;
    p.SetFromPoints(WVec3Type(-1, 5, 1), WVec3Type(1, 5, 1), WVec3Type(0, 5, -5)).IgnoreResult();

    W_TEST_VEC3(p.m_vNormal, WVec3Type(0, 1, 0), (Type)0.0001);
    W_TEST_FLOAT(p.m_fNegDistance, (Type)-5.0, (Type)0.0001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetFromPoints")
  {
    WVec3Type v[3] = {WVec3Type(-1, 5, 1), WVec3Type(1, 5, 1), WVec3Type(0, 5, -5)};

    WPlaneType p;
    p.SetFromPoints(v).IgnoreResult();

    W_TEST_VEC3(p.m_vNormal, WVec3Type(0, 1, 0), (Type)0.0001);
    W_TEST_FLOAT(p.m_fNegDistance, (Type)-5.0, (Type)0.0001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetFromPoints")
  {
    WVec3Type v[6] = {WVec3Type(-1, 5, 1), WVec3Type(-1, 5, 1), WVec3Type(1, 5, 1), WVec3Type(1, 5, 1), WVec3Type(0, 5, -5), WVec3Type(0, 5, -5)};

    WPlaneType p;
    p.SetFromPoints(v, 6).IgnoreResult();

    W_TEST_VEC3(p.m_vNormal, WVec3Type(0, 1, 0), (Type)0.0001);
    W_TEST_FLOAT(p.m_fNegDistance, (Type)-5.0, (Type)0.0001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetFromDirections")
  {
    WPlaneType p;
    p.SetFromDirections(WVec3Type(1, 0, 0), WVec3Type(1, 0, -1), WVec3Type(3, 5, 9)).IgnoreResult();

    W_TEST_VEC3(p.m_vNormal, WVec3Type(0, 1, 0), (Type)0.0001);
    W_TEST_FLOAT(p.m_fNegDistance, (Type)-5.0, (Type)0.0001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetInvalid")
  {
    WPlaneType p;
    p.SetFromDirections(WVec3Type(1, 0, 0), WVec3Type(1, 0, -1), WVec3Type(3, 5, 9)).IgnoreResult();

    p = WPlaneType::MakeInvalid();

    W_TEST_VEC3(p.m_vNormal, WVec3Type(0, 0, 0), (Type)0.0001);
    W_TEST_FLOAT(p.m_fNegDistance, (Type)0.0, (Type)0.0001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetDistanceTo")
  {
    WPlaneType p = WPlaneType::MakeFromNormalAndPoint(WVec3Type(1, 0, 0), WVec3Type(5, 0, 0));

    W_TEST_FLOAT(p.GetDistanceTo(WVec3Type(10, 3, 5)), (Type)5.0, (Type)0.0001);
    W_TEST_FLOAT(p.GetDistanceTo(WVec3Type(0, 7, 123)), (Type)-5.0, (Type)0.0001);
    W_TEST_FLOAT(p.GetDistanceTo(WVec3Type(5, 12, 23)), (Type)0.0, (Type)0.0001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetMinimumDistanceTo")
  {
    WVec3Type v1[3] = {WVec3Type(15, 3, 5), WVec3Type(6, 7, 123), WVec3Type(10, 12, 23)};
    WVec3Type v2[3] = {WVec3Type(3, 3, 5), WVec3Type(5, 7, 123), WVec3Type(10, 12, 23)};

    WPlaneType p = WPlaneType::MakeFromNormalAndPoint(WVec3Type(1, 0, 0), WVec3Type(5, 0, 0));

    W_TEST_FLOAT(p.GetMinimumDistanceTo(v1, 3), (Type)1.0, (Type)0.0001);
    W_TEST_FLOAT(p.GetMinimumDistanceTo(v2, 3), (Type)-2.0, (Type)0.0001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetMinMaxDistanceTo")
  {
    WVec3Type v1[3] = {WVec3Type(15, 3, 5), WVec3Type(5, 7, 123), WVec3Type(0, 12, 23)};
    WVec3Type v2[3] = {WVec3Type(8, 3, 5), WVec3Type(6, 7, 123), WVec3Type(10, 12, 23)};

    WPlaneType p = WPlaneType::MakeFromNormalAndPoint(WVec3Type(1, 0, 0), WVec3Type(5, 0, 0));

    Type fmin, fmax;

    p.GetMinMaxDistanceTo(fmin, fmax, v1, 3);
    W_TEST_FLOAT(fmin, (Type)-5.0, (Type)0.0001);
    W_TEST_FLOAT(fmax, (Type)10.0, (Type)0.0001);

    p.GetMinMaxDistanceTo(fmin, fmax, v2, 3);
    W_TEST_FLOAT(fmin, (Type)1, (Type)0.0001);
    W_TEST_FLOAT(fmax, (Type)5, (Type)0.0001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetPointPosition")
  {
    WPlaneType p = WPlaneType::MakeFromNormalAndPoint(WVec3Type(0, 1, 0), WVec3Type(0, 10, 0));

    W_TEST_BOOL(p.GetPointPosition(WVec3Type(0, 15, 0)) == WPositionOnPlane::Front);
    W_TEST_BOOL(p.GetPointPosition(WVec3Type(0, 5, 0)) == WPositionOnPlane::Back);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetPointPosition(planewidth)")
  {
    WPlaneType p = WPlaneType::MakeFromNormalAndPoint(WVec3Type(0, 1, 0), WVec3Type(0, 10, 0));

    W_TEST_BOOL(p.GetPointPosition(WVec3Type(0, 15, 0), (Type)0.01) == WPositionOnPlane::Front);
    W_TEST_BOOL(p.GetPointPosition(WVec3Type(0, 5, 0), (Type)0.01) == WPositionOnPlane::Back);
    W_TEST_BOOL(p.GetPointPosition(WVec3Type(0, 10, 0), (Type)0.01) == WPositionOnPlane::OnPlane);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetObjectPosition")
  {
    WPlaneType p = WPlaneType::MakeFromNormalAndPoint(WVec3Type(1, 0, 0), WVec3Type(10, 0, 0));

    WVec3Type v0[3] = {WVec3Type(12, 0, 0), WVec3Type(15, 0, 0), WVec3Type(20, 0, 0)};
    WVec3Type v1[3] = {WVec3Type(8, 0, 0), WVec3Type(6, 0, 0), WVec3Type(4, 0, 0)};
    WVec3Type v2[3] = {WVec3Type(12, 0, 0), WVec3Type(6, 0, 0), WVec3Type(4, 0, 0)};

    W_TEST_BOOL(p.GetObjectPosition(v0, 3) == WPositionOnPlane::Front);
    W_TEST_BOOL(p.GetObjectPosition(v1, 3) == WPositionOnPlane::Back);
    W_TEST_BOOL(p.GetObjectPosition(v2, 3) == WPositionOnPlane::Spanning);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetObjectPosition(fPlaneHalfWidth)")
  {
    WPlaneType p = WPlaneType::MakeFromNormalAndPoint(WVec3Type(1, 0, 0), WVec3Type(10, 0, 0));

    WVec3Type v0[3] = {WVec3Type(12, 0, 0), WVec3Type(15, 0, 0), WVec3Type(20, 0, 0)};
    WVec3Type v1[3] = {WVec3Type(8, 0, 0), WVec3Type(6, 0, 0), WVec3Type(4, 0, 0)};
    WVec3Type v2[3] = {WVec3Type(12, 0, 0), WVec3Type(6, 0, 0), WVec3Type(4, 0, 0)};
    WVec3Type v3[3] = {WVec3Type(10, 1, 0), WVec3Type(10, 5, 7), WVec3Type(10, 3, -5)};

    W_TEST_BOOL(p.GetObjectPosition(v0, 3, (Type)0.001) == WPositionOnPlane::Front);
    W_TEST_BOOL(p.GetObjectPosition(v1, 3, (Type)0.001) == WPositionOnPlane::Back);
    W_TEST_BOOL(p.GetObjectPosition(v2, 3, (Type)0.001) == WPositionOnPlane::Spanning);
    W_TEST_BOOL(p.GetObjectPosition(v3, 3, (Type)0.001) == WPositionOnPlane::OnPlane);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetObjectPosition(sphere)")
  {
    WPlaneType p = WPlaneType::MakeFromNormalAndPoint(WVec3Type(1, 0, 0), WVec3Type(10, 0, 0));

    W_TEST_BOOL(p.GetObjectPosition(WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(15, 2, 3), (Type)3.0)) == WPositionOnPlane::Front);
    W_TEST_BOOL(p.GetObjectPosition(WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(5, 2, 3), (Type)3.0)) == WPositionOnPlane::Back);
    W_TEST_BOOL(p.GetObjectPosition(WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type((Type)15, (Type)2, (Type)4.999), (Type)3.0)) == WPositionOnPlane::Front);
    W_TEST_BOOL(p.GetObjectPosition(WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(5, 2, 3), (Type)4.999)) == WPositionOnPlane::Back);
    W_TEST_BOOL(p.GetObjectPosition(WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(8, 2, 3), (Type)3.0)) == WPositionOnPlane::Spanning);
    W_TEST_BOOL(p.GetObjectPosition(WBoundingSphereType::MakeFromCenterAndRadius(WVec3Type(12, 2, 3), (Type)3.0)) == WPositionOnPlane::Spanning);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetObjectPosition(box)")
  {
    {
      WPlaneType p = WPlaneType::MakeFromNormalAndPoint(WVec3Type(1, 0, 0), WVec3Type(10, 0, 0));
      W_TEST_BOOL(p.GetObjectPosition(WBoundingBoxType::MakeFromMinMax(WVec3Type((Type)10.1), WVec3Type(15))) == WPositionOnPlane::Front);
      W_TEST_BOOL(p.GetObjectPosition(WBoundingBoxType::MakeFromMinMax(WVec3Type(7), WVec3Type((Type)9.9))) == WPositionOnPlane::Back);
      W_TEST_BOOL(p.GetObjectPosition(WBoundingBoxType::MakeFromMinMax(WVec3Type(7), WVec3Type(15))) == WPositionOnPlane::Spanning);
    }
    {
      WPlaneType p = WPlaneType::MakeFromNormalAndPoint(WVec3Type(0, 1, 0), WVec3Type(0, 10, 0));
      W_TEST_BOOL(p.GetObjectPosition(WBoundingBoxType::MakeFromMinMax(WVec3Type((Type)10.1), WVec3Type(15))) == WPositionOnPlane::Front);
      W_TEST_BOOL(p.GetObjectPosition(WBoundingBoxType::MakeFromMinMax(WVec3Type(7), WVec3Type((Type)9.9))) == WPositionOnPlane::Back);
      W_TEST_BOOL(p.GetObjectPosition(WBoundingBoxType::MakeFromMinMax(WVec3Type(7), WVec3Type(15))) == WPositionOnPlane::Spanning);
    }
    {
      WPlaneType p = WPlaneType::MakeFromNormalAndPoint(WVec3Type(0, 0, 1), WVec3Type(0, 0, 10));
      W_TEST_BOOL(p.GetObjectPosition(WBoundingBoxType::MakeFromMinMax(WVec3Type((Type)10.1), WVec3Type(15))) == WPositionOnPlane::Front);
      W_TEST_BOOL(p.GetObjectPosition(WBoundingBoxType::MakeFromMinMax(WVec3Type(7), WVec3Type((Type)9.9))) == WPositionOnPlane::Back);
      W_TEST_BOOL(p.GetObjectPosition(WBoundingBoxType::MakeFromMinMax(WVec3Type(7), WVec3Type(15))) == WPositionOnPlane::Spanning);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ProjectOntoPlane")
  {
    WPlaneType p = WPlaneType::MakeFromNormalAndPoint(WVec3Type(0, 1, 0), WVec3Type(0, 10, 0));

    W_TEST_VEC3(p.ProjectOntoPlane(WVec3Type(3, 15, 2)), WVec3Type(3, 10, 2), (Type)0.001);
    W_TEST_VEC3(p.ProjectOntoPlane(WVec3Type(-1, 5, -5)), WVec3Type(-1, 10, -5), (Type)0.001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Mirror")
  {
    WPlaneType p = WPlaneType::MakeFromNormalAndPoint(WVec3Type(0, 1, 0), WVec3Type(0, 10, 0));

    W_TEST_VEC3(p.Mirror(WVec3Type(3, 15, 2)), WVec3Type(3, 5, 2), (Type)0.001);
    W_TEST_VEC3(p.Mirror(WVec3Type(-1, 5, -5)), WVec3Type(-1, 15, -5), (Type)0.001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetCoplanarDirection")
  {
    WPlaneType p = WPlaneType::MakeFromNormalAndPoint(WVec3Type(0, 1, 0), WVec3Type(0, 10, 0));

    W_TEST_VEC3(p.GetCoplanarDirection(WVec3Type(0, 1, 0)), WVec3Type(0, 0, 0), (Type)0.001);
    W_TEST_VEC3(p.GetCoplanarDirection(WVec3Type(1, 1, 0)).GetNormalized(), WVec3Type(1, 0, 0), (Type)0.001);
    W_TEST_VEC3(p.GetCoplanarDirection(WVec3Type(-1, 1, 0)).GetNormalized(), WVec3Type(-1, 0, 0), (Type)0.001);
    W_TEST_VEC3(p.GetCoplanarDirection(WVec3Type(0, 1, 1)).GetNormalized(), WVec3Type(0, 0, 1), (Type)0.001);
    W_TEST_VEC3(p.GetCoplanarDirection(WVec3Type(0, 1, -1)).GetNormalized(), WVec3Type(0, 0, -1), (Type)0.001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsIdentical / operator== / operator!=")
  {
    WPlaneType p1 = WPlaneType::MakeFromNormalAndPoint(WVec3Type(0, 1, 0), WVec3Type(0, 10, 0));
    WPlaneType p2 = WPlaneType::MakeFromNormalAndPoint(WVec3Type(0, 1, 0), WVec3Type(0, 10, 0));
    WPlaneType p3 = WPlaneType::MakeFromNormalAndPoint(WVec3Type(0, 1, 0), WVec3Type(0, 10 + WMath::DefaultEpsilon<Type>(), 0));

    W_TEST_BOOL(p1.IsIdentical(p1));
    W_TEST_BOOL(p2.IsIdentical(p2));
    W_TEST_BOOL(p3.IsIdentical(p3));

    W_TEST_BOOL(p1.IsIdentical(p2));
    W_TEST_BOOL(p2.IsIdentical(p1));

    W_TEST_BOOL(!p1.IsIdentical(p3));
    W_TEST_BOOL(!p2.IsIdentical(p3));


    W_TEST_BOOL(p1 == p2);
    W_TEST_BOOL(p2 == p1);

    W_TEST_BOOL(p1 != p3);
    W_TEST_BOOL(p2 != p3);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsEqual")
  {
    WPlaneType p1 = WPlaneType::MakeFromNormalAndPoint(WVec3Type(0, 1, 0), WVec3Type(0, 10, 0));
    WPlaneType p2 = WPlaneType::MakeFromNormalAndPoint(WVec3Type(0, 1, 0), WVec3Type(0, 10, 0));
    WPlaneType p3 = WPlaneType::MakeFromNormalAndPoint(WVec3Type(0, 1, 0), WVec3Type(0, 10 + WMath::DefaultEpsilon<Type>(), 0));

    W_TEST_BOOL(p1.IsEqual(p1));
    W_TEST_BOOL(p2.IsEqual(p2));
    W_TEST_BOOL(p3.IsEqual(p3));

    W_TEST_BOOL(p1.IsEqual(p2));
    W_TEST_BOOL(p2.IsEqual(p1));

    W_TEST_BOOL(p1.IsEqual(p3));
    W_TEST_BOOL(p2.IsEqual(p3));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsValid")
  {
    WPlaneType p1 = WPlaneType::MakeFromNormalAndPoint(WVec3Type(0, 1, 0), WVec3Type(0, 10, 0));

    W_TEST_BOOL(p1.IsValid());

    p1 = WPlaneType::MakeInvalid();
    W_TEST_BOOL(!p1.IsValid());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Transform(Mat3)")
  {
    const Type matrixScale[] = {(Type)1, (Type)2, (Type)99};

    for (WUInt32 loopIndex = 0; loopIndex < W_ARRAY_SIZE(matrixScale); ++loopIndex)
    {
      WPlaneType p = WPlaneType::MakeFromNormalAndPoint(WVec3Type(0, 1, 0), WVec3Type(0, 10, 0));

      WMat3Type m;
      {
        m = WMat3Type::MakeRotationX(WAngleTemplate<Type>::MakeFromDegree((Type)90));

        WMat3Type rot = WMat3Type::MakeScaling(WVec3Type(1) * matrixScale[loopIndex]);
        m = m * rot;
      }

      p.Transform(m);

      W_TEST_VEC3(p.m_vNormal, WVec3Type(0, 0, 1), (Type)0.0001);
      W_TEST_FLOAT(p.m_fNegDistance, (Type)-10.0 * matrixScale[loopIndex], (Type)0.0001);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Transform(Mat4)")
  {
    const Type matrixScale[] = {(Type)1, (Type)2, (Type)99};

    for (WUInt32 loopIndex = 0; loopIndex < W_ARRAY_SIZE(matrixScale); ++loopIndex)
    {
      {
        WPlaneType p = WPlaneType::MakeFromNormalAndPoint(WVec3Type(0, 1, 0), WVec3Type(0, 10, 0));

        WMat4Type m;
        {
          m = WMat4Type::MakeRotationX(WAngleTemplate<Type>::MakeFromDegree((Type)90));
          m.SetTranslationVector(WVec3Type(0, 5, 0));

          WMat4Type rot = WMat4Type::MakeScaling(WVec3Type(1) * matrixScale[loopIndex]);
          m = m * rot;
        }

        p.Transform(m);

        W_TEST_VEC3(p.m_vNormal, WVec3Type(0, 0, 1), (Type)0.0001);
        W_TEST_FLOAT(p.m_fNegDistance, (Type)-10.0 * matrixScale[loopIndex], (Type)0.0001);
      }

      {
        WPlaneType p = WPlaneType::MakeFromNormalAndPoint(WVec3Type(0, 1, 0), WVec3Type(0, 10, 0));

        WMat4Type m;
        {
          m = WMat4Type::MakeRotationX(WAngleTemplate<Type>::MakeFromDegree((Type)90));
          m.SetTranslationVector(WVec3Type(0, 0, 5));

          WMat4Type rot = WMat4Type::MakeScaling(WVec3Type(1) * matrixScale[loopIndex]);
          m = m * rot;
        }

        p.Transform(m);

        W_TEST_VEC3(p.m_vNormal, WVec3Type(0, 0, 1), (Type)0.0001);
        W_TEST_FLOAT(p.m_fNegDistance, (Type)-10.0 * matrixScale[loopIndex] - (Type)5.0, (Type)0.0001);
      }
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Flip")
  {
    WPlaneType p = WPlaneType::MakeFromNormalAndPoint(WVec3Type(0, 1, 0), WVec3Type(0, 10, 0));

    W_TEST_VEC3(p.m_vNormal, WVec3Type(0, 1, 0), (Type)0.0001);
    W_TEST_FLOAT(p.m_fNegDistance, (Type)-10.0, (Type)0.0001);

    p.Flip();

    W_TEST_VEC3(p.m_vNormal, WVec3Type(0, -1, 0), (Type)0.0001);
    W_TEST_FLOAT(p.m_fNegDistance, (Type)10.0, (Type)0.0001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "FlipIfNecessary")
  {
    {
      WPlaneType p = WPlaneType::MakeFromNormalAndPoint(WVec3Type(0, 1, 0), WVec3Type(0, 10, 0));

      W_TEST_VEC3(p.m_vNormal, WVec3Type(0, 1, 0), (Type)0.0001);
      W_TEST_FLOAT(p.m_fNegDistance, (Type)-10.0, (Type)0.0001);

      W_TEST_BOOL(p.FlipIfNecessary(WVec3Type(0, 11, 0), true) == false);

      W_TEST_VEC3(p.m_vNormal, WVec3Type(0, 1, 0), (Type)0.0001);
      W_TEST_FLOAT(p.m_fNegDistance, (Type)-10.0, (Type)0.0001);
    }

    {
      WPlaneType p = WPlaneType::MakeFromNormalAndPoint(WVec3Type(0, 1, 0), WVec3Type(0, 10, 0));

      W_TEST_VEC3(p.m_vNormal, WVec3Type(0, 1, 0), (Type)0.0001);
      W_TEST_FLOAT(p.m_fNegDistance, (Type)-10.0, (Type)0.0001);

      W_TEST_BOOL(p.FlipIfNecessary(WVec3Type(0, 11, 0), false) == true);

      W_TEST_VEC3(p.m_vNormal, WVec3Type(0, -1, 0), (Type)0.0001);
      W_TEST_FLOAT(p.m_fNegDistance, (Type)10.0, (Type)0.0001);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetRayIntersection")
  {
    WPlaneType p = WPlaneType::MakeFromNormalAndPoint(WVec3Type(0, 1, 0), WVec3Type(0, 10, 0));

    Type f;
    WVec3Type v;

    W_TEST_BOOL(p.GetRayIntersection(WVec3Type(3, 1, 7), WVec3Type(0, 1, 0), &f, &v));
    W_TEST_FLOAT(f, (Type)9, (Type)0.0001);
    W_TEST_VEC3(v, WVec3Type(3, 10, 7), (Type)0.0001);

    W_TEST_BOOL(p.GetRayIntersection(WVec3Type(3, 20, 7), WVec3Type(0, -1, 0), &f, &v));
    W_TEST_FLOAT(f, (Type)10, (Type)0.0001);
    W_TEST_VEC3(v, WVec3Type(3, 10, 7), (Type)0.0001);

    W_TEST_BOOL(!p.GetRayIntersection(WVec3Type(3, 1, 7), WVec3Type(1, 0, 0), &f, &v));
    W_TEST_BOOL(!p.GetRayIntersection(WVec3Type(3, 1, 7), WVec3Type(0, -1, 0), &f, &v));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetRayIntersectionBiDirectional")
  {
    WPlaneType p = WPlaneType::MakeFromNormalAndPoint(WVec3Type(0, 1, 0), WVec3Type(0, 10, 0));

    Type f;
    WVec3Type v;

    W_TEST_BOOL(p.GetRayIntersectionBiDirectional(WVec3Type(3, 1, 7), WVec3Type(0, 1, 0), &f, &v));
    W_TEST_FLOAT(f, (Type)9, (Type)0.0001);
    W_TEST_VEC3(v, WVec3Type(3, 10, 7), (Type)0.0001);

    W_TEST_BOOL(!p.GetRayIntersectionBiDirectional(WVec3Type(3, 1, 7), WVec3Type(1, 0, 0), &f, &v));

    W_TEST_BOOL(p.GetRayIntersectionBiDirectional(WVec3Type(3, 1, 7), WVec3Type(0, -1, 0), &f, &v));
    W_TEST_FLOAT(f, (Type)-9, (Type)0.0001);
    W_TEST_VEC3(v, WVec3Type(3, 10, 7), (Type)0.0001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetLineSegmentIntersection")
  {
    WPlaneType p = WPlaneType::MakeFromNormalAndPoint(WVec3Type(0, 1, 0), WVec3Type(0, 10, 0));

    Type f;
    WVec3Type v;

    W_TEST_BOOL(p.GetLineSegmentIntersection(WVec3Type(3, 5, 7), WVec3Type(3, 15, 7), &f, &v));
    W_TEST_FLOAT(f, (Type)0.5, (Type)0.0001);
    W_TEST_VEC3(v, WVec3Type(3, 10, 7), (Type)0.0001);

    W_TEST_BOOL(!p.GetLineSegmentIntersection(WVec3Type(3, 5, 7), WVec3Type(13, 5, 7), &f, &v));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetPlanesIntersectionPoint")
  {
    WPlaneType p1 = WPlaneType::MakeFromNormalAndPoint(WVec3Type(1, 0, 0), WVec3Type(0, 10, 0));
    WPlaneType p2 = WPlaneType::MakeFromNormalAndPoint(WVec3Type(0, 1, 0), WVec3Type(0, 10, 0));
    WPlaneType p3 = WPlaneType::MakeFromNormalAndPoint(WVec3Type(0, 0, 1), WVec3Type(0, 10, 0));

    WVec3Type r;

    W_TEST_BOOL(WPlaneType::GetPlanesIntersectionPoint(p1, p2, p3, r) == W_SUCCESS);
    W_TEST_VEC3(r, WVec3Type(0, 10, 0), (Type)0.0001);

    W_TEST_BOOL(WPlaneType::GetPlanesIntersectionPoint(p1, p1, p3, r) == W_FAILURE);
    W_TEST_BOOL(WPlaneType::GetPlanesIntersectionPoint(p1, p2, p2, r) == W_FAILURE);
    W_TEST_BOOL(WPlaneType::GetPlanesIntersectionPoint(p3, p2, p3, r) == W_FAILURE);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "FindSupportPoints")
  {
    WVec3Type v[6] = {WVec3Type(-1, 5, 1), WVec3Type(-1, 5, 1), WVec3Type(1, 5, 1), WVec3Type(1, 5, 1), WVec3Type(0, 5, -5), WVec3Type(0, 5, -5)};

    WInt32 i1, i2, i3;

    WPlaneType::FindSupportPoints(v, 6, i1, i2, i3).IgnoreResult();

    W_TEST_INT(i1, 0);
    W_TEST_INT(i2, 2);
    W_TEST_INT(i3, 4);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsNaN")
  {
    if (WMath::SupportsNaN<Type>())
    {
      WPlaneType p;

      p = WPlaneType::MakeInvalid();
      W_TEST_BOOL(!p.IsNaN());

      p = WPlaneType::MakeInvalid();
      p.m_fNegDistance = WMath::NaN<Type>();
      W_TEST_BOOL(p.IsNaN());

      p = WPlaneType::MakeInvalid();
      p.m_vNormal.x = WMath::NaN<Type>();
      W_TEST_BOOL(p.IsNaN());

      p = WPlaneType::MakeInvalid();
      p.m_vNormal.y = WMath::NaN<Type>();
      W_TEST_BOOL(p.IsNaN());

      p = WPlaneType::MakeInvalid();
      p.m_vNormal.z = WMath::NaN<Type>();
      W_TEST_BOOL(p.IsNaN());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsFinite")
  {
    if (WMath::SupportsInfinity<Type>())
    {
      WPlaneType p;

      p.m_vNormal = WVec3Type(1, 2, 3).GetNormalized();
      p.m_fNegDistance = (Type)42;
      W_TEST_BOOL(p.IsValid());
      W_TEST_BOOL(p.IsFinite());

      p = WPlaneType::MakeInvalid();
      p.m_vNormal = WVec3Type(1, 2, 3).GetNormalized();
      p.m_fNegDistance = WMath::Infinity<Type>();
      W_TEST_BOOL(p.IsValid());
      W_TEST_BOOL(!p.IsFinite());

      p = WPlaneType::MakeInvalid();
      p.m_vNormal.x = WMath::NaN<Type>();
      p.m_fNegDistance = WMath::Infinity<Type>();
      W_TEST_BOOL(!p.IsValid());
      W_TEST_BOOL(!p.IsFinite());

      p = WPlaneType::MakeInvalid();
      p.m_vNormal = WVec3Type(1, 2, 3);
      p.m_fNegDistance = (Type)42;
      W_TEST_BOOL(!p.IsValid());
      W_TEST_BOOL(p.IsFinite());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetMinimumDistanceTo/GetMaximumDistanceTo")
  {
    const WUInt32 numTestLoops = 1000 * 1000;

    WRandom randomGenerator;
    randomGenerator.Initialize(0x83482343);

    const auto randomNonZeroVec3T = [&randomGenerator]() -> WVec3Type
    {
      const Type extent = (Type)1000.0;
      const WVec3Type v(randomGenerator.FloatMinMax(-extent, extent), randomGenerator.FloatMinMax(-extent, extent), randomGenerator.FloatMinMax(-extent, extent));
      return v.GetLength() > (Type)0.001 ? v : WVec3Type::MakeAxisX();
    };

    for (WUInt32 loopIndex = 0; loopIndex < numTestLoops; ++loopIndex)
    {
      const WPlaneType plane = WPlaneType::MakeFromNormalAndPoint(randomNonZeroVec3T().GetNormalized(), randomNonZeroVec3T());

      WVec3Type boxCorners[8];
      WBoundingBoxType box;
      {
        const WVec3Type boxPoint0 = randomNonZeroVec3T();
        const WVec3Type boxPoint1 = randomNonZeroVec3T();
        const WVec3Type boxMins(WMath::Min(boxPoint0.x, boxPoint1.x), WMath::Min(boxPoint0.y, boxPoint1.y), WMath::Min(boxPoint0.z, boxPoint1.z));
        const WVec3Type boxMaxs(WMath::Max(boxPoint0.x, boxPoint1.x), WMath::Max(boxPoint0.y, boxPoint1.y), WMath::Max(boxPoint0.z, boxPoint1.z));
        box = WBoundingBoxType::MakeFromMinMax(boxMins, boxMaxs);
        box.GetCorners(boxCorners);
      }

      Type distanceMin;
      Type distanceMax;
      {
        distanceMin = plane.GetMinimumDistanceTo(box);
        distanceMax = plane.GetMaximumDistanceTo(box);
      }

      Type referenceDistanceMin = WMath::MaxValue<Type>();
      Type referenceDistanceMax = -WMath::MaxValue<Type>();
      {
        for (WUInt32 cornerIndex = 0; cornerIndex < W_ARRAY_SIZE(boxCorners); ++cornerIndex)
        {
          const Type cornerDist = plane.GetDistanceTo(boxCorners[cornerIndex]);
          referenceDistanceMin = WMath::Min(referenceDistanceMin, cornerDist);
          referenceDistanceMax = WMath::Max(referenceDistanceMax, cornerDist);
        }
      }

      // Break at first error to not spam the log with other potential error (the loop here is very long)
      {
        bool currIterSucceeded = true;
        currIterSucceeded = currIterSucceeded && W_TEST_FLOAT(distanceMin, referenceDistanceMin, (Type)0.0001);
        currIterSucceeded = currIterSucceeded && W_TEST_FLOAT(distanceMax, referenceDistanceMax, (Type)0.0001);
        if (!currIterSucceeded)
        {
          break;
        }
      }
    }
  }
}

W_CREATE_SIMPLE_TEST(Math, Planef)
{
  TestPlane<float>();
}
W_CREATE_SIMPLE_TEST(Math, Planed)
{
  TestPlane<double>();
}
