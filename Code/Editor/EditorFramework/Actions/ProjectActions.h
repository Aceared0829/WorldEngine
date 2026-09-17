#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <ToolsFoundation/Project/ToolsProject.h>

class WCppSettings;

///
class W_EDITORFRAMEWORK_DLL WProjectActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(WStringView sMapping, const WBitflags<WStandardMenuTypes> menus = WStandardMenuTypes::Default);

  static WActionDescriptorHandle s_hCatProjectGeneral;
  static WActionDescriptorHandle s_hCatProjectAssets;
  static WActionDescriptorHandle s_hCatProjectConfig;
  static WActionDescriptorHandle s_hCatProjectExternal;

  static WActionDescriptorHandle s_hCatFilesGeneral;
  static WActionDescriptorHandle s_hCatFileCommon;
  static WActionDescriptorHandle s_hCatFileSpecial;

  static WActionDescriptorHandle s_hCreateDocument;
  static WActionDescriptorHandle s_hOpenDocument;
  static WActionDescriptorHandle s_hRecentDocuments;

  static WActionDescriptorHandle s_hOpenDashboard;
  static WActionDescriptorHandle s_hCreateProject;
  static WActionDescriptorHandle s_hOpenProject;
  static WActionDescriptorHandle s_hRecentProjects;
  static WActionDescriptorHandle s_hCloseProject;

  static WActionDescriptorHandle s_hDocsAndCommunity;

  static WActionDescriptorHandle s_hCatProjectSettings;
  static WActionDescriptorHandle s_hCatPluginSettings;
  static WActionDescriptorHandle s_hShortcutEditor;
  static WActionDescriptorHandle s_hDataDirectories;
  static WActionDescriptorHandle s_hWindowConfig;
  static WActionDescriptorHandle s_hInputConfig;
  static WActionDescriptorHandle s_hPreferencesDlg;
  static WActionDescriptorHandle s_hTagsConfig;
  static WActionDescriptorHandle s_hAssetProfiles;
  static WActionDescriptorHandle s_hExportProject;
  static WActionDescriptorHandle s_hPluginSelection;

  static WActionDescriptorHandle s_hCatToolsExternal;
  static WActionDescriptorHandle s_hCatToolsEditor;
  static WActionDescriptorHandle s_hCatToolsDocument;
  static WActionDescriptorHandle s_hCatEditorSettings;
  static WActionDescriptorHandle s_hReloadResources;
  static WActionDescriptorHandle s_hReloadEngine;
  static WActionDescriptorHandle s_hLaunchFileserve;
  static WActionDescriptorHandle s_hInspectorMenu;
  static WActionDescriptorHandle s_hLaunchInspectorPlayer;
  static WActionDescriptorHandle s_hLaunchInspectorEditorEngine;
  static WActionDescriptorHandle s_hLaunchTracy;
  static WActionDescriptorHandle s_hSaveProfiling;
  static WActionDescriptorHandle s_hOpenVsCode;
  static WActionDescriptorHandle s_hImportAsset;
  static WActionDescriptorHandle s_hClearAssetCaches;

  static WActionDescriptorHandle s_hCppProjectMenu;
  static WActionDescriptorHandle s_hSetupCppProject;
  static WActionDescriptorHandle s_hOpenCppProject;
  static WActionDescriptorHandle s_hCompileCppProject;
  static WActionDescriptorHandle s_hRegenerateCppSolution;
};

///
class W_EDITORFRAMEWORK_DLL WRecentDocumentsMenuAction : public WDynamicMenuAction
{
  W_ADD_DYNAMIC_REFLECTION(WRecentDocumentsMenuAction, WDynamicMenuAction);

public:
  WRecentDocumentsMenuAction(const WActionContext& context, const char* szName, const char* szIconPath)
    : WDynamicMenuAction(context, szName, szIconPath)
  {
  }
  virtual void GetEntries(WDynamicArray<Item>& out_entries) override;
  virtual void Execute(const WVariant& value) override;
};

///
class W_EDITORFRAMEWORK_DLL WRecentProjectsMenuAction : public WDynamicMenuAction
{
  W_ADD_DYNAMIC_REFLECTION(WRecentProjectsMenuAction, WDynamicMenuAction);

public:
  WRecentProjectsMenuAction(const WActionContext& context, const char* szName, const char* szIconPath)
    : WDynamicMenuAction(context, szName, szIconPath)
  {
  }
  virtual void GetEntries(WDynamicArray<Item>& out_entries) override;
  virtual void Execute(const WVariant& value) override;
};

///
class W_EDITORFRAMEWORK_DLL WProjectAction : public WButtonAction
{
  W_ADD_DYNAMIC_REFLECTION(WProjectAction, WButtonAction);

public:
  enum class ButtonType
  {
    CreateDocument,
    OpenDocument,
    OpenDashboard,
    CreateProject,
    OpenProject,
    CloseProject,
    ReloadResources,
    ReloadEngine,
    LaunchFileserve,
    LaunchInspectorPlayer,
    LaunchInspectorEditorEngine,
    LaunchTracy,
    SaveProfiling,
    OpenVsCode,
    Shortcuts,
    DataDirectories,
    WindowConfig,
    InputConfig,
    PreferencesDialog,
    TagsDialog,
    ImportAsset,
    AssetProfiles,
    SetupCppProject,
    OpenCppProject,
    CompileCppProject,
    RegenerateCppSolution,
    ShowDocsAndCommunity,
    ExportProject,
    PluginSelection,
    ClearAssetCaches,
  };

  WProjectAction(const WActionContext& context, const char* szName, ButtonType button);
  ~WProjectAction();

  virtual void Execute(const WVariant& value) override;

private:
  void ProjectEventHandler(const WToolsProjectEvent& e);
  void CppEventHandler(const WCppSettings& e);

  ButtonType m_ButtonType;
};
