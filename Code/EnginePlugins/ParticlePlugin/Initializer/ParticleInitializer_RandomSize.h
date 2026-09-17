#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>

using WCurve1DResourceHandle = WTypedResourceHandle<class WCurve1DResource>;

/// Initializer that sets random particle sizes
///
/// Sizes can be picked from a variance value or sampled from a curve.
class W_PARTICLEPLUGIN_DLL WParticleInitializerFactory_RandomSize final : public WParticleInitializerFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleInitializerFactory_RandomSize, WParticleInitializerFactory);

public:
  virtual const WRTTI* GetInitializerType() const override;
  virtual void CopyInitializerProperties(WParticleInitializer* pInitializer, bool bFirstTime) const override;

  virtual void Save(WStreamWriter& inout_stream) const override;
  virtual void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor) override;

  WVarianceTypeFloat m_Size;
  WCurve1DResourceHandle m_hCurve;
  WHashedString m_sSizeScaleParameter;
};


class W_PARTICLEPLUGIN_DLL WParticleInitializer_RandomSize final : public WParticleInitializer
{
  W_ADD_DYNAMIC_REFLECTION(WParticleInitializer_RandomSize, WParticleInitializer);

public:
  WVarianceTypeFloat m_Size;
  WCurve1DResourceHandle m_hCurve;
  WTempHashedString m_sSizeScaleParameter;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements) override;

  WProcessingStream* m_pStreamSize = nullptr;
};
