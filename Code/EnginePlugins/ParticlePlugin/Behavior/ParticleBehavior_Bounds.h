#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class WPhysicsWorldModuleInterface;

/// Behavior that restricts particles to a box volume
///
/// Particles can be killed, teleported or bounced when leaving the box bounds.
class W_PARTICLEPLUGIN_DLL WParticleBehaviorFactory_Bounds final : public WParticleBehaviorFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleBehaviorFactory_Bounds, WParticleBehaviorFactory);

public:
  WParticleBehaviorFactory_Bounds();

  virtual const WRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(WParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void Save(WStreamWriter& inout_stream) const override;
  virtual void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor) override;

  WVec3 m_vPositionOffset;
  WVec3 m_vBoxExtents;
  WEnum<WParticleOutOfBoundsMode> m_OutOfBoundsMode;
};


class W_PARTICLEPLUGIN_DLL WParticleBehavior_Bounds final : public WParticleBehavior
{
  W_ADD_DYNAMIC_REFLECTION(WParticleBehavior_Bounds, WParticleBehavior);

public:
  WVec3 m_vPositionOffset;
  WVec3 m_vBoxExtents;
  WEnum<WParticleOutOfBoundsMode> m_OutOfBoundsMode;

protected:
  virtual void Process(WUInt64 uiNumElements) override;

  virtual void CreateRequiredStreams() override;
  virtual void QueryOptionalStreams() override;

  WProcessingStream* m_pStreamPosition = nullptr;
  WProcessingStream* m_pStreamLastPosition = nullptr;
};
