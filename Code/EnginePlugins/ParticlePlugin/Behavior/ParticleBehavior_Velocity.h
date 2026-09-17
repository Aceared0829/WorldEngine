#pragma once

#include <Core/Curves/Curve1DResource.h>
#include <Foundation/Tracks/Curve1D.h>
#include <Foundation/Tracks/CurveEditData.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class WPhysicsWorldModuleInterface;

/// How velocity behavior changes particle speed
struct W_PARTICLEPLUGIN_DLL WVelocityChangeMode
{
  using StorageType = WUInt8;

  enum Enum
  {
    CustomCurve, ///< Use embedded curve data
    SharedCurve, ///< Reference shared curve resource
    Friction,    ///< Apply friction to reduce speed

    Default = Friction
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_PARTICLEPLUGIN_DLL, WVelocityChangeMode);

/// Behavior that applies friction and rise/fall forces to particles
///
/// Friction reduces particle velocity over time.
/// Rise speed makes particles move along the inverse gravity direction.
class W_PARTICLEPLUGIN_DLL WParticleBehaviorFactory_Velocity final : public WParticleBehaviorFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleBehaviorFactory_Velocity, WParticleBehaviorFactory);

public:
  WParticleBehaviorFactory_Velocity();
  ~WParticleBehaviorFactory_Velocity();

  virtual const WRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(WParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void Save(WStreamWriter& inout_stream) const override;
  virtual void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual void QueryFinalizerDependencies(WSet<const WRTTI*>& inout_finalizerDeps) const override;

  float m_fFriction = 0;
  WEnum<WVelocityChangeMode> m_ChangeSpeedWith;
  WSingleCurveData m_SpeedCurve;
  WCurve1DResourceHandle m_hSpeedSharedCurve;
  float m_fSpeedCurveOffset = 0.0f;
  float m_fSpeedCurveScale = 1.0f;
  mutable WCurve1D m_RuntimeSpeedCurve;
};


class W_PARTICLEPLUGIN_DLL WParticleBehavior_Velocity final : public WParticleBehavior
{
  W_ADD_DYNAMIC_REFLECTION(WParticleBehavior_Velocity, WParticleBehavior);

public:
  virtual void CreateRequiredStreams() override;

  WEnum<WVelocityChangeMode> m_ChangeSpeedWith;
  const WCurve1D* m_pCurve = nullptr;
  float m_fSpeedCurveOffset = 0;
  float m_fSpeedCurveScale = 1;
  float m_fFriction = 0;

protected:
  friend class WParticleBehaviorFactory_Velocity;

  virtual void Process(WUInt64 uiNumElements) override;

  void RequestRequiredWorldModulesForCache(WParticleWorldModule* pParticleModule) override;

  WProcessingStream* m_pStreamVelocity = nullptr;
  WProcessingStream* m_pStreamLifeTime = nullptr;
};
