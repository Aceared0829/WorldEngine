#include <RendererTest/RendererTestPCH.h>

#include <Foundation/Utilities/CommandLineUtils.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/TestSetup.h>

#include <RendererTest/Advanced/OffscreenRenderer.h>

W_TESTFRAMEWORK_ENTRY_POINT_BEGIN("RendererTest", "Renderer Tests")
{
  WTextureUtils::s_bForceFullQualityAlways = true; // never allow to use low-res textures

  WCommandLineUtils cmd;
  cmd.SetCommandLine(argc, (const char**)argv, WCommandLineUtils::PreferOsArgs);

  if (cmd.GetBoolOption("-offscreen"))
  {
    WOffscreenRendererTest offScreenTest;
    offScreenTest.SetCommandLineArguments(argc, (const char**)argv);
    WRun(&offScreenTest); // Life cycle & run method calling
    const int iReturnCode = offScreenTest.GetReturnCode();
    // shutdown with exit code
    WTestSetup::DeInitTestFramework(true);
    return iReturnCode;
  }
}
W_TESTFRAMEWORK_ENTRY_POINT_END()
