#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Strings/String.h>

class W_EDITORFRAMEWORK_DLL WCppSettings
{
public:
  WResult Save(WStringView sFile = ":project/Editor/CppProject.ddl");
  WResult Load(WStringView sFile = ":project/Editor/CppProject.ddl");

  WString m_sPluginName;
};
