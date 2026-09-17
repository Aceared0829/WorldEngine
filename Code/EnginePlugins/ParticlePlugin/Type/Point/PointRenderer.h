#pragma once

#include <ParticlePlugin/ParticlePluginDLL.h>
#include <ParticlePlugin/Renderer/ParticleRenderer.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/RenderData.h>

#include <RendererCore/../../../Data/Plugins/ParticlePlugin/Shaders/Particles/BillboardQuadParticleShaderData.h>
#include <RendererFoundation/Resources/BufferPool.h>

/// Render data for point particles.
class W_PARTICLEPLUGIN_DLL WParticlePointRenderData final : public WRenderData
{
  W_ADD_DYNAMIC_REFLECTION(WParticlePointRenderData, WRenderData);

public:
  WArrayPtr<WBaseParticleShaderData> m_BaseParticleData;               ///< Base particle data
  WArrayPtr<WBillboardQuadParticleShaderData> m_BillboardParticleData; ///< Billboard data
  WTransform m_GlobalTransform;                                         ///< World transform of the particle system
  WTime m_TotalEffectLifeTime;                                          ///< Total lifetime of the effect
};

/// Renderer for point particle systems.
class W_PARTICLEPLUGIN_DLL WParticlePointRenderer final : public WParticleRenderer
{
  W_ADD_DYNAMIC_REFLECTION(WParticlePointRenderer, WParticleRenderer);
  W_DISALLOW_COPY_AND_ASSIGN(WParticlePointRenderer);

public:
  WParticlePointRenderer();
  ~WParticlePointRenderer();

  virtual void GetSupportedRenderDataTypes(WDynamicArray<const WRTTI*>& out_types) const override;
  virtual void RenderBatch(
    const WRenderViewContext& renderContext, const WRenderPipelinePass* pPass, const WRenderDataBatch& batch) const override;

protected:
  static const WUInt32 s_uiParticlesPerBatch = 1024;
  WGALBufferPool m_BaseDataBuffer;
  WGALBufferPool m_BillboardDataBuffer;
};
