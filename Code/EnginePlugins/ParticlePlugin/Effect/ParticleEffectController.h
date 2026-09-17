#pragma once

#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class W_PARTICLEPLUGIN_DLL WParticleEffectController
{
public:
  WParticleEffectController();
  WParticleEffectController(const WParticleEffectController& rhs);
  void operator=(const WParticleEffectController& rhs);

  void Create(const WParticleEffectResourceHandle& hEffectResource, WParticleWorldModule* pModule, WUInt64 uiRandomSeed,
    const char* szSharedName /*= nullptr*/, const void* pSharedInstanceOwner /*= nullptr*/, WArrayPtr<WParticleEffectFloatParam> floatParams,
    WArrayPtr<WParticleEffectColorParam> colorParams);

  bool IsValid() const;
  void Invalidate();

  bool IsAlive() const;
  bool IsSharedInstance() const { return m_pSharedInstanceOwner != nullptr; }

  bool IsContinuousEffect() const { return GetInstance()->IsContinuous(); }

  void SetTransform(const WTransform& t, const WVec3& vParticleStartVelocity) const;

  void CombineSystemBoundingVolumes();

  void Tick(const WTime& diff) const;

  void ExtractRenderData(WMsgExtractRenderData& ref_msg, const WTransform& systemTransform) const;

  void StopImmediate();

  /// Returns the bounding volume of the effect.
  /// The volume is in the local space of the effect.
  void GetBoundingVolume(WBoundingBoxSphere& ref_volume) const;

  void UpdateWindSamples(WTime diff);
  void FindNearbyAttractors(WTime diff);

  /// Ensures that the effect is considered to be 'visible', which affects the update rate.
  void ForceVisible();

  WUInt64 GetNumActiveParticles() const;

  /// \name Effect Parameters
  ///@{
public:
  /// Passes an effect parameter on to the effect instance
  void SetParameter(const WTempHashedString& sName, float value);

  /// Passes an effect parameter on to the effect instance
  void SetParameter(const WTempHashedString& sName, const WColor& value);

  ///@}

private:
  friend class WParticleWorldModule;

  WParticleEffectController(WParticleWorldModule* pModule, WParticleEffectHandle hEffect);
  WParticleEffectInstance* GetInstance() const;

  const void* m_pSharedInstanceOwner = nullptr;
  WParticleWorldModule* m_pModule = nullptr;
  WParticleEffectHandle m_hEffect;
};
