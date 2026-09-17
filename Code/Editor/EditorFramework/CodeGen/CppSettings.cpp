#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/CodeGen/CppSettings.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>

WResult WCppSettings::Save(WStringView sFile)
{
  WFileWriter file;
  W_SUCCEED_OR_RETURN(file.Open(sFile));

  WOpenDdlWriter ddl;
  ddl.SetOutputStream(&file);

  ddl.BeginObject("Target", "Default");

  WOpenDdlUtils::StoreString(ddl, m_sPluginName, "PluginName");

  ddl.EndObject();

  return W_SUCCESS;
}

WResult WCppSettings::Load(WStringView sFile)
{
  WFileReader file;
  W_SUCCEED_OR_RETURN(file.Open(sFile));

  WOpenDdlReader ddl;
  W_SUCCEED_OR_RETURN(ddl.ParseDocument(file));

  if (auto pTarget = ddl.GetRootElement()->FindChildOfType("Target", "Default"))
  {
    if (auto pValue = pTarget->FindChildOfType(WOpenDdlPrimitiveType::String, "PluginName"))
    {
      m_sPluginName = pValue->GetPrimitivesString()[0];
    }
  }

  return W_SUCCESS;
}
