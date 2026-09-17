#include <CoreTest/CoreTestPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/System/Process.h>
#include <Foundation/System/ProcessGroup.h>
#include <Texture/Image/Image.h>

#if W_ENABLED(W_SUPPORTS_PROCESSES) && (W_ENABLED(W_PLATFORM_WINDOWS) || W_ENABLED(W_PLATFORM_LINUX)) && defined(BUILDSYSTEM_TEXCONV_PRESENT)

class WTexConvTest : public WTestBaseClass
{
public:
  virtual const char* GetTestName() const override { return "TexConvTool"; }

  virtual WResult GetImage(WImage& ref_img, const WSubTestEntry& subTest, WUInt32 uiImageNumber) override
  {
    ref_img.ResetAndMove(std::move(m_pState->m_image));
    return W_SUCCESS;
  }

private:
  enum SubTest
  {
    RgbaToRgbPNG,
    Combine4,
    LinearUsage,
    ExtractChannel,
    TGA,
  };

  virtual void SetupSubTests() override;

  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) override;

  virtual WResult InitializeTest() override
  {
    WStartup::StartupCoreSystems();

    m_pState = W_DEFAULT_NEW(State);

    const WStringBuilder sReadDir(">sdk/", WTestFramework::GetInstance()->GetRelTestDataPath());

    if (WFileSystem::AddDataDirectory(sReadDir.GetData(), "TexConvTest", "testdata").Failed())
    {
      return W_FAILURE;
    }

    WFileSystem::AddDataDirectory(">Wtest/", "TexConvDataDir", "imgout", WDataDirUsage::AllowWrites).IgnoreResult();

    WTestFramework::GetInstance()->SetImageReferenceTagsFromEnvironment(W_PLATFORM_NAME, {}, {});

    return W_SUCCESS;
  }

  virtual WResult DeInitializeTest() override
  {
    m_pState.Clear();

    WFileSystem::RemoveDataDirectoryGroup("TexConvTest");
    WFileSystem::RemoveDataDirectoryGroup("TexConvDataDir");

    WStartup::ShutdownCoreSystems();

    return W_SUCCESS;
  }

  void RunTexConv(WProcessOptions& options, const char* szOutName)
  {
#  if W_ENABLED(W_PLATFORM_WINDOWS)
    const char* szTexConvExecutableName = "WTexConv.exe";
#  else
    const char* szTexConvExecutableName = "WTexConv";
#  endif
    WStringBuilder sTexConvExe = WOSFile::GetApplicationDirectory();
    sTexConvExe.AppendPath(szTexConvExecutableName);
    sTexConvExe.MakeCleanPath();

    if (!W_TEST_BOOL_MSG(WOSFile::ExistsFile(sTexConvExe), "%s does not exist", szTexConvExecutableName))
      return;

    options.m_sProcess = sTexConvExe;

    WStringBuilder sOut = WTestFramework::GetInstance()->GetAbsOutputPath();
    sOut.AppendPath("Temp", szOutName);

    options.AddArgument("-out");
    options.AddArgument(sOut);

    if (!W_TEST_BOOL(m_pState->m_TexConvGroup.Launch(options).Succeeded()))
      return;

    if (!W_TEST_BOOL_MSG(m_pState->m_TexConvGroup.WaitToFinish(WTime::MakeFromMinutes(1.0)).Succeeded(), "TexConv did not finish in time."))
      return;

    W_TEST_INT_MSG(m_pState->m_TexConvGroup.GetProcesses().PeekBack().GetExitCode(), 0, "TexConv failed to process the image");

    if (!W_TEST_BOOL_MSG(m_pState->m_image.LoadFrom(sOut).Succeeded(), "Failed to load converted image"))
      return;

    WByteBlobPtr rawImgData = m_pState->m_image.GetByteBlobPtr();
    WUInt64 rawDataHash = WHashingUtils::xxHash64(rawImgData.GetPtr(), rawImgData.GetCount(), 1234);
    // The [test] tag tells the UnitTest to actually output this:
    WLog::Info("[test]Converted file '{0}' has raw data hash: 0x{1}", szOutName, WArgU(rawDataHash, 16, true, 16, false));
  }

  struct State
  {
    WProcessGroup m_TexConvGroup;
    WImage m_image;
  };

  WUniquePtr<State> m_pState;
};

void WTexConvTest::SetupSubTests()
{
  AddSubTest("RGBA to RGB - PNG", SubTest::RgbaToRgbPNG);
  AddSubTest("Combine4 - DDS", SubTest::Combine4);
  AddSubTest("Linear Usage", SubTest::LinearUsage);
  AddSubTest("Extract Channel", SubTest::ExtractChannel);
  AddSubTest("TGA loading", SubTest::TGA);
}

