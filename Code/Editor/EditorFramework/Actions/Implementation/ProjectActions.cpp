#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Actions/WindowLayoutActions.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/Dialogs/AssetProfilesDlg.moc.h>
#include <EditorFramework/Dialogs/CppProjectDlg.moc.h>
#include <EditorFramework/Dialogs/DataDirsDlg.moc.h>
#include <EditorFramework/Dialogs/ExportProjectDlg.moc.h>
#include <EditorFramework/Dialogs/InputConfigDlg.moc.h>
#include <EditorFramework/Dialogs/LaunchFileserveDlg.moc.h>
#include <EditorFramework/Dialogs/PluginSelectionDlg.moc.h>
#include <EditorFramework/Dialogs/PreferencesDlg.moc.h>
#include <EditorFramework/Dialogs/TagsDlg.moc.h>
#include <EditorFramework/Dialogs/WindowCfgDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Profiling/ProfilingUtils.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <GuiFoundation/Dialogs/ShortcutEditorDlg.moc.h>

WActionDescriptorHandle WProjectActions::s_hCatProjectGeneral;
WActionDescriptorHandle WProjectActions::s_hCatProjectAssets;
WActionDescriptorHandle WProjectActions::s_hCatProjectConfig;
WActionDescriptorHandle WProjectActions::s_hCatProjectExternal;

WActionDescriptorHandle WProjectActions::s_hCatFilesGeneral;
WActionDescriptorHandle WProjectActions::s_hCatFileCommon;
WActionDescriptorHandle WProjectActions::s_hCatFileSpecial;

WActionDescriptorHandle WProjectActions::s_hCreateDocument;
WActionDescriptorHandle WProjectActions::s_hOpenDocument;
WActionDescriptorHandle WProjectActions::s_hRecentDocuments;

WActionDescriptorHandle WProjectActions::s_hOpenDashboard;
WActionDescriptorHandle WProjectActions::s_hCreateProject;
WActionDescriptorHandle WProjectActions::s_hOpenProject;
WActionDescriptorHandle WProjectActions::s_hRecentProjects;
WActionDescriptorHandle WProjectActions::s_hCloseProject;
WActionDescriptorHandle WProjectActions::s_hDocsAndCommunity;

WActionDescriptorHandle WProjectActions::s_hCatProjectSettings;
WActionDescriptorHandle WProjectActions::s_hCatPluginSettings;
WActionDescriptorHandle WProjectActions::s_hShortcutEditor;
WActionDescriptorHandle WProjectActions::s_hDataDirectories;
WActionDescriptorHandle WProjectActions::s_hWindowConfig;
WActionDescriptorHandle WProjectActions::s_hInputConfig;
WActionDescriptorHandle WProjectActions::s_hPreferencesDlg;
WActionDescriptorHandle WProjectActions::s_hTagsConfig;
WActionDescriptorHandle WProjectActions::s_hImportAsset;
WActionDescriptorHandle WProjectActions::s_hAssetProfiles;
WActionDescriptorHandle WProjectActions::s_hExportProject;
WActionDescriptorHandle WProjectActions::s_hPluginSelection;
WActionDescriptorHandle WProjectActions::s_hClearAssetCaches;

WActionDescriptorHandle WProjectActions::s_hCatToolsExternal;
WActionDescriptorHandle WProjectActions::s_hCatToolsEditor;
WActionDescriptorHandle WProjectActions::s_hCatToolsDocument;
WActionDescriptorHandle WProjectActions::s_hCatEditorSettings;
WActionDescriptorHandle WProjectActions::s_hReloadResources;
WActionDescriptorHandle WProjectActions::s_hReloadEngine;
WActionDescriptorHandle WProjectActions::s_hLaunchFileserve;
WActionDescriptorHandle WProjectActions::s_hInspectorMenu;
WActionDescriptorHandle WProjectActions::s_hLaunchInspectorPlayer;
WActionDescriptorHandle WProjectActions::s_hLaunchInspectorEditorEngine;
WActionDescriptorHandle WProjectActions::s_hLaunchTracy;
WActionDescriptorHandle WProjectActions::s_hSaveProfiling;
WActionDescriptorHandle WProjectActions::s_hOpenVsCode;

WActionDescriptorHandle WProjectActions::s_hCppProjectMenu;
WActionDescriptorHandle WProjectActions::s_hSetupCppProject;
WActionDescriptorHandle WProjectActions::s_hOpenCppProject;
WActionDescriptorHandle WProjectActions::s_hCompileCppProject;
WActionDescriptorHandle WProjectActions::s_hRegenerateCppSolution;

namespace
{
  void OpenConfiguredCppIde()
  {
    WCppSettings cpp;
    cpp.Load().IgnoreResult();

    if (!WCppProject::ExistsProjectCMakeListsTxt())
    {
      WQtUiServices::GetSingleton()->MessageBoxInformation("C++ code has not been set up, opening a solution is not possible.");
      return;
    }

    if (WCppProject::RunCMakeIfNecessary(cpp).Failed())
    {
      WQtUiServices::GetSingleton()->MessageBoxWarning("Generating the C++ solution failed.");
      return;
    }

    if (auto status = WCppProject::OpenSolution(cpp); status.Failed())
    {
      WQtUiServices::GetSingleton()->MessageBoxWarning(status.GetMessageString().GetView());
    }
  }
}

