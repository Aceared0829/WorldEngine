#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/SimdMath/SimdMath.h>

namespace
{
  WSimdVec4f SimdDegree(float fDegree)
  {
    return WSimdVec4f(WAngle::MakeFromDegree(fDegree));
  }
} // namespace

W_CREATE_SIMPLE_TEST(SimdMath, SimdMath)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Exp")
  {
    float testVals[] = {0.0f, 1.0f, 2.0f};
    for (WUInt32 i = 0; i < W_ARRAY_SIZE(testVals); ++i)
    {
      const float v = testVals[i];
      const float r = WMath::Exp(v);
      W_TEST_BOOL(WSimdMath::Exp(WSimdVec4f(v)).IsEqual(WSimdVec4f(r), 0.000001f).AllSet());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Ln")
  {
    float testVals[] = {1.0f, 2.7182818284f, 7.3890560989f};
    for (WUInt32 i = 0; i < W_ARRAY_SIZE(testVals); ++i)
    {
      const float v = testVals[i];
      const float r = WMath::Ln(v);
      W_TEST_BOOL(WSimdMath::Ln(WSimdVec4f(v)).IsEqual(WSimdVec4f(r), 0.000001f).AllSet());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Log2")
  {
    float testVals[] = {1.0f, 2.0f, 4.0f};
    for (WUInt32 i = 0; i < W_ARRAY_SIZE(testVals); ++i)
    {
      const float v = testVals[i];
      const float r = WMath::Log2(v);
      W_TEST_BOOL(WSimdMath::Log2(WSimdVec4f(v)).IsEqual(WSimdVec4f(r), 0.000001f).AllSet());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Log2i")
  {
    int testVals[] = {0, 1, 2, 3, 4, 6, 7, 8};
    for (WUInt32 i = 0; i < W_ARRAY_SIZE(testVals); ++i)
    {
      const int v = testVals[i];
      const int r = WMath::Log2i(v);
      W_TEST_BOOL((WSimdMath::Log2i(WSimdVec4i(v)) == WSimdVec4i(r)).AllSet());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Log10")
  {
    float testVals[] = {1.0f, 10.0f, 100.0f};
    for (WUInt32 i = 0; i < W_ARRAY_SIZE(testVals); ++i)
    {
      const float v = testVals[i];
      const float r = WMath::Log10(v);
      W_TEST_BOOL(WSimdMath::Log10(WSimdVec4f(v)).IsEqual(WSimdVec4f(r), 0.000001f).AllSet());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Pow2")
  {
    float testVals[] = {0.0f, 1.0f, 2.0f};
    for (WUInt32 i = 0; i < W_ARRAY_SIZE(testVals); ++i)
    {
      const float v = testVals[i];
      const float r = WMath::Pow2(v);
      W_TEST_BOOL(WSimdMath::Pow2(WSimdVec4f(v)).IsEqual(WSimdVec4f(r), 0.000001f).AllSet());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Sin")
  {
    W_TEST_BOOL(WSimdMath::Sin(SimdDegree(0.0f)).IsEqual(WSimdVec4f(0.0f), 0.000001f).AllSet());
    W_TEST_BOOL(WSimdMath::Sin(SimdDegree(90.0f)).IsEqual(WSimdVec4f(1.0f), 0.000001f).AllSet());
    W_TEST_BOOL(WSimdMath::Sin(SimdDegree(180.0f)).IsEqual(WSimdVec4f(0.0f), 0.000001f).AllSet());
    W_TEST_BOOL(WSimdMath::Sin(SimdDegree(270.0f)).IsEqual(WSimdVec4f(-1.0f), 0.000001f).AllSet());

    W_TEST_BOOL(WSimdMath::Sin(SimdDegree(45.0f)).IsEqual(WSimdVec4f(0.7071067f), 0.000001f).AllSet());
    W_TEST_BOOL(WSimdMath::Sin(SimdDegree(135.0f)).IsEqual(WSimdVec4f(0.7071067f), 0.000001f).AllSet());
    W_TEST_BOOL(WSimdMath::Sin(SimdDegree(225.0f)).IsEqual(WSimdVec4f(-0.7071067f), 0.000001f).AllSet());
    W_TEST_BOOL(WSimdMath::Sin(SimdDegree(315.0f)).IsEqual(WSimdVec4f(-0.7071067f), 0.000001f).AllSet());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Cos")
  {
    W_TEST_BOOL(WSimdMath::Cos(SimdDegree(0.0f)).IsEqual(WSimdVec4f(1.0f), 0.000001f).AllSet());
    W_TEST_BOOL(WSimdMath::Cos(SimdDegree(90.0f)).IsEqual(WSimdVec4f(0.0f), 0.000001f).AllSet());
    W_TEST_BOOL(WSimdMath::Cos(SimdDegree(180.0f)).IsEqual(WSimdVec4f(-1.0f), 0.000001f).AllSet());
    W_TEST_BOOL(WSimdMath::Cos(SimdDegree(270.0f)).IsEqual(WSimdVec4f(0.0f), 0.000001f).AllSet());

    W_TEST_BOOL(WSimdMath::Cos(SimdDegree(45.0f)).IsEqual(WSimdVec4f(0.7071067f), 0.000001f).AllSet());
    W_TEST_BOOL(WSimdMath::Cos(SimdDegree(135.0f)).IsEqual(WSimdVec4f(-0.7071067f), 0.000001f).AllSet());
    W_TEST_BOOL(WSimdMath::Cos(SimdDegree(225.0f)).IsEqual(WSimdVec4f(-0.7071067f), 0.000001f).AllSet());
    W_TEST_BOOL(WSimdMath::Cos(SimdDegree(315.0f)).IsEqual(WSimdVec4f(0.7071067f), 0.000001f).AllSet());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Tan")
  {
    W_TEST_BOOL(WSimdMath::Tan(SimdDegree(0.0f)).IsEqual(WSimdVec4f(0.0f), 0.000001f).AllSet());
    W_TEST_BOOL(WSimdMath::Tan(SimdDegree(45.0f)).IsEqual(WSimdVec4f(1.0f), 0.000001f).AllSet());
    W_TEST_BOOL(WSimdMath::Tan(SimdDegree(-45.0f)).IsEqual(WSimdVec4f(-1.0f), 0.000001f).AllSet());
    W_TEST_BOOL((WSimdMath::Tan(SimdDegree(90.00001f)) < WSimdVec4f(1000000.0f)).AllSet());
    W_TEST_BOOL((WSimdMath::Tan(SimdDegree(89.9999f)) > WSimdVec4f(100000.0f)).AllSet());

    // Testing the period of tan(x) centered at 0 and the adjacent ones
    WAngle angle = WAngle::MakeFromDegree(-89.0f);
    while (angle.GetDegree() < 89.0f)
    {
      WSimdVec4f simdAngle(angle.GetRadian());

      WSimdVec4f fTan = WSimdMath::Tan(simdAngle);
      WSimdVec4f fTanPrev = WSimdMath::Tan(SimdDegree(angle.GetDegree() - 180.0f));
      WSimdVec4f fTanNext = WSimdMath::Tan(SimdDegree(angle.GetDegree() + 180.0f));
      WSimdVec4f fSin = WSimdMath::Sin(simdAngle);
      WSimdVec4f fCos = WSimdMath::Cos(simdAngle);

      W_TEST_BOOL((fTan - fTanPrev).IsEqual(WSimdVec4f::MakeZero(), 0.002f).AllSet());
      W_TEST_BOOL((fTan - fTanNext).IsEqual(WSimdVec4f::MakeZero(), 0.002f).AllSet());
      W_TEST_BOOL((fTan - fSin.CompDiv(fCos)).IsEqual(WSimdVec4f::MakeZero(), 0.0005f).AllSet());
      angle += WAngle::MakeFromDegree(1.234f);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ASin")
  {
    W_TEST_BOOL(WSimdMath::ASin(WSimdVec4f(0.0f)).IsEqual(SimdDegree(0.0f), 0.0001f).AllSet());
    W_TEST_BOOL(WSimdMath::ASin(WSimdVec4f(1.0f)).IsEqual(SimdDegree(90.0f), 0.00001f).AllSet());
    W_TEST_BOOL(WSimdMath::ASin(WSimdVec4f(-1.0f)).IsEqual(SimdDegree(-90.0f), 0.00001f).AllSet());

    W_TEST_BOOL(WSimdMath::ASin(WSimdVec4f(0.7071067f)).IsEqual(SimdDegree(45.0f), 0.0001f).AllSet());
    W_TEST_BOOL(WSimdMath::ASin(WSimdVec4f(-0.7071067f)).IsEqual(SimdDegree(-45.0f), 0.0001f).AllSet());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ACos")
  {
    W_TEST_BOOL(WSimdMath::ACos(WSimdVec4f(0.0f)).IsEqual(SimdDegree(90.0f), 0.0001f).AllSet());
    W_TEST_BOOL(WSimdMath::ACos(WSimdVec4f(1.0f)).IsEqual(SimdDegree(0.0f), 0.00001f).AllSet());
    W_TEST_BOOL(WSimdMath::ACos(WSimdVec4f(-1.0f)).IsEqual(SimdDegree(180.0f), 0.0001f).AllSet());

    W_TEST_BOOL(WSimdMath::ACos(WSimdVec4f(0.7071067f)).IsEqual(SimdDegree(45.0f), 0.0001f).AllSet());
    W_TEST_BOOL(WSimdMath::ACos(WSimdVec4f(-0.7071067f)).IsEqual(SimdDegree(135.0f), 0.0001f).AllSet());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ATan")
  {
    W_TEST_BOOL(WSimdMath::ATan(WSimdVec4f(0.0f)).IsEqual(SimdDegree(0.0f), 0.0000001f).AllSet());
    W_TEST_BOOL(WSimdMath::ATan(WSimdVec4f(1.0f)).IsEqual(SimdDegree(45.0f), 0.00001f).AllSet());
    W_TEST_BOOL(WSimdMath::ATan(WSimdVec4f(-1.0f)).IsEqual(SimdDegree(-45.0f), 0.00001f).AllSet());
    W_TEST_BOOL(WSimdMath::ATan(WSimdVec4f(10000000.0f)).IsEqual(SimdDegree(90.0f), 0.00002f).AllSet());
    W_TEST_BOOL(WSimdMath::ATan(WSimdVec4f(-10000000.0f)).IsEqual(SimdDegree(-90.0f), 0.00002f).AllSet());
  }
}
