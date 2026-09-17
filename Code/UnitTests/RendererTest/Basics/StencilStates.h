#pragma once

#include <RendererTest/TestClass/TestClass.h>

/// Tests stencil buffer operations and compare functions.
class WRendererTestStencilStates : public WGraphicsTest
{
public:
  virtual const char* GetTestName() const override { return "StencilStates"; }

private:
  enum SubTests
  {
    ST_StencilOperations,
    ST_StencilCompareFunctions,
    ST_StencilRefValue,
  };

  virtual void SetupSubTests() override
  {
    AddSubTest("Stencil Operations", SubTests::ST_StencilOperations);
    AddSubTest("Stencil Compare Functions", SubTests::ST_StencilCompareFunctions);
    AddSubTest("Stencil Reference Value", SubTests::ST_StencilRefValue);
  }

  virtual WResult InitializeSubTest(WInt32 iIdentifier) override;
  virtual WResult DeInitializeSubTest(WInt32 iIdentifier) override;

  WTestAppRun SubtestStencilOperations();
  WTestAppRun SubtestStencilCompareFunctions();
  WTestAppRun SubtestStencilRefValue();

  void RenderQuad(const WMat4& mTransform, const WColor& color, WBitflags<WShaderBindFlags> ShaderBindFlags = WShaderBindFlags::Default);

  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) override
  {
    m_iFrame = uiInvocationCount;

    if (iIdentifier == SubTests::ST_StencilOperations)
      return SubtestStencilOperations();

    if (iIdentifier == SubTests::ST_StencilCompareFunctions)
      return SubtestStencilCompareFunctions();

    if (iIdentifier == SubTests::ST_StencilRefValue)
      return SubtestStencilRefValue();

    return WTestAppRun::Quit;
  }

  WMeshBufferResourceHandle m_hQuadMesh;
  WShaderResourceHandle m_hStencilShader;
};
