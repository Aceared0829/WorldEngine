#pragma once

#include "../TestClass/TestClass.h"
#include <Foundation/Communication/IpcChannel.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererTest/Advanced/OffscreenRenderer.h>

class WRendererTestAdvancedFeatures : public WGraphicsTest
{
public:
  virtual const char* GetTestName() const override { return "AdvancedFeatures"; }

private:
  enum SubTests
  {
    ST_ReadRenderTarget,
    ST_VertexShaderRenderTargetArrayIndex,
    ST_SharedTexture,
    ST_Tessellation,
    ST_Compute,
    ST_FloatSampling, // Either natively or emulated sampling of floating point textures e.g. depth textures.
    ST_ProxyTexture,
    ST_Material,
    ST_MSAAResolve,
    ST_ViewFormatOverride,
    ST_DepthBias,
    ST_ConservativeRasterization
  };

  enum ImageCaptureFrames
  {
    DefaultCapture = 5,
    Material_ColorChange = 6,
    Material_ColorChange2 = 7,
    Material_ChangeTexture = 8
  };

  virtual void SetupSubTests() override;

  virtual WResult InitializeSubTest(WInt32 iIdentifier) override;
  virtual WResult DeInitializeSubTest(WInt32 iIdentifier) override;
  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) override;

  void ReadRenderTarget();
  void FloatSampling();
  void ProxyTexture();
  void VertexShaderRenderTargetArrayIndex();
  void Tessellation();
  void Compute();
  void MSAAResolve();
  void ViewFormatOverride();
  void DepthBias();
  void ConservativeRasterization();
  WTestAppRun Material();
  WTestAppRun SharedTexture();
  void OffscreenProcessMessageFunc(const WIpcProcessMessageProtocol::Event& msg);

private:
  WShaderResourceHandle m_hShader2;
  WShaderResourceHandle m_hShader3;

  WGALTextureHandle m_hTexture2D;
  WGALTextureRange m_Texture2DRange;
  WGALTextureHandle m_hTexture2DArray;

  // Proxy texture test
  WGALTextureHandle m_hProxyTexture2D[2];

  // Render target view format override test
  WGALTextureHandle m_hOverrideTexture2D[2];
  WGALRenderTargetViewHandle m_hOverrideRTV[2];
  WEnum<WGALResourceFormat> m_OverrideSrgbFormat;
  // Float sampling test
  WGALSamplerStateHandle m_hDepthSamplerState;

  // Tessellation Test
  WMeshBufferResourceHandle m_hSphereMesh;

  // MSAA Resolve Test
  WGALTextureHandle m_hMSAAColor;
  WGALTextureHandle m_hMSAADepthStencil;
  WGALTextureHandle m_hMSAAResolveTarget;
  WMeshBufferResourceHandle m_hMSAAQuadMesh;
  WShaderResourceHandle m_hMSAAStencilShader;
  WGALReadbackTextureHelper m_MSAAReadback;
  WEnum<WGALMSAASampleCount> m_MSAASamples;

  // Depth Bias Test
  WGALTextureHandle m_hDepthBiasColor;
  WGALTextureHandle m_hDepthBiasDepth;
  WMeshBufferResourceHandle m_hDepthBiasQuadMesh;
  WShaderResourceHandle m_hDepthBiasShader;
  WGALReadbackTextureHelper m_DepthBiasReadback;
  float m_fDepthBiasUnit = 0.0f; ///< Minimum resolvable depth difference of the chosen depth format, the unit that m_iDepthBias is measured in.

  // Conservative Rasterization Test
  WGALTextureHandle m_hConservativeRasterColor;
  WMeshBufferResourceHandle m_hConservativeRasterQuadMesh;
  WShaderResourceHandle m_hConservativeRasterShader;
  WGALReadbackTextureHelper m_ConservativeRasterReadback;

  // Material Test
  WTexture2DResourceHandle m_hTexture;
  WTexture2DResourceHandle m_hTexture2;
  WMaterialResourceHandle m_hMaterial;
  WHashedString m_sBaseColor;
  WHashedString m_sBaseColor2;
  WHashedString m_sTexture;

  // Shared Texture Test
#if W_ENABLED(W_SUPPORTS_PROCESSES)
  WUniquePtr<WProcess> m_pOffscreenProcess;
  WUniquePtr<WIpcChannel> m_pChannel;
  WUniquePtr<WIpcProcessMessageProtocol> m_pProtocol;
  WGALTextureCreationDescription m_SharedTextureDesc;

  static constexpr WUInt32 s_SharedTextureCount = 3;

  bool m_bExiting = false;
  WGALTextureHandle m_hSharedTextures[s_SharedTextureCount];
  WDeque<WOffscreenTest_SharedTexture> m_SharedTextureQueue;
  WUInt32 m_uiReceivedTextures = 0;
  float m_fOldProfilingThreshold = 0.0f;
  WShaderUtils::WBuiltinShader m_CopyShader;
#endif
};