void WProjectActions::RegisterActions()
{
  s_hCatProjectGeneral = W_REGISTER_CATEGORY("G.Project.General");
  s_hCatProjectAssets = W_REGISTER_CATEGORY("G.Project.Assets");
  s_hCatProjectExternal = W_REGISTER_CATEGORY("G.Project.External");
  s_hCatProjectConfig = W_REGISTER_CATEGORY("G.Project.Config");
  s_hCatEditorSettings = W_REGISTER_CATEGORY("G.Editor.Settings");

  s_hCatFilesGeneral = W_REGISTER_CATEGORY("G.Files.General");
  s_hCatFileCommon = W_REGISTER_CATEGORY("G.File.Common");
  s_hCatFileSpecial = W_REGISTER_CATEGORY("G.File.Special");


  s_hOpenDashboard = W_REGISTER_ACTION_1("Editor.OpenDashboard", WActionScope::Global, "Editor", "", WProjectAction, WProjectAction::ButtonType::OpenDashboard);

  s_hCreateProject = W_REGISTER_ACTION_1("Project.Create", WActionScope::Global, "Project", "", WProjectAction, WProjectAction::ButtonType::CreateProject);

  s_hOpenProject = W_REGISTER_ACTION_1("Project.Open", WActionScope::Global, "Project", "Ctrl+Shift+D", WProjectAction, WProjectAction::ButtonType::OpenProject);

  s_hRecentProjects = W_REGISTER_DYNAMIC_MENU("Project.RecentProjects.Menu", WRecentProjectsMenuAction, "");
  s_hCloseProject = W_REGISTER_ACTION_1("Project.Close", WActionScope::Global, "Project", "", WProjectAction, WProjectAction::ButtonType::CloseProject);

  s_hImportAsset = W_REGISTER_ACTION_1("Project.ImportAsset", WActionScope::Global, "Project", "Ctrl+I", WProjectAction, WProjectAction::ButtonType::ImportAsset);
  s_hClearAssetCaches = W_REGISTER_ACTION_1("Project.ClearAssetCaches", WActionScope::Global, "Project", "", WProjectAction, WProjectAction::ButtonType::ClearAssetCaches);

  s_hExportProject = W_REGISTER_ACTION_1("Project.ExportProject", WActionScope::Global, "Project", "", WProjectAction, WProjectAction::ButtonType::ExportProject);

  s_hCppProjectMenu = W_REGISTER_MENU("G.Project.Cpp");
  {
    s_hSetupCppProject = W_REGISTER_ACTION_1("Project.SetupCppProject", WActionScope::Global, "Project", "", WProjectAction, WProjectAction::ButtonType::SetupCppProject);
    s_hOpenCppProject = W_REGISTER_ACTION_1("Project.OpenCppProject", WActionScope::Global, "Project", "Ctrl+Shift+O", WProjectAction, WProjectAction::ButtonType::OpenCppProject);
    s_hCompileCppProject = W_REGISTER_ACTION_1("Project.CompileCppProject", WActionScope::Global, "Project", "", WProjectAction, WProjectAction::ButtonType::CompileCppProject);
    s_hRegenerateCppSolution = W_REGISTER_ACTION_1("Project.RegenerateCppSolution", WActionScope::Global, "Project", "", WProjectAction, WProjectAction::ButtonType::RegenerateCppSolution);
  }

  s_hCatProjectSettings = W_REGISTER_MENU("G.Project.Settings");

  s_hPluginSelection = W_REGISTER_ACTION_1("Project.PluginSelection", WActionScope::Global, "Project", "", WProjectAction, WProjectAction::ButtonType::PluginSelection);
  s_hDataDirectories = W_REGISTER_ACTION_1("Project.DataDirectories", WActionScope::Global, "Project", "", WProjectAction, WProjectAction::ButtonType::DataDirectories);
  s_hTagsConfig = W_REGISTER_ACTION_1("Engine.Tags", WActionScope::Global, "Editor", "", WProjectAction, WProjectAction::ButtonType::TagsDialog);
  s_hInputConfig = W_REGISTER_ACTION_1("Project.InputConfig", WActionScope::Global, "Project", "", WProjectAction, WProjectAction::ButtonType::InputConfig);
  s_hWindowConfig = W_REGISTER_ACTION_1("Project.WindowConfig", WActionScope::Global, "Project", "", WProjectAction, WProjectAction::ButtonType::WindowConfig);
  s_hAssetProfiles = W_REGISTER_ACTION_1("Project.AssetProfiles", WActionScope::Global, "Project", "", WProjectAction, WProjectAction::ButtonType::AssetProfiles);

  s_hCatPluginSettings = W_REGISTER_MENU("G.Plugins.Settings");

  //////////////////////////////////////////////////////////////////////////

  s_hCreateDocument = W_REGISTER_ACTION_1("Document.Create", WActionScope::Global, "Project", "Ctrl+N", WProjectAction, WProjectAction::ButtonType::CreateDocument);
  s_hOpenDocument = W_REGISTER_ACTION_1("Document.Open", WActionScope::Global, "Project", "Ctrl+O", WProjectAction, WProjectAction::ButtonType::OpenDocument);
  s_hRecentDocuments = W_REGISTER_DYNAMIC_MENU("Project.RecentDocuments.Menu", WRecentDocumentsMenuAction, "");

  s_hShortcutEditor = W_REGISTER_ACTION_1("Editor.Shortcuts", WActionScope::Global, "Editor", "", WProjectAction, WProjectAction::ButtonType::Shortcuts);
  s_hPreferencesDlg = W_REGISTER_ACTION_1("Editor.Preferences", WActionScope::Global, "Editor", "", WProjectAction, WProjectAction::ButtonType::PreferencesDialog);

  s_hCatToolsExternal = W_REGISTER_CATEGORY("G.Tools.External");
  s_hCatToolsEditor = W_REGISTER_CATEGORY("G.Tools.Editor");
  s_hCatToolsDocument = W_REGISTER_CATEGORY("G.Tools.Document");

  s_hReloadResources = W_REGISTER_ACTION_1("Engine.ReloadResources", WActionScope::Global, "Engine", "F4", WProjectAction, WProjectAction::ButtonType::ReloadResources);
  s_hReloadEngine = W_REGISTER_ACTION_1("Engine.ReloadEngine", WActionScope::Global, "Engine", "Ctrl+Shift+F4", WProjectAction, WProjectAction::ButtonType::ReloadEngine);
  s_hLaunchFileserve = W_REGISTER_ACTION_1("Editor.LaunchFileserve", WActionScope::Global, "Engine", "", WProjectAction, WProjectAction::ButtonType::LaunchFileserve);
  s_hInspectorMenu = W_REGISTER_MENU("G.Inspector");
  s_hLaunchInspectorPlayer = W_REGISTER_ACTION_1("Editor.LaunchInspectorPlayer", WActionScope::Global, "Engine", "", WProjectAction, WProjectAction::ButtonType::LaunchInspectorPlayer);
  s_hLaunchInspectorEditorEngine = W_REGISTER_ACTION_1("Editor.LaunchInspectorEditorEngine", WActionScope::Global, "Engine", "", WProjectAction, WProjectAction::ButtonType::LaunchInspectorEditorEngine);
  s_hLaunchTracy = W_REGISTER_ACTION_1("Editor.LaunchTracy", WActionScope::Global, "Engine", "", WProjectAction, WProjectAction::ButtonType::LaunchTracy);
  s_hSaveProfiling = W_REGISTER_ACTION_1("Editor.SaveProfiling", WActionScope::Global, "Engine", "Ctrl+Alt+P", WProjectAction, WProjectAction::ButtonType::SaveProfiling);
  s_hOpenVsCode = W_REGISTER_ACTION_1("Editor.OpenVsCode", WActionScope::Global, "Project", "Ctrl+Alt+O", WProjectAction, WProjectAction::ButtonType::OpenVsCode);



  s_hDocsAndCommunity = W_REGISTER_ACTION_1("Editor.DocsAndCommunity", WActionScope::Global, "Editor", "", WProjectAction, WProjectAction::ButtonType::ShowDocsAndCommunity);
}

void WProjectActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hCatProjectGeneral);
  WActionManager::UnregisterAction(s_hCatProjectAssets);
  WActionManager::UnregisterAction(s_hCatProjectConfig);
  WActionManager::UnregisterAction(s_hCatProjectExternal);

  WActionManager::UnregisterAction(s_hCatFilesGeneral);
  WActionManager::UnregisterAction(s_hCatFileCommon);
  WActionManager::UnregisterAction(s_hCatFileSpecial);

  WActionManager::UnregisterAction(s_hCreateDocument);
  WActionManager::UnregisterAction(s_hOpenDocument);
  WActionManager::UnregisterAction(s_hRecentDocuments);
  WActionManager::UnregisterAction(s_hOpenDashboard);
  WActionManager::UnregisterAction(s_hDocsAndCommunity);
  WActionManager::UnregisterAction(s_hCreateProject);
  WActionManager::UnregisterAction(s_hOpenProject);
  WActionManager::UnregisterAction(s_hRecentProjects);
  WActionManager::UnregisterAction(s_hCloseProject);
  WActionManager::UnregisterAction(s_hCatProjectSettings);
  WActionManager::UnregisterAction(s_hCatPluginSettings);
  WActionManager::UnregisterAction(s_hCatToolsExternal);
  WActionManager::UnregisterAction(s_hCatToolsEditor);
  WActionManager::UnregisterAction(s_hCatToolsDocument);
  WActionManager::UnregisterAction(s_hCatEditorSettings);
  WActionManager::UnregisterAction(s_hReloadResources);
  WActionManager::UnregisterAction(s_hReloadEngine);
  WActionManager::UnregisterAction(s_hLaunchFileserve);
  WActionManager::UnregisterAction(s_hInspectorMenu);
  WActionManager::UnregisterAction(s_hLaunchInspectorPlayer);
  WActionManager::UnregisterAction(s_hLaunchInspectorEditorEngine);
  WActionManager::UnregisterAction(s_hLaunchTracy);
  WActionManager::UnregisterAction(s_hSaveProfiling);
  WActionManager::UnregisterAction(s_hOpenVsCode);
  WActionManager::UnregisterAction(s_hShortcutEditor);
  WActionManager::UnregisterAction(s_hPreferencesDlg);
  WActionManager::UnregisterAction(s_hTagsConfig);
  WActionManager::UnregisterAction(s_hDataDirectories);
  WActionManager::UnregisterAction(s_hWindowConfig);
  WActionManager::UnregisterAction(s_hImportAsset);
  WActionManager::UnregisterAction(s_hClearAssetCaches);
  WActionManager::UnregisterAction(s_hInputConfig);
  WActionManager::UnregisterAction(s_hAssetProfiles);
  WActionManager::UnregisterAction(s_hCppProjectMenu);
  WActionManager::UnregisterAction(s_hSetupCppProject);
  WActionManager::UnregisterAction(s_hOpenCppProject);
  WActionManager::UnregisterAction(s_hCompileCppProject);
  WActionManager::UnregisterAction(s_hRegenerateCppSolution);
  WActionManager::UnregisterAction(s_hExportProject);
  WActionManager::UnregisterAction(s_hPluginSelection);
}

