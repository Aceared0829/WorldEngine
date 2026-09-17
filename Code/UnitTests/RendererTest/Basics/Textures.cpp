#include <RendererTest/RendererTestPCH.h>

#include "Basics.h"
#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>

WTestAppRun WRendererTestBasics::SubtestTextures2D()
{
  BeginFrame();
  BeginCommands("Textures2D");
  TransitionTexture(GetBackbuffer(), WGALResourceState::RenderTarget);

  WRenderContext::GetDefaultInstance()->SetDefaultTextureQuality(WGALTextureQuality::Trilinear);

  const WInt32 iNumFrames = 14;

  m_hShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/Textured.WShader");
  WEnum<WGALResourceFormat> textureFormat;
  WStringView sTextureResourceId;

  if (m_iFrame == 0)
  {
    textureFormat = WGALResourceFormat::RGBAUByteNormalizedsRGB;
    sTextureResourceId = "SharedData/Textures/WLogo_ABGR_Mips_D.dds";
  }

  if (m_iFrame == 1)
  {
    textureFormat = WGALResourceFormat::RGBAUByteNormalizedsRGB;
    sTextureResourceId = "SharedData/Textures/WLogo_ABGR_NoMips_D.dds";
  }

  if (m_iFrame == 2)
  {
    textureFormat = WGALResourceFormat::BGRAUByteNormalizedsRGB;
    sTextureResourceId = "SharedData/Textures/WLogo_ARGB_Mips_D.dds";
  }

  if (m_iFrame == 3)
  {
    textureFormat = WGALResourceFormat::BGRAUByteNormalizedsRGB;
    sTextureResourceId = "SharedData/Textures/WLogo_ARGB_NoMips_D.dds";
  }

  if (m_iFrame == 4)
  {
    textureFormat = WGALResourceFormat::BC1sRGB;
    sTextureResourceId = "SharedData/Textures/WLogo_DXT1_Mips_D.dds";
  }

  if (m_iFrame == 5)
  {
    textureFormat = WGALResourceFormat::BC1sRGB;
    sTextureResourceId = "SharedData/Textures/WLogo_DXT1_NoMips_D.dds";
  }

  if (m_iFrame == 6)
  {
    textureFormat = WGALResourceFormat::BC2sRGB;
    sTextureResourceId = "SharedData/Textures/WLogo_DXT3_Mips_D.dds";
  }

  if (m_iFrame == 7)
  {
    textureFormat = WGALResourceFormat::BC2sRGB;
    sTextureResourceId = "SharedData/Textures/WLogo_DXT3_NoMips_D.dds";
  }

  if (m_iFrame == 8)
  {
    textureFormat = WGALResourceFormat::BC3sRGB;
    sTextureResourceId = "SharedData/Textures/WLogo_DXT5_Mips_D.dds";
  }

  if (m_iFrame == 9)
  {
    textureFormat = WGALResourceFormat::BC3sRGB;
    sTextureResourceId = "SharedData/Textures/WLogo_DXT5_NoMips_D.dds";
  }

  if (m_iFrame == 10)
  {
    textureFormat = WGALResourceFormat::BGRAUByteNormalizedsRGB;
    sTextureResourceId = "SharedData/Textures/WLogo_RGB_Mips_D.dds";
  }

  if (m_iFrame == 11)
  {
    textureFormat = WGALResourceFormat::BGRAUByteNormalizedsRGB;
    sTextureResourceId = "SharedData/Textures/WLogo_RGB_NoMips_D.dds";
  }

  if (m_iFrame == 12)
  {
    textureFormat = WGALResourceFormat::B5G6R5UNormalized;
    sTextureResourceId = "SharedData/Textures/WLogo_R5G6B5_NoMips_D.dds";
  }

  if (m_iFrame == 13)
  {
    textureFormat = WGALResourceFormat::B5G6R5UNormalized;
    sTextureResourceId = "SharedData/Textures/WLogo_R5G6B5_MipsD.dds";
  }

  const bool bSupported = m_pDevice->GetCapabilities().m_FormatSupport[textureFormat].AreAllSet(WGALResourceFormatSupport::Texture);
  if (bSupported)
  {
    WBindGroupBuilder& bindGroupTest = WRenderContext::GetDefaultInstance()->GetBindGroup();
    m_hTexture2D = WResourceManager::LoadResource<WTexture2DResource>(sTextureResourceId);
    bindGroupTest.BindTexture("DiffuseTexture", m_hTexture2D);
  }
  BeginRendering(WColor::Black);

  if (bSupported)
  {
    RenderObjects(WShaderBindFlags::Default);
  }

  EndRendering();

  if (bSupported)
  {
    TransitionTexture(GetBackbuffer(), WGALResourceState::CopySource);
    W_TEST_IMAGE(m_iFrame, 300);
  }

  EndCommands();
  EndFrame();

  return m_iFrame < (iNumFrames - 1) ? WTestAppRun::Continue : WTestAppRun::Quit;
}


