#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Random.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Vec4.h>

template <typename Type>
void TestVec3()
{
  using WVec3Type = WVec3Template<Type>;
  using WVec2Type = WVec2Template<Type>;
  using WVec4Type = WVec4Template<Type>;

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor")
  {
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
    if (WMath::SupportsNaN<Type>())
    {
      // In debug the default constructor initializes everything with NaN.
      WVec3Type vDefCtor;
      W_TEST_BOOL(WMath::IsNaN(vDefCtor.x) && WMath::IsNaN(vDefCtor.y) && WMath::IsNaN(vDefCtor.z));
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    Type testBlock[3] = {(Type)1, (Type)2, (Type)3};
    WVec3Type* pDefCtor = ::new ((void*)&testBlock[0]) WVec3Type;
    W_TEST_BOOL(pDefCtor->x == (Type)1 && pDefCtor->y == (Type)2 && pDefCtor->z == (Type)3);
#endif

    // Make sure the class didn't accidentally change in size.
    W_TEST_BOOL(sizeof(WVec3) == 12);
    W_TEST_BOOL(sizeof(WVec3d) == 24);

    WVec3Type vInit1F(2.0f);
    W_TEST_BOOL(vInit1F.x == 2.0f && vInit1F.y == 2.0f && vInit1F.z == 2.0f);

    WVec3Type vInit4F(1.0f, 2.0f, 3.0f);
    W_TEST_BOOL(vInit4F.x == 1.0f && vInit4F.y == 2.0f && vInit4F.z == 3.0f);

    WVec3Type vCopy(vInit4F);
    W_TEST_BOOL(vCopy.x == 1.0f && vCopy.y == 2.0f && vCopy.z == 3.0f);

    WVec3Type vZero = WVec3Type::MakeZero();
    W_TEST_BOOL(vZero.x == 0.0f && vZero.y == 0.0f && vZero.z == 0.0f);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Conversion")
  {
    WVec3Type vData(1.0f, 2.0f, 3.0f);
    WVec2Type vToVec2 = vData.GetAsVec2();
    W_TEST_BOOL(vToVec2.x == vData.x && vToVec2.y == vData.y);

    WVec4Type vToVec4 = vData.GetAsVec4(42.0f);
    W_TEST_BOOL(vToVec4.x == vData.x && vToVec4.y == vData.y && vToVec4.z == vData.z && vToVec4.w == 42.0f);

    WVec4Type vToVec4Pos = vData.GetAsPositionVec4();
    W_TEST_BOOL(vToVec4Pos.x == vData.x && vToVec4Pos.y == vData.y && vToVec4Pos.z == vData.z && vToVec4Pos.w == 1.0f);

    WVec4Type vToVec4Dir = vData.GetAsDirectionVec4();
    W_TEST_BOOL(vToVec4Dir.x == vData.x && vToVec4Dir.y == vData.y && vToVec4Dir.z == vData.z && vToVec4Dir.w == 0.0f);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Setter")
  {
    WVec3Type vSet1F;
    vSet1F.Set(2.0f);
    W_TEST_BOOL(vSet1F.x == 2.0f && vSet1F.y == 2.0f && vSet1F.z == 2.0f);

    WVec3Type vSet4F;
    vSet4F.Set(1.0f, 2.0f, 3.0f);
    W_TEST_BOOL(vSet4F.x == 1.0f && vSet4F.y == 2.0f && vSet4F.z == 3.0f);

    WVec3Type vSetZero;
    vSetZero.SetZero();
    W_TEST_BOOL(vSetZero.x == 0.0f && vSetZero.y == 0.0f && vSetZero.z == 0.0f);
  }


  {
    const WVec3Type vOp1(-4.0, 4.0f, -2.0f);
    const WVec3Type compArray[3] = {WVec3Type(1.0f, 0.0f, 0.0f), WVec3Type(0.0f, 1.0f, 0.0f), WVec3Type(0.0f, 0.0f, 1.0f)};

    W_TEST_BLOCK(WTestBlock::Enabled, "GetLength")
    {
      W_TEST_FLOAT(vOp1.GetLength(), 6.0f, WMath::SmallEpsilon<Type>());
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "SetLength")
    {
      WVec3Type vSetLength = vOp1.GetNormalized() * WMath::DefaultEpsilon<Type>();
      W_TEST_BOOL(vSetLength.SetLength(4.0f, WMath::LargeEpsilon<Type>()) == W_FAILURE);
      W_TEST_BOOL(vSetLength == WVec3Type::MakeZero());
      vSetLength = vOp1.GetNormalized() * (Type)0.001;
      W_TEST_BOOL(vSetLength.SetLength(4.0f, (Type)WMath::DefaultEpsilon<Type>()) == W_SUCCESS);
      W_TEST_FLOAT(vSetLength.GetLength(), 4.0f, WMath::SmallEpsilon<Type>());
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "GetLengthSquared")
    {
      W_TEST_FLOAT(vOp1.GetLengthSquared(), 36.0f, WMath::SmallEpsilon<Type>());
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "GetLengthAndNormalize")
    {
      WVec3Type vLengthAndNorm = vOp1;
      Type fLength = vLengthAndNorm.GetLengthAndNormalize();
      W_TEST_FLOAT(vLengthAndNorm.GetLength(), 1.0f, WMath::SmallEpsilon<Type>());
      W_TEST_FLOAT(fLength, 6.0f, WMath::SmallEpsilon<Type>());
      W_TEST_FLOAT(vLengthAndNorm.x * vLengthAndNorm.x + vLengthAndNorm.y * vLengthAndNorm.y + vLengthAndNorm.z * vLengthAndNorm.z, 1.0f,
        WMath::SmallEpsilon<Type>());
      W_TEST_BOOL(vLengthAndNorm.IsNormalized(WMath::SmallEpsilon<Type>()));
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "GetNormalized")
    {
      WVec3Type vGetNorm = vOp1.GetNormalized();
      W_TEST_FLOAT(vGetNorm.x * vGetNorm.x + vGetNorm.y * vGetNorm.y + vGetNorm.z * vGetNorm.z, 1.0f, WMath::SmallEpsilon<Type>());
      W_TEST_BOOL(vGetNorm.IsNormalized(WMath::SmallEpsilon<Type>()));
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "Normalize")
    {
      WVec3Type vNorm = vOp1;
      vNorm.Normalize();
      W_TEST_FLOAT(vNorm.x * vNorm.x + vNorm.y * vNorm.y + vNorm.z * vNorm.z, 1.0f, WMath::SmallEpsilon<Type>());
      W_TEST_BOOL(vNorm.IsNormalized(WMath::SmallEpsilon<Type>()));
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "NormalizeIfNotZero")
    {
      WVec3Type vNorm = vOp1;
      vNorm.Normalize();

      WVec3Type vNormCond = vNorm * WMath::DefaultEpsilon<Type>();
      W_TEST_BOOL(vNormCond.NormalizeIfNotZero(vOp1, WMath::LargeEpsilon<Type>()) == W_FAILURE);
      W_TEST_BOOL(vNormCond == vOp1);
      vNormCond = vNorm * WMath::DefaultEpsilon<Type>();
      W_TEST_BOOL(vNormCond.NormalizeIfNotZero(vOp1, WMath::SmallEpsilon<Type>()) == W_SUCCESS);
      W_TEST_VEC3(vNormCond, vNorm, WMath::DefaultEpsilon<Type>());
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "IsZero")
    {
      W_TEST_BOOL(WVec3Type::MakeZero().IsZero());
      for (int i = 0; i < 3; ++i)
      {
        W_TEST_BOOL(!compArray[i].IsZero());
      }
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "IsZero(float)")
    {
      W_TEST_BOOL(WVec3Type::MakeZero().IsZero(0.0f));
      for (int i = 0; i < 3; ++i)
      {
        W_TEST_BOOL(!compArray[i].IsZero(0.0f));
        W_TEST_BOOL(compArray[i].IsZero(1.0f));
        W_TEST_BOOL((-compArray[i]).IsZero(1.0f));
      }
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "IsNormalized (2)")
    {
      for (int i = 0; i < 3; ++i)
      {
        W_TEST_BOOL(compArray[i].IsNormalized());
        W_TEST_BOOL((-compArray[i]).IsNormalized());
        W_TEST_BOOL((compArray[i] * (Type)2).IsNormalized((Type)4));
        W_TEST_BOOL((compArray[i] * (Type)2).IsNormalized((Type)4));
      }
    }

    if (WMath::SupportsNaN<Type>())
    {
      Type fNaN = WMath::NaN<Type>();
      const WVec3Type nanArray[3] = {WVec3Type(fNaN, 0.0f, 0.0f), WVec3Type(0.0f, fNaN, 0.0f), WVec3Type(0.0f, 0.0f, fNaN)};

      W_TEST_BLOCK(WTestBlock::Enabled, "IsNaN")
      {
        for (int i = 0; i < 3; ++i)
        {
          W_TEST_BOOL(nanArray[i].IsNaN());
          W_TEST_BOOL(!compArray[i].IsNaN());
        }
      }

      W_TEST_BLOCK(WTestBlock::Enabled, "IsValid")
      {
        for (int i = 0; i < 3; ++i)
        {
          W_TEST_BOOL(!nanArray[i].IsValid());
          W_TEST_BOOL(compArray[i].IsValid());

          W_TEST_BOOL(!(compArray[i] * WMath::Infinity<Type>()).IsValid());
          W_TEST_BOOL(!(compArray[i] * -WMath::Infinity<Type>()).IsValid());
        }
      }
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Operators")
  {
    const WVec3Type vOp1((Type)-4.0, (Type)0.2, (Type)-7.0);
    const WVec3Type vOp2((Type)2.0, (Type)0.3, (Type)0.0);
    const WVec3Type compArray[3] = {WVec3Type((Type)1.0, (Type)0.0, (Type)0.0), WVec3Type((Type)0.0, (Type)1.0, (Type)0.0), WVec3Type((Type)0.0, (Type)0.0, (Type)1.0)};
    // IsIdentical
    W_TEST_BOOL(vOp1.IsIdentical(vOp1));
    for (int i = 0; i < 3; ++i)
    {
      W_TEST_BOOL(!vOp1.IsIdentical(vOp1 + (Type)WMath::SmallEpsilon<Type>() * compArray[i]));
      W_TEST_BOOL(!vOp1.IsIdentical(vOp1 - (Type)WMath::SmallEpsilon<Type>() * compArray[i]));
    }

    // IsEqual
    W_TEST_BOOL(vOp1.IsEqual(vOp1, 0.0));
    for (int i = 0; i < 3; ++i)
    {
      W_TEST_BOOL(vOp1.IsEqual(vOp1 + WMath::SmallEpsilon<Type>() * compArray[i], 2 * WMath::SmallEpsilon<Type>()));
      W_TEST_BOOL(vOp1.IsEqual(vOp1 - WMath::SmallEpsilon<Type>() * compArray[i], 2 * WMath::SmallEpsilon<Type>()));
      W_TEST_BOOL(vOp1.IsEqual(vOp1 + WMath::DefaultEpsilon<Type>() * compArray[i], 2 * WMath::DefaultEpsilon<Type>()));
      W_TEST_BOOL(vOp1.IsEqual(vOp1 - WMath::DefaultEpsilon<Type>() * compArray[i], 2 * WMath::DefaultEpsilon<Type>()));
    }

    // operator-
    WVec3Type vNegated = -vOp1;
    W_TEST_BOOL(vOp1.x == -vNegated.x && vOp1.y == -vNegated.y && vOp1.z == -vNegated.z);

    // operator+= (WVec3Type)
    WVec3Type vPlusAssign = vOp1;
    vPlusAssign += vOp2;
    W_TEST_BOOL(vPlusAssign.IsEqual(WVec3Type((Type)-2.0, (Type)0.5, (Type)-7.0), WMath::SmallEpsilon<Type>()));

    // operator-= (WVec3Type)
    WVec3Type vMinusAssign = vOp1;
    vMinusAssign -= vOp2;
    W_TEST_BOOL(vMinusAssign.IsEqual(WVec3Type((Type)-6.0, (Type)-0.1, (Type)-7.0), WMath::SmallEpsilon<Type>()));

    // operator*= (float)
    WVec3Type vMulFloat = vOp1;
    vMulFloat *= (Type)2.0;
    W_TEST_BOOL(vMulFloat.IsEqual(WVec3Type((Type)-8.0, (Type)0.4, (Type)-14.0), WMath::SmallEpsilon<Type>()));
    vMulFloat *= (Type)0.0;
    W_TEST_BOOL(vMulFloat.IsEqual(WVec3Type::MakeZero(), WMath::SmallEpsilon<Type>()));

    // operator/= (float)
    WVec3Type vDivFloat = vOp1;
    vDivFloat /= (Type)2.0;
    W_TEST_BOOL(vDivFloat.IsEqual(WVec3Type((Type)-2.0, (Type)0.1, (Type)-3.5), WMath::SmallEpsilon<Type>()));

    // operator+ (WVec3Type, WVec3Type)
    WVec3Type vPlus = (vOp1 + vOp2);
    W_TEST_BOOL(vPlus.IsEqual(WVec3Type((Type)-2.0, (Type)0.5, (Type)-7.0), WMath::SmallEpsilon<Type>()));

    // operator- (WVec3Type, WVec3Type)
    WVec3Type vMinus = (vOp1 - vOp2);
    W_TEST_BOOL(vMinus.IsEqual(WVec3Type((Type)-6.0, (Type)-0.1, (Type)-7.0), WMath::SmallEpsilon<Type>()));

    // operator* (float, WVec3Type)
    WVec3Type vMulFloatVec3 = ((Type)2 * vOp1);
    W_TEST_BOOL(
      vMulFloatVec3.IsEqual(WVec3Type((Type)-8.0, (Type)0.4, (Type)-14.0), WMath::SmallEpsilon<Type>()));
    vMulFloatVec3 = ((Type)0 * vOp1);
    W_TEST_BOOL(vMulFloatVec3.IsEqual(WVec3Type::MakeZero(), WMath::SmallEpsilon<Type>()));

    // operator* (WVec3Type, float)
    WVec3Type vMulVec3Float = (vOp1 * (Type)2);
    W_TEST_BOOL(vMulVec3Float.IsEqual(WVec3Type((Type)-8.0, (Type)0.4, (Type)-14.0), WMath::SmallEpsilon<Type>()));
    vMulVec3Float = (vOp1 * (Type)0);
    W_TEST_BOOL(vMulVec3Float.IsEqual(WVec3Type::MakeZero(), WMath::SmallEpsilon<Type>()));

    // operator/ (WVec3Type, float)
    WVec3Type vDivVec3Float = (vOp1 / (Type)2);
    W_TEST_BOOL(vDivVec3Float.IsEqual(WVec3Type((Type)-2.0, (Type)0.1, (Type)-3.5), WMath::SmallEpsilon<Type>()));

    // operator== (WVec3Type, WVec3Type)
    W_TEST_BOOL(vOp1 == vOp1);
    for (int i = 0; i < 3; ++i)
    {
      W_TEST_BOOL(!(vOp1 == (vOp1 + (Type)WMath::SmallEpsilon<Type>() * compArray[i])));
      W_TEST_BOOL(!(vOp1 == (vOp1 - (Type)WMath::SmallEpsilon<Type>() * compArray[i])));
    }

    // operator!= (WVec3Type, WVec3Type)
    W_TEST_BOOL(!(vOp1 != vOp1));
    for (int i = 0; i < 3; ++i)
    {
      W_TEST_BOOL(vOp1 != (vOp1 + (Type)WMath::SmallEpsilon<Type>() * compArray[i]));
      W_TEST_BOOL(vOp1 != (vOp1 - (Type)WMath::SmallEpsilon<Type>() * compArray[i]));
    }

    // operator< (WVec3Type, WVec3Type)
    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
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
    const WVec3Type vOp1((Type)-4.0, (Type)0.2, (Type)-7.0);
    const WVec3Type vOp2((Type)2.0, (Type)-0.3, (Type)0.5);

    const WVec3Type compArray[3] = {WVec3Type((Type)1.0, (Type)0.0, (Type)0.0), WVec3Type((Type)0.0, (Type)1.0, (Type)0.0), WVec3Type((Type)0.0, (Type)0.0, (Type)1.0)};

    // GetAngleBetween
    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        W_TEST_FLOAT(compArray[i].GetAngleBetween(compArray[j]).GetDegree(), i == j ? (Type)0.0 : (Type)90.0, WMath::DefaultEpsilon<float>());
      }
    }

    // Dot
    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        W_TEST_FLOAT(compArray[i].Dot(compArray[j]), i == j ? (Type)1.0 : (Type)0.0, WMath::SmallEpsilon<Type>());
      }
    }
    W_TEST_FLOAT(vOp1.Dot(vOp2), (Type)-11.56, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(vOp2.Dot(vOp1), (Type)-11.56, WMath::SmallEpsilon<Type>());

    // Cross
    // Right-handed coordinate system check
    W_TEST_BOOL(compArray[0].CrossRH(compArray[1]).IsEqual(compArray[2], WMath::SmallEpsilon<Type>()));
    W_TEST_BOOL(compArray[1].CrossRH(compArray[2]).IsEqual(compArray[0], WMath::SmallEpsilon<Type>()));
    W_TEST_BOOL(compArray[2].CrossRH(compArray[0]).IsEqual(compArray[1], WMath::SmallEpsilon<Type>()));

    // CompMin
    W_TEST_BOOL(vOp1.CompMin(vOp2).IsEqual(WVec3Type((Type)-4.0, (Type)-0.3, (Type)-7.0), WMath::SmallEpsilon<Type>()));
    W_TEST_BOOL(vOp2.CompMin(vOp1).IsEqual(WVec3Type((Type)-4.0, (Type)-0.3, (Type)-7.0), WMath::SmallEpsilon<Type>()));

    // CompMax
    W_TEST_BOOL(vOp1.CompMax(vOp2).IsEqual(WVec3Type((Type)2.0, (Type)0.2, (Type)0.5), WMath::SmallEpsilon<Type>()));
    W_TEST_BOOL(vOp2.CompMax(vOp1).IsEqual(WVec3Type((Type)2.0, (Type)0.2, (Type)0.5), WMath::SmallEpsilon<Type>()));

    // CompClamp
    W_TEST_BOOL(vOp1.CompClamp(vOp1, vOp2).IsEqual(WVec3Type((Type)-4.0, (Type)-0.3, (Type)-7.0), WMath::SmallEpsilon<Type>()));
    W_TEST_BOOL(vOp2.CompClamp(vOp1, vOp2).IsEqual(WVec3Type((Type)2.0, (Type)0.2, (Type)0.5), WMath::SmallEpsilon<Type>()));

    // CompMul
    W_TEST_BOOL(vOp1.CompMul(vOp2).IsEqual(WVec3Type((Type)-8.0, (Type)-0.06, (Type)-3.5), WMath::SmallEpsilon<Type>()));
    W_TEST_BOOL(vOp2.CompMul(vOp1).IsEqual(WVec3Type((Type)-8.0, (Type)-0.06, (Type)-3.5), WMath::SmallEpsilon<Type>()));

    // CompDiv
    W_TEST_BOOL(vOp1.CompDiv(vOp2).IsEqual(WVec3Type((Type)-2.0, (Type)(-2.0/3.0), (Type)-14.0), WMath::SmallEpsilon<Type>()));

    // Abs
    W_TEST_VEC3(vOp1.Abs(), WVec3Type((Type)4.0, (Type)0.2, (Type)7.0), WMath::SmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetAngleBetween")
  {
    WVec3Type v;

    v.Set(1, 0, 0);
    W_TEST_FLOAT(v.GetAngleBetween(WVec3Type(1, 0, 0), WVec3Type(0, 0, 1)).GetDegree(), 0.0f, 0.001f);

    v.Set(1, -1, 0);
    v.Normalize();
    W_TEST_FLOAT(v.GetAngleBetween(WVec3Type(1, 0, 0), WVec3Type(0, 0, 1)).GetDegree(), 45.0f, 0.001f);

    v.Set(1, 1, 0);
    v.Normalize();
    W_TEST_FLOAT(v.GetAngleBetween(WVec3Type(1, 0, 0), WVec3Type(0, 0, 1)).GetDegree(), -45.0f, 0.001f);

    v.Set(-1, 0, 0);
    v.Normalize();
    W_TEST_FLOAT(v.GetAngleBetween(WVec3Type(1, 0, 0), WVec3Type(0, 0, 1)).GetDegree(), 180.0f, 0.001f);

    v.Set(1, 0, 0);
    v.Normalize();
    W_TEST_FLOAT(v.GetAngleBetween(WVec3Type(0, 1, 0), WVec3Type(0, 0, 1)).GetDegree(), 90.0f, 0.001f);

    v.Set(0, 1, 0);
    v.Normalize();
    W_TEST_FLOAT(v.GetAngleBetween(WVec3Type(1, 0, 0), WVec3Type(0, 0, 1)).GetDegree(), -90.0f, 0.001f);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CalculateNormal")
  {
    WVec3Type n;
    W_TEST_BOOL(n.CalculateNormal(WVec3Type(-1, 0, 1), WVec3Type(1, 0, 1), WVec3Type(0, 0, -1)) == W_SUCCESS);
    W_TEST_VEC3(n, WVec3Type(0, 1, 0), 0.001f);

    W_TEST_BOOL(n.CalculateNormal(WVec3Type(-1, 0, -1), WVec3Type(1, 0, -1), WVec3Type(0, 0, 1)) == W_SUCCESS);
    W_TEST_VEC3(n, WVec3Type(0, -1, 0), 0.001f);

    W_TEST_BOOL(n.CalculateNormal(WVec3Type(-1, 0, 1), WVec3Type(1, 0, 1), WVec3Type(1, 0, 1)) == W_FAILURE);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeOrthogonalTo")
  {
    WVec3Type v;

    v.Set(1, 1, 0);
    v.MakeOrthogonalTo(WVec3Type(1, 0, 0));
    W_TEST_VEC3(v, WVec3Type(0, 1, 0), 0.001f);

    v.Set(1, 1, 0);
    v.MakeOrthogonalTo(WVec3Type(0, 1, 0));
    W_TEST_VEC3(v, WVec3Type(1, 0, 0), 0.001f);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetOrthogonalVector")
  {
    WVec3Type v;

    for (Type i = 1; i < 360; i += 3.0f)
    {
      v.Set(i, i * 3, i * 7);
      W_TEST_FLOAT(v.GetOrthogonalVector().Dot(v), 0.0f, 0.001f);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetReflectedVector")
  {
    WVec3Type v, v2;

    v.Set(1, 1, 0);
    v2 = v.GetReflectedVector(WVec3Type(0, -1, 0));
    W_TEST_VEC3(v2, WVec3Type(1, -1, 0), 0.0001f);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeRandomPointInSphere")
  {
    WVec3Type v;

    WRandom rng;
    rng.Initialize(0xEEFF0011AABBCCDDULL);

    WVec3Type avg;
    avg.SetZero();

    const WUInt32 uiNumSamples = 100'000;
    for (WUInt32 i = 0; i < uiNumSamples; ++i)
    {
      v = WVec3Type::MakeRandomPointInSphere(rng);

      W_TEST_BOOL(v.GetLength() <= (Type)1.0 + WMath::SmallEpsilon<Type>());
      W_TEST_BOOL(!v.IsZero());

      avg += v;
    }

    avg /= (float)uiNumSamples;

    // the average point cloud center should be within at least 10% of the sphere's center
    // otherwise the points aren't equally distributed
    W_TEST_BOOL(avg.IsZero((Type)0.1));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeRandomDirection")
  {
    WVec3Type v;

    WRandom rng;
    rng.InitializeFromCurrentTime();

    WVec3Type avg;
    avg.SetZero();

    const WUInt32 uiNumSamples = 100'000;
    for (WUInt32 i = 0; i < uiNumSamples; ++i)
    {
      v = WVec3Type::MakeRandomDirection(rng);

      W_TEST_BOOL(v.IsNormalized());

      avg += v;
    }

    avg /= (float)uiNumSamples;

    // the average point cloud center should be within at least 10% of the sphere's center
    // otherwise the points aren't equally distributed
    W_TEST_BOOL(avg.IsZero((Type)0.1));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeRandomDeviationX")
  {
    WVec3Type v;
    WVec3Type avg;
    avg.SetZero();

    WRandom rng;
    rng.InitializeFromCurrentTime();

    const WAngleTemplate<Type> dev = WAngleTemplate<Type>::MakeFromDegree(65);
    const WUInt32 uiNumSamples = 100'000;
    const WVec3Type vAxis(1, 0, 0);

    for (WUInt32 i = 0; i < uiNumSamples; ++i)
    {
      v = WVec3Type::MakeRandomDeviationX(rng, dev);

      W_TEST_BOOL(v.IsNormalized());

      W_TEST_BOOL(vAxis.GetAngleBetween(v).GetRadian() <= dev.GetRadian() + WMath::DefaultEpsilon<float>());

      avg += v;
    }

    // average direction should be close to the main axis
    avg.Normalize();
    W_TEST_BOOL(avg.IsEqual(vAxis, (Type)0.1));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeRandomDeviationY")
  {
    WVec3Type v;
    WVec3Type avg;
    avg.SetZero();

    WRandom rng;
    rng.InitializeFromCurrentTime();

    const WAngleTemplate<Type> dev = WAngleTemplate<Type>::MakeFromDegree(65);
    const WUInt32 uiNumSamples = 100'000;
    const WVec3Type vAxis(0, 1, 0);

    for (WUInt32 i = 0; i < uiNumSamples; ++i)
    {
      v = WVec3Type::MakeRandomDeviationY(rng, dev);

      W_TEST_BOOL(v.IsNormalized());

      W_TEST_BOOL(vAxis.GetAngleBetween(v).GetRadian() <= dev.GetRadian() + WMath::DefaultEpsilon<float>());

      avg += v;
    }

    // average direction should be close to the main axis
    avg.Normalize();
    W_TEST_BOOL(avg.IsEqual(vAxis, (Type)0.1));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeRandomDeviationZ")
  {
    WVec3Type v;
    WVec3Type avg;
    avg.SetZero();

    WRandom rng;
    rng.InitializeFromCurrentTime();

    const WAngleTemplate<Type> dev = WAngleTemplate<Type>::MakeFromDegree(65);
    const WUInt32 uiNumSamples = 100'000;
    const WVec3Type vAxis(0, 0, 1);

    for (WUInt32 i = 0; i < uiNumSamples; ++i)
    {
      v = WVec3Type::MakeRandomDeviationZ(rng, dev);

      W_TEST_BOOL(v.IsNormalized());

      W_TEST_BOOL(vAxis.GetAngleBetween(v).GetRadian() <= dev.GetRadian() + WMath::DefaultEpsilon<float>());

      avg += v;
    }

    // average direction should be close to the main axis
    avg.Normalize();
    W_TEST_BOOL(avg.IsEqual(vAxis, (Type)0.1));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeRandomDeviation")
  {
    WVec3Type v;

    WRandom rng;
    rng.InitializeFromCurrentTime();

    const WAngleTemplate<Type> dev = WAngleTemplate<Type>::MakeFromDegree(65);
    const WUInt32 uiNumSamples = 100'000;
    WVec3Type vAxis;

    for (WUInt32 i = 0; i < uiNumSamples; ++i)
    {
      vAxis = WVec3Type::MakeRandomDirection(rng);

      v = WVec3Type::MakeRandomDeviation(rng, dev, vAxis);

      W_TEST_BOOL(v.IsNormalized());

      W_TEST_BOOL(vAxis.GetAngleBetween(v).GetDegree() <= dev.GetDegree() + (Type)1.0);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeRandomDeviation")
  {
    W_TEST_VEC3(WVec3Type::MakeOrthogonalVector(WVec3Type(1, 0, 0)), WVec3Type(0, 0, 1), 0.001f);
    W_TEST_VEC3(WVec3Type::MakeOrthogonalVector(WVec3Type(0, 1, 0)), WVec3Type(0, 0, -1), 0.001f);
    W_TEST_VEC3(WVec3Type::MakeOrthogonalVector(WVec3Type(0, 0, 1)), WVec3Type(-1, 0, 0), 0.001f);

    WVec3Type dir;

    dir.Set(1, 2, 3);
    dir.Normalize();
    W_TEST_FLOAT(dir.Dot(WVec3Type::MakeOrthogonalVector(dir)), 0.0f, 0.001f);

    dir.Set(1, 0, -2);
    dir.Normalize();
    W_TEST_FLOAT(dir.Dot(WVec3Type::MakeOrthogonalVector(dir)), 0.0f, 0.001f);

    dir.Set(-1, 2, 0);
    dir.Normalize();
    W_TEST_FLOAT(dir.Dot(WVec3Type::MakeOrthogonalVector(dir)), 0.0f, 0.001f);
  }
}


W_CREATE_SIMPLE_TEST(Math, Vec3f)
{
  TestVec3<float>();
}
W_CREATE_SIMPLE_TEST(Math, Vec3d)
{
  TestVec3<double>();
}
