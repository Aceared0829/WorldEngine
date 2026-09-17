#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

/// Render pass that pre-filters cubemaps for image-based lighting.
///
/// Generates filtered specular reflections and irradiance data from an input cubemap.
/// Creates mipmap chains with increasing roughness for specular reflections and computes
/// diffuse irradiance. Used for physically-based rendering with environment maps.
class W_RENDERERCORE_DLL WReflectionFilterPass : public WRenderPipelinePass
{
  W_ADD_DYNAMIC_REFLECTION(WReflectionFilterPass, WRenderPipelinePass);

public:
  WReflectionFilterPass();
  ~WReflectionFilterPass();

  virtual WStatus AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs) override;
  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;

  WUInt32 GetInputCubemap() const;
  void SetInputCubemap(WUInt32 uiCubemapHandle);

protected:
  void UpdateFilteredSpecularConstantBuffer(WUInt32 uiMipMapIndex, WUInt32 uiNumMipMaps, WUInt32 uiWidth, WUInt32 uiHeight);
  void UpdateIrradianceConstantBuffer();

  WRenderPipelineNodeOutputPin m_PinFilteredSpecular;
  WRenderPipelineNodeOutputPin m_PinAvgLuminance;
  WRenderPipelineNodeOutputPin m_PinIrradianceData;

  float m_fDiffuseIntensity = 1.0f;
  float m_fDiffuseSaturation = 1.0f;
  float m_fSpecularIntensity = 1.0f;
  WUInt32 m_uiSpecularOutputIndex = 0;
  WUInt32 m_uiIrradianceOutputIndex = 0;

  WGALTextureHandle m_hInputCubemap;

  WConstantBufferStorageHandle m_hFilteredSpecularConstantBuffer;
  WShaderResourceHandle m_hFilteredSpecularShader;

  WConstantBufferStorageHandle m_hIrradianceConstantBuffer;
  WShaderResourceHandle m_hIrradianceShader;
};