WTestAppRun WRendererTestBasics::SubtestTextures3D()
{
  BeginFrame();
  BeginCommands("Textures3D");
  TransitionTexture(GetBackbuffer(), WGALResourceState::RenderTarget);
  WRenderContext::GetDefaultInstance()->SetDefaultTextureQuality(WGALTextureQuality::Trilinear);

  const WInt32 iNumFrames = 1;

  m_hShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/TexturedVolume.WShader");

  if (m_iFrame == 0)
  {
    m_hTexture2D = WResourceManager::LoadResource<WTexture2DResource>("SharedData/Textures/Volume/WLogo_Volume_A8_NoMips_D.dds");
  }
  WBindGroupBuilder& bindGroupTest = WRenderContext::GetDefaultInstance()->GetBindGroup();
  bindGroupTest.BindTexture("DiffuseTexture", m_hTexture2D);

  BeginRendering(WColor::Black);

  RenderObjects(WShaderBindFlags::Default);

  TransitionTexture(GetBackbuffer(), WGALResourceState::CopySource);
  W_TEST_IMAGE(m_iFrame, 100);
  EndRendering();
  EndCommands();
  EndFrame();

  return m_iFrame < (iNumFrames - 1) ? WTestAppRun::Continue : WTestAppRun::Quit;
}


WTestAppRun WRendererTestBasics::SubtestTexturesCube()
{
  BeginFrame();
  BeginCommands("TexturesCube");
  TransitionTexture(GetBackbuffer(), WGALResourceState::RenderTarget);

  WRenderContext::GetDefaultInstance()->SetDefaultTextureQuality(WGALTextureQuality::Trilinear);

  const WInt32 iNumFrames = 12;

  m_hShader = WResourceManager::LoadResource<WShaderResource>("RendererTest/Shaders/TexturedCube.WShader");
  WEnum<WGALResourceFormat> textureFormat;
  WStringView sTextureResourceId;

  if (m_iFrame == 0)
  {
    textureFormat = WGALResourceFormat::BGRAUByteNormalizedsRGB;
    sTextureResourceId = "SharedData/Textures/Cubemap/WLogo_Cube_XRGB_NoMips_D.dds";
  }

  if (m_iFrame == 1)
  {
    textureFormat = WGALResourceFormat::BGRAUByteNormalizedsRGB;
    sTextureResourceId = "SharedData/Textures/Cubemap/WLogo_Cube_XRGB_Mips_D.dds";
  }

  if (m_iFrame == 2)
  {
    textureFormat = WGALResourceFormat::RGBAUByteNormalizedsRGB;
    sTextureResourceId = "SharedData/Textures/Cubemap/WLogo_Cube_RGBA_NoMips_D.dds";
  }

  if (m_iFrame == 3)
  {
    textureFormat = WGALResourceFormat::RGBAUByteNormalizedsRGB;
    sTextureResourceId = "SharedData/Textures/Cubemap/WLogo_Cube_RGBA_Mips_D.dds";
  }

  if (m_iFrame == 4)
  {
    textureFormat = WGALResourceFormat::BC1sRGB;
    sTextureResourceId = "SharedData/Textures/Cubemap/WLogo_Cube_DXT1_NoMips_D.dds";
  }

  if (m_iFrame == 5)
  {
    textureFormat = WGALResourceFormat::BC1sRGB;
    sTextureResourceId = "SharedData/Textures/Cubemap/WLogo_Cube_DXT1_Mips_D.dds";
  }

  if (m_iFrame == 6)
  {
    textureFormat = WGALResourceFormat::BC2sRGB;
    sTextureResourceId = "SharedData/Textures/Cubemap/WLogo_Cube_DXT3_NoMips_D.dds";
  }

  if (m_iFrame == 7)
  {
    textureFormat = WGALResourceFormat::BC2sRGB;
    sTextureResourceId = "SharedData/Textures/Cubemap/WLogo_Cube_DXT3_Mips_D.dds";
  }

  if (m_iFrame == 8)
  {
    textureFormat = WGALResourceFormat::BC3sRGB;
    sTextureResourceId = "SharedData/Textures/Cubemap/WLogo_Cube_DXT5_NoMips_D.dds";
  }

  if (m_iFrame == 9)
  {
    textureFormat = WGALResourceFormat::BC3sRGB;
    sTextureResourceId = "SharedData/Textures/Cubemap/WLogo_Cube_DXT5_Mips_D.dds";
  }

  if (m_iFrame == 10)
  {
    textureFormat = WGALResourceFormat::BGRAUByteNormalizedsRGB;
    sTextureResourceId = "SharedData/Textures/Cubemap/WLogo_Cube_RGB_NoMips_D.dds";
  }

  if (m_iFrame == 11)
  {
    textureFormat = WGALResourceFormat::BGRAUByteNormalizedsRGB;
    sTextureResourceId = "SharedData/Textures/Cubemap/WLogo_Cube_RGB_Mips_D.dds";
  }

  const bool bSupported = m_pDevice->GetCapabilities().m_FormatSupport[textureFormat].AreAllSet(WGALResourceFormatSupport::Texture);
  if (bSupported)
  {
    WBindGroupBuilder& bindGroupTest = WRenderContext::GetDefaultInstance()->GetBindGroup();
    m_hTextureCube = WResourceManager::LoadResource<WTextureCubeResource>(sTextureResourceId);
    bindGroupTest.BindTexture("DiffuseTexture", m_hTextureCube);
  }
  BeginRendering(WColor::Black);

  if (bSupported)
  {
    RenderObjects(WShaderBindFlags::Default);
  }

  EndRendering();

  if (bSupported)
  {
    TransitionTexture(GetBackbuffer(), WGALResourceState::CopySource);
    W_TEST_IMAGE(m_iFrame, 200);
  }
  EndCommands();
  EndFrame();

  return m_iFrame < (iNumFrames - 1) ? WTestAppRun::Continue : WTestAppRun::Quit;
}
