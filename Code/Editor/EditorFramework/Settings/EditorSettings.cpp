#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <Foundation/Profiling/Profiling.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
#  include <EditorFramework/EditorApp/WindowsJumpList.h>
#endif

void WQtEditorApp::SaveRecentFiles()
{
  W_PROFILE_SCOPE("SaveRecentFiles");
  if (m_StartupFlags.IsAnySet(StartupFlags::Headless | StartupFlags::UnitTest | StartupFlags::Background))
    return;

  m_RecentProjects.Save(":appdata/Settings/RecentProjects.txt");
  m_RecentDocuments.Save(":appdata/Settings/RecentDocuments.txt");

#if W_ENABLED(W_PLATFORM_WINDOWS_DESKTOP)
  // Update Windows taskbar jump list with recent projects
  WWindowsJumpList::UpdateJumpList(m_RecentProjects);
#endif
}

void WQtEditorApp::LoadRecentFiles()
{
  W_PROFILE_SCOPE("LoadRecentFiles");
  m_RecentProjects.Load(":appdata/Settings/RecentProjects.txt");
  m_RecentDocuments.Load(":appdata/Settings/RecentDocuments.txt");
}

void WQtEditorApp::SaveOpenDocumentsList()
{
  const WDynamicArray<WQtDocumentWindow*>& windows = WQtDocumentWindow::GetAllDocumentWindows();

  if (windows.IsEmpty())
    return;

  WRecentFilesList allDocs(windows.GetCount());

  WDynamicArray<WQtDocumentWindow*> allWindows;
  allWindows.Reserve(windows.GetCount());
  {
    auto* container = WQtContainerWindow::GetContainerWindow();
    WTempHybridArray<WQtDocumentWindow*, 16> docWindows;
    container->GetDocumentWindows(docWindows);
    for (auto* pWindow : docWindows)
    {
      allWindows.PushBack(pWindow);
    }
  }
  for (WInt32 w = (WInt32)allWindows.GetCount() - 1; w >= 0; --w)
  {
    if (allWindows[w]->GetDocument())
    {
      allDocs.Insert(allWindows[w]->GetDocument()->GetDocumentPath(), 0);
    }
  }

  WStringBuilder sFile = WApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
  sFile.AppendPath("LastDocuments.txt");

  allDocs.Save(sFile);
}

WRecentFilesList WQtEditorApp::LoadOpenDocumentsList()
{
  WRecentFilesList allDocs(15);

  WStringBuilder sFile = WApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
  sFile.AppendPath("LastDocuments.txt");

  allDocs.Load(sFile);

  return allDocs;
}

void WQtEditorApp::SaveSettings()
{
  // headless mode should never store any settings on disk
  if (m_StartupFlags.IsAnySet(StartupFlags::Headless | StartupFlags::UnitTest | StartupFlags::Background))
    return;

  SaveRecentFiles();

  WPreferences::SaveApplicationPreferences();

  // this setting is needed before we have loaded the preferences, so we duplicate it in the QSettings (registry)
  {
    WEditorPreferencesUser* pPreferences = WPreferences::QueryPreferences<WEditorPreferencesUser>();

    QSettings s;
    s.beginGroup("EditorPreferences");
    s.setValue("ShowSplashscreen", pPreferences->m_bShowSplashscreen);
    s.endGroup();
  }

  if (WToolsProject::IsProjectOpen())
  {
    WPreferences::SaveProjectPreferences();
    SaveOpenDocumentsList();

    m_FileSystemConfig.Save().IgnoreResult();
    GetRuntimePluginConfig(false).Save().IgnoreResult();
  }
}
