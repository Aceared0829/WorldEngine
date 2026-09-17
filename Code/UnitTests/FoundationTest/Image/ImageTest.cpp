#include <FoundationTest/FoundationTestPCH.h>


#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Texture/Image/Formats/BmpFileFormat.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Formats/ImageFileFormat.h>
#include <Texture/Image/Formats/SvgFileFormat.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/Image/ImageUtils.h>

W_CREATE_SIMPLE_TEST_GROUP(Image);

W_CREATE_SIMPLE_TEST(Image, Image)
{
  const WStringBuilder sReadDir(">sdk/", WTestFramework::GetInstance()->GetRelTestDataPath());
  const WStringBuilder sWriteDir = WTestFramework::GetInstance()->GetAbsOutputPath();

  W_TEST_BOOL(WOSFile::CreateDirectoryStructure(sWriteDir) == W_SUCCESS);

  W_TEST_BOOL(WFileSystem::AddDataDirectory(sReadDir, "ImageTest") == W_SUCCESS);
  W_TEST_BOOL(WFileSystem::AddDataDirectory(sWriteDir, "ImageTest", "output", WDataDirUsage::AllowWrites) == W_SUCCESS);

  W_TEST_BLOCK(WTestBlock::Enabled, "BMP - Good")
  {
    const char* testImagesGood[] = {
      "BMPTestImages/good/pal1", "BMPTestImages/good/pal1bg", "BMPTestImages/good/pal1wb", "BMPTestImages/good/pal4", "BMPTestImages/good/pal4rle",
      "BMPTestImages/good/pal8", "BMPTestImages/good/pal8-0", "BMPTestImages/good/pal8nonsquare",
      /*"BMPTestImages/good/pal8os2",*/ "BMPTestImages/good/pal8rle",
      /*"BMPTestImages/good/pal8topdown",*/ "BMPTestImages/good/pal8v4", "BMPTestImages/good/pal8v5", "BMPTestImages/good/pal8w124",
      "BMPTestImages/good/pal8w125", "BMPTestImages/good/pal8w126", "BMPTestImages/good/rgb16", "BMPTestImages/good/rgb16-565pal",
      "BMPTestImages/good/rgb24", "BMPTestImages/good/rgb24pal", "BMPTestImages/good/rgb32", /*"BMPTestImages/good/rgb32bf"*/
    };

    for (int i = 0; i < W_ARRAY_SIZE(testImagesGood); i++)
    {
      WImage image;
      {
        WStringBuilder fileName;
        fileName.SetFormat("{0}.bmp", testImagesGood[i]);

        W_TEST_BOOL_MSG(WFileSystem::ExistsFile(fileName), "Image file does not exist: '%s'", fileName.GetData());
        W_TEST_BOOL_MSG(image.LoadFrom(fileName) == W_SUCCESS, "Reading image failed: '%s'", fileName.GetData());
      }

      {
        WStringBuilder fileName;
        fileName.SetFormat(":output/{0}_out.bmp", testImagesGood[i]);

        W_TEST_BOOL_MSG(image.SaveTo(fileName) == W_SUCCESS, "Writing image failed: '%s'", fileName.GetData());
        W_TEST_BOOL_MSG(WFileSystem::ExistsFile(fileName), "Output image file is missing: '%s'", fileName.GetData());
      }
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "BMP - Bad")
  {
    const char* testImagesBad[] = {"BMPTestImages/bad/badbitcount", "BMPTestImages/bad/badbitssize",
      /*"BMPTestImages/bad/baddens1", "BMPTestImages/bad/baddens2", "BMPTestImages/bad/badfilesize", "BMPTestImages/bad/badheadersize",*/
      "BMPTestImages/bad/badpalettesize",
      /*"BMPTestImages/bad/badplanes",*/ "BMPTestImages/bad/badrle", "BMPTestImages/bad/badwidth",
      /*"BMPTestImages/bad/pal2",*/ "BMPTestImages/bad/pal8badindex", "BMPTestImages/bad/reallybig", "BMPTestImages/bad/rletopdown",
      "BMPTestImages/bad/shortfile"};


    for (int i = 0; i < W_ARRAY_SIZE(testImagesBad); i++)
    {
      WImage image;
      {
        WStringBuilder fileName;
        fileName.SetFormat("{0}.bmp", testImagesBad[i]);

        W_TEST_BOOL_MSG(WFileSystem::ExistsFile(fileName), "File does not exist: '%s'", fileName.GetData());

        W_LOG_BLOCK_MUTE();
        W_TEST_BOOL_MSG(image.LoadFrom(fileName) == W_FAILURE, "Reading image should have failed: '%s'", fileName.GetData());
      }
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "TGA")
  {
    const char* testImagesGood[] = {"TGATestImages/good/RGB", "TGATestImages/good/RGBA", "TGATestImages/good/RGB_RLE", "TGATestImages/good/RGBA_RLE"};

    for (int i = 0; i < W_ARRAY_SIZE(testImagesGood); i++)
    {
      WImage image;
      {
        WStringBuilder fileName;
        fileName.SetFormat("{0}.tga", testImagesGood[i]);

        W_TEST_BOOL_MSG(WFileSystem::ExistsFile(fileName), "Image file does not exist: '%s'", fileName.GetData());
        W_TEST_BOOL_MSG(image.LoadFrom(fileName) == W_SUCCESS, "Reading image failed: '%s'", fileName.GetData());
      }

      {
        WStringBuilder fileName;
        fileName.SetFormat(":output/{0}_out.bmp", testImagesGood[i]);

        WStringBuilder fileNameExpected;
        fileNameExpected.SetFormat("{0}_expected.bmp", testImagesGood[i]);

        W_TEST_BOOL_MSG(image.SaveTo(fileName) == W_SUCCESS, "Writing image failed: '%s'", fileName.GetData());
        W_TEST_BOOL_MSG(WFileSystem::ExistsFile(fileName), "Output image file is missing: '%s'", fileName.GetData());

        W_TEST_FILES(fileName, fileNameExpected, "");
      }

      {
        WStringBuilder fileName;
        fileName.SetFormat(":output/{0}_out.tga", testImagesGood[i]);

        WStringBuilder fileNameExpected;
        fileNameExpected.SetFormat("{0}_expected.tga", testImagesGood[i]);

        W_TEST_BOOL_MSG(image.SaveTo(fileName) == W_SUCCESS, "Writing image failed: '%s'", fileName.GetData());
        W_TEST_BOOL_MSG(WFileSystem::ExistsFile(fileName), "Output image file is missing: '%s'", fileName.GetData());

        W_TEST_FILES(fileName, fileNameExpected, "");
      }
    }
  }

#ifdef BUILDSYSTEM_ENABLE_LUNASVG_SUPPORT
  W_TEST_BLOCK(WTestBlock::Enabled, "SVG")
  {
    WSvgFileFormat svgFormat;
    svgFormat.m_uiResolutionX = 256;
    svgFormat.m_uiResolutionY = 256;

    WImage image;

    WFileReader fileReader;
    W_TEST_BOOL(fileReader.Open("SVGTestImages/W.svg") == W_SUCCESS);
    W_TEST_BOOL(svgFormat.ReadImage(fileReader, image, "svg") == W_SUCCESS);

    W_TEST_INT(image.GetWidth(), 256);
    W_TEST_INT(image.GetHeight(), 256);
    W_TEST_BOOL(image.GetImageFormat() == WImageFormat::R8G8B8A8_UNORM);

    W_TEST_BOOL(image.SaveTo(":output/SVGTestImages/W_out.tga") == W_SUCCESS);

    W_TEST_FILES(":output/SVGTestImages/W_out.tga", "SVGTestImages/W_expected.tga", "");
  }
#endif

  W_TEST_BLOCK(WTestBlock::Enabled, "Write Image Formats")
  {
    struct ImgTest
    {
      const char* szImage;
      const char* szFormat;
      WUInt32 uiMSE;
    };

    ImgTest imgTests[] = {
      {"RGB", "tga", 0},
      {"RGBA", "tga", 0},
      {"RGB", "png", 0},
      {"RGBA", "png", 0},
      {"RGB", "jpg", 4650},
      {"RGBA", "jpeg", 16670},
#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
      {"RGB", "tif", 0},
      {"RGBA", "tif", 0},
#endif
    };

    const char* szTestImagePath = "TGATestImages/good";

    for (int idx = 0; idx < W_ARRAY_SIZE(imgTests); ++idx)
    {
      WImage image;
      {
        WStringBuilder fileName;
        fileName.SetFormat("{}/{}.tga", szTestImagePath, imgTests[idx].szImage);

        W_TEST_BOOL_MSG(WFileSystem::ExistsFile(fileName), "Image file does not exist: '%s'", fileName.GetData());
        W_TEST_BOOL_MSG(image.LoadFrom(fileName) == W_SUCCESS, "Reading image failed: '%s'", fileName.GetData());
      }

      {
        WStringBuilder fileName;
        fileName.SetFormat(":output/WriteImageTest/{}.{}", imgTests[idx].szImage, imgTests[idx].szFormat);

        WFileSystem::DeleteFile(fileName);

        W_TEST_BOOL_MSG(image.SaveTo(fileName) == W_SUCCESS, "Writing image failed: '%s'", fileName.GetData());
        W_TEST_BOOL_MSG(WFileSystem::ExistsFile(fileName), "Output image file is missing: '%s'", fileName.GetData());

        WImage image2;
        W_TEST_BOOL_MSG(image2.LoadFrom(fileName).Succeeded(), "Reading written image failed: '%s'", fileName.GetData());

        image.Convert(WImageFormat::R8G8B8A8_UNORM_SRGB).IgnoreResult();
        image2.Convert(WImageFormat::R8G8B8A8_UNORM_SRGB).IgnoreResult();

        WImage diff;
        WImageUtils::ComputeImageDifferenceABS(image, image2, diff);

        const WUInt32 uiMSE = WImageUtils::ComputeMeanSquareError(diff, 32);

        W_TEST_BOOL_MSG(uiMSE <= imgTests[idx].uiMSE, "MSE %u is larger than %u for image '%s'", uiMSE, imgTests[idx].uiMSE, fileName.GetData());
      }
    }
  }

  WFileSystem::RemoveDataDirectoryGroup("ImageTest");
}
