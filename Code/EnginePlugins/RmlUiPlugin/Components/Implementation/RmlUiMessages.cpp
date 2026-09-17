#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <RmlUiPlugin/Components/RmlUiMessages.h>

// clang-format off
W_IMPLEMENT_MESSAGE_TYPE(WMsgRmlUiReload);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgRmlUiReload, 1, WRTTIDefaultAllocator<WMsgRmlUiReload>)
{
  W_BEGIN_ATTRIBUTES
  {
    new WExcludeFromScript(),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_IMPLEMENT_MESSAGE_TYPE(WMsgRmlUiEvent);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgRmlUiEvent, 1, WRTTIDefaultAllocator<WMsgRmlUiEvent>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Identifier", m_sIdentifier),
    W_MEMBER_PROPERTY("Type", m_sType),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

W_STATICLINK_FILE(RmlUiPlugin, RmlUiPlugin_Components_Implementation_RmlUiMessages);
