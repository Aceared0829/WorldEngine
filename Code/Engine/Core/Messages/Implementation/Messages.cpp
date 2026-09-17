#include <Core/CorePCH.h>

#include <Core/Messages/CommonMessages.h>
#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/HierarchyChangedMessages.h>
#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>

// clang-format off

W_BEGIN_STATIC_REFLECTED_ENUM(WTriggerState, 1)
  W_ENUM_CONSTANTS(WTriggerState::Activated, WTriggerState::Continuing, WTriggerState::Deactivated)
W_END_STATIC_REFLECTED_ENUM;

//////////////////////////////////////////////////////////////////////////

W_IMPLEMENT_MESSAGE_TYPE(WMsgDeleteGameObject);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgDeleteGameObject, 1, WRTTIDefaultAllocator<WMsgDeleteGameObject>)
W_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

W_IMPLEMENT_MESSAGE_TYPE(WMsgComponentInternalTrigger);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgComponentInternalTrigger, 1, WRTTIDefaultAllocator<WMsgComponentInternalTrigger>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Message", m_sMessage),
    W_MEMBER_PROPERTY("Payload", m_iPayload),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

W_IMPLEMENT_MESSAGE_TYPE(WMsgUpdateLocalBounds);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgUpdateLocalBounds, 1, WRTTIDefaultAllocator<WMsgUpdateLocalBounds>)
{
  W_BEGIN_ATTRIBUTES
  {
    new WExcludeFromScript()
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

W_IMPLEMENT_MESSAGE_TYPE(WMsgSetPlaying);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgSetPlaying, 1, WRTTIDefaultAllocator<WMsgSetPlaying>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Play", m_bPlay)->AddAttributes(new WDefaultValueAttribute(true)),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

W_IMPLEMENT_MESSAGE_TYPE(WMsgInterruptPlaying);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgInterruptPlaying, 1, WRTTIDefaultAllocator<WMsgInterruptPlaying>)
W_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

W_IMPLEMENT_MESSAGE_TYPE(WMsgParentChanged);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgParentChanged, 1, WRTTIDefaultAllocator<WMsgParentChanged>)
{
  W_BEGIN_ATTRIBUTES
  {
    new WExcludeFromScript()
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

W_IMPLEMENT_MESSAGE_TYPE(WMsgChildrenChanged);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgChildrenChanged, 1, WRTTIDefaultAllocator<WMsgChildrenChanged>)
{
  W_BEGIN_ATTRIBUTES
  {
    new WExcludeFromScript()
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

W_IMPLEMENT_MESSAGE_TYPE(WMsgComponentsChanged);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgComponentsChanged, 1, WRTTIDefaultAllocator<WMsgComponentsChanged>)
{
  W_BEGIN_ATTRIBUTES
  {
    new WExcludeFromScript()
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

W_IMPLEMENT_MESSAGE_TYPE(WMsgTransformChanged);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgTransformChanged, 1, WRTTIDefaultAllocator<WMsgTransformChanged>)
{
  W_BEGIN_ATTRIBUTES
  {
    new WExcludeFromScript()
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

W_IMPLEMENT_MESSAGE_TYPE(WMsgSetFloatParameter);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgSetFloatParameter, 1, WRTTIDefaultAllocator<WMsgSetFloatParameter>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Name", m_sParameterName),
    W_MEMBER_PROPERTY("Value", m_fValue),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

W_IMPLEMENT_MESSAGE_TYPE(WMsgSetColorParameter);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgSetColorParameter, 1, WRTTIDefaultAllocator<WMsgSetColorParameter>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Name", m_sParameterName),
    W_MEMBER_PROPERTY("Value", m_Value),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

W_IMPLEMENT_MESSAGE_TYPE(WMsgGenericEvent);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgGenericEvent, 1, WRTTIDefaultAllocator<WMsgGenericEvent>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Message", m_sMessage),
    W_MEMBER_PROPERTY("Value", m_Value)->AddAttributes(new WDefaultValueAttribute(0))
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

W_IMPLEMENT_MESSAGE_TYPE(WMsgAnimationReachedEnd);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgAnimationReachedEnd, 1, WRTTIDefaultAllocator<WMsgAnimationReachedEnd>)
W_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

W_IMPLEMENT_MESSAGE_TYPE(WMsgTriggerTriggered)
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgTriggerTriggered, 1, WRTTIDefaultAllocator<WMsgTriggerTriggered>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Message", m_sMessage),
    W_ENUM_MEMBER_PROPERTY("TriggerState", WTriggerState, m_TriggerState),
    W_MEMBER_PROPERTY("GameObject", m_hTriggeringObject),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

// clang-format on

W_STATICLINK_FILE(Core, Core_Messages_Implementation_Messages);
