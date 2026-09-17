#pragma once

#include <Core/Physics/SurfaceResourceDescriptor.h>
#include <Foundation/Types/RangeView.h>
#include <Foundation/Types/SharedPtr.h>
#include <ParticlePlugin/Events/ParticleEventReaction.h>

/// Factory for creating effect spawn reactions.
///
/// Configures reactions that spawn a particle effect at the event location.
class W_PARTICLEPLUGIN_DLL WParticleEventReactionFactory_Effect final : public WParticleEventReactionFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleEventReactionFactory_Effect, WParticleEventReactionFactory);

public:
  WParticleEventReactionFactory_Effect();

  virtual const WRTTI* GetEventReactionType() const override;
  virtual void CopyReactionProperties(WParticleEventReaction* pObject, bool bFirstTime) const override;

  virtual void Save(WStreamWriter& inout_stream) const override;
  virtual void Load(WStreamReader& inout_stream) override;

  WString m_sEffect;
  WEnum<WSurfaceInteractionAlignment> m_Alignment;

  WParticleEffectResourceHandle m_hEffect;

  //////////////////////////////////////////////////////////////////////////
  // Exposed Parameters
public:
  const WRangeView<const char*, WUInt32> GetParameters() const;
  void SetParameter(const char* szKey, const WVariant& value);
  void RemoveParameter(const char* szKey);
  bool GetParameter(const char* szKey, WVariant& out_value) const;

private:
  WSharedPtr<WParticleEffectParameters> m_pParameters;
};

/// Event reaction that spawns a particle effect.
///
/// When triggered, spawns the configured effect at the event's position and orientation.
/// The effect can be aligned according to the event's direction and normal vectors.
class W_PARTICLEPLUGIN_DLL WParticleEventReaction_Effect final : public WParticleEventReaction
{
  W_ADD_DYNAMIC_REFLECTION(WParticleEventReaction_Effect, WParticleEventReaction);

public:
  WParticleEventReaction_Effect();
  ~WParticleEventReaction_Effect();

  WParticleEffectResourceHandle m_hEffect;
  WEnum<WSurfaceInteractionAlignment> m_Alignment;
  WSharedPtr<WParticleEffectParameters> m_Parameters;

protected:
  virtual void ProcessEvent(const WParticleEvent& e) override;
};
