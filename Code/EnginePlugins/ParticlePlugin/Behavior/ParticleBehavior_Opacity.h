#pragma once

#include <Core/Curves/Curve1DResource.h>
#include <Foundation/Tracks/Curve1D.h>
#include <Foundation/Tracks/CurveEditData.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>

/// Behavior that modifies particle alpha over their lifetime using a curve
///
/// The curve is sampled based on the particle's normalized lifetime (0-1).
class W_PARTICLEPLUGIN_DLL WParticleBehaviorFactory_Opacity final : public WParticleBehaviorFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleBehaviorFactory_Opacity, WParticleBehaviorFactory);

public:
  virtual const WRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(WParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void Save(WStreamWriter& inout_stream) const override;
  virtual void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor) override;

  // ************************************* PROPERTIES ***********************************

  WEnum<WCurveSource> m_CurveSource;
  WSingleCurveData m_Curve;
  WCurve1DResourceHandle m_hSharedCurve;
  mutable WCurve1D m_RuntimeCurve;
};


class W_PARTICLEPLUGIN_DLL WParticleBehavior_Opacity final : public WParticleBehavior
{
  W_ADD_DYNAMIC_REFLECTION(WParticleBehavior_Opacity, WParticleBehavior);

public:
  const WCurve1D* m_pCurve = nullptr;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void Process(WUInt64 uiNumElements) override;

  WProcessingStream* m_pStreamLifeTime = nullptr;
  WProcessingStream* m_pStreamColor = nullptr;
  WUInt8 m_uiFirstToUpdate = 0;
  WUInt8 m_uiCurrentUpdateInterval = 2;
};
