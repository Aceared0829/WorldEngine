#pragma once

#include <Core/Curves/Curve1DResource.h>
#include <Foundation/Tracks/Curve1D.h>
#include <Foundation/Tracks/CurveEditData.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>

/// Behavior that modifies particle size over their lifetime using a curve
///
/// The curve is sampled based on the particle's normalized lifetime (0-1).
/// The final size is: base size + (curve value * curve scale).
class W_PARTICLEPLUGIN_DLL WParticleBehaviorFactory_SizeCurve final : public WParticleBehaviorFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleBehaviorFactory_SizeCurve, WParticleBehaviorFactory);

public:
  virtual const WRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(WParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void Save(WStreamWriter& inout_stream) const override;
  virtual void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor) override;

  WEnum<WCurveSource> m_CurveSource;
  WSingleCurveData m_Curve;
  WCurve1DResourceHandle m_hSharedCurve;
  float m_fSizeCurveOffset = 0;
  float m_fSizeCurveScale = 1;
  mutable WCurve1D m_RuntimeCurve;
};

class W_PARTICLEPLUGIN_DLL WParticleBehavior_SizeCurve final : public WParticleBehavior
{
  W_ADD_DYNAMIC_REFLECTION(WParticleBehavior_SizeCurve, WParticleBehavior);

public:
  const WCurve1D* m_pCurve = nullptr;
  float m_fSizeCurveOffset = 0;
  float m_fSizeCurveScale = 1;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements) override;
  virtual void Process(WUInt64 uiNumElements) override;

  WProcessingStream* m_pStreamLifeTime = nullptr;
  WProcessingStream* m_pStreamSize = nullptr;
  WUInt8 m_uiFirstToUpdate = 0;
  WUInt8 m_uiCurrentUpdateInterval = 8;
};
