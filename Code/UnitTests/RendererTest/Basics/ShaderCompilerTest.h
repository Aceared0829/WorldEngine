#pragma once

#include "../TestClass/TestClass.h"
#include <RendererCore/Textures/Texture2DResource.h>

class WRendererTestShaderCompiler : public WGraphicsTest
{
  using SUPER = WGraphicsTest;

  enum SubTests
  {
    ST_ShaderResources,
  };

public:
  virtual const char* GetTestName() const override { return "ShaderCompiler"; }

private:
  virtual void SetupSubTests() override;

  virtual WResult InitializeSubTest(WInt32 iIdentifier) override;
  virtual WResult DeInitializeSubTest(WInt32 iIdentifier) override;
  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) override;

private:
  WShaderResourceHandle m_hUVColorShader;
};
