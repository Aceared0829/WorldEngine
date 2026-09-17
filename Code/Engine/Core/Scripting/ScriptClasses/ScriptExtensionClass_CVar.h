#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

/// Script extension class providing access to console variables (CVars) from scripts.
///
/// Allows scripts to read and modify CVars for configuration and debugging purposes.
/// Provides type-safe accessors for common CVar types as well as generic variant access.
class W_CORE_DLL WScriptExtensionClass_CVar
{
public:
  static WVariant GetValue(WStringView sName);
  static bool GetBoolValue(WStringView sName);
  static int GetIntValue(WStringView sName);
  static float GetFloatValue(WStringView sName);
  static WString GetStringValue(WStringView sName);

  static void SetValue(WStringView sName, const WVariant& value);
  static void SetBoolValue(WStringView sName, bool bValue);
  static void SetIntValue(WStringView sName, int iValue);
  static void SetFloatValue(WStringView sName, float fValue);
  static void SetStringValue(WStringView sName, const WString& sValue);
};

W_DECLARE_REFLECTABLE_TYPE(W_CORE_DLL, WScriptExtensionClass_CVar);
