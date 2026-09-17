#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/Messages/EventMessageSender.h>

struct WMsgTriggerTriggered;
struct WMsgComponentInternalTrigger;

using WTriggerDelayModifierComponentManager = WComponentManager<class WTriggerDelayModifierComponent, WBlockStorageType::Compact>;

/// Handles WMsgTriggerTriggered events and sends new messages after a delay.
///
/// The 'enter' and 'leave' messages are sent only when an empty trigger is entered or when the last object leaves the trigger.
/// While any object is already inside the trigger, no change event is sent.
/// Therefore this component can't be used to keep track of all the objects inside the trigger.
///
/// The 'enter' and 'leave' events can be sent with a delay. The 'enter' event is only sent, if the trigger had at least one object
/// inside it for the full duration of the delay. Which exact object may change, but once the trigger contains no object, the timer is reset.
///
/// The sent WMsgTriggerTriggered does not contain a reference to the 'triggering' object, since there may be multiple and they may change randomly.
class W_GAMEENGINE_DLL WTriggerDelayModifierComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WTriggerDelayModifierComponent, WComponent, WTriggerDelayModifierComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WTriggerDelayModifierComponent

public:
  WTriggerDelayModifierComponent();
  ~WTriggerDelayModifierComponent();

protected:
  virtual void Initialize() override;

  void OnMsgTriggerTriggered(WMsgTriggerTriggered& msg);
  void OnMsgComponentInternalTrigger(WMsgComponentInternalTrigger& msg);

  bool m_bIsActivated = false;
  WInt32 m_iElementsInside = 0;
  WInt32 m_iValidActivationToken = 0;
  WInt32 m_iValidDeactivationToken = 0;

  WTime m_ActivationDelay;
  WTime m_DeactivationDelay;
  WHashedString m_sMessage;

  WEventMessageSender<WMsgTriggerTriggered> m_TriggerEventSender; // [ event ]
};
