#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>

using WCurve1DResourceHandle = WTypedResourceHandle<class WCurve1DResource>;

/// Initializer that sets random particle rotation speeds
///
/// Optionally also sets a random starting rotation angle.
class W_PARTICLEPLUGIN_DLL WParticleInitializerFactory_RandomRotationSpeed final : public WParticleInitializerFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleInitializerFactory_RandomRotationSpeed, WParticleInitializerFactory);

public:
  virtual const WRTTI* GetInitializerType() const override;
  virtual void CopyInitializerProperties(WParticleInitializer* pInitializer, bool bFirstTime) const override;

  virtual void Save(WStreamWriter& inout_stream) const override;
  virtual void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor) override;

  bool m_bRandomStartAngle = false;
  WVarianceTypeAngle m_RotationSpeed;
};


class W_PARTICLEPLUGIN_DLL WParticleInitializer_RandomRotationSpeed final : public WParticleInitializer
{
  W_ADD_DYNAMIC_REFLECTION(WParticleInitializer_RandomRotationSpeed, WParticleInitializer);

public:
  bool m_bRandomStartAngle = false;
  WVarianceTypeAngle m_RotationSpeed;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements) override;

  bool m_bPositiveSign = false;
  WProcessingStream* m_pStreamRotationSpeed = nullptr;
  WProcessingStream* m_pStreamRotationOffset = nullptr;
};
