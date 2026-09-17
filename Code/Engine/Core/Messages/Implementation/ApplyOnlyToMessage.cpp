#include <Core/CorePCH.h>

#include <Core/Messages/ApplyOnlyToMessage.h>

// clang-format off
W_IMPLEMENT_MESSAGE_TYPE(WMsgOnlyApplyToObject);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgOnlyApplyToObject, 1, WRTTIDefaultAllocator<WMsgOnlyApplyToObject>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Object", m_hObject),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


W_STATICLINK_FILE(Core, Core_Messages_Implementation_ApplyOnlyToMessage);
