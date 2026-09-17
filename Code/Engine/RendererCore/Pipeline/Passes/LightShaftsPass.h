#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/Texture2DResource.h>

class WClusteredDataCPU;

/// Screen-space light shafts (crepuscular rays) post-processing pass.
///
/// Generates volumetric light shaft effects from the brightest directional light by
/// building a mask from the depth buffer and applying a radial blur toward
/// the projected light position. The result is additively composited into the scene color.
class W_RENDERERCORE_DLL WLightShaftsPass : public WRenderPipelinePass
{
  W_ADD_DYNAMIC_REFLECTION(WLightShaftsPass, WRenderPipelinePass);

public:
  WLightShaftsPass();
  ~WLightShaftsPass();

  virtual WStatus AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs) override;
  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;

  void SetDownsampleFactor(WUInt32 uiFactor);                          // [ property ]
  WUInt32 GetDownsampleFactor() const { return m_uiDownsampleFactor; } // [ property ]

  void SetNumBlurPasses(WUInt32 uiPasses);                             // [ property ]
  WUInt32 GetNumBlurPasses() const { return m_uiNumBlurPasses; }       // [ property ]

  void SetNumSamples(WUInt32 uiSamples);                               // [ property ]
  WUInt32 GetNumSamples() const { return m_uiNumSamples; }             // [ property ]

  void SetMaxBlurDistance(float fDistance);                             // [ property ]
  float GetMaxBlurDistance() const { return m_fMaxBlurDistance; }       // [ property ]

protected:
  WVec4 CalculateOriginUVs(const WVec3& vLightDirection, const WViewData& viewData, const WCamera& camera) const;
  void UpdateConstantBuffer(const WClusteredDataCPU& clusteredData, const WVec2& vLightOriginUVs, float fBlurStep);

  WRenderPipelineNodePassThroughPin m_PinColor;
  WRenderPipelineNodeInputPin m_PinDepthInput;

  WConstantBufferStorageHandle m_hConstantBuffer;
  WShaderResourceHandle m_hMaskShader;
  WShaderResourceHandle m_hRadialBlurShader;
  WShaderResourceHandle m_hApplyShader;

  WEnum<WGALResourceFormat> m_TextureFormat;

  // Properties
  WUInt8 m_uiDownsampleFactor = 3;
  WUInt8 m_uiNumBlurPasses = 3;
  WUInt8 m_uiNumSamples = 12;
  float m_fMaxBlurDistance = 1.0f;
};
