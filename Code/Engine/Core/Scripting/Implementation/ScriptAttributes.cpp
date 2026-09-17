#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WScriptExtensionAttribute, 1, WRTTIDefaultAllocator<WScriptExtensionAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("TypeName", m_sTypeName),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WScriptExtensionAttribute::WScriptExtensionAttribute() = default;
WScriptExtensionAttribute::WScriptExtensionAttribute(WStringView sTypeName)
  : m_sTypeName(sTypeName)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WScriptBaseClassFunctionAttribute, 1, WRTTIDefaultAllocator<WScriptBaseClassFunctionAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Index", m_uiIndex),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WScriptBaseClassFunctionAttribute::WScriptBaseClassFunctionAttribute() = default;
WScriptBaseClassFunctionAttribute::WScriptBaseClassFunctionAttribute(WUInt16 uiIndex)
  : m_uiIndex(uiIndex)
{
}


W_STATICLINK_FILE(Core, Core_Scripting_Implementation_ScriptAttributes);
