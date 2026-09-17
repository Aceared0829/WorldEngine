#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Color8UNorm.h>


W_CREATE_SIMPLE_TEST(Math, Color8UNorm)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor empty")
  {
    // Placement new of the default constructor should not have any effect on the previous data.
    WUInt8 testBlock[4] = {0, 64, 128, 255};
    WColorLinearUB* pDefCtor = ::new ((void*)&testBlock[0]) WColorLinearUB;
    W_TEST_BOOL(pDefCtor->r == 0 && pDefCtor->g == 64 && pDefCtor->b == 128 && pDefCtor->a == 255);

    // Make sure the class didn't accidentally change in size
    W_TEST_BOOL(sizeof(WColorLinearUB) == sizeof(WUInt8) * 4);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor components")
  {
    WColorLinearUB init3(100, 123, 255);
    W_TEST_BOOL(init3.r == 100 && init3.g == 123 && init3.b == 255 && init3.a == 255);

    WColorLinearUB init4(100, 123, 255, 42);
    W_TEST_BOOL(init4.r == 100 && init4.g == 123 && init4.b == 255 && init4.a == 42);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor copy")
  {
    WColorLinearUB init4(100, 123, 255, 42);
    WColorLinearUB copy(init4);
    W_TEST_BOOL(copy.r == 100 && copy.g == 123 && copy.b == 255 && copy.a == 42);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor WColor")
  {
    WColorLinearUB fromColor32f(WColor(0.39f, 0.58f, 0.93f));
    W_TEST_BOOL(WMath::IsEqual<WUInt8>(fromColor32f.r, static_cast<WUInt8>(WColor(0.39f, 0.58f, 0.93f).r * 255), 2) &&
                 WMath::IsEqual<WUInt8>(fromColor32f.g, static_cast<WUInt8>(WColor(0.39f, 0.58f, 0.93f).g * 255), 2) &&
                 WMath::IsEqual<WUInt8>(fromColor32f.b, static_cast<WUInt8>(WColor(0.39f, 0.58f, 0.93f).b * 255), 2) &&
                 WMath::IsEqual<WUInt8>(fromColor32f.a, static_cast<WUInt8>(WColor(0.39f, 0.58f, 0.93f).a * 255), 2));
  }

  // conversion
  {
    WColorLinearUB cornflowerBlue(WColor(0.39f, 0.58f, 0.93f));

    W_TEST_BLOCK(WTestBlock::Enabled, "Conversion WColor")
    {
      WColor color32f = cornflowerBlue;
      W_TEST_BOOL(WMath::IsEqual<float>(color32f.r, WColor(0.39f, 0.58f, 0.93f).r, 2.0f / 255.0f) &&
                   WMath::IsEqual<float>(color32f.g, WColor(0.39f, 0.58f, 0.93f).g, 2.0f / 255.0f) &&
                   WMath::IsEqual<float>(color32f.b, WColor(0.39f, 0.58f, 0.93f).b, 2.0f / 255.0f) &&
                   WMath::IsEqual<float>(color32f.a, WColor(0.39f, 0.58f, 0.93f).a, 2.0f / 255.0f));
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "Conversion WUInt*")
    {
      const WUInt8* pUIntsConst = cornflowerBlue.GetData();
      W_TEST_BOOL(pUIntsConst[0] == cornflowerBlue.r && pUIntsConst[1] == cornflowerBlue.g && pUIntsConst[2] == cornflowerBlue.b &&
                   pUIntsConst[3] == cornflowerBlue.a);

      WUInt8* pUInts = cornflowerBlue.GetData();
      W_TEST_BOOL(pUInts[0] == cornflowerBlue.r && pUInts[1] == cornflowerBlue.g && pUInts[2] == cornflowerBlue.b && pUInts[3] == cornflowerBlue.a);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WColorGammaUB: Constructor")
  {
    WColorGammaUB c(50, 150, 200, 100);
    W_TEST_INT(c.r, 50);
    W_TEST_INT(c.g, 150);
    W_TEST_INT(c.b, 200);
    W_TEST_INT(c.a, 100);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WColorGammaUB: Constructor (WColor)")
  {
    WColorGammaUB c2 = WColor::RebeccaPurple;

    WColor c3 = c2;

    W_TEST_BOOL(c3.IsEqualRGBA(WColor::RebeccaPurple, WMath::SmallEpsilon<float>()));
  }
}
