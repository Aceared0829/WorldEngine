#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/Texture3DResource.h>

/// Render pass that applies tonemapping and color grading to HDR images.
///
/// Converts high dynamic range color values to display-ready output by applying
/// exposure adjustment, color grading via lookup tables, saturation, contrast, and
/// optional effects like vignetting and mood color. Combines with bloom if provided.
class W_RENDERERCORE_DLL WTonemapPass : public WRenderPipelinePass
{
  W_ADD_DYNAMIC_REFLECTION(WTonemapPass, WRenderPipelinePass);

public:
  WTonemapPass();
  ~WTonemapPass();

  virtual WStatus AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs) override;
  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;

protected:
  WRenderPipelineNodeInputPin m_PinColorInput;                             ///< HDR color input to tonemap.
  WRenderPipelineNodeInputPin m_PinBloomInput;                             ///< Optional bloom texture to add.
  WRenderPipelineNodeOutputPin m_PinOutput;                                ///< LDR output after tonemapping.

  W_ADD_RESOURCEHANDLE_ACCESSORS(VignettingTexture, m_hVignettingTexture); ///< Vignette texture.
  W_ADD_RESOURCEHANDLE_ACCESSORS(LUT1Texture, m_hLUT1);                    ///< Primary color grading LUT.
  W_ADD_RESOURCEHANDLE_ACCESSORS(LUT2Texture, m_hLUT2);                    ///< Secondary color grading LUT.

  WTexture2DResourceHandle m_hVignettingTexture;                           ///< Vignetting effect texture.
  WTexture2DResourceHandle m_hNoiseTexture;                                ///< Film grain noise texture.
  WTexture2DResourceHandle m_hBlackTexture;                                ///< Black texture for fallback.
  WTexture3DResourceHandle m_hLUT1;                                        ///< First 3D lookup table for color grading.
  WTexture3DResourceHandle m_hLUT2;                                        ///< Second 3D lookup table for color grading.

  WColor m_MoodColor;                                                      ///< Mood color tint.
  float m_fMoodStrength;                                                    ///< Strength of mood color effect.
  float m_fSaturation;                                                      ///< Color saturation multiplier.
  float m_fContrast;                                                        ///< Contrast adjustment.
  float m_fLut1Strength;                                                    ///< Blend strength for first LUT.
  float m_fLut2Strength;                                                    ///< Blend strength for second LUT.
  float m_fWhitePoint;                                                      ///< White point for tone curve.

  WConstantBufferStorageHandle m_hConstantBuffer;                          ///< Constant buffer for tonemap parameters.
  WShaderResourceHandle m_hShader;                                         ///< Tonemap shader.
};
