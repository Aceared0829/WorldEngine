#pragma once

#include <ParticlePlugin/Type/ParticleType.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class WView;
class WExtractedRenderData;

/// Factory for creating light particle types.
class W_PARTICLEPLUGIN_DLL WParticleTypeLightFactory final : public WParticleTypeFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleTypeLightFactory, WParticleTypeFactory);

public:
  WParticleTypeLightFactory();

  virtual const WRTTI* GetTypeType() const override;
  virtual void CopyTypeProperties(WParticleType* pObject, bool bFirstTime) const override;

  virtual void Save(WStreamWriter& inout_stream) const override;
  virtual void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor) override;

  float m_fSizeFactor;
  float m_fIntensity;
  WUInt32 m_uiPercentage;
  WHashedString m_sTintColorParameter;
  WHashedString m_sIntensityParameter;
  WHashedString m_sSizeScaleParameter;
};

/// Renders particles as dynamic point lights.
///
/// Each particle creates a point light with range based on particle size.
/// Not all particles need to emit light - the percentage can be controlled
/// to reduce performance cost. Light properties can be modulated by effect parameters.
class W_PARTICLEPLUGIN_DLL WParticleTypeLight final : public WParticleType
{
  W_ADD_DYNAMIC_REFLECTION(WParticleTypeLight, WParticleType);

public:
  virtual void CreateRequiredStreams() override;

  float m_fSizeFactor;
  float m_fIntensity;
  WUInt32 m_uiPercentage;
  WTempHashedString m_sTintColorParameter;
  WTempHashedString m_sIntensityParameter;
  WTempHashedString m_sSizeScaleParameter;

  virtual float GetMaxParticleRadius(float fParticleSize) const override { return 0.5f * fParticleSize * m_fSizeFactor; }

  virtual void ExtractTypeRenderData(WMsgExtractRenderData& ref_msg, const WTransform& instanceTransform) const override;

protected:
  virtual void Process(WUInt64 uiNumElements) override {}

  WProcessingStream* m_pStreamPosition;
  WProcessingStream* m_pStreamSize;
  WProcessingStream* m_pStreamColor;
  WProcessingStream* m_pStreamOnOff;
};
