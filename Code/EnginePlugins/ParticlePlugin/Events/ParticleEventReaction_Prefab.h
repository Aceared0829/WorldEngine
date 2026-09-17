#pragma once

#include <Core/Physics/SurfaceResourceDescriptor.h>
#include <Foundation/Types/RangeView.h>
#include <Foundation/Types/SharedPtr.h>
#include <ParticlePlugin/Events/ParticleEventReaction.h>

using WPrefabResourceHandle = WTypedResourceHandle<class WPrefabResource>;

/// Factory for creating prefab spawn reactions.
///
/// Configures reactions that instantiate a prefab at the event location.
class W_PARTICLEPLUGIN_DLL WParticleEventReactionFactory_Prefab final : public WParticleEventReactionFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleEventReactionFactory_Prefab, WParticleEventReactionFactory);

public:
  WParticleEventReactionFactory_Prefab();

  virtual const WRTTI* GetEventReactionType() const override;
  virtual void CopyReactionProperties(WParticleEventReaction* pObject, bool bFirstTime) const override;

  virtual void Save(WStreamWriter& inout_stream) const override;
  virtual void Load(WStreamReader& inout_stream) override;

  WString m_sPrefab;
  WEnum<WSurfaceInteractionAlignment> m_Alignment;

  WPrefabResourceHandle m_hPrefab;

  //////////////////////////////////////////////////////////////////////////
  // Exposed Parameters
public:
  // const WRangeView<const char*, WUInt32> GetParameters() const;
  // void SetParameter(const char* szKey, const WVariant& value);
  // void RemoveParameter(const char* szKey);
  // bool GetParameter(const char* szKey, WVariant& out_value) const;

private:
  // WSharedPtr<WParticlePrefabParameters> m_Parameters;
};

/// Event reaction that instantiates a prefab.
///
/// When triggered, spawns the configured prefab at the event's position and orientation.
/// The prefab can be aligned according to the event's direction and normal vectors.
class W_PARTICLEPLUGIN_DLL WParticleEventReaction_Prefab final : public WParticleEventReaction
{
  W_ADD_DYNAMIC_REFLECTION(WParticleEventReaction_Prefab, WParticleEventReaction);

public:
  WParticleEventReaction_Prefab();
  ~WParticleEventReaction_Prefab();

  WPrefabResourceHandle m_hPrefab;
  WEnum<WSurfaceInteractionAlignment> m_Alignment;

  // WSharedPtr<WParticlePrefabParameters> m_Parameters;

protected:
  virtual void ProcessEvent(const WParticleEvent& e) override;
};