void WProjectActions::MapActions(WStringView sMapping, const WBitflags<WStandardMenuTypes> menus)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  // Add categories
  pMap->MapAction(s_hCatProjectGeneral, "G.Project", 1.0f);
  pMap->MapAction(s_hCatProjectConfig, "G.Project", 2.0f);
  pMap->MapAction(s_hCatProjectAssets, "G.Project", 4.0f);
  pMap->MapAction(s_hCatProjectExternal, "G.Project", 5.0f);

  pMap->MapAction(s_hCatToolsExternal, "G.Tools", 1.0f);
  pMap->MapAction(s_hCatToolsEditor, "G.Tools", 2.0f);
  pMap->MapAction(s_hCatToolsDocument, "G.Tools", 3.0f);
  pMap->MapAction(s_hCatEditorSettings, "G.Tools", 1000.0f);

  pMap->MapAction(s_hCatProjectSettings, "G.Project.Config", 1.0f);
  pMap->MapAction(s_hCatPluginSettings, "G.Project.Config", 1.0f);

  if (menus.IsSet(WStandardMenuTypes::File))
  {
    pMap->MapAction(s_hCatFilesGeneral, "G.File", 1.0f);
    pMap->MapAction(s_hCatFileCommon, "G.File", 2.0f);
    pMap->MapAction(s_hCatFileSpecial, "G.File", 4.0f);
  }

  // Add actions
  // pMap->MapAction(s_hOpenDashboard, "G.Project.General", 1.0f);
  pMap->MapAction(s_hOpenProject, "G.Project.General", 2.0f);   // use dashboard
  pMap->MapAction(s_hCreateProject, "G.Project.General", 3.0f); // use dashboard
  // pMap->MapAction(s_hRecentProjects, "G.Project.General", 4.0f);// use dashboard
  pMap->MapAction(s_hCloseProject, "G.Project.General", 5.0f);

  pMap->MapAction(s_hImportAsset, "G.Project.Assets", 1.0f);
  pMap->MapAction(s_hClearAssetCaches, "G.Project.Assets", 5.0f);

  pMap->MapAction(s_hDataDirectories, "G.Project.Settings", 1.0f);
  pMap->MapAction(s_hInputConfig, "G.Project.Settings", 2.0f);
  pMap->MapAction(s_hWindowConfig, "G.Project.Settings", 3.0f);
  pMap->MapAction(s_hTagsConfig, "G.Project.Settings", 4.0f);
  pMap->MapAction(s_hAssetProfiles, "G.Project.Settings", 5.0f);

  pMap->MapAction(s_hPluginSelection, "G.Plugins.Settings", -1000.0f);

  pMap->MapAction(s_hCppProjectMenu, "G.Project.External", 1.0f);
  pMap->MapAction(s_hSetupCppProject, "G.Project.Cpp", 1.0f);
  pMap->MapAction(s_hOpenCppProject, "G.Project.Cpp", 2.0f);
  pMap->MapAction(s_hCompileCppProject, "G.Project.Cpp", 3.0f);
  pMap->MapAction(s_hRegenerateCppSolution, "G.Project.Cpp", 4.0f);
  pMap->MapAction(s_hExportProject, "G.Project.External", 10.0f);

  pMap->MapAction(s_hOpenVsCode, "G.Tools.External", 1.0f);
  pMap->MapAction(s_hInspectorMenu, "G.Tools.External", 2.0f);
  pMap->MapAction(s_hLaunchInspectorPlayer, "G.Inspector", 1.0f);
  pMap->MapAction(s_hLaunchInspectorEditorEngine, "G.Inspector", 2.0f);
  pMap->MapAction(s_hLaunchTracy, "G.Tools.External", 3.0f);
  pMap->MapAction(s_hLaunchFileserve, "G.Tools.External", 4.0f);

  pMap->MapAction(s_hReloadResources, "G.Tools.Editor", 1.0f);
  pMap->MapAction(s_hReloadEngine, "G.Tools.Editor", 2.0f);
  pMap->MapAction(s_hSaveProfiling, "G.Tools.Editor", 3.0f);

  pMap->MapAction(s_hShortcutEditor, "G.Editor.Settings", 1.0f);
  pMap->MapAction(s_hPreferencesDlg, "G.Editor.Settings", 2.0f);

  WWindowLayoutActions::MapActions(sMapping);

  if (menus.IsSet(WStandardMenuTypes::Help))
  {
    pMap->MapAction(s_hDocsAndCommunity, "G.Help", 0.0f);
  }

  if (menus.IsSet(WStandardMenuTypes::File))
  {
    pMap->MapAction(s_hCreateDocument, "G.Files.General", 1.0f);
    pMap->MapAction(s_hOpenDocument, "G.Files.General", 2.0f);
    pMap->MapAction(s_hRecentDocuments, "G.Files.General", 3.0f);
  }
}

////////////////////////////////////////////////////////////////////////
// WRecentDocumentsMenuAction
////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WRecentDocumentsMenuAction, 0, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

void WRecentDocumentsMenuAction::GetEntries(WDynamicArray<Item>& out_entries)
{
  out_entries.Clear();

  if (WQtEditorApp::GetSingleton()->GetRecentDocumentsList().GetFileList().IsEmpty())
    return;

  WInt32 iMaxDocumentsToAdd = 10;
  for (auto file : WQtEditorApp::GetSingleton()->GetRecentDocumentsList().GetFileList())
  {
    QAction* pAction = nullptr;

    if (!WOSFile::ExistsFile(file.m_File))
      continue;

    WDynamicMenuAction::Item item;

    const WDocumentTypeDescriptor* pTypeDesc = nullptr;
    if (WDocumentManager::FindDocumentTypeFromPath(file.m_File, false, pTypeDesc).Failed())
      continue;

    item.m_UserValue = file.m_File;
    item.m_Icon = WQtUiServices::GetCachedIconResource(pTypeDesc->m_sIcon, WColorScheme::GetCategoryColor(pTypeDesc->m_sAssetCategory, WColorScheme::CategoryColorUsage::MenuEntryIcon));

    if (WToolsProject::IsProjectOpen())
    {
      WString sRelativePath;
      if (!WToolsProject::GetSingleton()->IsDocumentInAllowedRoot(file.m_File, &sRelativePath))
        continue;

      item.m_sDisplay = sRelativePath;

      out_entries.PushBack(item);
    }
    else
    {
      item.m_sDisplay = file.m_File;

      out_entries.PushBack(item);
    }

    --iMaxDocumentsToAdd;

    if (iMaxDocumentsToAdd <= 0)
      break;
  }
}

