#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Profiling/Profiling.h>

void WQtEditorApp::StoreEnginePluginModificationTimes()
{
  for (auto it : m_PluginBundles.m_Plugins)
  {
    WPluginBundle& plugin = it.Value();

    for (const WString& rt : plugin.m_RuntimePlugins)
    {
      WStringBuilder sPath, sCopy;
      WPlugin::GetPluginPaths(rt, sPath, sCopy, 0);

      WFileStats stats;
      if (WOSFile::GetFileStats(sPath, stats).Succeeded())
      {
        if (!plugin.m_LastModificationTime.IsValid() || stats.m_LastModificationTime.Compare(plugin.m_LastModificationTime, WTimestamp::CompareMode::Newer))
        {
          // store the maximum (latest) modification timestamp
          plugin.m_LastModificationTime = stats.m_LastModificationTime;
        }
      }
    }
  }
}

bool WQtEditorApp::CheckForEnginePluginModifications()
{
  for (auto it : m_PluginBundles.m_Plugins)
  {
    WPluginBundle& plugin = it.Value();

    if (plugin.m_bMissing)
    {
      DetectAvailablePluginBundles(WOSFile::GetApplicationDirectory());

      WCppSettings cppSettings;
      if (cppSettings.Load().Succeeded())
      {
        WQtEditorApp::GetSingleton()->DetectAvailablePluginBundles(WCppProject::GetPluginSourceDir(cppSettings));
      }

      break;
    }
  }

  for (auto it : m_PluginBundles.m_Plugins)
  {
    WPluginBundle& plugin = it.Value();

    if (!plugin.m_bSelected || !plugin.m_bLoadCopy)
      continue;

    for (const WString& rt : plugin.m_RuntimePlugins)
    {
      WStringBuilder sPath, sCopy;
      WPlugin::GetPluginPaths(rt, sPath, sCopy, 0);

      WFileStats stats;
      if (WOSFile::GetFileStats(sPath, stats).Succeeded())
      {
        if (!plugin.m_LastModificationTime.IsValid() || stats.m_LastModificationTime.Compare(plugin.m_LastModificationTime, WTimestamp::CompareMode::Newer))
        {
          return true;
        }
      }
    }
  }

  return false;
}

void WQtEditorApp::RestartEngineProcessIfPluginsChanged(bool bForce)
{
  if (!WToolsProject::IsProjectOpen())
    return;

  if (!bForce)
  {
    if (m_LastPluginModificationCheck + WTime::MakeFromSeconds(2) > WTime::Now())
      return;
  }

  m_LastPluginModificationCheck = WTime::Now();

  for (auto pMan : WDocumentManager::GetAllDocumentManagers())
  {
    for (auto pDoc : pMan->WDocumentManager::GetAllOpenDocuments())
    {
      if (!pDoc->CanEngineProcessBeRestarted())
      {
        // not allowed to restart at the moment
        return;
      }
    }
  }

  if (!CheckForEnginePluginModifications())
    return;

  WLog::Info("Engine plugins have changed, restarting engine process.");

  StoreEnginePluginModificationTimes();
  WEditorEngineProcessConnection::GetSingleton()->SetPluginConfig(GetRuntimePluginConfig(true));
  WEditorEngineProcessConnection::GetSingleton()->RestartProcess().IgnoreResult();
}
