#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

/// What happens when a particle is outside the sphere
struct W_PARTICLEPLUGIN_DLL WParticleSphereOutOfBoundsMode
{
  using StorageType = WUInt8;

  enum Enum
  {
    Kill,      ///< Remove particle immediately
    Constrain, ///< Clamp particle to sphere surface

    Default = Kill
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_PARTICLEPLUGIN_DLL, WParticleSphereOutOfBoundsMode);

/// Constrains particles to a spherical volume.
///
/// Particles outside the radius are either killed or pushed back to the sphere surface.
/// The center is relative to the effect's world position.
class W_PARTICLEPLUGIN_DLL WParticleBehaviorFactory_BoundsSphere final : public WParticleBehaviorFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleBehaviorFactory_BoundsSphere, WParticleBehaviorFactory);

public:
  WParticleBehaviorFactory_BoundsSphere();

  virtual const WRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(WParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void Save(WStreamWriter& inout_stream) const override;
  virtual void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor) override;

  WVec3 m_vCenterOffset = WVec3::MakeZero();
  float m_fRadius = 3.0f;
  WEnum<WParticleSphereOutOfBoundsMode> m_OutOfBoundsMode;
};

class W_PARTICLEPLUGIN_DLL WParticleBehavior_BoundsSphere final : public WParticleBehavior
{
  W_ADD_DYNAMIC_REFLECTION(WParticleBehavior_BoundsSphere, WParticleBehavior);

public:
  WVec3 m_vCenterOffset = WVec3::MakeZero();
  float m_fRadius = 3.0f;
  WEnum<WParticleSphereOutOfBoundsMode> m_OutOfBoundsMode;

protected:
  virtual void CreateRequiredStreams() override;
  virtual void Process(WUInt64 uiNumElements) override;

  WProcessingStream* m_pStreamPosition = nullptr;
};
