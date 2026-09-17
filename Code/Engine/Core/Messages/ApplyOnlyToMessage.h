#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Communication/Message.h>

/// Message used to restrict an operation to apply only to a specific game object.
///
/// This message carries a game object handle to specify which object should be affected
/// by a particular operation, allowing selective application of effects or behaviors.
struct W_CORE_DLL WMsgOnlyApplyToObject : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgOnlyApplyToObject, WMessage);

  WGameObjectHandle m_hObject;
};