WTestAppRun WTexConvTest::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  WStringBuilder sImageData;
  WFileSystem::ResolvePath(":testdata/TexConv", &sImageData, nullptr).IgnoreResult();

  const WStringBuilder sPathEZ(sImageData, "/W.png");
  const WStringBuilder sPathE(sImageData, "/E.png");
  const WStringBuilder sPathZ(sImageData, "/Z.png");
  const WStringBuilder sPathShape(sImageData, "/Shape.png");
  const WStringBuilder sPathTGAv(sImageData, "/W_flipped_v.tga");
  const WStringBuilder sPathTGAh(sImageData, "/W_flipped_h.tga");
  const WStringBuilder sPathTGAvhCompressed(sImageData, "/W_flipped_vh.tga");

  if (iIdentifier == SubTest::RgbaToRgbPNG)
  {
    WProcessOptions opt;
    opt.AddArgument("-rgb");
    opt.AddArgument("in0");

    opt.AddArgument("-in0");
    opt.AddArgument(sPathEZ);

    RunTexConv(opt, "RgbaToRgbPNG.png");

    W_TEST_IMAGE(0, 10);
  }

  if (iIdentifier == SubTest::Combine4)
  {
    WProcessOptions opt;
    opt.AddArgument("-in0");
    opt.AddArgument(sPathE);

    opt.AddArgument("-in1");
    opt.AddArgument(sPathZ);

    opt.AddArgument("-in2");
    opt.AddArgument(sPathEZ);

    opt.AddArgument("-in3");
    opt.AddArgument(sPathShape);

    opt.AddArgument("-r");
    opt.AddArgument("in1.r");

    opt.AddArgument("-g");
    opt.AddArgument("in0.r");

    opt.AddArgument("-b");
    opt.AddArgument("in2.r");

    opt.AddArgument("-a");
    opt.AddArgument("in3.r");

    opt.AddArgument("-type");
    opt.AddArgument("2D");

    opt.AddArgument("-compression");
    opt.AddArgument("medium");

    opt.AddArgument("-mipmaps");
    opt.AddArgument("linear");

    opt.AddArgument("-usage");
    opt.AddArgument("color");

    RunTexConv(opt, "Combine4.dds");

    // Threshold needs to be higher here since we might fall back to software dxt compression
    // which results in slightly different results than GPU dxt compression.
    W_TEST_IMAGE(1, 100);
  }

  if (iIdentifier == SubTest::LinearUsage)
  {
    WProcessOptions opt;
    opt.AddArgument("-in0");
    opt.AddArgument(sPathE);

    opt.AddArgument("-in1");
    opt.AddArgument(sPathZ);

    opt.AddArgument("-in2");
    opt.AddArgument(sPathEZ);

    opt.AddArgument("-in3");
    opt.AddArgument(sPathShape);

    opt.AddArgument("-r");
    opt.AddArgument("in3");

    opt.AddArgument("-g");
    opt.AddArgument("in0");

    opt.AddArgument("-b");
    opt.AddArgument("in2");

    opt.AddArgument("-compression");
    opt.AddArgument("high");

    opt.AddArgument("-mipmaps");
    opt.AddArgument("kaiser");

    opt.AddArgument("-usage");
    opt.AddArgument("linear");

    opt.AddArgument("-downscale");
    opt.AddArgument("1");

    RunTexConv(opt, "Linear.dds");

    W_TEST_IMAGE(2, 10);
  }

  if (iIdentifier == SubTest::ExtractChannel)
  {
    WProcessOptions opt;
    opt.AddArgument("-in0");
    opt.AddArgument(sPathEZ);

    opt.AddArgument("-r");
    opt.AddArgument("in0.r");

    opt.AddArgument("-compression");
    opt.AddArgument("none");

    opt.AddArgument("-mipmaps");
    opt.AddArgument("none");

    opt.AddArgument("-usage");
    opt.AddArgument("linear");

    opt.AddArgument("-maxRes");
    opt.AddArgument("64");

    RunTexConv(opt, "ExtractChannel.dds");

    W_TEST_IMAGE(3, 10);
  }

  if (iIdentifier == SubTest::TGA)
  {
    {
      WProcessOptions opt;
      opt.AddArgument("-in0");
      opt.AddArgument(sPathTGAv);

      opt.AddArgument("-rgba");
      opt.AddArgument("in0");

      opt.AddArgument("-usage");
      opt.AddArgument("linear");

      RunTexConv(opt, "W_flipped_v.dds");

      W_TEST_IMAGE(3, 10);
    }

    {
      WProcessOptions opt;
      opt.AddArgument("-in0");
      opt.AddArgument(sPathTGAh);

      opt.AddArgument("-rgba");
      opt.AddArgument("in0");

      opt.AddArgument("-usage");
      opt.AddArgument("linear");

      RunTexConv(opt, "W_flipped_h.dds");

      W_TEST_IMAGE(4, 10);
    }

    {
      WProcessOptions opt;
      opt.AddArgument("-in0");
      opt.AddArgument(sPathTGAvhCompressed);

      opt.AddArgument("-rgba");
      opt.AddArgument("in0");

      opt.AddArgument("-usage");
      opt.AddArgument("linear");

      RunTexConv(opt, "W_flipped_vh.dds");

      W_TEST_IMAGE(5, 10);
    }
  }

  return WTestAppRun::Quit;
}



static WTexConvTest s_WTexConvTest;

#endif
