#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/SimdMath/SimdNoise.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <Texture/Image/Image.h>

W_CREATE_SIMPLE_TEST(SimdMath, SimdNoise)
{
  WStringBuilder sReadDir(">sdk/", WTestFramework::GetInstance()->GetRelTestDataPath());
  WStringBuilder sWriteDir = WTestFramework::GetInstance()->GetAbsOutputPath();

  W_TEST_BOOL(WFileSystem::AddDataDirectory(sReadDir, "SimdNoise") == W_SUCCESS);
  W_TEST_BOOL_MSG(WFileSystem::AddDataDirectory(sWriteDir, "SimdNoise", "output", WDataDirUsage::AllowWrites) == W_SUCCESS,
    "Failed to mount data dir '%s'", sWriteDir.GetData());

  W_TEST_BLOCK(WTestBlock::Enabled, "Perlin")
  {
    const WUInt32 uiSize = 128;

    WImageHeader imageHeader;
    imageHeader.SetWidth(uiSize);
    imageHeader.SetHeight(uiSize);
    imageHeader.SetImageFormat(WImageFormat::R8G8B8A8_UNORM);

    WImage image;
    image.ResetAndAlloc(imageHeader);

    WSimdPerlinNoise perlin(12345);
    WSimdVec4f xOffset(0, 1, 2, 3);
    WSimdFloat scale(100);

    for (WUInt32 uiNumOctaves = 1; uiNumOctaves <= 6; ++uiNumOctaves)
    {
      WColorLinearUB* data = image.GetPixelPointer<WColorLinearUB>();
      for (WUInt32 y = 0; y < uiSize; ++y)
      {
        for (WUInt32 x = 0; x < uiSize / 4; ++x)
        {
          WSimdVec4f sX = (WSimdVec4f(x * 4.0f) + xOffset) / scale;
          WSimdVec4f sY = WSimdVec4f(y * 1.0f) / scale;

          WSimdVec4f noise = perlin.NoiseZeroToOne(sX, sY, WSimdVec4f::MakeZero(), uiNumOctaves);
          float p[4];
          p[0] = noise.x();
          p[1] = noise.y();
          p[2] = noise.z();
          p[3] = noise.w();

          WUInt32 uiPixelIndex = y * uiSize + x * 4;
          for (WUInt32 i = 0; i < 4; ++i)
          {
            data[uiPixelIndex + i] = WColor(p[i], p[i], p[i]);
          }
        }
      }

      WStringBuilder sOutFile;
      sOutFile.SetFormat(":output/SimdNoise/result-perlin_{}.tga", uiNumOctaves);

      W_TEST_BOOL(image.SaveTo(sOutFile).Succeeded());

      WStringBuilder sInFile;
      sInFile.SetFormat("SimdNoise/perlin_{}.tga", uiNumOctaves);
      W_TEST_BOOL_MSG(WFileSystem::ExistsFile(sInFile), "Noise image file is missing: '%s'", sInFile.GetData());

      W_TEST_FILES(sOutFile, sInFile, "");
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Random")
  {
    WUInt32 histogram[256] = {};

    for (WUInt32 i = 0; i < 10000; ++i)
    {
      WSimdVec4u seed = WSimdVec4u(i);
      WSimdVec4f randomValues = WSimdRandom::FloatMinMax(WSimdVec4i(0, 1, 2, 3), WSimdVec4f::MakeZero(), WSimdVec4f(256.0f), seed);
      WSimdVec4i randomValuesAsInt = WSimdVec4i::Truncate(randomValues);

      ++histogram[randomValuesAsInt.x()];
      ++histogram[randomValuesAsInt.y()];
      ++histogram[randomValuesAsInt.z()];
      ++histogram[randomValuesAsInt.w()];

      randomValues = WSimdRandom::FloatMinMax(WSimdVec4i(32, 33, 34, 35), WSimdVec4f::MakeZero(), WSimdVec4f(256.0f), seed);
      randomValuesAsInt = WSimdVec4i::Truncate(randomValues);

      ++histogram[randomValuesAsInt.x()];
      ++histogram[randomValuesAsInt.y()];
      ++histogram[randomValuesAsInt.z()];
      ++histogram[randomValuesAsInt.w()];
    }

    const char* szOutFile = ":output/SimdNoise/result-random.csv";
    {
      WFileWriter fileWriter;
      W_TEST_BOOL(fileWriter.Open(szOutFile).Succeeded());

      WStringBuilder sLine;
      for (WUInt32 i = 0; i < W_ARRAY_SIZE(histogram); ++i)
      {
        sLine.SetFormat("{},\n", histogram[i]);
        fileWriter.WriteBytes(sLine.GetData(), sLine.GetElementCount()).IgnoreResult();
      }
    }

    const char* szInFile = "SimdNoise/random.csv";
    W_TEST_BOOL_MSG(WFileSystem::ExistsFile(szInFile), "Random histogram file is missing: '%s'", szInFile);

    W_TEST_TEXT_FILES(szOutFile, szInFile, "");
  }

  WFileSystem::RemoveDataDirectoryGroup("SimdNoise");
}