void WRecentDocumentsMenuAction::Execute(const WVariant& value)
{
  WQtEditorApp::GetSingleton()->OpenDocumentQueued(value.ConvertTo<WString>());
}


////////////////////////////////////////////////////////////////////////
// WRecentDocumentsMenuAction
////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WRecentProjectsMenuAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

void WRecentProjectsMenuAction::GetEntries(WDynamicArray<Item>& out_entries)
{
  out_entries.Clear();

  WStringBuilder sTemp;

  for (auto file : WQtEditorApp::GetSingleton()->GetRecentProjectsList().GetFileList())
  {
    if (!WOSFile::ExistsFile(file.m_File))
      continue;

    sTemp = file.m_File;
    sTemp.PathParentDirectory();
    sTemp.Trim("/");

    WDynamicMenuAction::Item item;
    item.m_sDisplay = sTemp;
    item.m_UserValue = file.m_File;

    out_entries.PushBack(item);
  }
}

void WRecentProjectsMenuAction::Execute(const WVariant& value)
{
  WQtEditorApp::GetSingleton()->OpenProject(value.ConvertTo<WString>()).IgnoreResult();
}

////////////////////////////////////////////////////////////////////////
// WProjectAction
////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WProjectAction, 1, WRTTINoAllocator)
  ;
W_END_DYNAMIC_REFLECTED_TYPE;

WProjectAction::WProjectAction(const WActionContext& context, const char* szName, ButtonType button)
  : WButtonAction(context, szName, false, "")
{
  m_ButtonType = button;

  switch (m_ButtonType)
  {
    case WProjectAction::ButtonType::CreateDocument:
      SetIconPath(":/GuiFoundation/Icons/DocumentAdd.svg");
      break;
    case WProjectAction::ButtonType::OpenDocument:
      SetIconPath(":/GuiFoundation/Icons/Document.svg");
      break;
    case WProjectAction::ButtonType::OpenDashboard:
      SetIconPath(":/GuiFoundation/Icons/Project.svg");
      break;
    case WProjectAction::ButtonType::CreateProject:
      SetIconPath(":/GuiFoundation/Icons/ProjectAdd.svg");
      break;
    case WProjectAction::ButtonType::OpenProject:
      SetIconPath(":/GuiFoundation/Icons/Project.svg");
      break;
    case WProjectAction::ButtonType::CloseProject:
      SetIconPath(":/GuiFoundation/Icons/ProjectClose.svg");
      break;
    case WProjectAction::ButtonType::ReloadResources:
      SetIconPath(":/GuiFoundation/Icons/ReloadResources.svg");
      break;
    case WProjectAction::ButtonType::LaunchFileserve:
      SetIconPath(":/EditorFramework/Icons/Fileserve.svg");
      break;
    case WProjectAction::ButtonType::LaunchInspectorEditorEngine:
    case WProjectAction::ButtonType::LaunchInspectorPlayer:
      SetIconPath(":/EditorFramework/Icons/Inspector.svg");
      break;
    case WProjectAction::ButtonType::LaunchTracy:
      SetIconPath(":/EditorFramework/Icons/Tracy.svg");
      break;
    case WProjectAction::ButtonType::ReloadEngine:
      SetIconPath(":/GuiFoundation/Icons/ReloadEngine.svg");
      break;
    case WProjectAction::ButtonType::DataDirectories:
      SetIconPath(":/EditorFramework/Icons/DataDirectory.svg");
      break;
    case WProjectAction::ButtonType::WindowConfig:
      SetIconPath(":/EditorFramework/Icons/WindowConfig.svg");
      break;
    case WProjectAction::ButtonType::ImportAsset:
      SetIconPath(":/GuiFoundation/Icons/Import.svg");
      break;
    case WProjectAction::ButtonType::InputConfig:
      SetIconPath(":/EditorFramework/Icons/Input.svg");
      break;
    case WProjectAction::ButtonType::PluginSelection:
      SetIconPath(":/EditorFramework/Icons/Plugins.svg");
      break;
    case WProjectAction::ButtonType::PreferencesDialog:
      SetIconPath(":/EditorFramework/Icons/StoredSettings.svg");
      break;
    case WProjectAction::ButtonType::TagsDialog:
      SetIconPath(":/EditorFramework/Icons/Tag.svg");
      break;
    case WProjectAction::ButtonType::ExportProject:
      // TODO: SetIconPath(":/EditorFramework/Icons/Tag.svg");
      break;
    case WProjectAction::ButtonType::Shortcuts:
      SetIconPath(":/GuiFoundation/Icons/Shortcuts.svg");
      break;
    case WProjectAction::ButtonType::AssetProfiles:
      SetIconPath(":/EditorFramework/Icons/AssetProfile.svg");
      break;
    case WProjectAction::ButtonType::OpenVsCode:
      SetIconPath(":/GuiFoundation/Icons/vscode.svg");
      break;
    case WProjectAction::ButtonType::SaveProfiling:
      // no icon
      break;
    case WProjectAction::ButtonType::SetupCppProject:
      SetIconPath(":/EditorFramework/Icons/VisualStudio.svg");
      break;
    case WProjectAction::ButtonType::OpenCppProject:
      // SetIconPath(":/EditorFramework/Icons/VisualStudio.svg"); // TODO
      break;
    case WProjectAction::ButtonType::CompileCppProject:
      // SetIconPath(":/EditorFramework/Icons/VisualStudio.svg"); // TODO
      break;
    case WProjectAction::ButtonType::RegenerateCppSolution:
      // SetIconPath(":/EditorFramework/Icons/VisualStudio.svg"); // TODO
      break;
    case WProjectAction::ButtonType::ShowDocsAndCommunity:
      SetIconPath(":/GuiFoundation/Icons/Help.svg");
      break;
    case WProjectAction::ButtonType::ClearAssetCaches:
      // SetIconPath(":/GuiFoundation/Icons/Project.svg"); // TODO
      break;
  }

  if (m_ButtonType == ButtonType::CloseProject ||
      m_ButtonType == ButtonType::DataDirectories ||
      m_ButtonType == ButtonType::WindowConfig ||
      m_ButtonType == ButtonType::ImportAsset ||
      m_ButtonType == ButtonType::TagsDialog ||
      m_ButtonType == ButtonType::ReloadEngine ||
      m_ButtonType == ButtonType::ReloadResources ||
      m_ButtonType == ButtonType::LaunchFileserve ||
      m_ButtonType == ButtonType::LaunchTracy ||
      m_ButtonType == ButtonType::LaunchInspectorEditorEngine ||
      m_ButtonType == ButtonType::LaunchInspectorPlayer ||
      m_ButtonType == ButtonType::OpenVsCode ||
      m_ButtonType == ButtonType::InputConfig ||
      m_ButtonType == ButtonType::AssetProfiles ||
      m_ButtonType == ButtonType::SetupCppProject ||
      m_ButtonType == ButtonType::OpenCppProject ||
      m_ButtonType == ButtonType::CompileCppProject ||
      m_ButtonType == ButtonType::ExportProject ||
      m_ButtonType == ButtonType::ClearAssetCaches ||
      m_ButtonType == ButtonType::PluginSelection)
  {
    SetEnabled(WToolsProject::IsProjectOpen());

    WToolsProject::s_Events.AddEventHandler(WMakeDelegate(&WProjectAction::ProjectEventHandler, this));
  }

  if (m_ButtonType == ButtonType::OpenCppProject ||
      m_ButtonType == ButtonType::CompileCppProject)
  {
    SetEnabled(WCppProject::ExistsProjectCMakeListsTxt());

    WCppProject::s_ChangeEvents.AddEventHandler(WMakeDelegate(&WProjectAction::CppEventHandler, this));
  }
}

