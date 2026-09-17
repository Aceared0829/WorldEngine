#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Vec4.h>

#include <Foundation/Math/FixedPoint.h>

template <typename Type>
void TestVec2()
{
  using WVec2Type = WVec2Template<Type>;
  using WVec3Type = WVec3Template<Type>;
  using WVec4Type = WVec4Template<Type>;

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor")
  {
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
    if (WMath::SupportsNaN<Type>())
    {
      // In debug the default constructor initializes everything with NaN.
      WVec2Type vDefCtor;
      W_TEST_BOOL(WMath::IsNaN(vDefCtor.x) && WMath::IsNaN(vDefCtor.y));
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    Type testBlock[2] = {(Type)1, (Type)2};
    WVec2Type* pDefCtor = ::new ((void*)&testBlock[0]) WVec2Type;
    W_TEST_BOOL(pDefCtor->x == (Type)1 && pDefCtor->y == (Type)2);
#endif
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor(x,y)")
  {
    WVec2Type v(1, 2);
    W_TEST_FLOAT(v.x, 1, 0);
    W_TEST_FLOAT(v.y, 2, 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor(xy)")
  {
    WVec2Type v(3);
    W_TEST_VEC2(v, WVec2Type(3, 3), 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeZero")
  {
    W_TEST_VEC2(WVec2Type::MakeZero(), WVec2Type(0, 0), 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetAsVec3")
  {
    W_TEST_VEC3(WVec2Type(2, 3).GetAsVec3(4), WVec3Type(2, 3, 4), 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetAsVec4")
  {
    W_TEST_VEC4(WVec2Type(2, 3).GetAsVec4(4, 5), WVec4Type(2, 3, 4, 5), 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Set(x, y)")
  {
    WVec2Type v;
    v.Set(2, 3);

    W_TEST_FLOAT(v.x, 2, 0);
    W_TEST_FLOAT(v.y, 3, 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Set(xy)")
  {
    WVec2Type v;
    v.Set(4);

    W_TEST_FLOAT(v.x, 4, 0);
    W_TEST_FLOAT(v.y, 4, 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SetZero")
  {
    WVec2Type v;
    v.Set(4);
    v.SetZero();

    W_TEST_FLOAT(v.x, 0, 0);
    W_TEST_FLOAT(v.y, 0, 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetLength")
  {
    WVec2Type v(0);
    W_TEST_FLOAT(v.GetLength(), 0, (Type)0.0001);

    v.Set(1, 0);
    W_TEST_FLOAT(v.GetLength(), 1, (Type)0.0001);

    v.Set(0, 1);
    W_TEST_FLOAT(v.GetLength(), 1, (Type)0.0001);

    v.Set(2, 3);
    W_TEST_FLOAT(v.GetLength(), WMath::Sqrt((Type)(4 + 9)), (Type)0.0001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetLengthSquared")
  {
    WVec2Type v(0);
    W_TEST_FLOAT(v.GetLengthSquared(), 0, (Type)0.0001);

    v.Set(1, 0);
    W_TEST_FLOAT(v.GetLengthSquared(), 1, (Type)0.0001);

    v.Set(0, 1);
    W_TEST_FLOAT(v.GetLengthSquared(), 1, (Type)0.0001);

    v.Set(2, 3);
    W_TEST_FLOAT(v.GetLengthSquared(), 4 + 9, (Type)0.0001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetLengthAndNormalize")
  {
    WVec2Type v(0.5f, 0);
    Type l = v.GetLengthAndNormalize();
    W_TEST_FLOAT(l, 0.5f, (Type)0.0001);
    W_TEST_FLOAT(v.GetLength(), 1, (Type)0.0001);

    v.Set(1, 0);
    l = v.GetLengthAndNormalize();
    W_TEST_FLOAT(l, 1, (Type)0.0001);
    W_TEST_FLOAT(v.GetLength(), 1, (Type)0.0001);

    v.Set(0, 1);
    l = v.GetLengthAndNormalize();
    W_TEST_FLOAT(l, 1, (Type)0.0001);
    W_TEST_FLOAT(v.GetLength(), 1, (Type)0.0001);

    v.Set(2, 3);
    l = v.GetLengthAndNormalize();
    W_TEST_FLOAT(l, WMath::Sqrt((Type)(4 + 9)), (Type)0.0001);
    W_TEST_FLOAT(v.GetLength(), 1, (Type)0.0001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetNormalized")
  {
    WVec2Type v;

    v.Set(10, 0);
    W_TEST_VEC2(v.GetNormalized(), WVec2Type(1, 0), (Type)0.001);

    v.Set(0, 10);
    W_TEST_VEC2(v.GetNormalized(), WVec2Type(0, 1), (Type)0.001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Normalize")
  {
    WVec2Type v;

    v.Set(10, 0);
    v.Normalize();
    W_TEST_VEC2(v, WVec2Type(1, 0), (Type)0.001);

    v.Set(0, 10);
    v.Normalize();
    W_TEST_VEC2(v, WVec2Type(0, 1), (Type)0.001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "NormalizeIfNotZero")
  {
    WVec2Type v;

    v.Set(10, 0);
    W_TEST_BOOL(v.NormalizeIfNotZero() == W_SUCCESS);
    W_TEST_VEC2(v, WVec2Type(1, 0), (Type)0.001);

    v.Set(0, 10);
    W_TEST_BOOL(v.NormalizeIfNotZero() == W_SUCCESS);
    W_TEST_VEC2(v, WVec2Type(0, 1), (Type)0.001);

    v.SetZero();
    W_TEST_BOOL(v.NormalizeIfNotZero() == W_FAILURE);
    W_TEST_VEC2(v, WVec2Type(1, 0), (Type)0.001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsZero")
  {
    WVec2Type v;

    v.Set(1);
    W_TEST_BOOL(v.IsZero() == false);

    v.Set(0.001f);
    W_TEST_BOOL(v.IsZero() == false);
    W_TEST_BOOL(v.IsZero(0.01f) == true);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsNormalized")
  {
    WVec2Type v;

    v.SetZero();
    W_TEST_BOOL(v.IsNormalized(WMath::HugeEpsilon<Type>()) == false);

    v.Set(1, 0);
    W_TEST_BOOL(v.IsNormalized(WMath::HugeEpsilon<Type>()) == true);

    v.Set(0, 1);
    W_TEST_BOOL(v.IsNormalized(WMath::HugeEpsilon<Type>()) == true);

    v.Set(0.1f, 1);
    W_TEST_BOOL(v.IsNormalized(WMath::DefaultEpsilon<Type>()) == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsNaN")
  {
    if (WMath::SupportsNaN<Type>())
    {
      WVec2Type v(0);

      W_TEST_BOOL(!v.IsNaN());

      v.x = WMath::NaN<Type>();
      W_TEST_BOOL(v.IsNaN());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsValid")
  {
    if (WMath::SupportsNaN<Type>())
    {
      WVec2Type v(0);

      W_TEST_BOOL(v.IsValid());

      v.x = WMath::NaN<Type>();
      W_TEST_BOOL(!v.IsValid());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator-")
  {
    WVec2Type v(1);

    W_TEST_VEC2(-v, WVec2Type(-1), (Type)0.0001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator+=")
  {
    WVec2Type v(1, 2);

    v += WVec2Type(3, 4);
    W_TEST_VEC2(v, WVec2Type(4, 6), (Type)0.0001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator-=")
  {
    WVec2Type v(1, 2);

    v -= WVec2Type(3, 5);
    W_TEST_VEC2(v, WVec2Type(-2, -3), (Type)0.0001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator*=(float)")
  {
    WVec2Type v(1, 2);

    v *= 3;
    W_TEST_VEC2(v, WVec2Type(3, 6), (Type)0.0001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator/=(float)")
  {
    WVec2Type v(1, 2);

    v /= 2;
    W_TEST_VEC2(v, WVec2Type(0.5f, 1), (Type)0.0001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsIdentical")
  {
    WVec2Type v1(1, 2);
    WVec2Type v2 = v1;

    W_TEST_BOOL(v1.IsIdentical(v2));

    v2.x += (Type)0.001;
    W_TEST_BOOL(!v1.IsIdentical(v2));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsEqual")
  {
    WVec2Type v1(1, 2);
    WVec2Type v2 = v1;

    W_TEST_BOOL(v1.IsEqual(v2, WMath::DefaultEpsilon<Type>()));

    v2.x += (Type)0.001;
    W_TEST_BOOL(!v1.IsEqual(v2, (Type)0.0001));
    W_TEST_BOOL(v1.IsEqual(v2, (Type)0.01));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetAngleBetween")
  {
    WVec2Type v1(1, 0);
    WVec2Type v2(0, 1);

    W_TEST_FLOAT(v1.GetAngleBetween(v1).GetDegree(), 0, (Type)0.001);
    W_TEST_FLOAT(v2.GetAngleBetween(v2).GetDegree(), 0, (Type)0.001);
    W_TEST_FLOAT(v1.GetAngleBetween(v2).GetDegree(), 90, (Type)0.001);
    W_TEST_FLOAT(v1.GetAngleBetween(-v1).GetDegree(), 180, (Type)0.001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Dot")
  {
    WVec2Type v1(1, 0);
    WVec2Type v2(0, 1);
    WVec2Type v0(0, 0);

    W_TEST_FLOAT(v0.Dot(v0), 0, (Type)0.001);
    W_TEST_FLOAT(v1.Dot(v1), 1, (Type)0.001);
    W_TEST_FLOAT(v2.Dot(v2), 1, (Type)0.001);
    W_TEST_FLOAT(v1.Dot(v2), 0, (Type)0.001);
    W_TEST_FLOAT(v1.Dot(-v1), -1, (Type)0.001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CompMin")
  {
    WVec2Type v1(2, 3);
    WVec2Type v2 = v1.CompMin(WVec2Type(1, 4));
    W_TEST_VEC2(v2, WVec2Type(1, 3), 0);

    v2 = v1.CompMin(WVec2Type(3, 1));
    W_TEST_VEC2(v2, WVec2Type(2, 1), 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CompMax")
  {
    WVec2Type v1(2, 3.5f);
    WVec2Type v2 = v1.CompMax(WVec2Type(1, 4));
    W_TEST_VEC2(v2, WVec2Type(2, 4), 0);

    v2 = v1.CompMax(WVec2Type(3, 1));
    W_TEST_VEC2(v2, WVec2Type(3, 3.5f), 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CompClamp")
  {
    const WVec2Type vOp1(-4.0, 0.2f);
    const WVec2Type vOp2(2.0, -0.3f);

    W_TEST_BOOL(vOp1.CompClamp(vOp1, vOp2).IsEqual(WVec2Type(-4.0f, -0.3f), WMath::SmallEpsilon<Type>()));
    W_TEST_BOOL(vOp2.CompClamp(vOp1, vOp2).IsEqual(WVec2Type(2.0f, 0.2f), WMath::SmallEpsilon<Type>()));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CompMul")
  {
    WVec2Type v1(2, 3);
    WVec2Type v2 = v1.CompMul(WVec2Type(2, 4));
    W_TEST_VEC2(v2, WVec2Type(4, 12), 0);

    v2 = v1.CompMul(WVec2Type(3, 7));
    W_TEST_VEC2(v2, WVec2Type(6, 21), 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CompDiv")
  {
    WVec2Type v1(12, 32);
    WVec2Type v2 = v1.CompDiv(WVec2Type(3, 4));
    W_TEST_VEC2(v2, WVec2Type(4, 8), 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Abs")
  {
    WVec2Type v1(-5, 7);
    WVec2Type v2 = v1.Abs();
    W_TEST_VEC2(v2, WVec2Type(5, 7), 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeOrthogonalTo")
  {
    WVec2Type v;

    v.Set(1, 1);
    v.MakeOrthogonalTo(WVec2Type(1, 0));
    W_TEST_VEC2(v, WVec2Type(0, 1), (Type)0.001);

    v.Set(1, 1);
    v.MakeOrthogonalTo(WVec2Type(0, 1));
    W_TEST_VEC2(v, WVec2Type(1, 0), (Type)0.001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetOrthogonalVector")
  {
    WVec2Type v;

    for (Type i = 1; i < 360; i += 3)
    {
      v.Set(i, i * 3);
      W_TEST_FLOAT(v.GetOrthogonalVector().Dot(v), 0, (Type)0.001);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetReflectedVector")
  {
    WVec2Type v, v2;

    v.Set(1, 1);
    v2 = v.GetReflectedVector(WVec2Type(0, -1));
    W_TEST_VEC2(v2, WVec2Type(1, -1), (Type)0.0001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator+")
  {
    WVec2Type v = WVec2Type(1, 2) + WVec2Type(3, 4);
    W_TEST_VEC2(v, WVec2Type(4, 6), (Type)0.0001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator-")
  {
    WVec2Type v = WVec2Type(1, 2) - WVec2Type(3, 5);
    W_TEST_VEC2(v, WVec2Type(-2, -3), (Type)0.0001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator* (vec, float) | operator* (float, vec)")
  {
    WVec2Type v = WVec2Type(1, 2) * (Type)3;
    W_TEST_VEC2(v, WVec2Type(3, 6), (Type)0.0001);

    v = (Type)7 * WVec2Type(1, 2);
    W_TEST_VEC2(v, WVec2Type(7, 14), (Type)0.0001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator/ (vec, float)")
  {
    WVec2Type v = WVec2Type(2, 4) / (Type)2;
    W_TEST_VEC2(v, WVec2Type(1, 2), (Type)0.0001);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "operator== | operator!=")
  {
    WVec2Type v1(1, 2);
    WVec2Type v2 = v1;

    W_TEST_BOOL(v1 == v2);

    v2.x += (Type)0.001;
    W_TEST_BOOL(v1 != v2);
  }
}


W_CREATE_SIMPLE_TEST(Math, Vec2f)
{
  TestVec2<float>();
}
W_CREATE_SIMPLE_TEST(Math, Vec2d)
{
  TestVec2<double>();
}
