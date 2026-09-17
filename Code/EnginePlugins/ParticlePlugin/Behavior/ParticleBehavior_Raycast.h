#pragma once

#include <Foundation/Strings/String.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class WPhysicsWorldModuleInterface;

/// How particles react when hitting a physics surface
struct W_PARTICLEPLUGIN_DLL WParticleRaycastHitReaction
{
  using StorageType = WUInt8;

  enum Enum
  {
    Bounce, ///< Particle bounces off the surface
    Die,    ///< Particle is killed on impact
    Stop,   ///< Particle stops moving

    Default = Bounce
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_PARTICLEPLUGIN_DLL, WParticleRaycastHitReaction);

/// Behavior that performs physics raycasts to detect collisions
///
/// Raycasts from the particle's last position to its current position.
/// On collision, particles can bounce, die, or stop.
/// Optionally triggers an event on collision.
class W_PARTICLEPLUGIN_DLL WParticleBehaviorFactory_Raycast final : public WParticleBehaviorFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleBehaviorFactory_Raycast, WParticleBehaviorFactory);

public:
  WParticleBehaviorFactory_Raycast();
  ~WParticleBehaviorFactory_Raycast();

  virtual const WRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(WParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void QueryFinalizerDependencies(WSet<const WRTTI*>& inout_finalizerDeps) const override;

  virtual void Save(WStreamWriter& inout_stream) const override;
  virtual void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor) override;

  WEnum<WParticleRaycastHitReaction> m_Reaction; ///< How particles react to collisions
  WUInt8 m_uiCollisionLayer = 0;                  ///< Physics collision layer to raycast against
  WString m_sOnCollideEvent;                      ///< Optional event name to raise on collision
  float m_fBounceFactor = 0.5f;                    ///< Velocity multiplier when bouncing (energy loss)
  float m_fSlideFactor = 0.5f;                     ///< How much particles slide along the surface
  float m_fSizeFactor = 0.1f;                      ///< Multiplier for particle size used in raycast length
};


class W_PARTICLEPLUGIN_DLL WParticleBehavior_Raycast final : public WParticleBehavior
{
  W_ADD_DYNAMIC_REFLECTION(WParticleBehavior_Raycast, WParticleBehavior);

public:
  WParticleBehavior_Raycast();

  virtual void CreateRequiredStreams() override;
  virtual void QueryOptionalStreams() override;

  WEnum<WParticleRaycastHitReaction> m_Reaction;
  WUInt8 m_uiCollisionLayer = 0;
  WTempHashedString m_sOnCollideEvent;
  float m_fBounceFactor = 0.5f;
  float m_fSlideFactor = 0.5f;
  float m_fSizeFactor = 0.1f;

protected:
  friend class WParticleBehaviorFactory_Raycast;

  virtual void Process(WUInt64 uiNumElements) override;

  void RequestRequiredWorldModulesForCache(WParticleWorldModule* pParticleModule) override;

  WPhysicsWorldModuleInterface* m_pPhysicsModule;

  WProcessingStream* m_pStreamPosition = nullptr;
  WProcessingStream* m_pStreamLastPosition = nullptr;
  WProcessingStream* m_pStreamVelocity = nullptr;
  const WProcessingStream* m_pStreamSize = nullptr;
};