WProjectAction::~WProjectAction()
{
  if (m_ButtonType == ButtonType::CloseProject ||
      m_ButtonType == ButtonType::DataDirectories ||
      m_ButtonType == ButtonType::WindowConfig ||
      m_ButtonType == ButtonType::ImportAsset ||
      m_ButtonType == ButtonType::TagsDialog ||
      m_ButtonType == ButtonType::ReloadEngine ||
      m_ButtonType == ButtonType::ReloadResources ||
      m_ButtonType == ButtonType::LaunchFileserve ||
      m_ButtonType == ButtonType::LaunchInspectorEditorEngine ||
      m_ButtonType == ButtonType::LaunchInspectorPlayer ||
      m_ButtonType == ButtonType::LaunchTracy ||
      m_ButtonType == ButtonType::OpenVsCode ||
      m_ButtonType == ButtonType::InputConfig ||
      m_ButtonType == ButtonType::AssetProfiles ||
      m_ButtonType == ButtonType::SetupCppProject ||
      m_ButtonType == ButtonType::OpenCppProject ||
      m_ButtonType == ButtonType::CompileCppProject ||
      m_ButtonType == ButtonType::ExportProject ||
      m_ButtonType == ButtonType::ClearAssetCaches ||
      m_ButtonType == ButtonType::PluginSelection)
  {
    WToolsProject::s_Events.RemoveEventHandler(WMakeDelegate(&WProjectAction::ProjectEventHandler, this));
  }

  if (m_ButtonType == ButtonType::OpenCppProject ||
      m_ButtonType == ButtonType::CompileCppProject)
  {
    WCppProject::s_ChangeEvents.RemoveEventHandler(WMakeDelegate(&WProjectAction::CppEventHandler, this));
  }
}

void WProjectAction::ProjectEventHandler(const WToolsProjectEvent& e)
{
  if (m_ButtonType == ButtonType::OpenCppProject ||
      m_ButtonType == ButtonType::CompileCppProject)
  {
    SetEnabled(WCppProject::ExistsProjectCMakeListsTxt());
  }
  else
  {
    SetEnabled(WToolsProject::IsProjectOpen());
  }
}

void WProjectAction::CppEventHandler(const WCppSettings& e)
{
  if (m_ButtonType == ButtonType::OpenCppProject ||
      m_ButtonType == ButtonType::CompileCppProject)
  {
    SetEnabled(WCppProject::ExistsProjectCMakeListsTxt());
  }
}

