#pragma once

#include <ParticlePlugin/Type/ParticleType.h>
#include <RendererFoundation/RendererFoundationDLL.h>

using WParticleEffectResourceHandle = WTypedResourceHandle<class WParticleEffectResource>;

/// Factory for creating effect particle types.
class W_PARTICLEPLUGIN_DLL WParticleTypeEffectFactory final : public WParticleTypeFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleTypeEffectFactory, WParticleTypeFactory);

public:
  WParticleTypeEffectFactory();
  ~WParticleTypeEffectFactory();

  virtual const WRTTI* GetTypeType() const override;
  virtual void CopyTypeProperties(WParticleType* pObject, bool bFirstTime) const override;

  virtual void Save(WStreamWriter& inout_stream) const override;
  virtual void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor) override;

  WHashedString m_sEffect;
  WHashedString m_sSharedInstanceName; // to be removed

  WParticleEffectResourceHandle m_hEffect;
};

/// Spawns nested particle effects at each particle position.
///
/// Each particle spawns an independent particle effect instance at its position.
/// The spawned effects are automatically cleaned up when the parent particle dies.
class W_PARTICLEPLUGIN_DLL WParticleTypeEffect final : public WParticleType
{
  W_ADD_DYNAMIC_REFLECTION(WParticleTypeEffect, WParticleType);

public:
  WParticleTypeEffect();
  ~WParticleTypeEffect();

  WParticleEffectResourceHandle m_hEffect;
  // WString m_sSharedInstanceName;

  virtual void CreateRequiredStreams() override;
  virtual void ExtractTypeRenderData(WMsgExtractRenderData& ref_msg, const WTransform& instanceTransform) const override;

  /// Returns the maximum effect radius for culling.
  ///
  /// This is an approximation based on the spawned effect's bounding radius.
  virtual float GetMaxParticleRadius(float fParticleSize) const override { return m_fMaxEffectRadius; }

protected:
  friend class WParticleTypeEffectFactory;

  virtual void OnReset() override;
  virtual void Process(WUInt64 uiNumElements) override;
  void OnParticleDeath(const WStreamGroupElementRemovedEvent& e);
  void ClearEffects(bool bInterruptImmediately);

  float m_fMaxEffectRadius = 1.0f;
  WProcessingStream* m_pStreamPosition = nullptr;
  WProcessingStream* m_pStreamEffectID = nullptr;
};
