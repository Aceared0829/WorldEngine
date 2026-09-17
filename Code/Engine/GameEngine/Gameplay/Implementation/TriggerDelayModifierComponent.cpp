#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/TriggerMessage.h>
#include <Core/World/GameObject.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/TriggerDelayModifierComponent.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WTriggerDelayModifierComponent, 1 /* version */, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ActivationDelay", m_ActivationDelay),
    W_MEMBER_PROPERTY("DeactivationDelay", m_DeactivationDelay),
  }
  W_END_PROPERTIES;

  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(WMsgComponentInternalTrigger, OnMsgComponentInternalTrigger),
    W_MESSAGE_HANDLER(WMsgTriggerTriggered, OnMsgTriggerTriggered),
  }
  W_END_MESSAGEHANDLERS;

  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Logic"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WTriggerDelayModifierComponent::WTriggerDelayModifierComponent() = default;
WTriggerDelayModifierComponent::~WTriggerDelayModifierComponent() = default;

void WTriggerDelayModifierComponent::Initialize()
{
  SUPER::Initialize();
}

void WTriggerDelayModifierComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_ActivationDelay;
  s << m_DeactivationDelay;
}

void WTriggerDelayModifierComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_ActivationDelay;
  s >> m_DeactivationDelay;
}

void WTriggerDelayModifierComponent::OnMsgTriggerTriggered(WMsgTriggerTriggered& msg)
{
  if (msg.m_TriggerState == WTriggerState::Activated)
  {
    if (m_iElementsInside++ == 0) // was 0 before the increment
    {
      // the first object entered the trigger

      if (!m_bIsActivated)
      {
        // the trigger is not active yet -> send an activation message with a new activation token

        ++m_iValidActivationToken;

        // store the original trigger message for later
        m_sMessage = msg.m_sMessage;

        WMsgComponentInternalTrigger intMsg;
        intMsg.m_sMessage.Assign("Activate");
        intMsg.m_iPayload = m_iValidActivationToken;

        PostMessage(intMsg, m_ActivationDelay, WObjectMsgQueueType::PostTransform);
      }
      else
      {
        // the trigger is already active -> there are pending deactivations (otherwise we wouldn't have had an element count of zero)
        // -> invalidate those pending deactivations
        ++m_iValidDeactivationToken;

        // no need to send an activation message
      }
    }

    return;
  }

  if (msg.m_TriggerState == WTriggerState::Deactivated)
  {
    if (--m_iElementsInside == 0) // 0 after the decrement
    {
      // the last object left the trigger

      if (m_bIsActivated)
      {
        // if the trigger is active, we need to send a deactivation message and we give it a new token

        ++m_iValidDeactivationToken;

        // store the original trigger message for later
        m_sMessage = msg.m_sMessage;

        WMsgComponentInternalTrigger intMsg;
        intMsg.m_sMessage.Assign("Deactivate");
        intMsg.m_iPayload = m_iValidDeactivationToken;

        PostMessage(intMsg, m_DeactivationDelay, WObjectMsgQueueType::PostTransform);
      }
      else
      {
        // when we are already inactive, all that's needed is to invalidate any pending activations
        ++m_iValidActivationToken;
      }
    }

    return;
  }
}

void WTriggerDelayModifierComponent::OnMsgComponentInternalTrigger(WMsgComponentInternalTrigger& msg)
{
  if (msg.m_sMessage == WTempHashedString("Activate"))
  {
    if (msg.m_iPayload == m_iValidActivationToken && !m_bIsActivated)
    {
      m_bIsActivated = true;

      WMsgTriggerTriggered newMsg;
      newMsg.m_sMessage = m_sMessage;
      newMsg.m_TriggerState = WTriggerState::Activated;

      m_TriggerEventSender.PostEventMessage(newMsg, this, GetOwner()->GetParent(), WTime::MakeZero(), WObjectMsgQueueType::PostTransform);
    }
  }
  else if (msg.m_sMessage == WTempHashedString("Deactivate"))
  {
    if (msg.m_iPayload == m_iValidDeactivationToken && m_bIsActivated)
    {
      m_bIsActivated = false;

      WMsgTriggerTriggered newMsg;
      newMsg.m_sMessage = m_sMessage;
      newMsg.m_TriggerState = WTriggerState::Deactivated;

      m_TriggerEventSender.PostEventMessage(newMsg, this, GetOwner()->GetParent(), WTime::MakeZero(), WObjectMsgQueueType::PostTransform);
    }
  }
}


W_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_TriggerDelayModifierComponent);