void WProjectAction::Execute(const WVariant& value)
{
  switch (m_ButtonType)
  {
    case WProjectAction::ButtonType::CreateDocument:
      WQtEditorApp::GetSingleton()->GuiCreateDocument();
      break;

    case WProjectAction::ButtonType::OpenDocument:
      WQtEditorApp::GetSingleton()->GuiOpenDocument();
      break;

    case WProjectAction::ButtonType::OpenDashboard:
      WQtEditorApp::GetSingleton()->GuiOpenDashboard();
      break;

    case WProjectAction::ButtonType::CreateProject:
      WQtEditorApp::GetSingleton()->GuiCreateProject();
      break;

    case WProjectAction::ButtonType::OpenProject:
      WQtEditorApp::GetSingleton()->GuiOpenDashboard();
      // WQtEditorApp::GetSingleton()->GuiOpenProject();
      break;

    case WProjectAction::ButtonType::CloseProject:
    {
      if (WToolsProject::CanCloseProject())
        WQtEditorApp::GetSingleton()->CloseProject();
    }
    break;

    case WProjectAction::ButtonType::DataDirectories:
    {
      WQtDataDirsDlg dlg(nullptr);
      dlg.exec();
    }
    break;

    case WProjectAction::ButtonType::WindowConfig:
    {
      WQtWindowCfgDlg dlg(nullptr);
      dlg.exec();
    }
    break;

    case WProjectAction::ButtonType::ImportAsset:
    {
      WAssetDocumentGenerator::ImportAssets();
    }
    break;

    case WProjectAction::ButtonType::InputConfig:
    {
      WQtInputConfigDlg dlg(nullptr);
      if (dlg.exec() == QDialog::Accepted)
      {
        WToolsProject::BroadcastConfigChanged();
      }
    }
    break;

    case WProjectAction::ButtonType::PluginSelection:
    {
      WQtEditorApp::GetSingleton()->DetectAvailablePluginBundles(WOSFile::GetApplicationDirectory());

      WCppSettings cppSettings;
      if (cppSettings.Load().Succeeded())
      {
        WQtEditorApp::GetSingleton()->DetectAvailablePluginBundles(WCppProject::GetPluginSourceDir(cppSettings));
      }

      WQtPluginSelectionDlg dlg(&WQtEditorApp::GetSingleton()->GetPluginBundles());
      dlg.exec();

      WToolsProject::SaveProjectState();
    }
    break;

    case WProjectAction::ButtonType::PreferencesDialog:
    {
      WQtPreferencesDlg dlg(nullptr);
      if (dlg.exec() == QDialog::Accepted)
      {
        // save modified preferences right away
        WToolsProject::SaveProjectState();

        WToolsProject::BroadcastConfigChanged();
      }
    }
    break;

    case WProjectAction::ButtonType::TagsDialog:
    {
      WQtTagsDlg dlg(value, nullptr);
      if (dlg.exec() == QDialog::Accepted)
      {
        WToolsProject::BroadcastConfigChanged();
      }
    }
    break;

    case WProjectAction::ButtonType::ExportProject:
    {
      WQtExportProjectDlg dlg(nullptr);
      dlg.exec();
    }
    break;

    case WProjectAction::ButtonType::ClearAssetCaches:
    {
      auto res = WQtUiServices::GetSingleton()->MessageBoxQuestion("Delete ALL cached asset files?\n\n* 'Yes All' deletes everything and takes a long time to re-process. This is rarely needed.\n* 'No All' only deletes assets that are likely to make problems.", QMessageBox::StandardButton::YesAll | QMessageBox::StandardButton::NoAll | QMessageBox::StandardButton::Cancel, QMessageBox::StandardButton::Cancel, QMessageBox::StandardButton::NoAll);

      if (res == QMessageBox::StandardButton::Cancel)
        break;

      if (res == QMessageBox::StandardButton::YesAll)
        WAssetCurator::GetSingleton()->ClearAssetCaches(WAssetDocumentManager::Perfect);
      else
        WAssetCurator::GetSingleton()->ClearAssetCaches(WAssetDocumentManager::Unknown);
    }
    break;

    case WProjectAction::ButtonType::Shortcuts:
    {
      WQtShortcutEditorDlg dlg(nullptr);
      dlg.exec();
    }
    break;

    case WProjectAction::ButtonType::ReloadResources:
    {
      WQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage("Reloading Resources...", WTime::MakeFromSeconds(5));

      WSimpleConfigMsgToEngine msg;
      msg.m_sWhatToDo = "ReloadResources";
      msg.m_sPayload = "ReloadAllResources";
      WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);

      WEditorAppEvent e;
      e.m_Type = WEditorAppEvent::Type::ReloadResources;
      WQtEditorApp::GetSingleton()->m_Events.Broadcast(e);

      // keep this here to make live color palette editing available, when needed
      if (false)
      {
        QTimer::singleShot(1, [this]()
          { WQtEditorApp::GetSingleton()->SetStyleSheet(); });
        QTimer::singleShot(500, [this]()
          { WQtEditorApp::GetSingleton()->SetStyleSheet(); });
      }

      if (m_Context.m_pDocument)
      {
        m_Context.m_pDocument->ShowDocumentStatus("Reloading Resources");
      }

      WTranslator::ReloadAllTranslators();
    }
    break;

    case WProjectAction::ButtonType::LaunchFileserve:
    {
      WQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage("Launching FileServe...", WTime::MakeFromSeconds(5));

      WQtLaunchFileserveDlg dlg(nullptr);
      dlg.exec();
    }
    break;

    case WProjectAction::ButtonType::LaunchInspectorPlayer:
    {
      WQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage("Launching WInspector...", WTime::MakeFromSeconds(5));

      WQtEditorApp::GetSingleton()->RunInspector(1040);
    }
    break;

    case WProjectAction::ButtonType::LaunchInspectorEditorEngine:
    {
      WQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage("Launching WInspector...", WTime::MakeFromSeconds(5));

      const WUInt16 uiPort = static_cast<WUInt16>(WCommandLineUtils::GetGlobalInstance()->GetIntOption("-TelemetryPort", 1050));
      WQtEditorApp::GetSingleton()->RunInspector(uiPort);
    }
    break;

    case WProjectAction::ButtonType::LaunchTracy:
    {
      WQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage("Launching Tracy...", WTime::MakeFromSeconds(5));

      WQtEditorApp::GetSingleton()->RunTracy();
    }
    break;

    case WProjectAction::ButtonType::ReloadEngine:
    {
      WEditorEngineProcessConnection::GetSingleton()->RestartProcess().IgnoreResult();
    }
    break;

    case WProjectAction::ButtonType::SaveProfiling:
    {
      const char* szEditorProfilingFile = ":appdata/profilingEditor.json";
      {
        // Start capturing profiling data on engine process
        WSimpleConfigMsgToEngine msg;
        msg.m_sWhatToDo = "SaveProfiling";
        msg.m_sPayload = ":appdata/profilingEngine.json";
        WEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
      }
      if (WProfilingUtils::SaveProfilingCapture(szEditorProfilingFile).Failed())
        return;

      WStringBuilder sEngineProfilingFile;
      {
        // Wait for engine process response
        auto callback = [&](WProcessMessage* pMsg) -> bool
        {
          auto pSimpleCfg = static_cast<WSaveProfilingResponseToEditor*>(pMsg);
          sEngineProfilingFile = pSimpleCfg->m_sProfilingFile;
          return true;
        };
        WProcessCommunicationChannel::WaitForMessageCallback cb = callback;

        if (WEditorEngineProcessConnection::GetSingleton()->WaitForMessage(WGetStaticRTTI<WSaveProfilingResponseToEditor>(), WTime::MakeFromSeconds(15), &cb).Failed())
        {
          WLog::Error("Timeout while waiting for engine process to create profiling capture. Captures will not be merged.");
          return;
        }
        if (sEngineProfilingFile.IsEmpty())
        {
          WLog::Error("Engine process failed to create profiling file.");
          return;
        }
      }

      WStringBuilder sMergedFile;
      const WDateTime dt = WDateTime::MakeFromTimestamp(WTimestamp::CurrentTimestamp());
      sMergedFile.AppendFormat(":appdata/profiling_{0}-{1}-{2}_{3}-{4}-{5}-{6}.json", dt.GetYear(), WArgU(dt.GetMonth(), 2, true), WArgU(dt.GetDay(), 2, true), WArgU(dt.GetHour(), 2, true), WArgU(dt.GetMinute(), 2, true), WArgU(dt.GetSecond(), 2, true), WArgU(dt.GetMicroseconds() / 1000, 3, true));

      WStringBuilder sAbsPath;
      if (WProfilingUtils::MergeProfilingCaptures(sEngineProfilingFile, szEditorProfilingFile, sMergedFile).Succeeded() && WFileSystem::ResolvePath(sMergedFile, &sAbsPath, nullptr).Succeeded())
      {
        WQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(WFmt("Merged profiling capture saved to '{0}'.", sAbsPath), WTime::MakeFromSeconds(5.0));
      }
    }
    break;

    case WProjectAction::ButtonType::OpenVsCode:
      OpenConfiguredCppIde();
      break;

    case WProjectAction::ButtonType::AssetProfiles:
    {
      WQtAssetProfilesDlg dlg(nullptr);
      if (dlg.exec() == QDialog::Accepted)
      {
        // we need to force the asset status reevaluation because when the profile settings have changed,
        // we need to figure out which assets are now out of date
        WAssetCurator::GetSingleton()->SetActiveAssetProfileByIndex(dlg.m_uiActiveConfig, true);

        // makes the scene re-select the current objects, which updates which enum values are shown in the property grid
        WToolsProject::BroadcastConfigChanged();
      }
    }
    break;

    case WProjectAction::ButtonType::SetupCppProject:
    {
      WQtCppProjectDlg dlg(nullptr);
      dlg.exec();
    }
    break;

    case WProjectAction::ButtonType::OpenCppProject:
      OpenConfiguredCppIde();
      break;

    case WProjectAction::ButtonType::CompileCppProject:
    {
      WCppSettings cpp;
      cpp.Load().IgnoreResult();

      if (WCppProject::ExistsProjectCMakeListsTxt())
      {
        if (WCppProject::BuildCodeIfNecessary(cpp).Succeeded())
        {
          WQtUiServices::GetSingleton()->MessageBoxInformation("Successfully compiled the C++ code.", "cpp-compile-success");
        }
        else
        {
          WQtUiServices::GetSingleton()->MessageBoxWarning("Compiling the code failed. See log for details.");
        }
      }
      else
      {
        WQtUiServices::GetSingleton()->MessageBoxInformation("C++ code has not been set up, compilation is not possible (or necessary).");
      }
    }
    break;

    case WProjectAction::ButtonType::RegenerateCppSolution:
    {
      WCppSettings cpp;
      cpp.Load().IgnoreResult();

      if (!WCppProject::ExistsProjectCMakeListsTxt() || !WCppProject::ExistsSolution(cpp))
      {
        WQtCppProjectDlg dlg(nullptr);
        dlg.exec();
      }
      else
      {
        // most likely the user executes this because there is a problem
        // so use this opportunity to update the CMake files, if they are outdated
        if (WCppProject::PopulateWithDefaultSources(cpp).Failed())
        {
          WQtUiServices::GetSingleton()->MessageBoxWarning("Failed to populate the CppSource directory with the default files.\n\nCheck the log for details.");
          break;
        }

        if (WCppProject::RunCMake(cpp).Succeeded())
        {
          WQtUiServices::GetSingleton()->MessageBoxInformation("Successfully regenerated the C++ solution.", "cpp-regen-success");
        }
        else
        {
          WQtUiServices::GetSingleton()->MessageBoxWarning("Regenerating the solution failed. See log for details.");
        }
      }
    }
    break;

    case WProjectAction::ButtonType::ShowDocsAndCommunity:
      WQtEditorApp::GetSingleton()->GuiOpenDocsAndCommunity();
      break;
  }
}
