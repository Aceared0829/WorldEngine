#pragma once

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorFrameworkDLL.h>

namespace WStackTraceLogParser
{
  W_EDITORFRAMEWORK_DLL bool ParseStackTraceFileNameAndLineNumber(const WStringView& sLine, WStringView& ref_sFileName, WInt32& ref_iLineNumber); // [tested]
  W_EDITORFRAMEWORK_DLL bool ParseAssertFileNameAndLineNumber(const WStringView& sLine, WStringView& ref_sFileName, WInt32& ref_iLineNumber);     // [tested]
  void Register();
  void Unregister();
} // namespace WStackTraceLogParser