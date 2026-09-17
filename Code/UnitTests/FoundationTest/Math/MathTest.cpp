#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Math/Vec2.h>

/// ********************* Binary to Int conversion *********************
/// Most significant bit comes first.
/// Adapted from http://bytes.com/topic/c/answers/219656-literal-binary
///
/// Sample usage:
/// W_8BIT(01010101) == 85
/// W_16BIT(10101010, 01010101) == 43605
/// W_32BIT(10000000, 11111111, 10101010, 01010101) == 2164238933
/// ********************************************************************
#define OCT__(n) 0##n##LU

#define W_8BIT__(iBits)                                                                                                           \
  (((iBits & 000000001) ? 1 : 0) + ((iBits & 000000010) ? 2 : 0) + ((iBits & 000000100) ? 4 : 0) + ((iBits & 000001000) ? 8 : 0) + \
    ((iBits & 000010000) ? 16 : 0) + ((iBits & 000100000) ? 32 : 0) + ((iBits & 001000000) ? 64 : 0) + ((iBits & 010000000) ? 128 : 0))

#define W_8BIT(B) ((WUInt8)W_8BIT__(OCT__(B)))

#define W_16BIT(B2, B1) (((WUInt8)W_8BIT(B2) << 8) + W_8BIT(B1))

#define W_32BIT(B4, B3, B2, B1) \
  ((unsigned long)W_8BIT(B4) << 24) + ((unsigned long)W_8BIT(B3) << 16) + ((unsigned long)W_8BIT(B2) << 8) + ((unsigned long)W_8BIT(B1))

namespace
{
  struct UniqueInt
  {
    int i, id;
    UniqueInt(int i, int iId)
      : i(i)
      , id(iId)
    {
    }

    bool operator<(const UniqueInt& rh) { return this->i < rh.i; }

    bool operator>(const UniqueInt& rh) { return this->i > rh.i; }
  };
}; // namespace


