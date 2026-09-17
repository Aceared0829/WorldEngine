#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

/// Behavior that fades particle alpha over their lifetime
///
/// Uses a power curve to control the fade speed.
class W_PARTICLEPLUGIN_DLL WParticleBehaviorFactory_FadeOut final : public WParticleBehaviorFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleBehaviorFactory_FadeOut, WParticleBehaviorFactory);

public:
  virtual const WRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(WParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void Save(WStreamWriter& inout_stream) const override;
  virtual void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor) override;

  // ************************************* PROPERTIES ***********************************

  float m_fStartAlpha = 1.0f;
  float m_fExponent = 1.0f;
};


class W_PARTICLEPLUGIN_DLL WParticleBehavior_FadeOut final : public WParticleBehavior
{
  W_ADD_DYNAMIC_REFLECTION(WParticleBehavior_FadeOut, WParticleBehavior);

public:
  float m_fStartAlpha = 1.0f;
  float m_fExponent = 1.0f;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void Process(WUInt64 uiNumElements) override;

  WProcessingStream* m_pStreamLifeTime = nullptr;
  WProcessingStream* m_pStreamColor = nullptr;
  WUInt8 m_uiFirstToUpdate = 0;
  WUInt8 m_uiCurrentUpdateInterval = 2;
};
