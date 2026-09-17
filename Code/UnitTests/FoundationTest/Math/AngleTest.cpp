#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Angle.h>

W_CREATE_SIMPLE_TEST(Math, Angle)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "DegToRad")
  {
    W_TEST_FLOAT(WAngle::DegToRad(0.0f), 0.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(WAngle::DegToRad(45.0f), 0.785398163f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(WAngle::DegToRad(90.0f), 1.570796327f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(WAngle::DegToRad(120.0f), 2.094395102f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(WAngle::DegToRad(170.0f), 2.967059728f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(WAngle::DegToRad(180.0f), 3.141592654f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(WAngle::DegToRad(250.0f), 4.36332313f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(WAngle::DegToRad(320.0f), 5.585053606f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(WAngle::DegToRad(360.0f), 6.283185307f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(WAngle::DegToRad(700.0f), 12.217304764f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(WAngle::DegToRad(-123.0f), -2.14675498f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(WAngle::DegToRad(-1234.0f), -21.53736297f, WMath::DefaultEpsilon<float>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "RadToDeg")
  {
    W_TEST_FLOAT(WAngle::RadToDeg(0.0f), 0.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(WAngle::RadToDeg(0.785398163f), 45.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(WAngle::RadToDeg(1.570796327f), 90.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(WAngle::RadToDeg(2.094395102f), 120.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(WAngle::RadToDeg(2.967059728f), 170.0f, WMath::LargeEpsilon<float>());
    W_TEST_FLOAT(WAngle::RadToDeg(3.141592654f), 180.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(WAngle::RadToDeg(4.36332313f), 250.0f, WMath::LargeEpsilon<float>());
    W_TEST_FLOAT(WAngle::RadToDeg(5.585053606f), 320.0f, WMath::LargeEpsilon<float>());
    W_TEST_FLOAT(WAngle::RadToDeg(6.283185307f), 360.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(WAngle::RadToDeg(12.217304764f), 700.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(WAngle::RadToDeg(-2.14675498f), -123.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(WAngle::RadToDeg(-21.53736297f), -1234.0f, WMath::HugeEpsilon<float>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Init")
  {
    WAngleT a0;
    W_TEST_FLOAT(a0.GetRadian(), 0.0f, 0.0f);
    W_TEST_FLOAT(a0.GetDegree(), 0.0f, 0.0f);

    WAngle a1 = WAngle::MakeFromRadian(1.570796327f);
    W_TEST_FLOAT(a1.GetRadian(), 1.570796327f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(a1.GetDegree(), 90.0f, WMath::DefaultEpsilon<float>());

    WAngle a2 = WAngle::MakeFromDegree(90);
    W_TEST_FLOAT(a2.GetRadian(), 1.570796327f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(a2.GetDegree(), 90.0f, WMath::DefaultEpsilon<float>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "NormalizeRange / IsEqual ")
  {
    WAngleT a;

    for (WInt32 i = 1; i < 359; i++)
    {
      a = WAngleT::MakeFromDegree((float)i);
      a.NormalizeRange();
      W_TEST_FLOAT(a.GetDegree(), (float)i, 0.01f);
      a = WAngleT::MakeFromDegree((float)i);
      a.NormalizeRange();
      W_TEST_FLOAT(a.GetDegree(), (float)i, 0.01f);
      a = WAngleT::MakeFromDegree((float)i + 360.0f);
      a.NormalizeRange();
      W_TEST_FLOAT(a.GetDegree(), (float)i, 0.01f);
      a = WAngleT::MakeFromDegree((float)i - 360.0f);
      a.NormalizeRange();
      W_TEST_FLOAT(a.GetDegree(), (float)i, 0.01f);
      a = WAngleT::MakeFromDegree((float)i + 3600.0f);
      a.NormalizeRange();
      W_TEST_FLOAT(a.GetDegree(), (float)i, 0.01f);
      a = WAngleT::MakeFromDegree((float)i - 3600.0f);
      a.NormalizeRange();
      W_TEST_FLOAT(a.GetDegree(), (float)i, 0.01f);
      a = WAngleT::MakeFromDegree((float)i + 36000.0f);
      a.NormalizeRange();
      W_TEST_FLOAT(a.GetDegree(), (float)i, 0.01f);
      a = WAngleT::MakeFromDegree((float)i - 36000.0f);
      a.NormalizeRange();
      W_TEST_FLOAT(a.GetDegree(), (float)i, 0.01f);
    }

    for (WInt32 i = 0; i < 360; i++)
    {
      a = WAngleT::MakeFromDegree((float)i);
      W_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(WAngleT::MakeFromDegree((float)i), WAngleT::MakeFromDegree(0.01f)));
      a = WAngleT::MakeFromDegree((float)i);
      W_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(WAngleT::MakeFromDegree((float)i), WAngleT::MakeFromDegree(0.01f)));
      a = WAngleT::MakeFromDegree((float)i + 360.0f);
      W_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(WAngleT::MakeFromDegree((float)i), WAngleT::MakeFromDegree(0.01f)));
      a = WAngleT::MakeFromDegree((float)i - 360.0f);
      W_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(WAngleT::MakeFromDegree((float)i), WAngleT::MakeFromDegree(0.01f)));
      a = WAngleT::MakeFromDegree((float)i + 3600.0f);
      W_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(WAngleT::MakeFromDegree((float)i), WAngleT::MakeFromDegree(0.01f)));
      a = WAngleT::MakeFromDegree((float)i - 3600.0f);
      W_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(WAngleT::MakeFromDegree((float)i), WAngleT::MakeFromDegree(0.01f)));
      a = WAngleT::MakeFromDegree((float)i + 36000.0f);
      W_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(WAngleT::MakeFromDegree((float)i), WAngleT::MakeFromDegree(0.01f)));
      a = WAngleT::MakeFromDegree((float)i - 36000.0f);
      W_TEST_BOOL(a.GetNormalizedRange().IsEqualSimple(WAngleT::MakeFromDegree((float)i), WAngleT::MakeFromDegree(0.01f)));
    }

    for (WInt32 i = 0; i < 360; i++)
    {
      a = WAngleT::MakeFromDegree((float)i);
      W_TEST_BOOL(a.IsEqualNormalized(WAngleT::MakeFromDegree((float)i), WAngleT::MakeFromDegree(0.01f)));
      a = WAngleT::MakeFromDegree((float)i);
      W_TEST_BOOL(a.IsEqualNormalized(WAngleT::MakeFromDegree((float)i), WAngleT::MakeFromDegree(0.01f)));
      a = WAngleT::MakeFromDegree((float)i + 360.0f);
      W_TEST_BOOL(a.IsEqualNormalized(WAngleT::MakeFromDegree((float)i), WAngleT::MakeFromDegree(0.01f)));
      a = WAngleT::MakeFromDegree((float)i - 360.0f);
      W_TEST_BOOL(a.IsEqualNormalized(WAngleT::MakeFromDegree((float)i), WAngleT::MakeFromDegree(0.01f)));
      a = WAngleT::MakeFromDegree((float)i + 3600.0f);
      W_TEST_BOOL(a.IsEqualNormalized(WAngleT::MakeFromDegree((float)i), WAngleT::MakeFromDegree(0.01f)));
      a = WAngleT::MakeFromDegree((float)i - 3600.0f);
      W_TEST_BOOL(a.IsEqualNormalized(WAngleT::MakeFromDegree((float)i), WAngleT::MakeFromDegree(0.01f)));
      a = WAngleT::MakeFromDegree((float)i + 36000.0f);
      W_TEST_BOOL(a.IsEqualNormalized(WAngleT::MakeFromDegree((float)i), WAngleT::MakeFromDegree(0.01f)));
      a = WAngleT::MakeFromDegree((float)i - 36000.0f);
      W_TEST_BOOL(a.IsEqualNormalized(WAngleT::MakeFromDegree((float)i), WAngleT::MakeFromDegree(0.01f)));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "AngleBetween")
  {
    W_TEST_FLOAT(WAngleT::AngleBetween(WAngleT::MakeFromDegree(0), WAngleT::MakeFromDegree(0)).GetDegree(), 0.0f, 0.0001f);
    W_TEST_FLOAT(WAngleT::AngleBetween(WAngleT::MakeFromDegree(0), WAngleT::MakeFromDegree(360)).GetDegree(), 0.0f, 0.0001f);
    W_TEST_FLOAT(WAngleT::AngleBetween(WAngleT::MakeFromDegree(360), WAngleT::MakeFromDegree(360)).GetDegree(), 0.0f, 0.0001f);
    W_TEST_FLOAT(WAngleT::AngleBetween(WAngleT::MakeFromDegree(360), WAngleT::MakeFromDegree(0)).GetDegree(), 0.0f, 0.0001f);

    W_TEST_FLOAT(WAngleT::AngleBetween(WAngleT::MakeFromDegree(5), WAngleT::MakeFromDegree(186)).GetDegree(), 179.0f, 0.0001f);
    W_TEST_FLOAT(WAngleT::AngleBetween(WAngleT::MakeFromDegree(-5), WAngleT::MakeFromDegree(-186)).GetDegree(), 179.0f, 0.0001f);

    W_TEST_FLOAT(WAngleT::AngleBetween(WAngleT::MakeFromDegree(360.0f + 5), WAngleT::MakeFromDegree(360.0f + 186)).GetDegree(), 179.0f, 0.0001f);
    W_TEST_FLOAT(WAngleT::AngleBetween(WAngleT::MakeFromDegree(360.0f + -5), WAngleT::MakeFromDegree(360.0f - 186)).GetDegree(), 179.0f, 0.0001f);

    for (WInt32 i = 0; i <= 179; ++i)
      W_TEST_FLOAT(WAngleT::AngleBetween(WAngleT::MakeFromDegree((float)i), WAngleT::MakeFromDegree((float)(i + i))).GetDegree(), (float)i, 0.0001f);

    for (WInt32 i = -179; i <= 0; ++i)
      W_TEST_FLOAT(WAngleT::AngleBetween(WAngleT::MakeFromDegree((float)i), WAngleT::MakeFromDegree((float)(i + i))).GetDegree(), (float)-i, 0.0001f);
  }
}
