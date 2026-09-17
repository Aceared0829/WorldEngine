#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

/// Render pass that computes screen-space ambient occlusion (SSAO).
///
/// Generates an ambient occlusion texture from the depth buffer. The effect darkens areas
/// where geometry is close together, simulating indirect lighting occlusion. Supports distance-based
/// fade-out and various quality parameters.
class W_RENDERERCORE_DLL WAOPass : public WRenderPipelinePass
{
  W_ADD_DYNAMIC_REFLECTION(WAOPass, WRenderPipelinePass);

public:
  WAOPass();
  ~WAOPass();

  virtual WStatus AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs) override;
  virtual WStatus AddRenderPassesInactive(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs) override;

  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;

  void SetFadeOutStart(float fStart);
  float GetFadeOutStart() const;

  void SetFadeOutEnd(float fEnd);
  float GetFadeOutEnd() const;

protected:
  void CreateSamplerState();

  WRenderPipelineNodeInputPin m_PinDepthInput;
  WRenderPipelineNodeOutputPin m_PinOutput;

  float m_fRadius = 1.0f;
  float m_fMaxScreenSpaceRadius = 1.0f;
  float m_fContrast = 2.0f;
  float m_fIntensity = 0.7f;

  float m_fFadeOutStart = 80.0f;
  float m_fFadeOutEnd = 100.0f;

  float m_fPositionBias = 5.0f;
  float m_fMipLevelScale = 10.0f;
  float m_fDepthBlurThreshold = 2.0f;

  WConstantBufferStorageHandle m_hDownscaleConstantBuffer;
  WConstantBufferStorageHandle m_hSSAOConstantBuffer;

  WTexture2DResourceHandle m_hNoiseTexture;

  WGALSamplerStateHandle m_hSSAOSamplerState;

  WShaderResourceHandle m_hDownscaleShader;
  WShaderResourceHandle m_hSSAOShader;
  WShaderResourceHandle m_hBlurShader;

  // Graph state
  WRenderGraphTextureHandle m_hHzbTexture;
  WTempHybridArray<WVec2, 8> m_HzbSizes;
  WTempHybridArray<WGALTextureRange, 8> m_HzbResourceViews;
  WRenderGraphTextureHandle m_hSSAOTemp;
};
