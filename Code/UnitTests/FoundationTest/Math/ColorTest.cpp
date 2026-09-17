#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/Mat4.h>

W_CREATE_SIMPLE_TEST(Math, Color)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor empty")
  {
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
    if (WMath::SupportsNaN<WMathTestType>())
    {
      // In debug the default constructor initializes everything with NaN.
      WColor defCtor;
      W_TEST_BOOL(WMath::IsNaN(defCtor.r) && WMath::IsNaN(defCtor.g) && WMath::IsNaN(defCtor.b) && WMath::IsNaN(defCtor.a));
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    float testBlock[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    WColor* pDefCtor = ::new ((void*)&testBlock[0]) WColor;
    W_TEST_BOOL(pDefCtor->r == 1.0f && pDefCtor->g == 2.0f && pDefCtor->b == 3.0f && pDefCtor->a == 4.0f);
#endif

    // Make sure the class didn't accidentally change in size
    W_TEST_BOOL(sizeof(WColor) == sizeof(float) * 4);
  }
  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor components")
  {
    WColor init3F(0.5f, 0.6f, 0.7f);
    W_TEST_BOOL(init3F.r == 0.5f && init3F.g == 0.6f && init3F.b == 0.7f && init3F.a == 1.0f);

    WColor init4F(0.5f, 0.6f, 0.7f, 0.8f);
    W_TEST_BOOL(init4F.r == 0.5f && init4F.g == 0.6f && init4F.b == 0.7f && init4F.a == 0.8f);
  }
  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor copy")
  {
    WColor init4F(0.5f, 0.6f, 0.7f, 0.8f);
    WColor copy(init4F);
    W_TEST_BOOL(copy.r == 0.5f && copy.g == 0.6f && copy.b == 0.7f && copy.a == 0.8f);
  }

  {
    WColor cornflowerBlue(WColor(0.39f, 0.58f, 0.93f));

    W_TEST_BLOCK(WTestBlock::Enabled, "Conversion float")
    {
      float* pFloats = cornflowerBlue.GetData();
      W_TEST_BOOL(
        pFloats[0] == cornflowerBlue.r && pFloats[1] == cornflowerBlue.g && pFloats[2] == cornflowerBlue.b && pFloats[3] == cornflowerBlue.a);

      const float* pConstFloats = cornflowerBlue.GetData();
      W_TEST_BOOL(pConstFloats[0] == cornflowerBlue.r && pConstFloats[1] == cornflowerBlue.g && pConstFloats[2] == cornflowerBlue.b &&
                   pConstFloats[3] == cornflowerBlue.a);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "HSV conversion")
  {
    WColor normalizedColor(0.0f, 1.0f, 0.999f, 0.0001f);
    W_TEST_BOOL(normalizedColor.IsNormalized());
    WColor notNormalizedColor0(-0.01f, 1.0f, 0.999f, 0.0001f);
    W_TEST_BOOL(!notNormalizedColor0.IsNormalized());
    WColor notNormalizedColor1(0.5f, 1.1f, 0.9f, 0.1f);
    W_TEST_BOOL(!notNormalizedColor1.IsNormalized());
    WColor notNormalizedColor2(0.1f, 1.0f, 1.999f, 0.1f);
    W_TEST_BOOL(!notNormalizedColor2.IsNormalized());
    WColor notNormalizedColor3(0.1f, 1.0f, 1.0f, -0.1f);
    W_TEST_BOOL(!notNormalizedColor3.IsNormalized());


    // hsv test - took some samples from http://www.javascripter.net/faq/rgb2hsv.htm
    const WColorGammaUB rgb[] = {WColorGammaUB(255, 255, 255), WColorGammaUB(0, 0, 0), WColorGammaUB(123, 12, 1), WColorGammaUB(31, 112, 153)};
    const WVec3 hsv[] = {WVec3(0, 0, 1), WVec3(0, 0, 0), WVec3(5.4f, 0.991f, 0.48f), WVec3(200.2f, 0.797f, 0.600f)};

    for (int i = 0; i < 4; ++i)
    {
      const WColor color = rgb[i];
      float hue, sat, val;
      color.GetHSV(hue, sat, val);

      W_TEST_FLOAT(hue, hsv[i].x, 0.1f);
      W_TEST_FLOAT(sat, hsv[i].y, 0.1f);
      W_TEST_FLOAT(val, hsv[i].z, 0.1f);

      WColor fromHSV = WColor::MakeHSV(hsv[i].x, hsv[i].y, hsv[i].z);
      W_TEST_FLOAT(fromHSV.r, color.r, 0.01f);
      W_TEST_FLOAT(fromHSV.g, color.g, 0.01f);
      W_TEST_FLOAT(fromHSV.b, color.b, 0.01f);
    }
  }

  {
    if (WMath::SupportsNaN<WMathTestType>())
    {
      float fNaN = WMath::NaN<float>();
      const WColor nanArray[4] = {
        WColor(fNaN, 0.0f, 0.0f, 0.0f), WColor(0.0f, fNaN, 0.0f, 0.0f), WColor(0.0f, 0.0f, fNaN, 0.0f), WColor(0.0f, 0.0f, 0.0f, fNaN)};
      const WColor compArray[4] = {
        WColor(1.0f, 0.0f, 0.0f, 0.0f), WColor(0.0f, 1.0f, 0.0f, 0.0f), WColor(0.0f, 0.0f, 1.0f, 0.0f), WColor(0.0f, 0.0f, 0.0f, 1.0f)};


      W_TEST_BLOCK(WTestBlock::Enabled, "IsNaN")
      {
        for (int i = 0; i < 4; ++i)
        {
          W_TEST_BOOL(nanArray[i].IsNaN());
          W_TEST_BOOL(!compArray[i].IsNaN());
        }
      }

      W_TEST_BLOCK(WTestBlock::Enabled, "IsValid")
      {
        for (int i = 0; i < 4; ++i)
        {
          W_TEST_BOOL(!nanArray[i].IsValid());
          W_TEST_BOOL(compArray[i].IsValid());

          W_TEST_BOOL(!(compArray[i] * WMath::Infinity<float>()).IsValid());
          W_TEST_BOOL(!(compArray[i] * -WMath::Infinity<float>()).IsValid());
        }
      }
    }
  }

  {
    const WColor op1(-4.0, 0.2f, -7.0f, -0.0f);
    const WColor op2(2.0, 0.3f, 0.0f, 1.0f);
    const WColor compArray[4] = {
      WColor(1.0f, 0.0f, 0.0f, 0.0f), WColor(0.0f, 1.0f, 0.0f, 0.0f), WColor(0.0f, 0.0f, 1.0f, 0.0f), WColor(0.0f, 0.0f, 0.0f, 1.0f)};

    W_TEST_BLOCK(WTestBlock::Enabled, "SetRGB / SetRGBA")
    {
      WColor c1(0, 0, 0, 0);

      c1.SetRGBA(1, 2, 3, 4);

      W_TEST_BOOL(c1 == WColor(1, 2, 3, 4));

      c1.SetRGB(5, 6, 7);

      W_TEST_BOOL(c1 == WColor(5, 6, 7, 4));
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "IsIdenticalRGB")
    {
      WColor c1(0, 0, 0, 0);
      WColor c2(0, 0, 0, 1);

      W_TEST_BOOL(c1.IsIdenticalRGB(c2));
      W_TEST_BOOL(!c1.IsIdenticalRGBA(c2));
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "IsIdenticalRGBA")
    {
      W_TEST_BOOL(op1.IsIdenticalRGBA(op1));
      for (int i = 0; i < 4; ++i)
      {
        W_TEST_BOOL(!op1.IsIdenticalRGBA(op1 + WMath::SmallEpsilon<float>() * compArray[i]));
        W_TEST_BOOL(!op1.IsIdenticalRGBA(op1 - WMath::SmallEpsilon<float>() * compArray[i]));
      }
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "IsEqualRGB")
    {
      WColor c1(0, 0, 0, 0);
      WColor c2(0, 0, 0.2f, 1);

      W_TEST_BOOL(!c1.IsEqualRGB(c2, 0.1f));
      W_TEST_BOOL(c1.IsEqualRGB(c2, 0.3f));
      W_TEST_BOOL(!c1.IsEqualRGBA(c2, 0.3f));
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "IsEqualRGBA")
    {
      W_TEST_BOOL(op1.IsEqualRGBA(op1, 0.0f));
      for (int i = 0; i < 4; ++i)
      {
        W_TEST_BOOL(op1.IsEqualRGBA(op1 + WMath::SmallEpsilon<float>() * compArray[i], 2 * WMath::SmallEpsilon<float>()));
        W_TEST_BOOL(op1.IsEqualRGBA(op1 - WMath::SmallEpsilon<float>() * compArray[i], 2 * WMath::SmallEpsilon<float>()));
        W_TEST_BOOL(op1.IsEqualRGBA(op1 + WMath::DefaultEpsilon<float>() * compArray[i], 2 * WMath::DefaultEpsilon<float>()));
        W_TEST_BOOL(op1.IsEqualRGBA(op1 - WMath::DefaultEpsilon<float>() * compArray[i], 2 * WMath::DefaultEpsilon<float>()));
      }
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "operator+= (WColor)")
    {
      WColor plusAssign = op1;
      plusAssign += op2;
      W_TEST_BOOL(plusAssign.IsEqualRGBA(WColor(-2.0f, 0.5f, -7.0f, 1.0f), WMath::SmallEpsilon<float>()));
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "operator-= (WColor)")
    {
      WColor minusAssign = op1;
      minusAssign -= op2;
      W_TEST_BOOL(minusAssign.IsEqualRGBA(WColor(-6.0f, -0.1f, -7.0f, -1.0f), WMath::SmallEpsilon<float>()));
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "ooperator*= (float)")
    {
      WColor mulFloat = op1;
      mulFloat *= 2.0f;
      W_TEST_BOOL(mulFloat.IsEqualRGBA(WColor(-8.0f, 0.4f, -14.0f, -0.0f), WMath::SmallEpsilon<float>()));
      mulFloat *= 0.0f;
      W_TEST_BOOL(mulFloat.IsEqualRGBA(WColor(0.0f, 0.0f, 0.0f, 0.0f), WMath::SmallEpsilon<float>()));
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "operator/= (float)")
    {
      WColor vDivFloat = op1;
      vDivFloat /= 2.0f;
      W_TEST_BOOL(vDivFloat.IsEqualRGBA(WColor(-2.0f, 0.1f, -3.5f, -0.0f), WMath::SmallEpsilon<float>()));
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "operator+ (WColor, WColor)")
    {
      WColor plus = (op1 + op2);
      W_TEST_BOOL(plus.IsEqualRGBA(WColor(-2.0f, 0.5f, -7.0f, 1.0f), WMath::SmallEpsilon<float>()));
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "operator- (WColor, WColor)")
    {
      WColor minus = (op1 - op2);
      W_TEST_BOOL(minus.IsEqualRGBA(WColor(-6.0f, -0.1f, -7.0f, -1.0f), WMath::SmallEpsilon<float>()));
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "operator* (float, WColor)")
    {
      WColor mulFloatVec4 = 2 * op1;
      W_TEST_BOOL(mulFloatVec4.IsEqualRGBA(WColor(-8.0f, 0.4f, -14.0f, -0.0f), WMath::SmallEpsilon<float>()));
      mulFloatVec4 = ((float)0 * op1);
      W_TEST_BOOL(mulFloatVec4.IsEqualRGBA(WColor(0.0f, 0.0f, 0.0f, 0.0f), WMath::SmallEpsilon<float>()));
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "operator* (WColor, float)")
    {
      WColor mulVec4Float = op1 * 2;
      W_TEST_BOOL(mulVec4Float.IsEqualRGBA(WColor(-8.0f, 0.4f, -14.0f, -0.0f), WMath::SmallEpsilon<float>()));
      mulVec4Float = (op1 * (float)0);
      W_TEST_BOOL(mulVec4Float.IsEqualRGBA(WColor(0.0f, 0.0f, 0.0f, 0.0f), WMath::SmallEpsilon<float>()));
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "operator/ (WColor, float)")
    {
      WColor vDivVec4Float = op1 / 2;
      W_TEST_BOOL(vDivVec4Float.IsEqualRGBA(WColor(-2.0f, 0.1f, -3.5f, -0.0f), WMath::SmallEpsilon<float>()));
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "operator== (WColor, WColor)")
    {
      W_TEST_BOOL(op1 == op1);
      for (int i = 0; i < 4; ++i)
      {
        W_TEST_BOOL(!(op1 == (op1 + WMath::SmallEpsilon<float>() * compArray[i])));
        W_TEST_BOOL(!(op1 == (op1 - WMath::SmallEpsilon<float>() * compArray[i])));
      }
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "operator< (WColor, WColor)")
    {
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

    W_TEST_BLOCK(WTestBlock::Enabled, "operator!= (WColor, WColor)")
    {
      W_TEST_BOOL(!(op1 != op1));
      for (int i = 0; i < 4; ++i)
      {
        W_TEST_BOOL(op1 != (op1 + WMath::SmallEpsilon<float>() * compArray[i]));
        W_TEST_BOOL(op1 != (op1 - WMath::SmallEpsilon<float>() * compArray[i]));
      }
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "operator= (WColorLinearUB)")
    {
      WColor c;
      WColorLinearUB lin(50, 100, 150, 255);

      c = lin;

      W_TEST_FLOAT(c.r, 50 / 255.0f, 0.001f);
      W_TEST_FLOAT(c.g, 100 / 255.0f, 0.001f);
      W_TEST_FLOAT(c.b, 150 / 255.0f, 0.001f);
      W_TEST_FLOAT(c.a, 1.0f, 0.001f);
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "operator= (WColorGammaUB) / constructor(WColorGammaUB)")
    {
      WColor c;
      WColorGammaUB gamma(50, 100, 150, 255);

      c = gamma;
      WColor c3 = gamma;

      W_TEST_BOOL(c == c3);

      W_TEST_FLOAT(c.r, 0.031f, 0.001f);
      W_TEST_FLOAT(c.g, 0.127f, 0.001f);
      W_TEST_FLOAT(c.b, 0.304f, 0.001f);
      W_TEST_FLOAT(c.a, 1.0f, 0.001f);

      WColorGammaUB c2 = c;

      W_TEST_INT(c2.r, 50);
      W_TEST_INT(c2.g, 100);
      W_TEST_INT(c2.b, 150);
      W_TEST_INT(c2.a, 255);
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "GetInvertedColor")
    {
      const WColor c1(0.1f, 0.3f, 0.7f, 0.9f);

      WColor c2 = c1.GetInvertedColor();

      W_TEST_BOOL(c2.IsEqualRGBA(WColor(0.9f, 0.7f, 0.3f, 0.1f), 0.01f));
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "GetLuminance")
    {
      W_TEST_FLOAT(WColor::Black.GetLuminance(), 0.0f, 0.001f);
      W_TEST_FLOAT(WColor::White.GetLuminance(), 1.0f, 0.001f);

      W_TEST_FLOAT(WColor(0.5f, 0.5f, 0.5f).GetLuminance(), 0.2126f * 0.5f + 0.7152f * 0.5f + 0.0722f * 0.5f, 0.001f);
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "GetComplementaryColor")
    {
      // black and white have no complementary colors, or rather, they are their own complementary colors, apparently
      W_TEST_BOOL(WColor::Black.GetComplementaryColor().IsEqualRGBA(WColor::Black, 0.001f));
      W_TEST_BOOL(WColor::White.GetComplementaryColor().IsEqualRGBA(WColor::White, 0.001f));

      W_TEST_BOOL(WColor::Red.GetComplementaryColor().IsEqualRGBA(WColor::Cyan, 0.001f));
      W_TEST_BOOL(WColor::Lime.GetComplementaryColor().IsEqualRGBA(WColor::Magenta, 0.001f));
      W_TEST_BOOL(WColor::Blue.GetComplementaryColor().IsEqualRGBA(WColor::Yellow, 0.001f));
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "GetSaturation")
    {
      W_TEST_FLOAT(WColor::Black.GetSaturation(), 0.0f, 0.001f);
      W_TEST_FLOAT(WColor::White.GetSaturation(), 0.0f, 0.001f);
      W_TEST_FLOAT(WColor::Red.GetSaturation(), 1.0f, 0.001f);
      W_TEST_FLOAT(WColor::Lime.GetSaturation(), 1.0f, 0.001f);
      ;
      W_TEST_FLOAT(WColor::Blue.GetSaturation(), 1.0f, 0.001f);
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "operator * / *= (WMat4)")
    {
      WMat4 m;
      m.SetIdentity();
      m = WMat4::MakeScaling(WVec3(0.5f, 0.75f, 0.25f));
      m.SetTranslationVector(WVec3(0.1f, 0.2f, 0.3f));

      WColor c1 = m * WColor::White;

      W_TEST_BOOL(c1.IsEqualRGBA(WColor(0.6f, 0.95f, 0.55f, 1.0f), 0.01f));
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "CalcAverageRGB")
    {
      WColor c1(0.6f, 0.3f, 0.9f, 0.5f);
      W_TEST_FLOAT(c1.CalcAverageRGB(), (0.6f + 0.3f + 0.9f) / 3.0f, 0.001f);

      WColor c2(1.0f, 1.0f, 1.0f, 0.0f);
      W_TEST_FLOAT(c2.CalcAverageRGB(), 1.0f, 0.001f);

      WColor c3(0.0f, 0.0f, 0.0f, 1.0f);
      W_TEST_FLOAT(c3.CalcAverageRGB(), 0.0f, 0.001f);
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "ScaleRGB")
    {
      WColor c1(0.5f, 0.6f, 0.7f, 0.8f);
      c1.ScaleRGB(2.0f);
      W_TEST_BOOL(c1.IsEqualRGBA(WColor(1.0f, 1.2f, 1.4f, 0.8f), 0.001f));

      WColor c2(0.4f, 0.3f, 0.2f, 0.1f);
      c2.ScaleRGB(0.5f);
      W_TEST_BOOL(c2.IsEqualRGBA(WColor(0.2f, 0.15f, 0.1f, 0.1f), 0.001f));
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "ScaleRGBA")
    {
      WColor c1(0.5f, 0.6f, 0.7f, 0.8f);
      c1.ScaleRGBA(2.0f);
      W_TEST_BOOL(c1.IsEqualRGBA(WColor(1.0f, 1.2f, 1.4f, 1.6f), 0.001f));

      WColor c2(0.4f, 0.3f, 0.2f, 0.1f);
      c2.ScaleRGBA(0.5f);
      W_TEST_BOOL(c2.IsEqualRGBA(WColor(0.2f, 0.15f, 0.1f, 0.05f), 0.001f));
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "ComputeHdrMultiplier")
    {
      // LDR colors should return 1.0
      WColor ldr1(0.5f, 0.3f, 0.7f, 1.0f);
      W_TEST_FLOAT(ldr1.ComputeHdrMultiplier(), 1.0f, 0.001f);

      WColor ldr2(1.0f, 0.9f, 0.8f, 0.5f);
      W_TEST_FLOAT(ldr2.ComputeHdrMultiplier(), 1.0f, 0.001f);

      // HDR colors should return the largest component
      WColor hdr1(2.0f, 1.5f, 1.0f, 0.5f);
      W_TEST_FLOAT(hdr1.ComputeHdrMultiplier(), 2.0f, 0.001f);

      WColor hdr2(1.0f, 3.5f, 2.2f, 1.0f);
      W_TEST_FLOAT(hdr2.ComputeHdrMultiplier(), 3.5f, 0.001f);
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "ComputeHdrExposureValue")
    {
      // LDR colors should return 0
      WColor ldr(0.5f, 0.3f, 0.7f, 1.0f);
      W_TEST_FLOAT(ldr.ComputeHdrExposureValue(), 0.0f, 0.001f);

      // HDR colors should return log2 of the multiplier
      WColor hdr1(2.0f, 1.0f, 1.0f, 0.5f);
      W_TEST_FLOAT(hdr1.ComputeHdrExposureValue(), 1.0f, 0.001f); // log2(2) = 1

      WColor hdr2(4.0f, 2.0f, 1.0f, 0.5f);
      W_TEST_FLOAT(hdr2.ComputeHdrExposureValue(), 2.0f, 0.001f); // log2(4) = 2
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "ApplyHdrExposureValue")
    {
      WColor c1(0.5f, 0.25f, 0.125f, 1.0f);
      c1.ApplyHdrExposureValue(2.0f); // 2^2 = 4
      W_TEST_BOOL(c1.IsEqualRGBA(WColor(2.0f, 1.0f, 0.5f, 1.0f), 0.001f));

      WColor c2(1.0f, 0.5f, 0.25f, 0.8f);
      c2.ApplyHdrExposureValue(-1.0f); // 2^-1 = 0.5
      W_TEST_BOOL(c2.IsEqualRGBA(WColor(0.5f, 0.25f, 0.125f, 0.8f), 0.001f));
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "NormalizeToLdrRange")
    {
      // HDR color should be normalized
      WColor hdr(4.0f, 2.0f, 1.0f, 0.5f);
      hdr.NormalizeToLdrRange();
      W_TEST_BOOL(hdr.IsEqualRGBA(WColor(1.0f, 0.5f, 0.25f, 0.5f), 0.001f));

      // LDR color should remain unchanged
      WColor ldr(0.8f, 0.6f, 0.4f, 1.0f);
      ldr.NormalizeToLdrRange();
      W_TEST_BOOL(ldr.IsEqualRGBA(WColor(0.8f, 0.6f, 0.4f, 1.0f), 0.001f));
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "GetDarker")
    {
      WColor bright(0.8f, 0.6f, 0.4f, 1.0f);
      WColor darker = bright.GetDarker(2.0f);

      // Should be darker (lower values) but same alpha
      W_TEST_BOOL(darker.r < bright.r && darker.g < bright.g && darker.b < bright.b);
      W_TEST_FLOAT(darker.a, bright.a, 0.001f);

      // Test default factor
      WColor darker2 = bright.GetDarker();
      W_TEST_BOOL(darker2.r < bright.r && darker2.g < bright.g && darker2.b < bright.b);
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "WithAlpha")
    {
      WColor c1(0.5f, 0.6f, 0.7f, 0.8f);
      WColor c2 = c1.WithAlpha(0.3f);

      W_TEST_BOOL(c2.IsEqualRGBA(WColor(0.5f, 0.6f, 0.7f, 0.3f), 0.001f));
      // Original should be unchanged
      W_TEST_BOOL(c1.IsEqualRGBA(WColor(0.5f, 0.6f, 0.7f, 0.8f), 0.001f));
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "ToRGBA8")
    {
      WColor c1(1.0f, 0.5f, 0.25f, 0.0f);
      WUInt32 rgba = c1.ToRGBA8();

      // R=255, G=128, B=64, A=0 -> 0xFF804000 (R in MSB, A in LSB)
      W_TEST_INT((rgba >> 24) & 0xFF, 255); // R
      W_TEST_INT((rgba >> 16) & 0xFF, 128); // G
      W_TEST_INT((rgba >> 8) & 0xFF, 64);   // B
      W_TEST_INT(rgba & 0xFF, 0);           // A
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "ToABGR8")
    {
      WColor c1(1.0f, 0.5f, 0.25f, 0.0f);
      WUInt32 abgr = c1.ToABGR8();

      // A=0, B=64, G=128, R=255 -> 0x004080FF (A in MSB, R in LSB)
      W_TEST_INT((abgr >> 24) & 0xFF, 0);  // A
      W_TEST_INT((abgr >> 16) & 0xFF, 64); // B
      W_TEST_INT((abgr >> 8) & 0xFF, 128); // G
      W_TEST_INT(abgr & 0xFF, 255);        // R
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "Static factory functions")
    {
      // MakeNaN
      if (WMath::SupportsNaN<float>())
      {
        WColor nanColor = WColor::MakeNaN();
        W_TEST_BOOL(nanColor.IsNaN());
      }

      // MakeZero
      WColor zeroColor = WColor::MakeZero();
      W_TEST_BOOL(zeroColor.IsEqualRGBA(WColor(0.0f, 0.0f, 0.0f, 0.0f), 0.001f));

      // MakeRGBA
      WColor rgbaColor = WColor::MakeRGBA(0.1f, 0.2f, 0.3f, 0.4f);
      W_TEST_BOOL(rgbaColor.IsEqualRGBA(WColor(0.1f, 0.2f, 0.3f, 0.4f), 0.001f));

      WColor rgbColor = WColor::MakeRGBA(0.5f, 0.6f, 0.7f);
      W_TEST_BOOL(rgbColor.IsEqualRGBA(WColor(0.5f, 0.6f, 0.7f, 1.0f), 0.001f));
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "GetAsVec4")
    {
      WColor c1(0.1f, 0.2f, 0.3f, 0.4f);
      WVec4 v1 = c1.GetAsVec4();
      W_TEST_BOOL(v1.IsEqual(WVec4(0.1f, 0.2f, 0.3f, 0.4f), 0.001f));
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "Gamma/Linear conversion functions")
    {
      // Test single float conversions
      float gamma = 0.5f;
      float linear = WColor::GammaToLinear(gamma);
      float backToGamma = WColor::LinearToGamma(linear);
      W_TEST_FLOAT(backToGamma, gamma, 0.001f);

      // Test Vec3 conversions
      WVec3 gammaVec(0.2f, 0.5f, 0.8f);
      WVec3 linearVec = WColor::GammaToLinear(gammaVec);
      WVec3 backToGammaVec = WColor::LinearToGamma(linearVec);
      W_TEST_BOOL(backToGammaVec.IsEqual(gammaVec, 0.001f));

      // Test edge cases
      W_TEST_FLOAT(WColor::GammaToLinear(0.0f), 0.0f, 0.001f);
      W_TEST_FLOAT(WColor::GammaToLinear(1.0f), 1.0f, 0.001f);
      W_TEST_FLOAT(WColor::LinearToGamma(0.0f), 0.0f, 0.001f);
      W_TEST_FLOAT(WColor::LinearToGamma(1.0f), 1.0f, 0.001f);
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "operator*= (WColor)")
    {
      WColor c1(0.5f, 0.6f, 0.8f, 1.0f);
      WColor c2(2.0f, 0.5f, 0.25f, 0.8f);
      c1 *= c2;
      W_TEST_BOOL(c1.IsEqualRGBA(WColor(1.0f, 0.3f, 0.2f, 0.8f), 0.001f));
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "operator* (WColor, WColor)")
    {
      WColor c1(0.5f, 0.6f, 0.8f, 1.0f);
      WColor c2(2.0f, 0.5f, 0.25f, 0.8f);
      WColor result = c1 * c2;
      W_TEST_BOOL(result.IsEqualRGBA(WColor(1.0f, 0.3f, 0.2f, 0.8f), 0.001f));
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "MakeFromKelvin")
    {
      // Test some known temperature points
      WColor warm = WColor::MakeFromKelvin(2700);     // Warm white (incandescent)
      WColor daylight = WColor::MakeFromKelvin(6500); // Daylight
      WColor cool = WColor::MakeFromKelvin(9000);     // Cool daylight

      // Warm should be more red/orange
      W_TEST_BOOL(warm.r > warm.b);

      // Cool should be more blue
      W_TEST_BOOL(cool.b > cool.r);

      // Alpha should always be 1
      W_TEST_FLOAT(warm.a, 1.0f, 0.001f);
      W_TEST_FLOAT(daylight.a, 1.0f, 0.001f);
      W_TEST_FLOAT(cool.a, 1.0f, 0.001f);

      // Test reasonable temperature values - all should produce valid colors
      W_TEST_BOOL(warm.IsValid());
      W_TEST_BOOL(daylight.IsValid());
      W_TEST_BOOL(cool.IsValid());
    }
  }
}
