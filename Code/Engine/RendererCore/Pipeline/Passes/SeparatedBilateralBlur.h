#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

/// Render pass that applies depth-aware blur to preserve edges.
///
/// Uses a bilateral filter that considers both spatial distance and depth difference
/// to avoid blurring across edges. Implemented as a two-pass separable filter, which
/// is technically approximate but works well in practice (hence "separated" not "separable").
class W_RENDERERCORE_DLL WSeparatedBilateralBlurPass : public WRenderPipelinePass
{
  W_ADD_DYNAMIC_REFLECTION(WSeparatedBilateralBlurPass, WRenderPipelinePass);

public:
  WSeparatedBilateralBlurPass();
  ~WSeparatedBilateralBlurPass();

  virtual WStatus AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs) override;
  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;

  void SetRadius(WUInt32 uiRadius);   ///< Sets the blur kernel radius in pixels.
  WUInt32 GetRadius() const;

  void SetGaussianSigma(float fSigma); ///< Sets the gaussian distribution sigma.
  float GetGaussianSigma() const;

  void SetSharpness(float fSharpness); ///< Sets edge preservation strength.
  float GetSharpness() const;

protected:
  WRenderPipelineNodeInputPin m_PinBlurSourceInput; ///< Source texture to blur.
  WRenderPipelineNodeInputPin m_PinDepthInput;      ///< Depth buffer for edge detection.
  WRenderPipelineNodeOutputPin m_PinOutput;         ///< Blurred output.

  WUInt32 m_uiRadius = 7;                           ///< Blur kernel radius in pixels.
  float m_fGaussianSigma = 3.5f;                     ///< Gaussian distribution sigma.
  float m_fSharpness = 120.0f;                       ///< Edge preservation strength (higher = sharper edges).
  WConstantBufferStorageHandle m_hBilateralBlurCB;  ///< Constant buffer for blur parameters.
  WShaderResourceHandle m_hShader;                  ///< Bilateral blur shader.
};
