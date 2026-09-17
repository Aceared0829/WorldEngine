#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/Declarations.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WTransformResult, 1)
  W_ENUM_CONSTANT(WTransformResult::Success),
  W_ENUM_CONSTANT(WTransformResult::Failure),
  W_ENUM_CONSTANT(WTransformResult::NeedsImport),
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_TYPE(WTransformStatus, WNoBase, 1, WRTTIDefaultAllocator<WTransformStatus>)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("Result", WTransformResult, m_Result),
    W_MEMBER_PROPERTY("Message", m_sMessage),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on
