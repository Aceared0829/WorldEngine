#pragma once

#include "../TestClass/TestClass.h"
#include <RendererCore/Textures/Texture2DResource.h>

class WRendererTestBasics : public WGraphicsTest
{
public:
  virtual const char* GetTestName() const override { return "Basics"; }

private:
  enum SubTests
  {
    ST_ClearScreen,
    ST_RasterizerStates,
    ST_BlendStates,
    ST_Textures2D,
    ST_Textures3D,
    ST_TexturesCube,
    ST_LineRendering,
  };

  virtual void SetupSubTests() override
  {
    AddSubTest("Clear Screen", SubTests::ST_ClearScreen);
    AddSubTest("Rasterizer States", SubTests::ST_RasterizerStates);
    AddSubTest("Blend States", SubTests::ST_BlendStates);
    AddSubTest("2D Textures", SubTests::ST_Textures2D);
    // AddSubTest("3D Textures", SubTests::ST_Textures3D); /// \todo 3D Texture support is currently not implemented
    AddSubTest("Cube Textures", SubTests::ST_TexturesCube);
    AddSubTest("Line Rendering", SubTests::ST_LineRendering);
  }

  virtual WResult InitializeSubTest(WInt32 iIdentifier) override;
  virtual WResult DeInitializeSubTest(WInt32 iIdentifier) override;

  WTestAppRun SubtestClearScreen();
  WTestAppRun SubtestRasterizerStates();
  WTestAppRun SubtestBlendStates();
  WTestAppRun SubtestTextures2D();
  WTestAppRun SubtestTextures3D();
  WTestAppRun SubtestTexturesCube();
  WTestAppRun SubtestLineRendering();

  void RenderObjects(WBitflags<WShaderBindFlags> ShaderBindFlags);
  void RenderLineObjects(WBitflags<WShaderBindFlags> ShaderBindFlags);

  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) override
  {
    m_iFrame = uiInvocationCount;

    if (iIdentifier == SubTests::ST_ClearScreen)
      return SubtestClearScreen();

    if (iIdentifier == SubTests::ST_RasterizerStates)
      return SubtestRasterizerStates();

    if (iIdentifier == SubTests::ST_BlendStates)
      return SubtestBlendStates();

    if (iIdentifier == SubTests::ST_Textures2D)
      return SubtestTextures2D();

    if (iIdentifier == SubTests::ST_Textures3D)
      return SubtestTextures3D();

    if (iIdentifier == SubTests::ST_TexturesCube)
      return SubtestTexturesCube();

    if (iIdentifier == SubTests::ST_LineRendering)
      return SubtestLineRendering();

    return WTestAppRun::Quit;
  }

  WMeshBufferResourceHandle m_hSphere;
  WMeshBufferResourceHandle m_hSphere2;
  WMeshBufferResourceHandle m_hTorus;
  WMeshBufferResourceHandle m_hLongBox;
  WMeshBufferResourceHandle m_hLineBox;
  WTexture2DResourceHandle m_hTexture2D;
  WTextureCubeResourceHandle m_hTextureCube;
};
