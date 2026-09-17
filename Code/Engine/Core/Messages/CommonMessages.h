#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Communication/Message.h>
#include <Foundation/Math/Color.h>

/// Common message for components that can be toggled between playing and paused states
struct W_CORE_DLL WMsgSetPlaying : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgSetPlaying, WMessage);

  bool m_bPlay = true;
};

/// Common message for components that can or need to be canceled immediately
struct W_CORE_DLL WMsgInterruptPlaying : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgInterruptPlaying, WMessage);
};

/// Basic message to set some generic parameter to a float value.
struct W_CORE_DLL WMsgSetFloatParameter : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgSetFloatParameter, WMessage);

  WString m_sParameterName;
  float m_fValue = 0;
};

/// Basic message to set some generic parameter to a color value.
struct W_CORE_DLL WMsgSetColorParameter : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgSetColorParameter, WMessage);

  WString m_sParameterName;
  WColor m_Value = WColor::White;
};

/// For use in scripts to signal a custom event that some game event has occurred.
///
/// This is a simple message for simple use cases. Create custom messages for more elaborate cases where a string is not sufficient
/// information.
struct W_CORE_DLL WMsgGenericEvent : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgGenericEvent, WMessage);

  /// A custom string to identify the intent.
  WHashedString m_sMessage;
  WVariant m_Value;
};

/// Sent when an animation reached its end (either forwards or backwards playing)
///
/// This is sent regardless of whether the animation is played once, looped or back and forth,
/// ie. it should be sent at each 'end' point, even when it then starts another cycle.
struct W_CORE_DLL WMsgAnimationReachedEnd : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgAnimationReachedEnd, WMessage);
};
