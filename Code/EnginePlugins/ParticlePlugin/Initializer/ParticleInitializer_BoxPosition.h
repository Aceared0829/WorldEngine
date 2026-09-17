#pragma once

#include <ParticlePlugin/Initializer/ParticleInitializer.h>

/// Initializer that spawns particles in a box volume
class W_PARTICLEPLUGIN_DLL WParticleInitializerFactory_BoxPosition final : public WParticleInitializerFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleInitializerFactory_BoxPosition, WParticleInitializerFactory);

public:
  WParticleInitializerFactory_BoxPosition();

  virtual const WRTTI* GetInitializerType() const override;
  virtual void CopyInitializerProperties(WParticleInitializer* pInitializer, bool bFirstTime) const override;
  virtual float GetSpawnCountMultiplier(const WParticleEffectInstance* pEffect) const override;

  virtual void Save(WStreamWriter& inout_stream) const override;
  virtual void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor) override;

public:
  WVec3 m_vPositionOffset;    ///< Center of the spawn box
  WVec3 m_vSize;              ///< Size of the spawn box (full size, not half extents)
  WString m_sScaleXParameter; ///< Optional parameter name to scale box width
  WString m_sScaleYParameter; ///< Optional parameter name to scale box depth
  WString m_sScaleZParameter; ///< Optional parameter name to scale box height
};


class W_PARTICLEPLUGIN_DLL WParticleInitializer_BoxPosition final : public WParticleInitializer
{
  W_ADD_DYNAMIC_REFLECTION(WParticleInitializer_BoxPosition, WParticleInitializer);

public:
  WVec3 m_vPositionOffset;
  WVec3 m_vSize;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements) override;

  WProcessingStream* m_pStreamPosition;
};
