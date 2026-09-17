#pragma once

#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>

/// Initializer that spawns particles in a sphere volume
///
/// Can spawn throughout the volume or only on the surface.
/// Optionally sets initial velocity pointing outward from the center.
class W_PARTICLEPLUGIN_DLL WParticleInitializerFactory_SpherePosition final : public WParticleInitializerFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleInitializerFactory_SpherePosition, WParticleInitializerFactory);

public:
  WParticleInitializerFactory_SpherePosition();

  virtual const WRTTI* GetInitializerType() const override;
  virtual void CopyInitializerProperties(WParticleInitializer* pInitializer, bool bFirstTime) const override;
  virtual float GetSpawnCountMultiplier(const WParticleEffectInstance* pEffect) const override;

  virtual void Save(WStreamWriter& inout_stream) const override;
  virtual void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual void QueryFinalizerDependencies(WSet<const WRTTI*>& inout_finalizerDeps) const override;

public:
  WVec3 m_vPositionOffset;         ///< Center of the sphere
  float m_fRadius;                  ///< Sphere radius
  bool m_bSpawnOnSurface;           ///< If true, spawn only on sphere surface
  bool m_bSetVelocity;              ///< If true, set velocity pointing outward from center
  WVarianceTypeFloat m_Speed;      ///< Speed value when setting velocity
  WString m_sScaleRadiusParameter; ///< Optional parameter name to scale radius
};


class W_PARTICLEPLUGIN_DLL WParticleInitializer_SpherePosition final : public WParticleInitializer
{
  W_ADD_DYNAMIC_REFLECTION(WParticleInitializer_SpherePosition, WParticleInitializer);

public:
  WVec3 m_vPositionOffset;
  float m_fRadius;
  bool m_bSpawnOnSurface;
  bool m_bSetVelocity;
  WVarianceTypeFloat m_Speed;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements) override;

  WProcessingStream* m_pStreamPosition;
  WProcessingStream* m_pStreamVelocity;
};
