#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/RendererFoundationDLL.h>

/// Screen-space contact shadows using per-pixel ray marching.
///
/// This implementation is using the "screen space shadow" code by Bend studio.
class W_RENDERERCORE_DLL WScreenSpaceShadowPass : public WRenderPipelinePass
{
  W_ADD_DYNAMIC_REFLECTION(WScreenSpaceShadowPass, WRenderPipelinePass);

public:
  WScreenSpaceShadowPass();
  ~WScreenSpaceShadowPass();

  virtual WStatus AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs) override;
  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;

protected:
  void CreateSamplerState();

  WRenderPipelineNodeInputPin m_PinDepthInput;
  WRenderPipelineNodeOutputPin m_PinOutput;

  WConstantBufferStorageHandle m_hConstantBuffer;
  WGALSamplerStateHandle m_hDepthSamplerState;

  WShaderResourceHandle m_hShader;

  float m_fSurfaceThickness = 0.005f;
  float m_fShadowContrast = 4.0f;
};
