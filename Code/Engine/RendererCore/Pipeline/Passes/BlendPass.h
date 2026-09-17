#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

/// Render pass that blends two input textures together.
///
/// Linearly interpolates between two input textures using a blend factor.
/// The output format matches InputA. Both inputs should have the same size and format.
class W_RENDERERCORE_DLL WBlendPass : public WRenderPipelinePass
{
  W_ADD_DYNAMIC_REFLECTION(WBlendPass, WRenderPipelinePass);

public:
  WBlendPass();
  ~WBlendPass();

  virtual WStatus AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs) override;
  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;

protected:
  WRenderPipelineNodeInputPin m_PinInputA;  ///< First input texture.
  WRenderPipelineNodeInputPin m_PinInputB;  ///< Second input texture.
  WRenderPipelineNodeOutputPin m_PinOutput; ///< Blended output.

  float m_fBlendFactor = 0.5f;               ///< Blend factor between inputs (0 = full A, 1 = full B).
  WShaderResourceHandle m_hShader;          ///< Shader for blending operation.
};
