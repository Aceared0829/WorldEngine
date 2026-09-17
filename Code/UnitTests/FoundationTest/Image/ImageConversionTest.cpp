#include <FoundationTest/FoundationTestPCH.h>


#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Memory/MemoryTracker.h>
#include <Texture/Image/Formats/BmpFileFormat.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Formats/ImageFileFormat.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>

static const WImageFormat::Enum defaultFormat = WImageFormat::R32G32B32A32_FLOAT;

class WImageConversionTest : public WTestBaseClass
{

public:
  virtual const char* GetTestName() const override { return "Image Conversion"; }

  virtual WResult GetImage(WImage& ref_img, const WSubTestEntry& subTest, WUInt32 uiImageNumber) override
  {
    ref_img.ResetAndMove(std::move(m_Image));
    return W_SUCCESS;
  }

private:
  virtual void SetupSubTests() override
  {
    for (WUInt32 i = 0; i < WImageFormat::NUM_FORMATS; ++i)
    {
      WImageFormat::Enum format = static_cast<WImageFormat::Enum>(i);

      const char* name = WImageFormat::GetName(format);
      W_ASSERT_DEV(name != nullptr, "Missing format information for format {}", i);

      bool isEncodable = WImageConversion::IsConvertible(defaultFormat, format);

      if (!isEncodable)
      {
        // If a format doesn't have an encoder, ignore
        continue;
      }

      AddSubTest(name, i);
    }
  }

  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) override
  {
    WImageFormat::Enum format = static_cast<WImageFormat::Enum>(iIdentifier);

    bool isDecodable = WImageConversion::IsConvertible(format, defaultFormat);

    if (!isDecodable)
    {
      W_TEST_BOOL_MSG(false, "Format %s can be encoded from %s but not decoded - add a decoder for this format please", WImageFormat::GetName(format), WImageFormat::GetName(defaultFormat));

      return WTestAppRun::Quit;
    }

    {
      WTempHybridArray<WImageConversion::ConversionPathNode, 16> decodingPath;
      WUInt32 decodingPathScratchBuffers;
      WImageConversion::BuildPath(format, defaultFormat, false, decodingPath, decodingPathScratchBuffers).IgnoreResult();

      // the [test] tag tells the test framework to output the log message in the GUI
      WLog::Info("[test]Default decoding Path:");
      for (WUInt32 i = 0; i < decodingPath.GetCount(); ++i)
      {
        WLog::Info("[test]  {} -> {}", WImageFormat::GetName(decodingPath[i].m_sourceFormat), WImageFormat::GetName(decodingPath[i].m_targetFormat));
      }
    }

    {
      WTempHybridArray<WImageConversion::ConversionPathNode, 16> encodingPath;
      WUInt32 encodingPathScratchBuffers;
      WImageConversion::BuildPath(defaultFormat, format, false, encodingPath, encodingPathScratchBuffers).IgnoreResult();

      // the [test] tag tells the test framework to output the log message in the GUI
      WLog::Info("[test]Default encoding Path:");
      for (WUInt32 i = 0; i < encodingPath.GetCount(); ++i)
      {
        WLog::Info("[test]  {} -> {}", WImageFormat::GetName(encodingPath[i].m_sourceFormat), WImageFormat::GetName(encodingPath[i].m_targetFormat));
      }
    }

    // Test LDR: Load, encode to target format, then do image comparison (which internally decodes to BGR8_UNORM again).
    // This visualizes quantization for low bit formats, block compression artifacts, or whether formats have fewer than 3 channels.
    {
      W_TEST_BOOL(m_Image.LoadFrom("ImageConversions/reference.png").Succeeded());

      W_TEST_BOOL(m_Image.Convert(format).Succeeded());

      W_TEST_IMAGE(iIdentifier * 2, WImageFormat::IsCompressed(format) ? 10 : 0);
    }

    // Test HDR: Load, decode to FLOAT32, stretch to [-range, range] and encode;
    // then decode to FLOAT32 again, bring back into LDR range and do image comparison.
    // If the format doesn't support negative values, the left half of the image will be black.
    // If the format doesn't support values with absolute value > 1, the image will appear clipped to fullbright.
    // Also, fill the first few rows in the top left with Infinity, -Infinity, and NaN, which should
    // show up as White, White, and Black, resp., in the comparison.
    {
      const float range = 8;

      W_TEST_BOOL(m_Image.LoadFrom("ImageConversions/reference.png").Succeeded());

      W_TEST_BOOL(m_Image.Convert(WImageFormat::R32G32B32A32_FLOAT).Succeeded());

      const float posInf = +WMath::Infinity<float>();
      const float negInf = -WMath::Infinity<float>();
      const float NaN = WMath::NaN<float>();

      for (WUInt32 y = 0; y < m_Image.GetHeight(); ++y)
      {
        WColor* pPixelPointer = m_Image.GetPixelPointer<WColor>(0, 0, 0, 0, y);

        for (WUInt32 x = 0; x < m_Image.GetWidth(); ++x)
        {
          // Fill with Inf or Nan resp. scale the image into positive and negative HDR range
          if (x < 30 && y < 10)
          {
            *pPixelPointer = WColor(posInf, posInf, posInf, posInf);
          }
          else if (x < 30 && y < 20)
          {
            *pPixelPointer = WColor(negInf, negInf, negInf, negInf);
          }
          else if (x < 30 && y < 30)
          {
            *pPixelPointer = WColor(NaN, NaN, NaN, NaN);
          }
          else
          {
            float scale = (x / float(m_Image.GetWidth()) - 0.5f) * 2.0f * range;

            if (WMath::Abs(scale) > 0.5)
            {
              *pPixelPointer *= scale;
            }
          }

          pPixelPointer++;
        }
      }

      W_TEST_BOOL(m_Image.Convert(format).Succeeded());

      W_TEST_BOOL(m_Image.Convert(WImageFormat::R32G32B32A32_FLOAT).Succeeded());

      for (WUInt32 y = 0; y < m_Image.GetHeight(); ++y)
      {
        WColor* pPixelPointer = m_Image.GetPixelPointer<WColor>(0, 0, 0, 0, y);

        for (WUInt32 x = 0; x < m_Image.GetWidth(); ++x)
        {
          // Scale the image back into LDR range if possible
          if (x < 30 && y < 10)
          {
            // Leave pos inf as is - this should be clipped to 1 in the LDR conversion for img cmp
          }
          else if (x < 30 && y < 20)
          {
            // Flip neg inf to pos inf
            *pPixelPointer *= -1.0f;
          }
          else if (x < 30 && y < 30)
          {
            // Leave nan as is - this should be clipped to 0 in the LDR conversion for img cmp
          }
          else
          {
            float scale = (x / float(m_Image.GetWidth()) - 0.5f) * 2.0f * range;
            if (WMath::Abs(scale) > 0.5)
            {
              *pPixelPointer /= scale;
            }
          }

          pPixelPointer++;
        }
      }

      W_TEST_IMAGE(iIdentifier * 2 + 1, WImageFormat::IsCompressed(format) ? 10 : 0);
    }

    return WTestAppRun::Quit;
  }

  virtual WResult InitializeTest() override
  {
    WStartup::StartupCoreSystems();

    const WStringBuilder sReadDir(">sdk/", WTestFramework::GetInstance()->GetRelTestDataPath());

    if (WFileSystem::AddDataDirectory(sReadDir.GetData(), "ImageConversionTest").Failed())
    {
      return W_FAILURE;
    }

    WFileSystem::AddDataDirectory(">Wtest/", "ImageComparisonDataDir", "imgout", WDataDirUsage::AllowWrites).IgnoreResult();

    // On linux we use CPU based BC6 and BC7 compression, which sometimes gives slightly different results from the GPU compression on Windows.
    WTestFramework::GetInstance()->SetImageReferenceTagsFromEnvironment(W_PLATFORM_NAME, {}, {});

    return W_SUCCESS;
  }

  virtual WResult DeInitializeTest() override
  {
    WFileSystem::RemoveDataDirectoryGroup("ImageConversionTest");
    WFileSystem::RemoveDataDirectoryGroup("ImageComparisonDataDir");

    WStartup::ShutdownCoreSystems();
    WMemoryTracker::DumpMemoryLeaks();

    return W_SUCCESS;
  }

  virtual WResult InitializeSubTest(WInt32 iIdentifier) override { return W_SUCCESS; }

  virtual WResult DeInitializeSubTest(WInt32 iIdentifier) override { return W_SUCCESS; }

  WImage m_Image;
};

static WImageConversionTest s_ImageConversionTest;
