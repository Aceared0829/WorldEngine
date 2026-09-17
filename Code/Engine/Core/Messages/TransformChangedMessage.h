#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Communication/Message.h>

/// Message sent when a game object's global transform changes.
///
/// Contains both the old and new global transforms, allowing components to respond
/// to position, rotation, or scale changes and calculate movement deltas if needed.
struct W_CORE_DLL WMsgTransformChanged : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgTransformChanged, WMessage);

  WTransform m_OldGlobalTransform;
  WTransform m_NewGlobalTransform;
};
