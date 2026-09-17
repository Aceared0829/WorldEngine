#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/VisualGraph/VisualGraphCommentNode.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WVisualGraphComment, 1, WRTTIDefaultAllocator<WVisualGraphComment>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Comment", m_sComment)->AddAttributes(new WDefaultValueAttribute("Comment")),
    W_MEMBER_PROPERTY("Size", m_vSize),
    W_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new WDefaultValueAttribute(WColorScheme::DarkUI(WColorScheme::Gray))),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Misc"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
