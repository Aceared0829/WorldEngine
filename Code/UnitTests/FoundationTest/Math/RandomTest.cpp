#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/StaticArray.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Math/Random.h>

// only works when also linking against CoreUtils
// #define USE_WIMAGE

#ifdef USE_WIMAGE
#  include <Texture/Image/Image.h>
#endif


W_CREATE_SIMPLE_TEST(Math, Random)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "UIntInRange")
  {
    WRandom r;
    r.Initialize(0xAABBCCDDEEFF0011ULL);

    for (WUInt32 i = 2; i < 10000; ++i)
    {
      const WUInt32 val = r.UIntInRange(i);
      W_TEST_BOOL(val < i);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IntMinMax")
  {
    WRandom r;
    r.Initialize(0xBBCCDDEEFF0011AAULL);

    W_TEST_INT(r.IntMinMax(5, 5), 5);
    W_TEST_INT(r.IntMinMax(-5, -5), -5);

    for (WInt32 i = 2; i < 10000; ++i)
    {
      const WInt32 val = r.IntMinMax(i, i + i);
      W_TEST_BOOL(val >= i);
      W_TEST_BOOL(val <= i + i);
    }

    for (WInt32 i = 2; i < 10000; ++i)
    {
      const WInt32 val = r.IntMinMax(-i, i);
      W_TEST_BOOL(val >= -i);
      W_TEST_BOOL(val <= i);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IntMinMax")
  {
    WRandom r;
    r.Initialize(0xCCDDEEFF0011AABBULL);

    W_TEST_INT(r.IntMinMax(5, 5), 5);
    W_TEST_INT(r.IntMinMax(-5, -5), -5);

    for (WInt32 i = 2; i < 10000; ++i)
    {
      const WInt32 val = r.IntMinMax(i, 2 * i);
      W_TEST_BOOL(val >= i);
      W_TEST_BOOL(val <= i + i);
    }

    for (WInt32 i = 2; i < 10000; ++i)
    {
      const WInt32 val = r.IntMinMax(-i, i);
      W_TEST_BOOL(val >= -i);
      W_TEST_BOOL(val <= i);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Bool")
  {
    WRandom r;
    r.Initialize(0x11AABBCCDDEEFFULL);

    WUInt32 falseCount = 0;
    WUInt32 trueCount = 0;
    WDynamicArray<bool> values;
    values.SetCount(1000);

    for (int i = 0; i < 1000; ++i)
    {
      values[i] = r.Bool();
      if (values[i])
      {
        ++trueCount;
      }
      else
      {
        ++falseCount;
      }
    }

    // This could be more elaborate, one could also test the variance
    // and assert that approximately an uniform distribution is yielded
    W_TEST_BOOL(trueCount > 0 && falseCount > 0);

    WRandom r2;
    r2.Initialize(0x11AABBCCDDEEFFULL);

    for (int i = 0; i < 1000; ++i)
    {
      W_TEST_BOOL(values[i] == r2.Bool());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "DoubleZeroToOneExclusive")
  {
    WRandom r;
    r.Initialize(0xDDEEFF0011AABBCCULL);

    for (WInt32 i = 2; i < 10000; ++i)
    {
      const double val = r.DoubleZeroToOneExclusive();
      W_TEST_BOOL(val >= 0.0);
      W_TEST_BOOL(val < 1.0);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "DoubleZeroToOneInclusive")
  {
    WRandom r;
    r.Initialize(0xEEFF0011AABBCCDDULL);

    for (WInt32 i = 2; i < 10000; ++i)
    {
      const double val = r.DoubleZeroToOneInclusive();
      W_TEST_BOOL(val >= 0.0);
      W_TEST_BOOL(val <= 1.0);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "DoubleInRange")
  {
    WRandom r;
    r.Initialize(0xFF0011AABBCCDDEEULL);

    W_TEST_DOUBLE(r.DoubleMinMax(5, 5), 5, 0.0);
    W_TEST_DOUBLE(r.DoubleMinMax(-5, -5), -5, 0.0);

    for (WInt32 i = 2; i < 10000; ++i)
    {
      const double val = r.DoubleMinMax(i, i + i);
      W_TEST_BOOL(val >= i);
      W_TEST_BOOL(val < i + i);
    }

    for (WInt32 i = 2; i < 10000; ++i)
    {
      const double val = r.DoubleMinMax(-i, i);
      W_TEST_BOOL(val >= -i);
      W_TEST_BOOL(val < -i + 2 * i);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "DoubleMinMax")
  {
    WRandom r;
    r.Initialize(0x0011AABBCCDDEEFFULL);

    W_TEST_DOUBLE(r.DoubleMinMax(5, 5), 5, 0.0);
    W_TEST_DOUBLE(r.DoubleMinMax(-5, -5), -5, 0.0);

    for (WInt32 i = 2; i < 10000; ++i)
    {
      const double val = r.DoubleMinMax(i, 2 * i);
      W_TEST_BOOL(val >= i);
      W_TEST_BOOL(val <= i + i);
    }

    for (WInt32 i = 2; i < 10000; ++i)
    {
      const double val = r.DoubleMinMax(-i, i);
      W_TEST_BOOL(val >= -i);
      W_TEST_BOOL(val <= i);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "FloatZeroToOneExclusive")
  {
    WRandom r;
    r.Initialize(0xDDEEFF0011AABBCCULL);

    for (WInt32 i = 2; i < 10000; ++i)
    {
      const float val = r.FloatZeroToOneExclusive();
      W_TEST_BOOL(val >= 0.f);
      W_TEST_BOOL(val < 1.f);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "FloatZeroToOneInclusive")
  {
    WRandom r;
    r.Initialize(0xEEFF0011AABBCCDDULL);

    for (WInt32 i = 2; i < 10000; ++i)
    {
      const float val = r.FloatZeroToOneInclusive();
      W_TEST_BOOL(val >= 0.f);
      W_TEST_BOOL(val <= 1.f);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "FloatInRange")
  {
    WRandom r;
    r.Initialize(0xFF0011AABBCCDDEEULL);

    W_TEST_FLOAT(r.FloatMinMax(5, 5), 5, 0.f);
    W_TEST_FLOAT(r.FloatMinMax(-5, -5), -5, 0.f);

    for (WInt32 i = 2; i < 10000; ++i)
    {
      const float val = r.FloatMinMax(static_cast<float>(i), static_cast<float>(i + i));
      W_TEST_BOOL(val >= i);
      W_TEST_BOOL(val < i + i);
    }

    for (WInt32 i = 2; i < 10000; ++i)
    {
      const float val = r.FloatMinMax(static_cast<float>(-i), static_cast<float>(i));
      W_TEST_BOOL(val >= -i);
      W_TEST_BOOL(val < -i + 2 * i);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "FloatMinMax")
  {
    WRandom r;
    r.Initialize(0x0011AABBCCDDEEFFULL);

    W_TEST_FLOAT(r.FloatMinMax(5, 5), 5, 0.f);
    W_TEST_FLOAT(r.FloatMinMax(-5, -5), -5, 0.f);

    for (WInt32 i = 2; i < 10000; ++i)
    {
      const float val = r.FloatMinMax(static_cast<float>(i), static_cast<float>(2 * i));
      W_TEST_BOOL(val >= i);
      W_TEST_BOOL(val <= i + i);
    }

    for (WInt32 i = 2; i < 10000; ++i)
    {
      const float val = r.FloatMinMax(static_cast<float>(-i), static_cast<float>(i));
      W_TEST_BOOL(val >= -i);
      W_TEST_BOOL(val <= i);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Save / Load")
  {
    WRandom r, r2;
    r.Initialize(0x0011AABBCCDDE11FULL);

    for (int i = 0; i < 1000; ++i)
      r.UInt();

    WDefaultMemoryStreamStorage storage;
    WMemoryStreamWriter writer(&storage);
    WMemoryStreamReader reader(&storage);

    r.Save(writer);

    WDynamicArray<WUInt32> temp;
    temp.SetCountUninitialized(1000);

    for (int i = 0; i < 1000; ++i)
      temp[i] = r.UInt();

    r2.Load(reader);

    for (int i = 0; i < 1000; ++i)
    {
      W_TEST_INT(temp[i], r2.UInt());
    }
  }
}

static void SaveToImage(WDynamicArray<WUInt32>& ref_values, WUInt32 uiMaxValue, const char* szFile)
{
#ifdef USE_WIMAGE
  W_TEST_BOOL(WFileSystem::AddDataDirectory("", WDataDirUsage::AllowWrites, "Clear") == W_SUCCESS);

  WImage img;
  img.SetWidth(Values.GetCount());
  img.SetHeight(100);
  img.SetImageFormat(WImageFormat::B8G8R8A8_UNORM);
  img.AllocateImageData();

  for (WUInt32 y = 0; y < img.GetHeight(); ++y)
  {
    for (WUInt32 x = 0; x < img.GetWidth(); ++x)
    {
      WUInt32* pPixel = img.GetPixelPointer<WUInt32>(0, 0, 0, x, y);
      *pPixel = 0xFF000000;
    }
  }

  for (WUInt32 i = 0; i < Values.GetCount(); ++i)
  {
    double val = ((double)Values[i] / (double)uiMaxValue) * 100.0;
    WUInt32 y = 99 - WMath::Clamp<WUInt32>((WUInt32)val, 0, 99);

    WUInt32* pPixel = img.GetPixelPointer<WUInt32>(0, 0, 0, i, y);
    *pPixel = 0xFFFFFFFF;
  }

  img.SaveTo(szFile);

  WFileSystem::RemoveDataDirectoryGroup("Clear");
#endif
}

W_CREATE_SIMPLE_TEST(Math, RandomGauss)
{
  const float fVariance = 1.0f;

  W_TEST_BLOCK(WTestBlock::Enabled, "UnsignedValue")
  {
    WRandomGauss r;
    r.Initialize(0xABCDEF0012345678ULL, 100, fVariance);

    WDynamicArray<WUInt32> Values;
    Values.SetCount(100);

    WUInt32 uiMaxValue = 0;

    const WUInt32 factor = 10; // with a factor of 100 the bell curve becomes more pronounced, with less samples it has more exceptions
    for (WUInt32 i = 0; i < 10000 * factor; ++i)
    {
      auto val = r.UnsignedValue();

      W_TEST_BOOL(val < 100);

      if (val < Values.GetCount())
      {
        Values[val]++;

        uiMaxValue = WMath::Max(uiMaxValue, Values[val]);
      }
    }

    SaveToImage(Values, uiMaxValue, "D:/GaussUnsigned.tga");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SignedValue")
  {
    WRandomGauss r;
    r.Initialize(0xABCDEF0012345678ULL, 100, fVariance);

    WDynamicArray<WUInt32> Values;
    Values.SetCount(2 * 100);

    WUInt32 uiMaxValue = 0;

    const WUInt32 factor = 10; // with a factor of 100 the bell curve becomes more pronounced, with less samples it has more exceptions
    for (WUInt32 i = 0; i < 10000 * factor; ++i)
    {
      auto val = r.SignedValue();

      W_TEST_BOOL(val > -100 && val < 100);

      val += 100;

      if (val < (WInt32)Values.GetCount())
      {
        Values[val]++;

        uiMaxValue = WMath::Max(uiMaxValue, Values[val]);
      }
    }

    SaveToImage(Values, uiMaxValue, "D:/GaussSigned.tga");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Save / Load")
  {
    WRandomGauss r, r2;
    r.Initialize(0x0011AABBCCDDE11FULL, 1000, 1.7f);

    for (int i = 0; i < 1000; ++i)
      r.UnsignedValue();

    WDefaultMemoryStreamStorage storage;
    WMemoryStreamWriter writer(&storage);
    WMemoryStreamReader reader(&storage);

    r.Save(writer);

    WDynamicArray<WUInt32> temp;
    temp.SetCountUninitialized(1000);

    for (int i = 0; i < 1000; ++i)
      temp[i] = r.UnsignedValue();

    r2.Load(reader);

    for (int i = 0; i < 1000; ++i)
    {
      W_TEST_INT(temp[i], r2.UnsignedValue());
    }
  }
}
