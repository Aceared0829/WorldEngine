#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Vec4.h>

template <typename Type>
void TestVec4()
{
  using WVec4Type = WVec4Template<Type>;
  using WVec2Type = WVec2Template<Type>;
  using WVec3Type = WVec3Template<Type>;

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor")
  {
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
    if (WMath::SupportsNaN<Type>())
    {
      // In debug the default constructor initializes everything with NaN.
      WVec4Type vDefCtor;
      W_TEST_BOOL(WMath::IsNaN(vDefCtor.x) && WMath::IsNaN(vDefCtor.y) /* && WMath::IsNaN(vDefCtor.z) && WMath::IsNaN(vDefCtor.w)*/);
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    Type testBlock[4] = {
      (Type)1, (Type)2, (Type)3, (Type)4};
    WVec4Type* pDefCtor = ::new ((void*)&testBlock[0]) WVec4Type;
    W_TEST_BOOL(pDefCtor->x == (Type)1 && pDefCtor->y == (Type)2 && pDefCtor->z == (Type)3 &&
                 pDefCtor->w == (Type)4);
#endif

    // Make sure the class didn't accidentally change in size.
    W_TEST_BOOL(sizeof(WVec4) == 16);
    W_TEST_BOOL(sizeof(WVec4d) == 32);

    WVec4Type vInit1F(2.0f);
    W_TEST_BOOL(vInit1F.x == 2.0f && vInit1F.y == 2.0f && vInit1F.z == 2.0f && vInit1F.w == 2.0f);

    WVec4Type vInit4F(1.0f, 2.0f, 3.0f, 4.0f);
    W_TEST_BOOL(vInit4F.x == 1.0f && vInit4F.y == 2.0f && vInit4F.z == 3.0f && vInit4F.w == 4.0f);

    WVec4Type vCopy(vInit4F);
    W_TEST_BOOL(vCopy.x == 1.0f && vCopy.y == 2.0f && vCopy.z == 3.0f && vCopy.w == 4.0f);

    WVec4Type vZero = WVec4Type::MakeZero();
    W_TEST_BOOL(vZero.x == 0.0f && vZero.y == 0.0f && vZero.z == 0.0f && vZero.w == 0.0f);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Conversion")
  {
    WVec4Type vData(1.0f, 2.0f, 3.0f, 4.0f);
    WVec2Type vToVec2 = vData.GetAsVec2();
    W_TEST_BOOL(vToVec2.x == vData.x && vToVec2.y == vData.y);

    WVec3Type vToVec3 = vData.GetAsVec3();
    W_TEST_BOOL(vToVec3.x == vData.x && vToVec3.y == vData.y && vToVec3.z == vData.z);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Setter")
  {
    WVec4Type vSet1F;
    vSet1F.Set(2.0f);
    W_TEST_BOOL(vSet1F.x == 2.0f && vSet1F.y == 2.0f && vSet1F.z == 2.0f && vSet1F.w == 2.0f);

    WVec4Type vSet4F;
    vSet4F.Set(1.0f, 2.0f, 3.0f, 4.0f);
    W_TEST_BOOL(vSet4F.x == 1.0f && vSet4F.y == 2.0f && vSet4F.z == 3.0f && vSet4F.w == 4.0f);

    WVec4Type vSetZero;
    vSetZero.SetZero();
    W_TEST_BOOL(vSetZero.x == 0.0f && vSetZero.y == 0.0f && vSetZero.z == 0.0f && vSetZero.w == 0.0f);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Length")
  {
    const WVec4Type vOp1((Type)-4.0, 4.0f, (Type)-2.0, (Type)-0.0);
    const WVec4Type compArray[4] = {
      WVec4Type(1.0f, 0.0f, 0.0f, 0.0f), WVec4Type(0.0f, 1.0f, 0.0f, 0.0f), WVec4Type(0.0f, 0.0f, 1.0f, 0.0f), WVec4Type(0.0f, 0.0f, 0.0f, 1.0f)};

    // GetLength
    W_TEST_FLOAT(vOp1.GetLength(), 6.0f, WMath::SmallEpsilon<Type>());

    // GetLengthSquared
    W_TEST_FLOAT(vOp1.GetLengthSquared(), 36.0f, WMath::SmallEpsilon<Type>());

    // GetLengthAndNormalize
    WVec4Type vLengthAndNorm = vOp1;
    Type fLength = vLengthAndNorm.GetLengthAndNormalize();
    W_TEST_FLOAT(vLengthAndNorm.GetLength(), 1.0f, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(fLength, 6.0f, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(vLengthAndNorm.x * vLengthAndNorm.x + vLengthAndNorm.y * vLengthAndNorm.y + vLengthAndNorm.z * vLengthAndNorm.z +
                    vLengthAndNorm.w * vLengthAndNorm.w,
      1.0f, WMath::SmallEpsilon<Type>());
    W_TEST_BOOL(vLengthAndNorm.IsNormalized(WMath::SmallEpsilon<Type>()));

    // GetNormalized
    WVec4Type vGetNorm = vOp1.GetNormalized();
    W_TEST_FLOAT(vGetNorm.x * vGetNorm.x + vGetNorm.y * vGetNorm.y + vGetNorm.z * vGetNorm.z + vGetNorm.w * vGetNorm.w, 1.0f,
      WMath::SmallEpsilon<Type>());
    W_TEST_BOOL(vGetNorm.IsNormalized(WMath::SmallEpsilon<Type>()));

    // Normalize
    WVec4Type vNorm = vOp1;
    vNorm.Normalize();
    W_TEST_FLOAT(vNorm.x * vNorm.x + vNorm.y * vNorm.y + vNorm.z * vNorm.z + vNorm.w * vNorm.w, 1.0f, WMath::SmallEpsilon<Type>());
    W_TEST_BOOL(vNorm.IsNormalized(WMath::SmallEpsilon<Type>()));

    // NormalizeIfNotZero
    WVec4Type vNormCond = vNorm * WMath::DefaultEpsilon<Type>();
    W_TEST_BOOL(vNormCond.NormalizeIfNotZero(vOp1, WMath::LargeEpsilon<Type>()) == W_FAILURE);
    W_TEST_BOOL(vNormCond == vOp1);
    vNormCond = vNorm * WMath::DefaultEpsilon<Type>();
    W_TEST_BOOL(vNormCond.NormalizeIfNotZero(vOp1, WMath::SmallEpsilon<Type>()) == W_SUCCESS);
    W_TEST_VEC4(vNormCond, vNorm, WMath::DefaultEpsilon<Type>());

    // IsZero
    W_TEST_BOOL(WVec4Type::MakeZero().IsZero());
    for (int i = 0; i < 4; ++i)
    {
      W_TEST_BOOL(!compArray[i].IsZero());
    }

    // IsZero(float)
    W_TEST_BOOL(WVec4Type::MakeZero().IsZero(0.0f));
    for (int i = 0; i < 4; ++i)
    {
      W_TEST_BOOL(!compArray[i].IsZero(0.0f));
      W_TEST_BOOL(compArray[i].IsZero(1.0f));
      W_TEST_BOOL((-compArray[i]).IsZero(1.0f));
    }

    // IsNormalized (already tested above)
    for (int i = 0; i < 4; ++i)
    {
      W_TEST_BOOL(compArray[i].IsNormalized());
      W_TEST_BOOL((-compArray[i]).IsNormalized());
      W_TEST_BOOL((compArray[i] * (Type)2).IsNormalized((Type)4));
      W_TEST_BOOL((compArray[i] * (Type)2).IsNormalized((Type)4));
    }

    if (WMath::SupportsNaN<Type>())
    {
      Type TypeNaN = WMath::NaN<Type>();
      const WVec4Type nanArray[4] = {WVec4Type(TypeNaN, 0.0f, 0.0f, 0.0f), WVec4Type(0.0f, TypeNaN, 0.0f, 0.0f), WVec4Type(0.0f, 0.0f, TypeNaN, 0.0f),
        WVec4Type(0.0f, 0.0f, 0.0f, TypeNaN)};

      // IsNaN
      for (int i = 0; i < 4; ++i)
      {
        W_TEST_BOOL(nanArray[i].IsNaN());
        W_TEST_BOOL(!compArray[i].IsNaN());
      }

      // IsValid
      for (int i = 0; i < 4; ++i)
      {
        W_TEST_BOOL(!nanArray[i].IsValid());
        W_TEST_BOOL(compArray[i].IsValid());

        W_TEST_BOOL(!(compArray[i] * WMath::Infinity<Type>()).IsValid());
        W_TEST_BOOL(!(compArray[i] * -WMath::Infinity<Type>()).IsValid());
      }
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Operators")
  {
    const WVec4Type vOp1((Type)-4.0, (Type)0.2, (Type)-7.0, (Type)-0.0);
    const WVec4Type vOp2((Type)2.0, (Type)0.3, (Type)0.0, (Type)1.0);
    const WVec4Type compArray[4] = {
      WVec4Type((Type)1.0, (Type)0.0, (Type)0.0, (Type)0.0), WVec4Type((Type)0.0, (Type)1.0, (Type)0.0, (Type)0.0), WVec4Type((Type)0.0, (Type)0.0, (Type)1.0, (Type)0.0), WVec4Type((Type)0.0, (Type)0.0, (Type)0.0, (Type)1.0)};
    // IsIdentical
    W_TEST_BOOL(vOp1.IsIdentical(vOp1));
    for (int i = 0; i < 4; ++i)
    {
      W_TEST_BOOL(!vOp1.IsIdentical(vOp1 + (Type)WMath::SmallEpsilon<Type>() * compArray[i]));
      W_TEST_BOOL(!vOp1.IsIdentical(vOp1 - (Type)WMath::SmallEpsilon<Type>() * compArray[i]));
    }

    // IsEqual
    W_TEST_BOOL(vOp1.IsEqual(vOp1, (Type)0.0));
    for (int i = 0; i < 4; ++i)
    {
      W_TEST_BOOL(vOp1.IsEqual(vOp1 + WMath::SmallEpsilon<Type>() * compArray[i], 2 * WMath::SmallEpsilon<Type>()));
      W_TEST_BOOL(vOp1.IsEqual(vOp1 - WMath::SmallEpsilon<Type>() * compArray[i], 2 * WMath::SmallEpsilon<Type>()));
      W_TEST_BOOL(vOp1.IsEqual(vOp1 + WMath::DefaultEpsilon<Type>() * compArray[i], 2 * WMath::DefaultEpsilon<Type>()));
      W_TEST_BOOL(vOp1.IsEqual(vOp1 - WMath::DefaultEpsilon<Type>() * compArray[i], 2 * WMath::DefaultEpsilon<Type>()));
    }

    // operator-
    WVec4Type vNegated = -vOp1;
    W_TEST_BOOL(vOp1.x == -vNegated.x && vOp1.y == -vNegated.y && vOp1.z == -vNegated.z && vOp1.w == -vNegated.w);

    // operator+= (WVec4Type)
    WVec4Type vPlusAssign = vOp1;
    vPlusAssign += vOp2;
    W_TEST_BOOL(vPlusAssign.IsEqual(WVec4Type((Type)-2.0, (Type)0.5, (Type)-7.0, (Type)1.0), WMath::SmallEpsilon<Type>()));

    // operator-= (WVec4Type)
    WVec4Type vMinusAssign = vOp1;
    vMinusAssign -= vOp2;
    W_TEST_BOOL(vMinusAssign.IsEqual(WVec4Type((Type)-6.0, (Type)-0.1, (Type)-7.0, (Type)-1.0), WMath::SmallEpsilon<Type>()));

    // operator*= (float)
    WVec4Type vMulFloat = vOp1;
    vMulFloat *= 2.0f;
    W_TEST_BOOL(vMulFloat.IsEqual(WVec4Type((Type)-8.0, (Type)0.4, (Type)-14.0, (Type)-0.0), WMath::SmallEpsilon<Type>()));
    vMulFloat *= 0.0f;
    W_TEST_BOOL(vMulFloat.IsEqual(WVec4Type::MakeZero(), WMath::SmallEpsilon<Type>()));

    // operator/= (float)
    WVec4Type vDivFloat = vOp1;
    vDivFloat /= 2.0f;
    W_TEST_BOOL(vDivFloat.IsEqual(WVec4Type((Type)-2.0, (Type)0.1, (Type)-3.5, (Type)-0.0), WMath::SmallEpsilon<Type>()));

    // operator+ (WVec4Type, WVec4Type)
    WVec4Type vPlus = (vOp1 + vOp2);
    W_TEST_BOOL(vPlus.IsEqual(WVec4Type((Type)-2.0, (Type)0.5, (Type)-7.0, (Type)1.0), WMath::SmallEpsilon<Type>()));

    // operator- (WVec4Type, WVec4Type)
    WVec4Type vMinus = (vOp1 - vOp2);
    W_TEST_BOOL(vMinus.IsEqual(WVec4Type((Type)-6.0, (Type)-0.1, (Type)-7.0, (Type)-1.0), WMath::SmallEpsilon<Type>()));

    // operator* (float, WVec4Type)
    WVec4Type vMulFloatVec4 = ((Type)2 * vOp1);
    W_TEST_BOOL(vMulFloatVec4.IsEqual(WVec4Type((Type)-8.0, (Type)0.4, (Type)-14.0, (Type)-0.0), WMath::SmallEpsilon<Type>()));
    vMulFloatVec4 = ((Type)0 * vOp1);
    W_TEST_BOOL(vMulFloatVec4.IsEqual(WVec4Type::MakeZero(), WMath::SmallEpsilon<Type>()));

    // operator* (WVec4Type, float)
    WVec4Type vMulVec4Float = (vOp1 * (Type)2);
    W_TEST_BOOL(vMulVec4Float.IsEqual(WVec4Type((Type)-8.0, (Type)0.4, (Type)-14.0, (Type)-0.0), WMath::SmallEpsilon<Type>()));
    vMulVec4Float = (vOp1 * (Type)0);
    W_TEST_BOOL(vMulVec4Float.IsEqual(WVec4Type::MakeZero(), WMath::SmallEpsilon<Type>()));

    // operator/ (WVec4Type, float)
    WVec4Type vDivVec4Float = (vOp1 / (Type)2);
    W_TEST_BOOL(vDivVec4Float.IsEqual(WVec4Type((Type)-2.0, (Type)0.1, (Type)-3.5, (Type)-0.0), WMath::SmallEpsilon<Type>()));

    // operator== (WVec4Type, WVec4Type)
    W_TEST_BOOL(vOp1 == vOp1);
    for (int i = 0; i < 4; ++i)
    {
      W_TEST_BOOL(!(vOp1 == (vOp1 + (Type)WMath::SmallEpsilon<Type>() * compArray[i])));
      W_TEST_BOOL(!(vOp1 == (vOp1 - (Type)WMath::SmallEpsilon<Type>() * compArray[i])));
    }

    // operator!= (WVec4Type, WVec4Type)
    W_TEST_BOOL(!(vOp1 != vOp1));
    for (int i = 0; i < 4; ++i)
    {
      W_TEST_BOOL(vOp1 != (vOp1 + (Type)WMath::SmallEpsilon<Type>() * compArray[i]));
      W_TEST_BOOL(vOp1 != (vOp1 - (Type)WMath::SmallEpsilon<Type>() * compArray[i]));
    }

    // operator< (WVec4Type, WVec4Type)
    for (int i = 0; i < 4; ++i)
    {
      for (int j = 0; j < 4; ++j)
      {
        if (i == j)
        {
          W_TEST_BOOL(!(compArray[i] < compArray[j]));
          W_TEST_BOOL(!(compArray[j] < compArray[i]));
        }
        else if (i < j)
        {
          W_TEST_BOOL(!(compArray[i] < compArray[j]));
          W_TEST_BOOL(compArray[j] < compArray[i]);
        }
        else
        {
          W_TEST_BOOL(!(compArray[j] < compArray[i]));
          W_TEST_BOOL(compArray[i] < compArray[j]);
        }
      }
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Common")
  {
    const WVec4Type vOp1((Type)-4.0, (Type)0.2, (Type)-7.0, (Type)-0.0);
    const WVec4Type vOp2((Type)2.0, (Type)-0.3, (Type)0.5, (Type)1.0);

    // Dot
    W_TEST_FLOAT(vOp1.Dot(vOp2), (Type)-11.56, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(vOp2.Dot(vOp1), (Type)-11.56, WMath::SmallEpsilon<Type>());

    // CompMin
    W_TEST_BOOL(vOp1.CompMin(vOp2).IsEqual(WVec4Type((Type)-4.0, (Type)-0.3, (Type)-7.0, (Type)-0.0), WMath::SmallEpsilon<Type>()));
    W_TEST_BOOL(vOp2.CompMin(vOp1).IsEqual(WVec4Type((Type)-4.0, (Type)-0.3, (Type)-7.0, (Type)-0.0), WMath::SmallEpsilon<Type>()));

    // CompMax
    W_TEST_BOOL(vOp1.CompMax(vOp2).IsEqual(WVec4Type((Type)2.0, (Type)0.2, (Type)0.5, (Type)1.0), WMath::SmallEpsilon<Type>()));
    W_TEST_BOOL(vOp2.CompMax(vOp1).IsEqual(WVec4Type((Type)2.0, (Type)0.2, (Type)0.5, (Type)1.0), WMath::SmallEpsilon<Type>()));

    // CompClamp
    W_TEST_BOOL(vOp1.CompClamp(vOp1, vOp2).IsEqual(WVec4Type((Type)-4.0, (Type)-0.3, (Type)-7.0, (Type)-0.0), WMath::SmallEpsilon<Type>()));
    W_TEST_BOOL(vOp2.CompClamp(vOp1, vOp2).IsEqual(WVec4Type((Type)2.0, (Type)0.2, (Type)0.5, (Type)1.0), WMath::SmallEpsilon<Type>()));

    // CompMul
    W_TEST_BOOL(vOp1.CompMul(vOp2).IsEqual(WVec4Type((Type)-8.0, (Type)-0.06, (Type)-3.5, (Type)0.0), WMath::SmallEpsilon<Type>()));
    W_TEST_BOOL(vOp2.CompMul(vOp1).IsEqual(WVec4Type((Type)-8.0, (Type)-0.06, (Type)-3.5, (Type)0.0), WMath::SmallEpsilon<Type>()));

    // CompDiv
    W_TEST_BOOL(vOp1.CompDiv(vOp2).IsEqual(WVec4Type((Type)-2.0, (Type)(-2.0/3.0), (Type)-14.0, (Type)0.0), WMath::SmallEpsilon<Type>()));

    // Abs
    W_TEST_VEC4(vOp1.Abs(), WVec4Type((Type)4.0, (Type)0.2, (Type)7.0, (Type)0.0), WMath::SmallEpsilon<Type>());
  }
}


W_CREATE_SIMPLE_TEST(Math, Vec4f)
{
  TestVec4<float>();
}
W_CREATE_SIMPLE_TEST(Math, Vec4d)
{
  TestVec4<double>();
}
