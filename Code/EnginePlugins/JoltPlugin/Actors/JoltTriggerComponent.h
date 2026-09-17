#pragma once

#include <Core/Messages/EventMessageSender.h>
#include <Core/Messages/TriggerMessage.h>
#include <JoltPlugin/Actors/JoltActorComponent.h>
#include <JoltPlugin/Utilities/JoltUserData.h>

//////////////////////////////////////////////////////////////////////////

class W_JOLTPLUGIN_DLL WJoltTriggerComponentManager : public WComponentManager<class WJoltTriggerComponent, WBlockStorageType::FreeList>
{
public:
  WJoltTriggerComponentManager(WWorld* pWorld);
  ~WJoltTriggerComponentManager();

private:
  friend class WJoltWorldModule;
  friend class WJoltTriggerComponent;

  void UpdateMovingTriggers();

  WSet<WJoltTriggerComponent*> m_MovingTriggers;
};

//////////////////////////////////////////////////////////////////////////

/// Turns an object into a trigger that is capable of detecting when other physics objects enter its volume.
///
/// Triggers are physics actors and thus are set up the same way, e.g. they use physics shapes for their geometry,
/// but they act very differently. Triggers do not affect other objects, instead all objects just pass through them.
/// However, the trigger detects overlap with other objects and sends a message when a new object enters its volume
/// or when one leaves it.
///
/// \note The physics trigger only sends enter and leave messages. It does not send any message when an object stays inside
/// the trigger.
///
/// The message WMsgTriggerTriggered is sent for every change. It references the object that entered or left the volume
/// and it also contains a trigger-specific message string to identify what this should be used for.
class W_JOLTPLUGIN_DLL WJoltTriggerComponent : public WJoltActorComponent
{
  W_DECLARE_COMPONENT_TYPE(WJoltTriggerComponent, WJoltActorComponent, WJoltTriggerComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // WJoltTriggerComponent
public:
  WJoltTriggerComponent();
  ~WJoltTriggerComponent();

  /// Sets the text that the WMsgTriggerTriggered should contain when the trigger fires.
  void SetTriggerMessage(const char* szSz) { m_sTriggerMessage.Assign(szSz); }  // [ property ]
  const char* GetTriggerMessage() const { return m_sTriggerMessage.GetData(); } // [ property ]

protected:
  friend class WJoltWorldModule;
  friend class WJoltContactListener;

  void PostTriggerMessage(const WGameObjectHandle& hOtherObject, WTriggerState::Enum triggerState) const;

  WHashedString m_sTriggerMessage;
  WEventMessageSender<WMsgTriggerTriggered> m_TriggerEventSender; // [ event ]
};
