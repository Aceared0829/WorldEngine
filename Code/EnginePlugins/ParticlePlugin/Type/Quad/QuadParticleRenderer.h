#pragma once

#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/Renderer/ParticleRenderer.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererFoundation/Resources/BufferPool.h>

#include <RendererCore/../../../Data/Plugins/ParticlePlugin/Shaders/Particles/BillboardQuadParticleShaderData.h>
#include <RendererCore/../../../Data/Plugins/ParticlePlugin/Shaders/Particles/TangentQuadParticleShaderData.h>

/// Render data for quad particles.
class W_PARTICLEPLUGIN_DLL WParticleQuadRenderData final : public WRenderData
{
  W_ADD_DYNAMIC_REFLECTION(WParticleQuadRenderData, WRenderData);

public:
  virtual bool CanBatch(const WRenderData& other) const override;

  WTexture2DResourceHandle m_hTexture;
  WArrayPtr<WBaseParticleShaderData> m_BaseParticleData;
  WArrayPtr<WBillboardQuadParticleShaderData> m_BillboardParticleData;
  WArrayPtr<WTangentQuadParticleShaderData> m_TangentParticleData;
  WTransform m_GlobalTransform;
  WTime m_TotalEffectLifeTime;
  WUInt8 m_uiNumVariationsX = 1;
  WUInt8 m_uiNumVariationsY = 1;
  WUInt8 m_uiNumFlipbookAnimationsX = 1;
  WUInt8 m_uiNumFlipbookAnimationsY = 1;
  WEnum<WParticleTextureAtlasOrientation> m_TextureAtlasOrientation;
  WEnum<WParticleTypeRenderMode> m_RenderMode;
  WEnum<WParticleLightingMode> m_LightingMode;

  WTempHashedString m_QuadModePermutation;

  float m_fNormalCurvature = 0.5f;
  float m_fLightDirectionality = 0.5f;
  float m_fGeometryProximityFadeOut = 0.1f;
  float m_fCameraProximityFadeOut = 0.5f;
  WMaterialResourceHandle m_hCustomMaterial;
};

/// Renderer for quad particle systems.
class W_PARTICLEPLUGIN_DLL WParticleQuadRenderer final : public WParticleRenderer
{
  W_ADD_DYNAMIC_REFLECTION(WParticleQuadRenderer, WParticleRenderer);
  W_DISALLOW_COPY_AND_ASSIGN(WParticleQuadRenderer);

public:
  WParticleQuadRenderer();
  ~WParticleQuadRenderer();

  virtual void GetSupportedRenderDataTypes(WDynamicArray<const WRTTI*>& out_types) const override;
  virtual void RenderBatch(const WRenderViewContext& renderContext, const WRenderPipelinePass* pPass, const WRenderDataBatch& batch) const override;


protected:
  void ConfigureRenderMode(const WParticleQuadRenderData* pRenderData, WRenderContext* pRenderContext) const;

  static const WUInt32 s_uiParticlesPerBatch = 1024;
  WGALBufferPool m_BaseDataBuffer;
  WGALBufferPool m_BillboardDataBuffer;
  WGALBufferPool m_TangentDataBuffer;
};
