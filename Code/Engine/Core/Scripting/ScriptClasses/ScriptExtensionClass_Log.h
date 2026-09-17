#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

/// Script extension class providing logging functionality from scripts.
///
/// Allows scripts to output formatted log messages at different severity levels.
/// Messages are sent to the standard WorldEngine logging system and will appear
/// in the console, log files, and other registered log writers.
class W_CORE_DLL WScriptExtensionClass_Log
{
public:
  static void Info(WStringView sText, const WVariantArray& params);
  static void Warning(WStringView sText, const WVariantArray& params);
  static void Error(WStringView sText, const WVariantArray& params);
};

W_DECLARE_REFLECTABLE_TYPE(W_CORE_DLL, WScriptExtensionClass_Log);
