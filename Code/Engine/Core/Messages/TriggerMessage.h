#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Communication/Message.h>

struct W_CORE_DLL WTriggerState
{
  using StorageType = WUInt8;

  enum Enum
  {
    Activated,   ///< The trigger was just activated (area entered, key pressed, etc.)
    Continuing,  ///< The trigger is active for more than one frame now.
    Deactivated, ///< The trigger was just deactivated (left area, key released, etc.)

    Default = Activated
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_CORE_DLL, WTriggerState);

/// For internal use by components to trigger some known behavior. Usually components will post this message to themselves with a
/// delay, e.g. to trigger self destruction.
struct W_CORE_DLL WMsgComponentInternalTrigger : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgComponentInternalTrigger, WMessage);

  /// Identifies what the message should trigger.
  WHashedString m_sMessage;

  WInt32 m_iPayload = 0;
};

/// Sent when something enters or leaves a trigger
struct W_CORE_DLL WMsgTriggerTriggered : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgTriggerTriggered, WMessage);

  /// Identifies what the message should trigger.
  WHashedString m_sMessage;

  /// Messages are only sent for 'entered' ('Activated') and 'left' ('Deactivated')
  WEnum<WTriggerState> m_TriggerState;

  /// The object that entered the trigger volume.
  WGameObjectHandle m_hTriggeringObject;
};
