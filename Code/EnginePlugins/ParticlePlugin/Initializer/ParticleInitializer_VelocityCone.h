#pragma once

#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>

/// Initializer that sets particle velocity within a cone
///
/// Velocities point along the local Z-axis with random deviation within the cone angle.
class W_PARTICLEPLUGIN_DLL WParticleInitializerFactory_VelocityCone final : public WParticleInitializerFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleInitializerFactory_VelocityCone, WParticleInitializerFactory);

public:
  WParticleInitializerFactory_VelocityCone();

  virtual const WRTTI* GetInitializerType() const override;
  virtual void CopyInitializerProperties(WParticleInitializer* pInitializer, bool bFirstTime) const override;

  virtual void Save(WStreamWriter& inout_stream) const override;
  virtual void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual void QueryFinalizerDependencies(WSet<const WRTTI*>& inout_finalizerDeps) const override;

public:
  WAngle m_Angle;
  WVarianceTypeFloat m_Speed;
  WString m_sSpeedScaleParameter;
};


class W_PARTICLEPLUGIN_DLL WParticleInitializer_VelocityCone final : public WParticleInitializer
{
  W_ADD_DYNAMIC_REFLECTION(WParticleInitializer_VelocityCone, WParticleInitializer);

public:
  WAngle m_Angle;
  WVarianceTypeFloat m_Speed;
  WTempHashedString m_sSpeedScaleParameter;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements) override;

  WProcessingStream* m_pStreamVelocity;
};
