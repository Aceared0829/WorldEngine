#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Texture/Image/ImageUtils.h>


W_CREATE_SIMPLE_TEST(Image, ImageUtils)
{
  WStringBuilder sReadDir(">sdk/", WTestFramework::GetInstance()->GetRelTestDataPath());
  WStringBuilder sWriteDir = WTestFramework::GetInstance()->GetAbsOutputPath();

  W_TEST_BOOL(WOSFile::CreateDirectoryStructure(sWriteDir.GetData()) == W_SUCCESS);

  WResult addDir = WFileSystem::AddDataDirectory(sReadDir.GetData(), "ImageTest");
  W_TEST_BOOL(addDir == W_SUCCESS);

  if (addDir.Failed())
    return;

  addDir = WFileSystem::AddDataDirectory(sWriteDir.GetData(), "ImageTest", "output", WDataDirUsage::AllowWrites);
  W_TEST_BOOL(addDir == W_SUCCESS);

  if (addDir.Failed())
    return;

  W_TEST_BLOCK(WTestBlock::Enabled, "ComputeImageDifferenceABS RGB")
  {
    WImage ImageA, ImageB, ImageDiff;
    ImageA.LoadFrom("ImageUtils/ImageA_RGB.tga").IgnoreResult();
    ImageB.LoadFrom("ImageUtils/ImageB_RGB.tga").IgnoreResult();

    WImageUtils::ComputeImageDifferenceABS(ImageA, ImageB, ImageDiff);

    ImageDiff.SaveTo(":output/ImageUtils/Diff_RGB.tga").IgnoreResult();

    W_TEST_FILES("ImageUtils/ExpectedDiff_RGB.tga", "ImageUtils/Diff_RGB.tga", "");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ComputeImageDifferenceABS RGBA")
  {
    WImage ImageA, ImageB, ImageDiff;
    ImageA.LoadFrom("ImageUtils/ImageA_RGBA.tga").IgnoreResult();
    ImageB.LoadFrom("ImageUtils/ImageB_RGBA.tga").IgnoreResult();

    WImageUtils::ComputeImageDifferenceABS(ImageA, ImageB, ImageDiff);

    ImageDiff.SaveTo(":output/ImageUtils/Diff_RGBA.tga").IgnoreResult();

    W_TEST_FILES("ImageUtils/ExpectedDiff_RGBA.tga", "ImageUtils/Diff_RGBA.tga", "");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Scaledown Half RGB")
  {
    WImage ImageA, ImageAc;
    ImageA.LoadFrom("ImageUtils/ImageA_RGB.tga").IgnoreResult();
    WImageUtils::Scale(ImageA, ImageAc, ImageA.GetWidth() / 2, ImageA.GetHeight() / 2).IgnoreResult();

    ImageAc.SaveTo(":output/ImageUtils/ScaledHalf_RGB.tga").IgnoreResult();

    W_TEST_FILES("ImageUtils/ExpectedScaledHalf_RGB.tga", "ImageUtils/ScaledHalf_RGB.tga", "");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Scaledown Half RGBA")
  {
    WImage ImageA, ImageAc;
    ImageA.LoadFrom("ImageUtils/ImageA_RGBA.tga").IgnoreResult();
    WImageUtils::Scale(ImageA, ImageAc, ImageA.GetWidth() / 2, ImageA.GetHeight() / 2).IgnoreResult();

    ImageAc.SaveTo(":output/ImageUtils/ScaledHalf_RGBA.tga").IgnoreResult();

    W_TEST_FILES("ImageUtils/ExpectedScaledHalf_RGBA.tga", "ImageUtils/ScaledHalf_RGBA.tga", "");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Scaleup 16x RGBA")
  {
    WImage ImageA, ImageUpscaled;
    ImageA.LoadFrom("ImageUtils/ColoredChecker.tga").IgnoreResult();

    WImageUtils::Scale(ImageA, ImageUpscaled, ImageA.GetWidth() * 16, ImageA.GetHeight() * 16, nullptr, WImageAddressMode::Repeat, WImageAddressMode::Repeat).IgnoreResult();

    ImageUpscaled.SaveTo(":output/ImageUtils/ColoredChecker16x.tga").IgnoreResult();

    W_TEST_FILES("ImageUtils/ExpectedColoredChecker16x.tga", "ImageUtils/ColoredChecker16x.tga", "");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CropImage RGB")
  {
    WImage ImageA, ImageAc;
    ImageA.LoadFrom("ImageUtils/ImageA_RGB.tga").IgnoreResult();
    WImageUtils::CropImage(ImageA, WVec2I32(100, 50), WSizeU32(300, 200), ImageAc);

    ImageAc.SaveTo(":output/ImageUtils/Crop_RGB.tga").IgnoreResult();

    W_TEST_FILES("ImageUtils/ExpectedCrop_RGB.tga", "ImageUtils/Crop_RGB.tga", "");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CropImage RGBA")
  {
    WImage ImageA, ImageAc;
    ImageA.LoadFrom("ImageUtils/ImageA_RGBA.tga").IgnoreResult();
    WImageUtils::CropImage(ImageA, WVec2I32(100, 75), WSizeU32(300, 180), ImageAc);

    ImageAc.SaveTo(":output/ImageUtils/Crop_RGBA.tga").IgnoreResult();

    W_TEST_FILES("ImageUtils/ExpectedCrop_RGBA.tga", "ImageUtils/Crop_RGBA.tga", "");
  }


  W_TEST_BLOCK(WTestBlock::Enabled, "ComputeMeanSquareError")
  {
    WImage ImageA, ImageB, ImageDiff;
    ImageA.LoadFrom("ImageUtils/ImageA_RGB.tga").IgnoreResult();
    ImageB.LoadFrom("ImageUtils/ImageB_RGB.tga").IgnoreResult();

    WImage ImageAc, ImageBc;
    WImageUtils::Scale(ImageA, ImageAc, ImageA.GetWidth() / 2, ImageA.GetHeight() / 2).IgnoreResult();
    WImageUtils::Scale(ImageB, ImageBc, ImageB.GetWidth() / 2, ImageB.GetHeight() / 2).IgnoreResult();

    WImageUtils::ComputeImageDifferenceABS(ImageAc, ImageBc, ImageDiff);

    ImageDiff.SaveTo(":output/ImageUtils/MeanSquareDiff_RGB.tga").IgnoreResult();

    W_TEST_FILES("ImageUtils/ExpectedMeanSquareDiff_RGB.tga", "ImageUtils/MeanSquareDiff_RGB.tga", "");

    WUInt32 uiError = WImageUtils::ComputeMeanSquareError(ImageDiff, 4);
    W_TEST_INT(uiError, 1433);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CopyChannel")
  {
    const WUInt32 uiWidth = 4;
    const WUInt32 uiHeight = 2;
    const WUInt32 uiNumPixels = uiWidth * uiHeight;

    // Helper to test CopyChannel between a source and destination format with potentially different channel counts.
    // Fills the source with a known pattern, copies one channel to the destination, and verifies.
    auto TestCopyChannel = [&](WImageFormat::Enum srcFormat, WImageFormat::Enum dstFormat, auto typeTag)
    {
      using T = decltype(typeTag);

      const WUInt32 uiSrcChannels = WImageFormat::GetNumChannels(srcFormat);
      const WUInt32 uiDstChannels = WImageFormat::GetNumChannels(dstFormat);

      for (WUInt8 srcCh = 0; srcCh < uiSrcChannels; ++srcCh)
      {
        for (WUInt8 dstCh = 0; dstCh < uiDstChannels; ++dstCh)
        {
          WImageHeader srcHeader;
          srcHeader.SetWidth(uiWidth);
          srcHeader.SetHeight(uiHeight);
          srcHeader.SetImageFormat(srcFormat);

          WImageHeader dstHeader;
          dstHeader.SetWidth(uiWidth);
          dstHeader.SetHeight(uiHeight);
          dstHeader.SetImageFormat(dstFormat);

          WImage srcImg, dstImg;
          srcImg.ResetAndAlloc(srcHeader);
          dstImg.ResetAndAlloc(dstHeader);

          // Fill source: channel value = (pixelIndex * srcChannels + channelIndex) so each value is unique.
          T* pSrc = srcImg.GetPixelPointer<T>();
          for (WUInt32 px = 0; px < uiNumPixels; ++px)
          {
            for (WUInt32 ch = 0; ch < uiSrcChannels; ++ch)
            {
              pSrc[px * uiSrcChannels + ch] = static_cast<T>(px * uiSrcChannels + ch + 1);
            }
          }

          // Fill dest with a constant sentinel value.
          const T sentinel = static_cast<T>(255);
          T* pDst = dstImg.GetPixelPointer<T>();
          for (WUInt32 i = 0; i < uiNumPixels * uiDstChannels; ++i)
          {
            pDst[i] = sentinel;
          }

          W_TEST_BOOL(WImageUtils::CopyChannel(dstImg, dstCh, srcImg, srcCh).Succeeded());

          // Verify: only dstCh should have changed; other channels remain sentinel.
          for (WUInt32 px = 0; px < uiNumPixels; ++px)
          {
            for (WUInt32 ch = 0; ch < uiDstChannels; ++ch)
            {
              T actual = pDst[px * uiDstChannels + ch];
              if (ch == dstCh)
              {
                T expected = static_cast<T>(px * uiSrcChannels + srcCh + 1);
                W_TEST_BOOL(actual == expected);
              }
              else
              {
                W_TEST_BOOL(actual == sentinel);
              }
            }
          }
        }
      }
    };

    // Shorthand for same-format tests.
    auto TestSameFormat = [&](WImageFormat::Enum format, auto typeTag)
    {
      TestCopyChannel(format, format, typeTag);
    };

    // 8-bit UNORM: same-format (stride 1-4)
    TestSameFormat(WImageFormat::R8_UNORM, WUInt8{});
    TestSameFormat(WImageFormat::R8G8_UNORM, WUInt8{});
    TestSameFormat(WImageFormat::R8G8B8_UNORM, WUInt8{});
    TestSameFormat(WImageFormat::R8G8B8A8_UNORM, WUInt8{});

    // 16-bit UNORM: same-format (stride 1-4)
    TestSameFormat(WImageFormat::R16_UNORM, WUInt16{});
    TestSameFormat(WImageFormat::R16G16_UNORM, WUInt16{});
    TestSameFormat(WImageFormat::R16G16B16_UNORM, WUInt16{});
    TestSameFormat(WImageFormat::R16G16B16A16_UNORM, WUInt16{});

    // 32-bit float: same-format (stride 1-4)
    TestSameFormat(WImageFormat::R32_FLOAT, WUInt32{});
    TestSameFormat(WImageFormat::R32G32_FLOAT, WUInt32{});
    TestSameFormat(WImageFormat::R32G32B32_FLOAT, WUInt32{});
    TestSameFormat(WImageFormat::R32G32B32A32_FLOAT, WUInt32{});

    // 8-bit UNORM: cross-format (different src/dst channel counts)
    TestCopyChannel(WImageFormat::R8_UNORM, WImageFormat::R8G8B8A8_UNORM, WUInt8{});
    TestCopyChannel(WImageFormat::R8G8B8A8_UNORM, WImageFormat::R8_UNORM, WUInt8{});
    TestCopyChannel(WImageFormat::R8G8_UNORM, WImageFormat::R8G8B8_UNORM, WUInt8{});
    TestCopyChannel(WImageFormat::R8G8B8_UNORM, WImageFormat::R8G8_UNORM, WUInt8{});
    TestCopyChannel(WImageFormat::R8_UNORM, WImageFormat::R8G8_UNORM, WUInt8{});
    TestCopyChannel(WImageFormat::R8G8B8A8_UNORM, WImageFormat::R8G8B8_UNORM, WUInt8{});

    // 16-bit UNORM: cross-format
    TestCopyChannel(WImageFormat::R16_UNORM, WImageFormat::R16G16B16A16_UNORM, WUInt16{});
    TestCopyChannel(WImageFormat::R16G16B16A16_UNORM, WImageFormat::R16_UNORM, WUInt16{});
    TestCopyChannel(WImageFormat::R16G16_UNORM, WImageFormat::R16G16B16_UNORM, WUInt16{});

    // 32-bit float: cross-format
    TestCopyChannel(WImageFormat::R32_FLOAT, WImageFormat::R32G32B32A32_FLOAT, WUInt32{});
    TestCopyChannel(WImageFormat::R32G32B32A32_FLOAT, WImageFormat::R32_FLOAT, WUInt32{});
    TestCopyChannel(WImageFormat::R32G32_FLOAT, WImageFormat::R32G32B32_FLOAT, WUInt32{});
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CopyChannel Failure Cases")
  {
    WImageHeader header4x4;
    header4x4.SetWidth(4);
    header4x4.SetHeight(4);

    // Mismatched dimensions
    {
      WImageHeader headerSmall;
      headerSmall.SetWidth(2);
      headerSmall.SetHeight(4);
      headerSmall.SetImageFormat(WImageFormat::R8G8B8A8_UNORM);

      WImageHeader headerBig;
      headerBig.SetWidth(4);
      headerBig.SetHeight(4);
      headerBig.SetImageFormat(WImageFormat::R8G8B8A8_UNORM);

      WImage src, dst;
      src.ResetAndAlloc(headerSmall);
      dst.ResetAndAlloc(headerBig);
      W_TEST_BOOL(WImageUtils::CopyChannel(dst, 0, src, 0).Failed());
    }

    // Mismatched bit depth (8-bit vs 32-bit) should fail
    {
      WImageHeader header8;
      header8.SetWidth(4);
      header8.SetHeight(4);
      header8.SetImageFormat(WImageFormat::R8G8B8A8_UNORM);

      WImageHeader header32;
      header32.SetWidth(4);
      header32.SetHeight(4);
      header32.SetImageFormat(WImageFormat::R32G32B32A32_FLOAT);

      WImage src, dst;
      src.ResetAndAlloc(header8);
      dst.ResetAndAlloc(header32);
      W_TEST_BOOL(WImageUtils::CopyChannel(dst, 0, src, 0).Failed());
    }

    // Mismatched type (unorm vs snorm) should fail
    {
      WImageHeader headerUnorm;
      headerUnorm.SetWidth(4);
      headerUnorm.SetHeight(4);
      headerUnorm.SetImageFormat(WImageFormat::R8G8B8A8_UNORM);

      WImageHeader headerSnorm;
      headerSnorm.SetWidth(4);
      headerSnorm.SetHeight(4);
      headerSnorm.SetImageFormat(WImageFormat::R8G8B8A8_SNORM);

      WImage src, dst;
      src.ResetAndAlloc(headerUnorm);
      dst.ResetAndAlloc(headerSnorm);
      W_TEST_BOOL(WImageUtils::CopyChannel(dst, 0, src, 0).Failed());
    }

    // Channel index out of range
    {
      header4x4.SetImageFormat(WImageFormat::R8G8_UNORM);
      WImage src, dst;
      src.ResetAndAlloc(header4x4);
      dst.ResetAndAlloc(header4x4);
      W_TEST_BOOL(WImageUtils::CopyChannel(dst, 2, src, 0).Failed());
      W_TEST_BOOL(WImageUtils::CopyChannel(dst, 0, src, 2).Failed());
    }
  }

  WFileSystem::RemoveDataDirectoryGroup("ImageTest");
}
