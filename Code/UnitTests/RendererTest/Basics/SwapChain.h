#pragma once

#include "../TestClass/TestClass.h"
#include <RendererCore/Textures/Texture2DResource.h>

class WRendererTestSwapChain : public WGraphicsTest
{
public:
  virtual const char* GetTestName() const override { return "SwapChain"; }

private:
  enum SubTests
  {
    ST_ColorOnly,
    ST_D16,
    ST_D24S8,
    ST_D32,
    ST_VSync,
    ST_ResizeWindow,
  };

  virtual void SetupSubTests() override
  {
    AddSubTest("Color Only", SubTests::ST_ColorOnly);
    AddSubTest("Depth D16", SubTests::ST_D16);
    AddSubTest("Depth D24S8", SubTests::ST_D24S8);
    AddSubTest("Depth D32", SubTests::ST_D32);
    AddSubTest("VSync", SubTests::ST_VSync);
    AddSubTest("Resize Window", SubTests::ST_ResizeWindow);
  }

  virtual WResult InitializeTest() override;
  virtual WResult DeInitializeTest() override;
  virtual WResult InitializeSubTest(WInt32 iIdentifier) override;
  virtual WResult DeInitializeSubTest(WInt32 iIdentifier) override;

  void ResizeTest(WUInt32 uiInvocationCount);
  WTestAppRun BasicRenderLoop(WInt32 iIdentifier, WUInt32 uiInvocationCount);

  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) override
  {
    m_iFrame = uiInvocationCount;

    switch (iIdentifier)
    {
      case SubTests::ST_ResizeWindow:
        ResizeTest(uiInvocationCount);
        [[fallthrough]];
      case SubTests::ST_ColorOnly:
      case SubTests::ST_D16:
      case SubTests::ST_D24S8:
      case SubTests::ST_D32:
      case SubTests::ST_VSync:
        return BasicRenderLoop(iIdentifier, uiInvocationCount);
      default:
        W_ASSERT_NOT_IMPLEMENTED;
        break;
    }
    return WTestAppRun::Quit;
  }

  WSizeU32 m_CurrentWindowSize = WSizeU32(320, 240);
};