template <typename Type>
void TestMath()
{
  using WVec2Type = WVec2Template<Type>;

  // W_TEST_BLOCK(WTestBlock::Enabled, "Constants")
  //{
  //  // Macro test
  //  W_TEST_BOOL(W_8BIT(01010101) == 85);
  //  W_TEST_BOOL(W_16BIT(10101010, 01010101) == 43605);
  //  W_TEST_BOOL(W_32BIT(10000000, 11111111, 10101010, 01010101) == 2164238933);
  
  //  // Infinity test
  //  //                           Sign:_
  //  //                       Exponent: _______  _
  //  //                       Fraction:           _______  ________  ________
  //  WIntFloatUnion uInf = { W_32BIT(01111111, 10000000, 00000000, 00000000) };
  //  W_TEST_BOOL(uInf.f == WMath::FloatInfinity());
  
  //  // FloatMax_Pos test
  //  WIntFloatUnion uMax = { W_32BIT(01111111, 01111111, 11111111, 11111111) };
  //  W_TEST_BOOL(uMax.f == WMath::FloatMax_Pos());
  
  //  // FloatMax_Neg test
  //  WIntFloatUnion uMin = { W_32BIT(11111111, 01111111, 11111111, 11111111) };
  //  W_TEST_BOOL(uMin.f == WMath::FloatMax_Neg());
  //}

  W_TEST_BLOCK(WTestBlock::Enabled, "Sin")
  {
    W_TEST_FLOAT(WMath::Sin(WAngleTemplate<Type>::MakeFromDegree(0.0)), 0.0, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::Sin(WAngleTemplate<Type>::MakeFromDegree(90.0)), 1.0, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::Sin(WAngleTemplate<Type>::MakeFromDegree(180.0)), 0.0, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::Sin(WAngleTemplate<Type>::MakeFromDegree(270.0)), -1.0, WMath::SmallEpsilon<Type>());

    W_TEST_FLOAT(WMath::Sin(WAngleTemplate<Type>::MakeFromDegree(45.0)), WMath::Sqrt(2.0)/2.0, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::Sin(WAngleTemplate<Type>::MakeFromDegree(135.0)), WMath::Sqrt(2.0)/2.0, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::Sin(WAngleTemplate<Type>::MakeFromDegree(225.0)), -WMath::Sqrt(2.0)/2.0, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::Sin(WAngleTemplate<Type>::MakeFromDegree(315.0)), -WMath::Sqrt(2.0)/2.0, WMath::SmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Cos")
  {
    W_TEST_FLOAT(WMath::Cos(WAngleTemplate<Type>::MakeFromDegree(0.0)), 1.0, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::Cos(WAngleTemplate<Type>::MakeFromDegree(90.0)), 0.0, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::Cos(WAngleTemplate<Type>::MakeFromDegree(180.0)), -1.0, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::Cos(WAngleTemplate<Type>::MakeFromDegree(270.0)), 0.0, WMath::SmallEpsilon<Type>());

    W_TEST_FLOAT(WMath::Cos(WAngleTemplate<Type>::MakeFromDegree(45.0)), WMath::Sqrt(2.0)/2.0, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::Cos(WAngleTemplate<Type>::MakeFromDegree(135.0)), -WMath::Sqrt(2.0)/2.0, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::Cos(WAngleTemplate<Type>::MakeFromDegree(225.0)), -WMath::Sqrt(2.0)/2.0, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::Cos(WAngleTemplate<Type>::MakeFromDegree(315.0)), WMath::Sqrt(2.0)/2.0, WMath::SmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Tan")
  {
    W_TEST_FLOAT(WMath::Tan(WAngleTemplate<Type>::MakeFromDegree(0.0)), 0.0, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::Tan(WAngleTemplate<Type>::MakeFromDegree(45.0)), 1.0, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::Tan(WAngleTemplate<Type>::MakeFromDegree(-45.0)), -1.0, WMath::SmallEpsilon<Type>());
    W_TEST_BOOL(WMath::Tan(WAngleTemplate<Type>::MakeFromDegree(90 + WMath::DefaultEpsilon<Type>())) < (Type)1000000.0);
    W_TEST_BOOL(WMath::Tan(WAngleTemplate<Type>::MakeFromDegree((Type)89.9999)) > (Type)100000.0);

    // Testing the period of tan(x) centered at 0 and the adjacent ones
    WAngleTemplate<Type> angle = WAngleTemplate<Type>::MakeFromDegree(-89.0);
    while (angle.GetDegree() < 89.0)
    {
      Type fTan = WMath::Tan(angle);
      Type fTanPrev = WMath::Tan(WAngleTemplate<Type>::MakeFromDegree(angle.GetDegree() - 180.0));
      Type fTanNext = WMath::Tan(WAngleTemplate<Type>::MakeFromDegree(angle.GetDegree() + 180.0));
      Type fSin = WMath::Sin(angle);
      Type fCos = WMath::Cos(angle);

      W_TEST_FLOAT(fTan - fTanPrev, 0.0, (Type)0.002);
      W_TEST_FLOAT(fTan - fTanNext, 0.0, (Type)0.002);
      W_TEST_FLOAT(fTan - (fSin / fCos), 0.0, (Type)0.0005);
      angle += WAngleTemplate<Type>::MakeFromDegree((Type)1.234);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ASin")
  {
    W_TEST_FLOAT(WMath::ASin(0.0).GetDegree(), 0.0, WMath::DefaultEpsilon<Type>());
    W_TEST_FLOAT(WMath::ASin(1.0).GetDegree(), 90.0, WMath::DefaultEpsilon<Type>());
    W_TEST_FLOAT(WMath::ASin(-1.0).GetDegree(), -90.0, WMath::DefaultEpsilon<Type>());

    W_TEST_FLOAT(WMath::ASin(WMath::Sqrt(2.0)/2.0).GetDegree(), 45.0, WMath::LargeEpsilon<Type>());
    W_TEST_FLOAT(WMath::ASin(-WMath::Sqrt(2.0)/2.0).GetDegree(), -45.0, WMath::LargeEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ACos")
  {
    W_TEST_FLOAT(WMath::ACos(0.0).GetDegree(), 90.0, WMath::DefaultEpsilon<Type>());
    W_TEST_FLOAT(WMath::ACos(1.0).GetDegree(), 0.0, WMath::DefaultEpsilon<Type>());
    W_TEST_FLOAT(WMath::ACos(-1.0).GetDegree(), 180.0, WMath::LargeEpsilon<Type>());

    W_TEST_FLOAT(WMath::ACos(WMath::Sqrt(2.0)/2.0).GetDegree(), 45.0, WMath::LargeEpsilon<Type>());
    W_TEST_FLOAT(WMath::ACos(-WMath::Sqrt(2.0)/2.0).GetDegree(), 135.0, WMath::LargeEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ATan")
  {
    W_TEST_FLOAT(WMath::ATan(0.0).GetDegree(), 0.0, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::ATan(1.0).GetDegree(), 45.0, WMath::DefaultEpsilon<Type>());
    W_TEST_FLOAT(WMath::ATan(-1.0).GetDegree(), -45.0, WMath::DefaultEpsilon<Type>());
    W_TEST_FLOAT(WMath::ATan(10000000.0).GetDegree(), 90.0, WMath::LargeEpsilon<float>()*2);
    W_TEST_FLOAT(WMath::ATan(-10000000.0).GetDegree(), -90.0, WMath::DefaultEpsilon<float>()*2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ATan2")
  {
    for (Type fScale = 0.125; fScale < 1000000.0; fScale *= 2.0)
    {
      W_TEST_FLOAT(WMath::ATan2((Type)0.0, fScale).GetDegree(), 0.0, WMath::VerySmallEpsilon<Type>());
      W_TEST_FLOAT(WMath::ATan2(fScale, fScale).GetDegree(), 45.0, WMath::DefaultEpsilon<Type>());
      W_TEST_FLOAT(WMath::ATan2(fScale, (Type)0.0).GetDegree(), 90.0, WMath::DefaultEpsilon<Type>());
      W_TEST_FLOAT(WMath::ATan2(-fScale, fScale).GetDegree(), -45.0, WMath::DefaultEpsilon<Type>());
      W_TEST_FLOAT(WMath::ATan2(-fScale, (Type)0.0).GetDegree(), -90.0, WMath::DefaultEpsilon<Type>());
      W_TEST_FLOAT(WMath::ATan2((Type)0.0, -fScale).GetDegree(), 180.0, WMath::LargeEpsilon<Type>());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Exp")
  {
    W_TEST_FLOAT(1.0, WMath::Exp(0.0), WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(2.7182818284, WMath::Exp(1.0), WMath::SmallEpsilon<float>());//todo improve double compatibility
    W_TEST_FLOAT(7.3890560989, WMath::Exp(2.0), WMath::SmallEpsilon<float>());//todo improve double compatibility
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Ln")
  {
    W_TEST_FLOAT(0.0, WMath::Ln(1.0), WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(1.0, WMath::Ln(2.7182818284), WMath::SmallEpsilon<float>());
    W_TEST_FLOAT(2.0, WMath::Ln(7.3890560989), WMath::SmallEpsilon<float>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Log2")
  {
    W_TEST_FLOAT(0.0, WMath::Log2(1.0), WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(1.0, WMath::Log2(2.0), WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(2.0, WMath::Log2(4.0), WMath::SmallEpsilon<Type>());
  }
  W_TEST_BLOCK(WTestBlock::Enabled, "Log2i")
  {
    W_TEST_BOOL(WMath::Log2i(0) == WUInt32(-1));
    W_TEST_BOOL(WMath::Log2i(1) == 0);
    W_TEST_BOOL(WMath::Log2i(2) == 1);
    W_TEST_BOOL(WMath::Log2i(3) == 1);
    W_TEST_BOOL(WMath::Log2i(4) == 2);
    W_TEST_BOOL(WMath::Log2i(6) == 2);
    W_TEST_BOOL(WMath::Log2i(7) == 2);
    W_TEST_BOOL(WMath::Log2i(8) == 3);
  }
  W_TEST_BLOCK(WTestBlock::Enabled, "Log10")
  {
    W_TEST_FLOAT(0.0, WMath::Log10(1.0), WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(1.0, WMath::Log10(10.0), WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(2.0, WMath::Log10(100.0), WMath::SmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Log")
  {
    W_TEST_FLOAT(0.0, WMath::Log(2.7182818284, 1.0), WMath::SmallEpsilon<float>());
    W_TEST_FLOAT(1.0, WMath::Log(2.7182818284, 2.7182818284), WMath::SmallEpsilon<float>());
    W_TEST_FLOAT(2.0, WMath::Log(2.7182818284, 7.3890560989), WMath::SmallEpsilon<float>());

    W_TEST_FLOAT(0.0, WMath::Log(2.0, 1.0), WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(1.0, WMath::Log(2.0, 2.0), WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(2.0, WMath::Log(2.0, 4.0), WMath::SmallEpsilon<Type>());

    W_TEST_FLOAT(0.0, WMath::Log(10.0, 1.0), WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(1.0, WMath::Log(10.0, 10.0), WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(2.0, WMath::Log(10.0, 100.0), WMath::SmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Pow2")
  {
    W_TEST_FLOAT(1.0, WMath::Pow2(0.0), WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(2.0, WMath::Pow2(1.0), WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(4.0, WMath::Pow2(2.0), WMath::SmallEpsilon<Type>());

    W_TEST_BOOL(WMath::Pow2(0) == 1);
    W_TEST_BOOL(WMath::Pow2(1) == 2);
    W_TEST_BOOL(WMath::Pow2(2) == 4);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Pow")
  {
    W_TEST_FLOAT(1.0, WMath::Pow(3.0, 0.0), WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(3.0, WMath::Pow(3.0, 1.0), WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(9.0, WMath::Pow(3.0, 2.0), WMath::SmallEpsilon<Type>());

    W_TEST_BOOL(WMath::Pow(3, 0) == 1);
    W_TEST_BOOL(WMath::Pow(3, 1) == 3);
    W_TEST_BOOL(WMath::Pow(3, 2) == 9);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Square")
  {
    W_TEST_FLOAT(0.0, WMath::Square(0.0), WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(1.0, WMath::Square(1.0), WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(4.0, WMath::Square(2.0), WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(4.0, WMath::Square(-2.0), WMath::SmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Sqrt")
  {
    W_TEST_FLOAT(0.0, WMath::Sqrt(0.0), WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(1.0, WMath::Sqrt(1.0), WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(2.0, WMath::Sqrt(4.0), WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(4.0, WMath::Sqrt(16.0), WMath::SmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Root")
  {
    W_TEST_FLOAT(3.0, WMath::Root(27.0, 3.0), WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(3.0, WMath::Root(81.0, 4.0), WMath::SmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Sign")
  {
    W_TEST_FLOAT(0.0, WMath::Sign(0.0), WMath::VeryVerySmallEpsilon<Type>());
    W_TEST_FLOAT(1.0, WMath::Sign(0.01), WMath::VeryVerySmallEpsilon<Type>());
    W_TEST_FLOAT(-1.0, WMath::Sign(-0.01), WMath::VeryVerySmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Abs")
  {
    W_TEST_FLOAT(0.0, WMath::Abs(0.0), WMath::VeryVerySmallEpsilon<Type>());
    W_TEST_FLOAT(20.0, WMath::Abs(20.0), WMath::VeryVerySmallEpsilon<Type>());
    W_TEST_FLOAT(20.0, WMath::Abs(-20.0), WMath::VeryVerySmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Min")
  {
    W_TEST_FLOAT(0.0, WMath::Min(0.0, 23.0), WMath::VeryVerySmallEpsilon<Type>());
    W_TEST_FLOAT(-23.0, WMath::Min(0.0, -23.0), WMath::VeryVerySmallEpsilon<Type>());

    W_TEST_BOOL(WMath::Min(1, 2, 3) == 1);
    W_TEST_BOOL(WMath::Min(4, 2, 3) == 2);
    W_TEST_BOOL(WMath::Min(4, 5, 3) == 3);

    W_TEST_BOOL(WMath::Min(1, 2, 3, 4) == 1);
    W_TEST_BOOL(WMath::Min(5, 2, 3, 4) == 2);
    W_TEST_BOOL(WMath::Min(5, 6, 3, 4) == 3);
    W_TEST_BOOL(WMath::Min(5, 6, 7, 4) == 4);

    W_TEST_BOOL(WMath::Min(UniqueInt(1, 0), UniqueInt(1, 1)).id == 0);
    W_TEST_BOOL(WMath::Min(UniqueInt(1, 1), UniqueInt(1, 0)).id == 1);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Max")
  {
    W_TEST_FLOAT(23.0, WMath::Max(0.0, 23.0), WMath::VeryVerySmallEpsilon<Type>());
    W_TEST_FLOAT(0.0, WMath::Max(0.0, -23.0), WMath::VeryVerySmallEpsilon<Type>());

    W_TEST_BOOL(WMath::Max(1, 2, 3) == 3);
    W_TEST_BOOL(WMath::Max(1, 2, 0) == 2);
    W_TEST_BOOL(WMath::Max(1, 0, 0) == 1);

    W_TEST_BOOL(WMath::Max(1, 2, 3, 4) == 4);
    W_TEST_BOOL(WMath::Max(1, 2, 3, 0) == 3);
    W_TEST_BOOL(WMath::Max(1, 2, 0, 0) == 2);
    W_TEST_BOOL(WMath::Max(1, 0, 0, 0) == 1);

    W_TEST_BOOL(WMath::Max(UniqueInt(1, 0), UniqueInt(1, 1)).id == 0);
    W_TEST_BOOL(WMath::Max(UniqueInt(1, 1), UniqueInt(1, 0)).id == 1);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Clamp")
  {
    W_TEST_FLOAT(15.0, WMath::Clamp(23.0, 12.0, 15.0), WMath::VeryVerySmallEpsilon<Type>());
    W_TEST_FLOAT(12.0, WMath::Clamp(3.0, 12.0, 15.0), WMath::VeryVerySmallEpsilon<Type>());
    W_TEST_FLOAT(14.0, WMath::Clamp(14.0, 12.0, 15.0), WMath::VeryVerySmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Saturate")
  {
    W_TEST_FLOAT(0.0, WMath::Saturate(-1.5), WMath::VeryVerySmallEpsilon<Type>());
    W_TEST_FLOAT(0.5, WMath::Saturate(0.5), WMath::VeryVerySmallEpsilon<Type>());
    W_TEST_FLOAT(1.0, WMath::Saturate(12345.0), WMath::VeryVerySmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Floor")
  {
    W_TEST_BOOL(12 == WMath::Floor(12.34));
    W_TEST_BOOL(-13 == WMath::Floor(-12.34));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Ceil")
  {
    W_TEST_BOOL(13 == WMath::Ceil(12.34));
    W_TEST_BOOL(-12 == WMath::Ceil(-12.34));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "FloorToInt")
  {
    W_TEST_BOOL(12 == WMath::FloorToInt((Type)12.34));
    W_TEST_BOOL(-13 == WMath::FloorToInt((Type)-12.34));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CeilToInt")
  {
    W_TEST_BOOL(13 == WMath::CeilToInt((Type)12.34));
    W_TEST_BOOL(-12 == WMath::CeilToInt((Type)-12.34));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "RoundDown (float)")
  {
    W_TEST_FLOAT(10.0, WMath::RoundDown(12.34, 5.0), WMath::VeryVerySmallEpsilon<Type>());
    W_TEST_FLOAT(-15.0, WMath::RoundDown(-12.34, 5.0), WMath::VeryVerySmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "RoundUp (float)")
  {
    W_TEST_FLOAT(15.0, WMath::RoundUp(12.34, 5.0), WMath::VeryVerySmallEpsilon<Type>());
    W_TEST_FLOAT(-10.0, WMath::RoundUp(-12.34, 5.0), WMath::VeryVerySmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "RoundDown (double)")
  {
    W_TEST_DOUBLE(10.0, WMath::RoundDown(12.34, 5.0), WMath::VeryVerySmallEpsilon<double>());
    W_TEST_DOUBLE(-15.0, WMath::RoundDown(-12.34, 5.0), WMath::VeryVerySmallEpsilon<double>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "RoundUp (double)")
  {
    W_TEST_DOUBLE(15.0, WMath::RoundUp(12.34, 5.0), WMath::VeryVerySmallEpsilon<double>());
    W_TEST_DOUBLE(-10.0, WMath::RoundUp(-12.34, 5.0), WMath::VeryVerySmallEpsilon<double>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Trunc")
  {
    W_TEST_BOOL(WMath::Trunc(12.34) == 12);
    W_TEST_BOOL(WMath::Trunc(-12.34) == -12);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "FloatToInt")
  {
    W_TEST_BOOL(WMath::FloatToInt32((Type)12.34) == 12);
    W_TEST_BOOL(WMath::FloatToInt32((Type)-12.34) == -12);

#if W_DISABLED(W_PLATFORM_ARCH_X86) || (_MSC_VER <= 1916)
    W_TEST_BOOL(WMath::FloatToInt(12000000000000.34) == 12000000000000);
    W_TEST_BOOL(WMath::FloatToInt(-12000000000000.34) == -12000000000000);
#endif
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Round")
  {
    W_TEST_BOOL(WMath::Round(12.34) == 12);
    W_TEST_BOOL(WMath::Round(-12.34) == -12);

    W_TEST_BOOL(WMath::Round(12.54) == 13);
    W_TEST_BOOL(WMath::Round(-12.54) == -13);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "RoundToInt")
  {
    W_TEST_BOOL(WMath::RoundToInt((Type)12.34) == 12);
    W_TEST_BOOL(WMath::RoundToInt((Type)-12.34) == -12);

    W_TEST_BOOL(WMath::RoundToInt((Type)12.54) == 13);
    W_TEST_BOOL(WMath::RoundToInt((Type)-12.54) == -13);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "RoundClosest (float)")
  {
    W_TEST_FLOAT(WMath::RoundToMultiple(12.0, 3.0), 12.0, WMath::DefaultEpsilon<Type>());
    W_TEST_FLOAT(WMath::RoundToMultiple(-12.0, 3.0), -12.0, WMath::DefaultEpsilon<Type>());
    W_TEST_FLOAT(WMath::RoundToMultiple(12.34, 7.0), 14.0, WMath::DefaultEpsilon<Type>());
    W_TEST_FLOAT(WMath::RoundToMultiple(-12.34, 7.0), -14.0, WMath::DefaultEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "RoundClosest (double)")
  {
    W_TEST_DOUBLE(WMath::RoundToMultiple(12.0, 3.0), 12.0, WMath::DefaultEpsilon<double>());
    W_TEST_DOUBLE(WMath::RoundToMultiple(-12.0, 3.0), -12.0, WMath::DefaultEpsilon<double>());
    W_TEST_DOUBLE(WMath::RoundToMultiple(12.34, 7.0), 14.0, WMath::DefaultEpsilon<double>());
    W_TEST_DOUBLE(WMath::RoundToMultiple(-12.34, 7.0), -14.0, WMath::DefaultEpsilon<double>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "RoundUp (int)")
  {
    W_TEST_INT(WMath::RoundUp(12, 7), 14);
    W_TEST_INT(WMath::RoundUp(-12, 7), -7);
    W_TEST_INT(WMath::RoundUp(16, 4), 16);
    W_TEST_INT(WMath::RoundUp(-16, 4), -16);
    W_TEST_INT(WMath::RoundUp(17, 4), 20);
    W_TEST_INT(WMath::RoundUp(-17, 4), -16);
    W_TEST_INT(WMath::RoundUp(15, 4), 16);
    W_TEST_INT(WMath::RoundUp(-15, 4), -12);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "RoundDown (int)")
  {
    W_TEST_INT(WMath::RoundDown(12, 7), 7);
    W_TEST_INT(WMath::RoundDown(-12, 7), -14);
    W_TEST_INT(WMath::RoundDown(16, 4), 16);
    W_TEST_INT(WMath::RoundDown(-16, 4), -16);
    W_TEST_INT(WMath::RoundDown(17, 4), 16);
    W_TEST_INT(WMath::RoundDown(-17, 4), -20);
    W_TEST_INT(WMath::RoundDown(15, 4), 12);
    W_TEST_INT(WMath::RoundDown(-15, 4), -16);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "RoundUp (unsigned int)")
  {
    W_TEST_INT(WMath::RoundUp(12u, 7), 14);
    W_TEST_INT(WMath::RoundUp(16u, 4), 16);
    W_TEST_INT(WMath::RoundUp(17u, 4), 20);
    W_TEST_INT(WMath::RoundUp(15u, 4), 16);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "RoundDown (unsigned int)")
  {
    W_TEST_INT(WMath::RoundDown(12u, 7), 7);
    W_TEST_INT(WMath::RoundDown(16u, 4), 16);
    W_TEST_INT(WMath::RoundDown(17u, 4), 16);
    W_TEST_INT(WMath::RoundDown(15u, 4), 12);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Fraction")
  {
    W_TEST_FLOAT(WMath::Fraction(12.34), 0.34, WMath::DefaultEpsilon<Type>());
    W_TEST_FLOAT(WMath::Fraction(-12.34), -0.34, WMath::DefaultEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Mod (float)")
  {
    W_TEST_FLOAT(2.34, WMath::Mod(12.34, 2.5), WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(-2.34, WMath::Mod(-12.34, 2.5), WMath::SmallEpsilon<Type>());

    W_TEST_FLOAT(2.34, WMath::Mod(12.34, -2.5), WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(-2.34, WMath::Mod(-12.34, -2.5), WMath::SmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Mod (double)")
  {
    W_TEST_DOUBLE(2.34, WMath::Mod(12.34, 2.5), WMath::SmallEpsilon<double>());
    W_TEST_DOUBLE(-2.34, WMath::Mod(-12.34, 2.5), WMath::SmallEpsilon<double>());

    W_TEST_DOUBLE(2.34, WMath::Mod(12.34, -2.5), WMath::SmallEpsilon<double>());
    W_TEST_DOUBLE(-2.34, WMath::Mod(-12.34, -2.5), WMath::SmallEpsilon<double>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Invert")
  {
    W_TEST_FLOAT(WMath::Invert(1.0), 1.0, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::Invert(2.0), 0.5, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::Invert(4.0), 0.25, WMath::SmallEpsilon<Type>());

    W_TEST_FLOAT(WMath::Invert(-1.0), -1.0, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::Invert(-2.0), -0.5, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::Invert(-4.0), -0.25, WMath::SmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Odd")
  {
    W_TEST_BOOL(WMath::IsOdd(0) == false);
    W_TEST_BOOL(WMath::IsOdd(1) == true);
    W_TEST_BOOL(WMath::IsOdd(2) == false);
    W_TEST_BOOL(WMath::IsOdd(-1) == true);
    W_TEST_BOOL(WMath::IsOdd(-2) == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Even")
  {
    W_TEST_BOOL(WMath::IsEven(0) == true);
    W_TEST_BOOL(WMath::IsEven(1) == false);
    W_TEST_BOOL(WMath::IsEven(2) == true);
    W_TEST_BOOL(WMath::IsEven(-1) == false);
    W_TEST_BOOL(WMath::IsEven(-2) == true);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Swap")
  {
    WInt32 a = 1;
    WInt32 b = 2;
    WMath::Swap(a, b);
    W_TEST_BOOL((a == 2) && (b == 1));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Lerp")
  {
    W_TEST_FLOAT(WMath::Lerp(-5.0, 5.0, 0.5), 0.0, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::Lerp(0.0, 5.0, 0.5), 2.5, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::Lerp(-5.0, 5.0, 0.0), -5.0, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::Lerp(-5.0, 5.0, 1.0), 5.0, WMath::SmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Unlerp")
  {
    W_TEST_FLOAT(WMath::Unlerp(-5.0, 5.0, 0.0), 0.5, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::Unlerp(0.0, 5.0, 2.5), 0.5, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::Unlerp(-5.0, 5.0, -5.0), 0.0, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::Unlerp(-5.0, 5.0, 5.0), 1.0, WMath::SmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Step")
  {
    W_TEST_FLOAT(WMath::Step(0.5, 0.4), 1.0, WMath::DefaultEpsilon<Type>());
    W_TEST_FLOAT(WMath::Step(0.3, 0.4), 0.0, WMath::DefaultEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SmoothStep")
  {
    // Only test values that must be true for any symmetric step function.
    // How should one test smoothness?
    for (int iScale = -19; iScale <= 19; iScale += 2)
    {
      W_TEST_FLOAT(WMath::SmoothStep(0.0 * iScale, 0.1 * iScale, 0.4 * iScale), 0.0, WMath::SmallEpsilon<Type>());
      W_TEST_FLOAT(WMath::SmoothStep(0.1 * iScale, 0.1 * iScale, 0.4 * iScale), 0.0, WMath::SmallEpsilon<Type>());
      W_TEST_FLOAT(WMath::SmoothStep(0.4 * iScale, 0.1 * iScale, 0.4 * iScale), 1.0, WMath::SmallEpsilon<Type>());
      W_TEST_FLOAT(WMath::SmoothStep(0.25 * iScale, 0.1 * iScale, 0.4 * iScale), 0.5, WMath::SmallEpsilon<Type>());
      W_TEST_FLOAT(WMath::SmoothStep(0.5 * iScale, 0.1 * iScale, 0.4 * iScale), 1.0, WMath::SmallEpsilon<Type>());

      W_TEST_FLOAT(WMath::SmoothStep(0.5 * iScale, 0.4 * iScale, 0.1 * iScale), 0.0, WMath::SmallEpsilon<Type>());
      W_TEST_FLOAT(WMath::SmoothStep(0.4 * iScale, 0.4 * iScale, 0.1 * iScale), 0.0, WMath::SmallEpsilon<Type>());
      W_TEST_FLOAT(WMath::SmoothStep(0.1 * iScale, 0.4 * iScale, 0.1 * iScale), 1.0, WMath::SmallEpsilon<Type>());
      W_TEST_FLOAT(WMath::SmoothStep(0.25 * iScale, 0.1 * iScale, 0.4 * iScale), 0.5, WMath::SmallEpsilon<Type>());
      W_TEST_FLOAT(WMath::SmoothStep(0.0 * iScale, 0.4 * iScale, 0.1 * iScale), 1.0, WMath::SmallEpsilon<Type>());

      // For edge1 == edge2 SmoothStep should behave like Step
      W_TEST_FLOAT(WMath::SmoothStep(0.0 * iScale, 0.1 * iScale, 0.1 * iScale), iScale > 0 ? 0.0 : 1.0, WMath::SmallEpsilon<Type>());
      W_TEST_FLOAT(WMath::SmoothStep(0.2 * iScale, 0.1 * iScale, 0.1 * iScale), iScale < 0 ? 0.0 : 1.0, WMath::SmallEpsilon<Type>());
    }

    W_TEST_FLOAT(WMath::SmoothStep(0.2, 0.0, 1.0), 0.104, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(WMath::SmoothStep(0.4, 0.2, 0.8), 0.259259, WMath::DefaultEpsilon<float>());

    W_TEST_FLOAT(WMath::SmootherStep(0.2, 0.0, 1.0), 0.05792, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(WMath::SmootherStep(0.4, 0.2, 0.8), 0.209876, WMath::DefaultEpsilon<float>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsPowerOf")
  {
    W_TEST_BOOL(WMath::IsPowerOf(4, 2) == true);
    W_TEST_BOOL(WMath::IsPowerOf(5, 2) == false);
    W_TEST_BOOL(WMath::IsPowerOf(0, 2) == false);
    W_TEST_BOOL(WMath::IsPowerOf(1, 2) == true);

    W_TEST_BOOL(WMath::IsPowerOf(4, 3) == false);
    W_TEST_BOOL(WMath::IsPowerOf(3, 3) == true);
    W_TEST_BOOL(WMath::IsPowerOf(1, 3) == true);
    W_TEST_BOOL(WMath::IsPowerOf(27, 3) == true);
    W_TEST_BOOL(WMath::IsPowerOf(28, 3) == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsPowerOf2")
  {
    W_TEST_BOOL(WMath::IsPowerOf2(4) == true);
    W_TEST_BOOL(WMath::IsPowerOf2(5) == false);
    W_TEST_BOOL(WMath::IsPowerOf2(0) == false);
    W_TEST_BOOL(WMath::IsPowerOf2(1) == true);
    W_TEST_BOOL(WMath::IsPowerOf2(0x7FFFFFFFu) == false);
    W_TEST_BOOL(WMath::IsPowerOf2(0x80000000u) == true);
    W_TEST_BOOL(WMath::IsPowerOf2(0x80000001u) == false);
    W_TEST_BOOL(WMath::IsPowerOf2(0xFFFFFFFFu) == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "PowerOf2_Floor")
  {
    W_TEST_INT(WMath::PowerOfTwo_Floor(64u), 64);
    W_TEST_INT(WMath::PowerOfTwo_Floor(33u), 32);
    W_TEST_INT(WMath::PowerOfTwo_Floor(4u), 4);
    W_TEST_INT(WMath::PowerOfTwo_Floor(5u), 4);
    W_TEST_INT(WMath::PowerOfTwo_Floor(1u), 1);
    W_TEST_INT(WMath::PowerOfTwo_Floor(0x80000000), 0x80000000);
    W_TEST_INT(WMath::PowerOfTwo_Floor(0x80000001), 0x80000000);
    // strange case...
    W_TEST_INT(WMath::PowerOfTwo_Floor(0u), 1);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "PowerOf2_Ceil")
  {
    W_TEST_INT(WMath::PowerOfTwo_Ceil(64u), 64);
    W_TEST_INT(WMath::PowerOfTwo_Ceil(33u), 64);
    W_TEST_INT(WMath::PowerOfTwo_Ceil(4u), 4);
    W_TEST_INT(WMath::PowerOfTwo_Ceil(5u), 8);
    W_TEST_INT(WMath::PowerOfTwo_Ceil(1u), 1);
    W_TEST_INT(WMath::PowerOfTwo_Ceil(0u), 1);
    W_TEST_INT(WMath::PowerOfTwo_Ceil(0x7FFFFFFFu), 0x80000000);
    W_TEST_INT(WMath::PowerOfTwo_Ceil(0x80000000), 0x80000000);
    // anything above 0x80000000 is undefined behavior due to how left-shift works
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GreatestCommonDivisor")
  {
    W_TEST_INT(WMath::GreatestCommonDivisor(13, 13), 13);
    W_TEST_INT(WMath::GreatestCommonDivisor(13, 0), 13);
    W_TEST_INT(WMath::GreatestCommonDivisor(0, 637), 637);
    W_TEST_INT(WMath::GreatestCommonDivisor(37, 600), 1);
    W_TEST_INT(WMath::GreatestCommonDivisor(20, 100), 20);
    W_TEST_INT(WMath::GreatestCommonDivisor(624129, 2061517), 18913);
  }


  W_TEST_BLOCK(WTestBlock::Enabled, "IsEqual")
  {
    W_TEST_BOOL(WMath::IsEqual((Type)1.0, (Type)1.0 - WMath::HugeEpsilon<Type>(), WMath::HugeEpsilon<Type>()) == true);
    W_TEST_BOOL(WMath::IsEqual((Type)1.0, (Type)1.0 + WMath::HugeEpsilon<Type>(), WMath::HugeEpsilon<Type>()) == true);
    W_TEST_BOOL(WMath::IsEqual((Type)1.0, (Type)1.0 - WMath::HugeEpsilon<Type>(), WMath::SmallEpsilon<Type>()) == false);
    W_TEST_BOOL(WMath::IsEqual((Type)1.0, (Type)1.0 + WMath::HugeEpsilon<Type>(), WMath::SmallEpsilon<Type>()) == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "NaN_Infinity")
  {
    if (WMath::SupportsNaN<Type>())
    {
      W_TEST_BOOL(WMath::IsNaN(WMath::NaN<Type>()) == true);

      W_TEST_BOOL(WMath::Infinity<Type>() == WMath::Infinity<Type>() - (Type)1);
      W_TEST_BOOL(WMath::Infinity<Type>() == WMath::Infinity<Type>() + (Type)1);

      W_TEST_BOOL(WMath::IsNaN(WMath::Infinity<Type>() - WMath::Infinity<Type>()));

      W_TEST_BOOL(!WMath::IsFinite(WMath::Infinity<Type>()));
      W_TEST_BOOL(!WMath::IsFinite(-WMath::Infinity<Type>()));
      W_TEST_BOOL(!WMath::IsFinite(WMath::NaN<Type>()));
      W_TEST_BOOL(!WMath::IsNaN(WMath::Infinity<Type>()));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsInRange")
  {
    W_TEST_BOOL(WMath::IsInRange(1.0, 0.0, 2.0) == true);
    W_TEST_BOOL(WMath::IsInRange(1.0, 0.0, 1.0) == true);
    W_TEST_BOOL(WMath::IsInRange(1.0, 1.0, 2.0) == true);
    W_TEST_BOOL(WMath::IsInRange(0.0, 1.0, 2.0) == false);
    W_TEST_BOOL(WMath::IsInRange(3.0, 0.0, 2.0) == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsZero")
  {
    W_TEST_BOOL(WMath::IsZero(WMath::HugeEpsilon<Type>()*((Type)9.0), WMath::VeryHugeEpsilon<Type>()) == true);
    W_TEST_BOOL(WMath::IsZero(WMath::HugeEpsilon<Type>(), WMath::VeryHugeEpsilon<Type>()) == true);
    W_TEST_BOOL(WMath::IsZero(WMath::HugeEpsilon<Type>()*((Type)9.0), WMath::SmallEpsilon<Type>()) == false);
    W_TEST_BOOL(WMath::IsZero(WMath::HugeEpsilon<Type>(), WMath::SmallEpsilon<Type>()) == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ColorFloatToByte")
  {
    W_TEST_INT(WMath::ColorFloatToByte(WMath::NaN<float>()), 0);
    W_TEST_INT(WMath::ColorFloatToByte((float)-1.0), 0);
    W_TEST_INT(WMath::ColorFloatToByte((float)0.0), 0);
    W_TEST_INT(WMath::ColorFloatToByte((float)0.4), 102);
    W_TEST_INT(WMath::ColorFloatToByte((float)1.0), 255);
    W_TEST_INT(WMath::ColorFloatToByte((float)1.5), 255);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ColorFloatToShort")
  {
    W_TEST_INT(WMath::ColorFloatToShort(WMath::NaN<float>()), 0);
    W_TEST_INT(WMath::ColorFloatToShort((float)-1.0), 0);
    W_TEST_INT(WMath::ColorFloatToShort((float)0.0), 0);
    W_TEST_INT(WMath::ColorFloatToShort((float)0.4), 26214);
    W_TEST_INT(WMath::ColorFloatToShort((float)1.0), 65535);
    W_TEST_INT(WMath::ColorFloatToShort((float)1.5), 65535);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ColorFloatToSignedByte")
  {
    W_TEST_INT(WMath::ColorFloatToSignedByte(WMath::NaN<float>()), 0);
    W_TEST_INT(WMath::ColorFloatToSignedByte((float)-1.0), -127);
    W_TEST_INT(WMath::ColorFloatToSignedByte((float)0.0), 0);
    W_TEST_INT(WMath::ColorFloatToSignedByte((float)0.4), 51);
    W_TEST_INT(WMath::ColorFloatToSignedByte((float)1.0), 127);
    W_TEST_INT(WMath::ColorFloatToSignedByte((float)1.5), 127);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ColorFloatToSignedShort")
  {
    W_TEST_INT(WMath::ColorFloatToSignedShort(WMath::NaN<float>()), 0);
    W_TEST_INT(WMath::ColorFloatToSignedShort((float)-1.0), -32767);
    W_TEST_INT(WMath::ColorFloatToSignedShort((float)0.0), 0);
    W_TEST_INT(WMath::ColorFloatToSignedShort((float)0.4), 13107);
    W_TEST_INT(WMath::ColorFloatToSignedShort((float)0.5), 16384);
    W_TEST_INT(WMath::ColorFloatToSignedShort((float)1.0), 32767);
    W_TEST_INT(WMath::ColorFloatToSignedShort((float)1.5), 32767);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ColorByteToFloat")
  {
    W_TEST_FLOAT(WMath::ColorByteToFloat(0), 0.0, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::ColorByteToFloat(128), 0.501960784, WMath::SmallEpsilon<float>());
    W_TEST_FLOAT(WMath::ColorByteToFloat(255), 1.0, WMath::SmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ColorShortToFloat")
  {
    W_TEST_FLOAT(WMath::ColorShortToFloat(0), 0.0, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::ColorShortToFloat(32768), 0.5000076, WMath::SmallEpsilon<float>());
    W_TEST_FLOAT(WMath::ColorShortToFloat(65535), 1.0, WMath::SmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ColorSignedByteToFloat")
  {
    W_TEST_FLOAT(WMath::ColorSignedByteToFloat(-128), -1.0, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::ColorSignedByteToFloat(-127), -1.0, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::ColorSignedByteToFloat(0), 0.0, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::ColorSignedByteToFloat(64), 0.50393700787, WMath::SmallEpsilon<float>());
    W_TEST_FLOAT(WMath::ColorSignedByteToFloat(127), 1.0, WMath::SmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ColorSignedShortToFloat")
  {
    W_TEST_FLOAT(WMath::ColorSignedShortToFloat(-32768), -1.0, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::ColorSignedShortToFloat(-32767), -1.0, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::ColorSignedShortToFloat(0), 0.0, WMath::SmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::ColorSignedShortToFloat(16384), 0.50001526, WMath::SmallEpsilon<float>());
    W_TEST_FLOAT(WMath::ColorSignedShortToFloat(32767), 1.0, WMath::SmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "EvaluateBezierCurve")
  {
    // Determined through the scientific method of manually comparing the result of the function with an online Bezier curve generator:
    // https://www.desmos.com/calculator/cahqdxeshd
    const WVec2Type res[] = {WVec2Type((Type)1, (Type)5), WVec2Type((Type)0.893, (Type)4.455), WVec2Type((Type)1.112, (Type)4.008), WVec2Type((Type)1.557, (Type)3.631), WVec2Type((Type)2.136, (Type)3.304), WVec2Type((Type)2.750, (Type)3.000),
      WVec2Type((Type)3.303, (Type)2.695), WVec2Type((Type)3.701, (Type)2.368), WVec2Type((Type)3.847, (Type)1.991), WVec2Type((Type)3.645, (Type)1.543), WVec2Type((Type)3, (Type)1)};

    const Type step = (Type)1.0 / (W_ARRAY_SIZE(res) - 1);
    for (int i = 0; i < W_ARRAY_SIZE(res); ++i)
    {
      const WVec2Type r = WMath::EvaluateBezierCurve<WVec2Type>(step * i, WVec2Type((Type)1, (Type)5), WVec2Type((Type)0, (Type)3), WVec2Type((Type)6, (Type)3), WVec2Type((Type)3, (Type)1));
      W_TEST_VEC2(r, res[i], (Type)0.002);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "FirstBitLow")
  {
    W_TEST_INT(WMath::FirstBitLow(WUInt32(0b1111)), 0);
    W_TEST_INT(WMath::FirstBitLow(WUInt32(0b1110)), 1);
    W_TEST_INT(WMath::FirstBitLow(WUInt32(0b1100)), 2);
    W_TEST_INT(WMath::FirstBitLow(WUInt32(0b1000)), 3);
    W_TEST_INT(WMath::FirstBitLow(WUInt32(0xFFFFFFFF)), 0);

    W_TEST_INT(WMath::FirstBitLow(WUInt64(0xFF000000FF00000F)), 0);
    W_TEST_INT(WMath::FirstBitLow(WUInt64(0xFF000000FF00000E)), 1);
    W_TEST_INT(WMath::FirstBitLow(WUInt64(0xFF000000FF00000C)), 2);
    W_TEST_INT(WMath::FirstBitLow(WUInt64(0xFF000000FF000008)), 3);
    W_TEST_INT(WMath::FirstBitLow(WUInt64(0xFFFFFFFFFFFFFFFF)), 0);

    // Edge cases specifically for 32-bit systems where upper and lower 32-bit are handled individually.
    W_TEST_INT(WMath::FirstBitLow(WUInt64(0x00000000FFFFFFFF)), 0);
    W_TEST_INT(WMath::FirstBitLow(WUInt64(0xFFFFFFFF00000000)), 32);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "FirstBitHigh")
  {
    W_TEST_INT(WMath::FirstBitHigh(WUInt32(0b1111)), 3);
    W_TEST_INT(WMath::FirstBitHigh(WUInt32(0b0111)), 2);
    W_TEST_INT(WMath::FirstBitHigh(WUInt32(0b0011)), 1);
    W_TEST_INT(WMath::FirstBitHigh(WUInt32(0b0001)), 0);
    W_TEST_INT(WMath::FirstBitHigh(WUInt32(0xFFFFFFFF)), 31);

    W_TEST_INT(WMath::FirstBitHigh(WUInt64(0x00FF000000FF000F)), 55);
    W_TEST_INT(WMath::FirstBitHigh(WUInt64(0x007F000000FF000F)), 54);
    W_TEST_INT(WMath::FirstBitHigh(WUInt64(0x003F000000FF000F)), 53);
    W_TEST_INT(WMath::FirstBitHigh(WUInt64(0x001F000000FF000F)), 52);
    W_TEST_INT(WMath::FirstBitHigh(WUInt64(0xFFFFFFFFFFFFFFFF)), 63);

    // Edge cases specifically for 32-bit systems where upper and lower 32-bit are handled individually.
    W_TEST_INT(WMath::FirstBitHigh(WUInt64(0x00000000FFFFFFFF)), 31);
    W_TEST_INT(WMath::FirstBitHigh(WUInt64(0xFFFFFFFF00000000)), 63);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CountTrailingZeros (32)")
  {
    W_TEST_INT(WMath::CountTrailingZeros(0b1111u), 0);
    W_TEST_INT(WMath::CountTrailingZeros(0b1110u), 1);
    W_TEST_INT(WMath::CountTrailingZeros(0b1100u), 2);
    W_TEST_INT(WMath::CountTrailingZeros(0b1000u), 3);
    W_TEST_INT(WMath::CountTrailingZeros(0xFFFFFFFF), 0);
    W_TEST_INT(WMath::CountTrailingZeros(0u), 32);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CountTrailingZeros (64)")
  {
    W_TEST_INT(WMath::CountTrailingZeros(0b1111llu), 0);
    W_TEST_INT(WMath::CountTrailingZeros(0b1110llu), 1);
    W_TEST_INT(WMath::CountTrailingZeros(0b1100llu), 2);
    W_TEST_INT(WMath::CountTrailingZeros(0b1000llu), 3);
    W_TEST_INT(WMath::CountTrailingZeros(0xFFFFFFFF0llu), 4);
    W_TEST_INT(WMath::CountTrailingZeros(0llu), 64);
    W_TEST_INT(WMath::CountTrailingZeros(0xFFFFFFFF00llu), 8);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CountLeadingZeros")
  {
    W_TEST_INT(WMath::CountLeadingZeros(0b1111), 28);
    W_TEST_INT(WMath::CountLeadingZeros(0b0111), 29);
    W_TEST_INT(WMath::CountLeadingZeros(0b0011), 30);
    W_TEST_INT(WMath::CountLeadingZeros(0b0001), 31);
    W_TEST_INT(WMath::CountLeadingZeros(0xFFFFFFFF), 0);
    W_TEST_INT(WMath::CountLeadingZeros(0), 32);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Bitmask_LowN")
  {
    W_TEST_INT(WMath::Bitmask_LowN<WUInt32>(0), 0);
    W_TEST_INT(WMath::Bitmask_LowN<WUInt32>(1), 1);
    W_TEST_INT(WMath::Bitmask_LowN<WUInt32>(2), 3);
    W_TEST_INT(WMath::Bitmask_LowN<WUInt32>(3), 7);
    W_TEST_INT(WMath::Bitmask_LowN<WUInt32>(31), 0x7fffffff);
    W_TEST_INT(WMath::Bitmask_LowN<WUInt32>(32), 0xffffffffu);
    W_TEST_INT(WMath::Bitmask_LowN<WUInt32>(33), 0xffffffffu);
    W_TEST_INT(WMath::Bitmask_LowN<WUInt32>(50), 0xffffffffu);

    W_TEST_INT(WMath::Bitmask_LowN<WUInt64>(0), 0);
    W_TEST_INT(WMath::Bitmask_LowN<WUInt64>(1), 1);
    W_TEST_INT(WMath::Bitmask_LowN<WUInt64>(2), 3);
    W_TEST_INT(WMath::Bitmask_LowN<WUInt64>(3), 7);
    W_TEST_INT(WMath::Bitmask_LowN<WUInt64>(31), 0x7fffffff);
    W_TEST_INT(WMath::Bitmask_LowN<WUInt64>(32), 0xffffffffu);
    W_TEST_INT(WMath::Bitmask_LowN<WUInt64>(63), 0x7fffffffffffffffull);
    W_TEST_BOOL(WMath::Bitmask_LowN<WUInt64>(64) == 0xffffffffffffffffull);
    W_TEST_BOOL(WMath::Bitmask_LowN<WUInt64>(65) == 0xffffffffffffffffull);
    W_TEST_BOOL(WMath::Bitmask_LowN<WUInt64>(100) == 0xffffffffffffffffull);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Bitmask_HighN")
  {
    W_TEST_INT(WMath::Bitmask_HighN<WUInt32>(0), 0u);
    W_TEST_INT(WMath::Bitmask_HighN<WUInt32>(1), 0x80000000u);
    W_TEST_INT(WMath::Bitmask_HighN<WUInt32>(2), 0xC0000000u);
    W_TEST_INT(WMath::Bitmask_HighN<WUInt32>(3), 0xE0000000u);
    W_TEST_INT(WMath::Bitmask_HighN<WUInt32>(31), 0xfffffffeu);
    W_TEST_INT(WMath::Bitmask_HighN<WUInt32>(32), 0xffffffffu);
    W_TEST_INT(WMath::Bitmask_HighN<WUInt32>(33), 0xffffffffu);
    W_TEST_INT(WMath::Bitmask_HighN<WUInt32>(60), 0xffffffffu);

    W_TEST_BOOL(WMath::Bitmask_HighN<WUInt64>(0) == 0);
    W_TEST_BOOL(WMath::Bitmask_HighN<WUInt64>(1) == 0x8000000000000000llu);
    W_TEST_BOOL(WMath::Bitmask_HighN<WUInt64>(2) == 0xC000000000000000llu);
    W_TEST_BOOL(WMath::Bitmask_HighN<WUInt64>(3) == 0xE000000000000000llu);
    W_TEST_BOOL(WMath::Bitmask_HighN<WUInt64>(31) == 0xfffffffe00000000llu);
    W_TEST_BOOL(WMath::Bitmask_HighN<WUInt64>(32) == 0xffffffff00000000llu);
    W_TEST_BOOL(WMath::Bitmask_HighN<WUInt64>(63) == 0xfffffffffffffffellu);
    W_TEST_BOOL(WMath::Bitmask_HighN<WUInt64>(64) == 0xffffffffffffffffull);
    W_TEST_BOOL(WMath::Bitmask_HighN<WUInt64>(65) == 0xffffffffffffffffull);
    W_TEST_BOOL(WMath::Bitmask_HighN<WUInt64>(1000) == 0xffffffffffffffffull);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "TryMultiply32")
  {
    WUInt32 res;

    res = 0;
    W_TEST_BOOL(WMath::TryMultiply32(res, 1, 1, 2, 3).Succeeded());
    W_TEST_INT(res, 6);

    res = 0;
    W_TEST_BOOL(WMath::TryMultiply32(res, 1, 1, 1, 0xFFFFFFFF).Succeeded());
    W_TEST_BOOL(res == 0xFFFFFFFF);

    res = 0;
    W_TEST_BOOL(WMath::TryMultiply32(res, 0xFFFF, 0x10001).Succeeded());
    W_TEST_BOOL(res == 0xFFFFFFFF);

    res = 0;
    W_TEST_BOOL(WMath::TryMultiply32(res, 0x3FFFFFF, 2, 4, 8).Succeeded());
    W_TEST_BOOL(res == 0xFFFFFFC0);

    res = 1;
    W_TEST_BOOL(WMath::TryMultiply32(res, 0xFFFFFFFF, 2).Failed());
    W_TEST_BOOL(res == 1);

    res = 0;
    W_TEST_BOOL(WMath::TryMultiply32(res, 0x80000000, 2).Failed()); // slightly above 0xFFFFFFFF
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "TryMultiply64")
  {
    WUInt64 res;

    res = 0;
    W_TEST_BOOL(WMath::TryMultiply64(res, 1, 1, 2, 3).Succeeded());
    W_TEST_INT(res, 6);

    res = 0;
    W_TEST_BOOL(WMath::TryMultiply64(res, 1, 1, 1, 0xFFFFFFFF).Succeeded());
    W_TEST_BOOL(res == 0xFFFFFFFF);

    res = 0;
    W_TEST_BOOL(WMath::TryMultiply64(res, 0xFFFF, 0x10001).Succeeded());
    W_TEST_BOOL(res == 0xFFFFFFFF);

    res = 0;
    W_TEST_BOOL(WMath::TryMultiply64(res, 0x3FFFFFF, 2, 4, 8).Succeeded());
    W_TEST_BOOL(res == 0xFFFFFFC0);

    res = 0;
    W_TEST_BOOL(WMath::TryMultiply64(res, 0xFFFFFFFF, 2).Succeeded());
    W_TEST_BOOL(res == 0x1FFFFFFFE);

    res = 0;
    W_TEST_BOOL(WMath::TryMultiply64(res, 0x80000000, 2).Succeeded());
    W_TEST_BOOL(res == 0x100000000);

    res = 0;
    W_TEST_BOOL(WMath::TryMultiply64(res, 0xFFFFFFFF, 0xFFFFFFFF).Succeeded());
    W_TEST_BOOL(res == 0xFFFFFFFE00000001);

    res = 0;
    W_TEST_BOOL(WMath::TryMultiply64(res, 0xFFFFFFFFFFFFFFFF, 2).Failed());

    res = 0;
    W_TEST_BOOL(WMath::TryMultiply64(res, 0xFFFFFFFF, 0xFFFFFFFF, 2).Failed());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "TryConvertToSizeT")
  {
    WUInt64 x = WMath::MaxValue<WUInt32>();
    WUInt64 y = x + 1;

    size_t res = 0;

    W_TEST_BOOL(WMath::TryConvertToSizeT(res, x).Succeeded());
    W_TEST_BOOL(res == x);

    res = 0;
#if W_ENABLED(W_PLATFORM_32BIT)
    W_TEST_BOOL(WMath::TryConvertToSizeT(res, y).Failed());
#else
    W_TEST_BOOL(WMath::TryConvertToSizeT(res, y).Succeeded());
    W_TEST_BOOL(res == y);
#endif
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ReplaceNaN")
  {
    W_TEST_FLOAT(WMath::ReplaceNaN(0.0, 42.0), 0.0, 0);
    W_TEST_FLOAT(WMath::ReplaceNaN(WMath::HighValue<Type>(), (Type)2.0), WMath::HighValue<Type>(), 0);
    W_TEST_FLOAT(WMath::ReplaceNaN(-WMath::HighValue<Type>(), (Type)2.0), -WMath::HighValue<Type>(), 0);

    W_TEST_FLOAT(WMath::ReplaceNaN(WMath::NaN<Type>(), (Type)2.0), 2.0, 0);
    W_TEST_FLOAT(WMath::ReplaceNaN(WMath::NaN<double>(), (double)3.0), 3.0, 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ComparisonOperator")
  {
    W_TEST_BOOL(WComparisonOperator::Compare(WComparisonOperator::Equal, 1.0, 1.0));
    W_TEST_BOOL(WComparisonOperator::Compare(WComparisonOperator::Equal, 1.0, 2.0) == false);

    W_TEST_BOOL(WComparisonOperator::Compare(WComparisonOperator::NotEqual, 1.0, 2.0));
    W_TEST_BOOL(WComparisonOperator::Compare(WComparisonOperator::NotEqual, 1.0, 1.0) == false);

    W_TEST_BOOL(WComparisonOperator::Compare(WComparisonOperator::Less, 1.0, 1.0) == false);
    W_TEST_BOOL(WComparisonOperator::Compare(WComparisonOperator::Less, 1.0, 2.0));
    W_TEST_BOOL(WComparisonOperator::Compare(WComparisonOperator::Less, -2.0, -1.0));
    W_TEST_BOOL(WComparisonOperator::Compare(WComparisonOperator::Less, 3.0, 2.0) == false);

    W_TEST_BOOL(WComparisonOperator::Compare(WComparisonOperator::LessEqual, 1.0, 1.0));
    W_TEST_BOOL(WComparisonOperator::Compare(WComparisonOperator::LessEqual, 1.0, 2.0));
    W_TEST_BOOL(WComparisonOperator::Compare(WComparisonOperator::LessEqual, -2.0, -1.0));
    W_TEST_BOOL(WComparisonOperator::Compare(WComparisonOperator::LessEqual, 3.0, 2.0) == false);

    W_TEST_BOOL(WComparisonOperator::Compare(WComparisonOperator::Greater, 1.0, 1.0) == false);
    W_TEST_BOOL(WComparisonOperator::Compare(WComparisonOperator::Greater, 3.0, 2.0));
    W_TEST_BOOL(WComparisonOperator::Compare(WComparisonOperator::Greater, -1.0, -2.0));
    W_TEST_BOOL(WComparisonOperator::Compare(WComparisonOperator::Greater, 2.0, 3.0) == false);

    W_TEST_BOOL(WComparisonOperator::Compare(WComparisonOperator::GreaterEqual, 1.0, 1.0));
    W_TEST_BOOL(WComparisonOperator::Compare(WComparisonOperator::GreaterEqual, 3.0, 2.0));
    W_TEST_BOOL(WComparisonOperator::Compare(WComparisonOperator::GreaterEqual, -1.0, -2.0));
    W_TEST_BOOL(WComparisonOperator::Compare(WComparisonOperator::GreaterEqual, 2.0, 3.0) == false);

    WStringView a = "a";
    WStringView b = "b";
    WStringView c = "c";
    W_TEST_BOOL(WComparisonOperator::Compare(WComparisonOperator::Equal, a, a));
    W_TEST_BOOL(WComparisonOperator::Compare(WComparisonOperator::Equal, a, b) == false);

    W_TEST_BOOL(WComparisonOperator::Compare(WComparisonOperator::NotEqual, a, c));
    W_TEST_BOOL(WComparisonOperator::Compare(WComparisonOperator::NotEqual, a, a) == false);

    W_TEST_BOOL(WComparisonOperator::Compare(WComparisonOperator::Less, a, a) == false);
    W_TEST_BOOL(WComparisonOperator::Compare(WComparisonOperator::Less, a, b));
    W_TEST_BOOL(WComparisonOperator::Compare(WComparisonOperator::Less, c, b) == false);

    W_TEST_BOOL(WComparisonOperator::Compare(WComparisonOperator::LessEqual, a, a));
    W_TEST_BOOL(WComparisonOperator::Compare(WComparisonOperator::LessEqual, a, b));
    W_TEST_BOOL(WComparisonOperator::Compare(WComparisonOperator::LessEqual, c, b) == false);

    W_TEST_BOOL(WComparisonOperator::Compare(WComparisonOperator::Greater, a, a) == false);
    W_TEST_BOOL(WComparisonOperator::Compare(WComparisonOperator::Greater, c, b));
    W_TEST_BOOL(WComparisonOperator::Compare(WComparisonOperator::Greater, a, b) == false);

    W_TEST_BOOL(WComparisonOperator::Compare(WComparisonOperator::GreaterEqual, a, a));
    W_TEST_BOOL(WComparisonOperator::Compare(WComparisonOperator::GreaterEqual, c, b));
    W_TEST_BOOL(WComparisonOperator::Compare(WComparisonOperator::GreaterEqual, a, b) == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WrapUInt")
  {
    W_TEST_INT(WMath::WrapUInt(0, 5), 0);
    W_TEST_INT(WMath::WrapUInt(1, 5), 1);
    W_TEST_INT(WMath::WrapUInt(2, 5), 2);
    W_TEST_INT(WMath::WrapUInt(3, 5), 3);
    W_TEST_INT(WMath::WrapUInt(4, 5), 4);
    W_TEST_INT(WMath::WrapUInt(5, 5), 0);
    W_TEST_INT(WMath::WrapUInt(6, 5), 1);

    W_TEST_INT(WMath::WrapUInt(0, 1), 0);
    W_TEST_INT(WMath::WrapUInt(1, 1), 0);
    W_TEST_INT(WMath::WrapUInt(2, 1), 0);

    W_TEST_INT(WMath::WrapUInt(0, 2), 0);
    W_TEST_INT(WMath::WrapUInt(1, 2), 1);
    W_TEST_INT(WMath::WrapUInt(2, 2), 0);
    W_TEST_INT(WMath::WrapUInt(3, 2), 1);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WrapInt")
  {
    W_TEST_INT(WMath::WrapInt(0, 5), 0);
    W_TEST_INT(WMath::WrapInt(1, 5), 1);
    W_TEST_INT(WMath::WrapInt(2, 5), 2);
    W_TEST_INT(WMath::WrapInt(3, 5), 3);
    W_TEST_INT(WMath::WrapInt(4, 5), 4);
    W_TEST_INT(WMath::WrapInt(5, 5), 0);
    W_TEST_INT(WMath::WrapInt(6, 5), 1);
    W_TEST_INT(WMath::WrapInt(7, 5), 2);

    W_TEST_INT(WMath::WrapInt(-1, 5), 4);
    W_TEST_INT(WMath::WrapInt(-2, 5), 3);
    W_TEST_INT(WMath::WrapInt(-4, 5), 1);
    W_TEST_INT(WMath::WrapInt(-5, 5), 0);
    W_TEST_INT(WMath::WrapInt(-6, 5), 4);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WrapInt (min, max)")
  {
    W_TEST_INT(WMath::WrapInt(0, 1, 5), 4);
    W_TEST_INT(WMath::WrapInt(1, 1, 5), 1);
    W_TEST_INT(WMath::WrapInt(2, 1, 5), 2);
    W_TEST_INT(WMath::WrapInt(3, 1, 5), 3);
    W_TEST_INT(WMath::WrapInt(4, 1, 5), 4);
    W_TEST_INT(WMath::WrapInt(5, 1, 5), 1);
    W_TEST_INT(WMath::WrapInt(6, 1, 5), 2);
    W_TEST_INT(WMath::WrapInt(7, 1, 5), 3);

    W_TEST_INT(WMath::WrapInt(-1, 1, 5), 3);
    W_TEST_INT(WMath::WrapInt(-2, 1, 5), 2);
    W_TEST_INT(WMath::WrapInt(-3, 1, 5), 1);
    W_TEST_INT(WMath::WrapInt(-4, 1, 5), 4);
    W_TEST_INT(WMath::WrapInt(-5, 1, 5), 3);
    W_TEST_INT(WMath::WrapInt(-6, 1, 5), 2);

    W_TEST_INT(WMath::WrapInt(-5, -5, -2), -5);
    W_TEST_INT(WMath::WrapInt(-6, -5, -2), -3);
    W_TEST_INT(WMath::WrapInt(-7, -5, -2), -4);
    W_TEST_INT(WMath::WrapInt(-8, -5, -2), -5);

    W_TEST_INT(WMath::WrapInt(0, -5, -2), -3);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WrapFloat01")
  {
    W_TEST_FLOAT(WMath::WrapFloat01(0.0), (Type)0.0, WMath::VerySmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::WrapFloat01(0.5), (Type)0.5, WMath::VerySmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::WrapFloat01(1.0), (Type)1.0, WMath::VerySmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::WrapFloat01(1.1), (Type)0.1, WMath::VerySmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::WrapFloat01(1.7), (Type)0.7, WMath::VerySmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::WrapFloat01(2.0), (Type)1.0, WMath::VerySmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::WrapFloat01(2.2), (Type)0.2, WMath::VerySmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::WrapFloat01(-0.2), (Type)0.8, WMath::VerySmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::WrapFloat01(-0.9), (Type)0.1, WMath::VerySmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::WrapFloat01(-1.0), (Type)0.0, WMath::VerySmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::WrapFloat01(-1.1), (Type)0.9, WMath::VerySmallEpsilon<Type>());
    W_TEST_FLOAT(WMath::WrapFloat01(-1.01), (Type)0.99, WMath::VerySmallEpsilon<Type>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WrapFloat")
  {
    W_TEST_FLOAT(WMath::WrapFloat(3.5, 3.5, 5.7), 3.5, WMath::DefaultEpsilon<Type>());
    W_TEST_FLOAT(WMath::WrapFloat(5.0, 3.5, 5.7), 5.0, WMath::DefaultEpsilon<Type>());
    W_TEST_FLOAT(WMath::WrapFloat(5.7, 3.5, 5.7), 5.7, WMath::DefaultEpsilon<Type>());
    W_TEST_FLOAT(WMath::WrapFloat(5.8, 3.5, 5.7), 3.6, WMath::DefaultEpsilon<Type>());
    W_TEST_FLOAT(WMath::WrapFloat(3.4, 3.5, 5.7), 5.6, WMath::DefaultEpsilon<Type>());

    W_TEST_FLOAT(WMath::WrapFloat(-1.2, -1.2, 0.5), -1.2, WMath::DefaultEpsilon<Type>());
    W_TEST_FLOAT(WMath::WrapFloat(0.0, -1.2, 0.5), 0.0, WMath::DefaultEpsilon<Type>());
    W_TEST_FLOAT(WMath::WrapFloat(0.5, -1.2, 0.5), 0.5, WMath::DefaultEpsilon<Type>());
    W_TEST_FLOAT(WMath::WrapFloat(0.6, -1.2, 0.5), -1.1, WMath::DefaultEpsilon<Type>());
    W_TEST_FLOAT(WMath::WrapFloat(-1.3, -1.2, 0.5), 0.4, WMath::DefaultEpsilon<Type>());
  }
}

W_CREATE_SIMPLE_TEST_GROUP(Math);

W_CREATE_SIMPLE_TEST(Math, Mathf)
{
  TestMath<float>();
}
W_CREATE_SIMPLE_TEST(Math, Mathd)
{
  TestMath<double>();
}
