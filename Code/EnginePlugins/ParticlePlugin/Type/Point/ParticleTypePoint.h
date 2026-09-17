#pragma once

#include <ParticlePlugin/Type/ParticleType.h>
#include <ParticlePlugin/Type/Point/PointRenderer.h>
#include <RendererFoundation/RendererFoundationDLL.h>

/// Factory for creating point particle types.
class W_PARTICLEPLUGIN_DLL WParticleTypePointFactory final : public WParticleTypeFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleTypePointFactory, WParticleTypeFactory);

public:
  virtual const WRTTI* GetTypeType() const override;
  virtual void CopyTypeProperties(WParticleType* pObject, bool bFirstTime) const override;

  virtual void Save(WStreamWriter& inout_stream) const override;
  virtual void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor) override;
};

/// Renders particles as single-pixel points.
class W_PARTICLEPLUGIN_DLL WParticleTypePoint final : public WParticleType
{
  W_ADD_DYNAMIC_REFLECTION(WParticleTypePoint, WParticleType);

public:
  WParticleTypePoint() = default;

  virtual void CreateRequiredStreams() override;

  virtual void ExtractTypeRenderData(WMsgExtractRenderData& ref_msg, const WTransform& instanceTransform) const override;

  /// Point particles have no radius for culling purposes.
  virtual float GetMaxParticleRadius(float fParticleSize) const override { return 0.0f; }

protected:
  virtual void Process(WUInt64 uiNumElements) override {}

  WProcessingStream* m_pStreamPosition;
  WProcessingStream* m_pStreamColor;

  mutable WArrayPtr<WBaseParticleShaderData> m_BaseParticleData;
  mutable WArrayPtr<WBillboardQuadParticleShaderData> m_BillboardParticleData;
};
