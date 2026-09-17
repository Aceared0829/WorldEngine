#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

/// Render pass that applies a gaussian blur to an input texture.
///
/// Performs a two-pass separable gaussian blur with configurable radius.
/// Output has the same format as the input.
class W_RENDERERCORE_DLL WBlurPass : public WRenderPipelinePass
{
  W_ADD_DYNAMIC_REFLECTION(WBlurPass, WRenderPipelinePass);

public:
  WBlurPass();
  ~WBlurPass();

  virtual WStatus AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs) override;
  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;

  void SetRadius(WInt32 iRadius);           ///< Sets the blur radius in pixels.
  WInt32 GetRadius() const;                 ///< Returns the current blur radius.

protected:
  WRenderPipelineNodeInputPin m_PinInput;   ///< Input texture to blur.
  WRenderPipelineNodeOutputPin m_PinOutput; ///< Blurred output texture.

  WInt32 m_iRadius = 15;                    ///< Blur radius in pixels.
  WConstantBufferStorageHandle m_hBlurCB;   ///< Constant buffer for blur parameters.
  WShaderResourceHandle m_hShader;          ///< Blur shader.
};
