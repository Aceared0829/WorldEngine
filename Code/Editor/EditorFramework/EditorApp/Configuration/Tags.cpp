#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <ToolsFoundation/Settings/ToolsTagRegistry.h>

WStatus WQtEditorApp::SaveTagRegistry()
{
  W_LOG_BLOCK("WQtEditorApp::SaveTagRegistry()");

  WStringBuilder sPath;
  sPath = WToolsProject::GetSingleton()->GetProjectDirectory();
  sPath.AppendPath("RuntimeConfigs/Tags.ddl");

  WDeferredFileWriter file;
  file.SetOutput(sPath);

  WToolsTagRegistry::WriteToDDL(file);

  if (file.Close().Failed())
  {
    return WStatus(WFmt("Could not open tags config file '{0}' for writing", sPath));
  }

  return WStatus(W_SUCCESS);
}

void WQtEditorApp::ReadTagRegistry()
{
  W_LOG_BLOCK("WQtEditorApp::ReadTagRegistry");

  WToolsTagRegistry::Clear();

  WStringBuilder sPath;
  sPath = WToolsProject::GetSingleton()->GetProjectDirectory();
  sPath.AppendPath("RuntimeConfigs/Tags.ddl");

  WFileReader file;
  if (file.Open(sPath).Failed())
  {
    WLog::Warning("Could not open tags config file '{0}'", sPath);

    SaveTagRegistry().LogFailure();
  }
  else
  {
    WToolsTagRegistry::ReadFromDDL(file).LogFailure();
  }


  // TODO: Add default tags
  WToolsTag tag;
  tag.m_sName = "EditorHidden";
  tag.m_sCategory = "Editor";
  WToolsTagRegistry::AddTag(tag);
}
