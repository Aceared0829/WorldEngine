#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

/// Add this attribute to a class to add script functions to the szTypeName class.
/// This might be necessary if the specified class is not reflected or to separate script functions from the specified class.
class W_CORE_DLL WScriptExtensionAttribute : public WPropertyAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WScriptExtensionAttribute, WPropertyAttribute);

public:
  WScriptExtensionAttribute();
  WScriptExtensionAttribute(WStringView sTypeName);

  WStringView GetTypeName() const { return m_sTypeName; }

private:
  WUntrackedString m_sTypeName;
};

//////////////////////////////////////////////////////////////////////////

/// Add this attribute to a script function to mark it as a base class function.
/// These are functions that can be entry points to visual scripts or over-writable functions in script languages.
class W_CORE_DLL WScriptBaseClassFunctionAttribute : public WPropertyAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WScriptBaseClassFunctionAttribute, WPropertyAttribute);

public:
  WScriptBaseClassFunctionAttribute();
  WScriptBaseClassFunctionAttribute(WUInt16 uiIndex);

  WUInt16 GetIndex() const { return m_uiIndex; }

private:
  WUInt16 m_uiIndex;
};
