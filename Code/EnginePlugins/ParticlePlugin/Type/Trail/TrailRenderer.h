#pragma once

#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/ParticlePluginDLL.h>
#include <ParticlePlugin/Renderer/ParticleRenderer.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererFoundation/Resources/BufferPool.h>

#include <RendererCore/../../../Data/Plugins/ParticlePlugin/Shaders/Particles/TrailShaderData.h>

/// Render data for trail particles.
class W_PARTICLEPLUGIN_DLL WParticleTrailRenderData final : public WRenderData
{
  W_ADD_DYNAMIC_REFLECTION(WParticleTrailRenderData, WRenderData);

public:
  virtual bool CanBatch(const WRenderData& other) const override;

  WTexture2DResourceHandle m_hTexture;
  WMaterialResourceHandle m_hCustomMaterial;
  WUInt16 m_uiMaxTrailPoints;
  float m_fSnapshotFraction;
  WArrayPtr<WBaseParticleShaderData> m_BaseParticleData;
  WArrayPtr<WTrailParticleShaderData> m_TrailParticleData;
  WArrayPtr<WVec4> m_TrailPointsShared;
  WTransform m_GlobalTransform;
  WTime m_TotalEffectLifeTime;
  WUInt8 m_uiNumVariationsX = 1;
  WUInt8 m_uiNumVariationsY = 1;
  WUInt8 m_uiNumFlipbookAnimationsX = 1;
  WUInt8 m_uiNumFlipbookAnimationsY = 1;
  WEnum<WParticleTextureAtlasOrientation> m_TextureAtlasOrientation;

  WEnum<WParticleTypeRenderMode> m_RenderMode;
  WEnum<WParticleLightingMode> m_LightingMode;
  float m_fNormalCurvature = 0.5f;
  float m_fLightDirectionality = 0.5f;
};

/// Implements rendering of a trail particle systems
class W_PARTICLEPLUGIN_DLL WParticleTrailRenderer final : public WParticleRenderer
{
  W_ADD_DYNAMIC_REFLECTION(WParticleTrailRenderer, WParticleRenderer);
  W_DISALLOW_COPY_AND_ASSIGN(WParticleTrailRenderer);

public:
  WParticleTrailRenderer();
  ~WParticleTrailRenderer();

  virtual void GetSupportedRenderDataTypes(WDynamicArray<const WRTTI*>& out_types) const override;
  virtual void RenderBatch(
    const WRenderViewContext& renderContext, const WRenderPipelinePass* pPass, const WRenderDataBatch& batch) const override;

protected:
  bool ConfigureShader(const WParticleTrailRenderData* pRenderData, const WRenderViewContext& renderViewContext) const;

  static const WUInt32 s_uiParticlesPerBatch = 512;
  WGALBufferPool m_BaseDataBuffer;
  WGALBufferPool m_TrailDataBuffer;
  WGALBufferPool m_TrailPointsDataBuffer8;
  WGALBufferPool m_TrailPointsDataBuffer16;
  WGALBufferPool m_TrailPointsDataBuffer32;
  WGALBufferPool m_TrailPointsDataBuffer64;

  mutable const WGALBufferPool* m_pActiveTrailPointsDataBuffer = nullptr;
};
