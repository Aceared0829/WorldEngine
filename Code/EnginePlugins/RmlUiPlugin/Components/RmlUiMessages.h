#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <Foundation/Communication/Message.h>

struct W_RMLUIPLUGIN_DLL WMsgRmlUiReload : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgRmlUiReload, WMessage);
};

//////////////////////////////////////////////////////////////////////////

struct W_RMLUIPLUGIN_DLL WMsgRmlUiEvent : WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgRmlUiEvent, WMessage);

  WHashedString m_sIdentifier;
  WHashedString m_sType;
};
