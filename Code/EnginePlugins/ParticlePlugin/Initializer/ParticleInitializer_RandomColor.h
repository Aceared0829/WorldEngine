#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Tracks/ColorGradient.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>

using WColorGradientResourceHandle = WTypedResourceHandle<class WColorGradientResource>;

/// Initializer that sets random particle colors
///
/// Colors are picked randomly between Color1 and Color2, or sampled from a gradient.
class W_PARTICLEPLUGIN_DLL WParticleInitializerFactory_RandomColor final : public WParticleInitializerFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleInitializerFactory_RandomColor, WParticleInitializerFactory);

public:
  virtual const WRTTI* GetInitializerType() const override;
  virtual void CopyInitializerProperties(WParticleInitializer* pInitializer, bool bFirstTime) const override;

  virtual void Save(WStreamWriter& inout_stream) const override;
  virtual void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor) override;

  WColor m_Color1;
  WColor m_Color2;
  WEnum<WGradientSource> m_GradientSource;
  WColorGradient m_Gradient;
  WColorGradientResourceHandle m_hSharedGradient;
};


class W_PARTICLEPLUGIN_DLL WParticleInitializer_RandomColor final : public WParticleInitializer
{
  W_ADD_DYNAMIC_REFLECTION(WParticleInitializer_RandomColor, WParticleInitializer);

public:
  WColor m_Color1;
  WColor m_Color2;
  const WColorGradient* m_pGradient = nullptr;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements) override;

  WProcessingStream* m_pStreamColor = nullptr;
};
