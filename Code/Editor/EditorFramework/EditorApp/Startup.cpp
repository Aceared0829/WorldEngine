#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/CameraModeSwitchActions.h>
#include <EditorFramework/Actions/CommonAssetActions.h>
#include <EditorFramework/Actions/GameObjectContextActions.h>
#include <EditorFramework/Actions/GameObjectDocumentActions.h>
#include <EditorFramework/Actions/GameObjectSelectionActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Actions/QuadViewActions.h>
#include <EditorFramework/Actions/TransformGizmoActions.h>
#include <EditorFramework/Actions/ViewActions.h>
#include <EditorFramework/Actions/ViewLightActions.h>
#include <EditorFramework/Actions/WindowLayoutActions.h>
#include <EditorFramework/Assets/AssetBrowserContext.h>
#include <EditorFramework/CodeGen/CodeEditorPreferencesWidget.moc.h>
#include <EditorFramework/CodeGen/CompilerPreferencesWidget.moc.h>
#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/EditorApp/CheckVersion.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/EditorApp/StackTraceLogParser.h>
#include <EditorFramework/GUI/DynamicDefaultStateProvider.h>
#include <EditorFramework/GUI/ExposedParametersDefaultStateProvider.h>
#include <EditorFramework/Manipulators/BoneManipulatorAdapter.h>
#include <EditorFramework/Manipulators/BoxManipulatorAdapter.h>
#include <EditorFramework/Manipulators/CapsuleManipulatorAdapter.h>
#include <EditorFramework/Manipulators/ConeAngleManipulatorAdapter.h>
#include <EditorFramework/Manipulators/ConeLengthManipulatorAdapter.h>
#include <EditorFramework/Manipulators/ManipulatorAdapterRegistry.h>
#include <EditorFramework/Manipulators/NonUniformBoxManipulatorAdapter.h>
#include <EditorFramework/Manipulators/SphereManipulatorAdapter.h>
#include <EditorFramework/Manipulators/SplineManipulatorAdapter.h>
#include <EditorFramework/Manipulators/SplineTangentManipulatorAdapter.h>
#include <EditorFramework/Manipulators/TransformManipulatorAdapter.h>
#include <EditorFramework/Panels/AssetBrowserPanel/AssetBrowserPanel.moc.h>
#include <EditorFramework/Panels/AssetCheckPanel/AssetCheckPanel.moc.h>
#include <EditorFramework/Panels/AssetCuratorPanel/AssetCuratorPanel.moc.h>
#include <EditorFramework/Panels/CVarPanel/CVarPanel.moc.h>
#include <EditorFramework/Panels/LogPanel/LogPanel.moc.h>
#include <EditorFramework/Panels/LongOpsPanel/LongOpsPanel.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <EditorFramework/Project/ProjectCreation.h>
#include <EditorFramework/PropertyGrid/AssetBrowserPropertyWidget.moc.h>
#include <EditorFramework/PropertyGrid/BlackboardConditionWidget.moc.h>
#include <EditorFramework/PropertyGrid/DynamicEnumPropertyWidget.moc.h>
#include <EditorFramework/PropertyGrid/DynamicStringEnumPropertyWidget.moc.h>
#include <EditorFramework/PropertyGrid/ExposedBoneWidget.moc.h>
#include <EditorFramework/PropertyGrid/ExposedParametersPropertyWidget.moc.h>
#include <EditorFramework/PropertyGrid/FileBrowserPropertyWidget.moc.h>
#include <EditorFramework/PropertyGrid/GameObjectReferencePropertyWidget.moc.h>
#include <EditorFramework/PropertyGrid/RttiTypeStringPropertyWidget.moc.h>
#include <EditorFramework/Visualizers/BoxVisualizerAdapter.h>
#include <EditorFramework/Visualizers/CameraVisualizerAdapter.h>
#include <EditorFramework/Visualizers/CapsuleVisualizerAdapter.h>
#include <EditorFramework/Visualizers/ConeVisualizerAdapter.h>
#include <EditorFramework/Visualizers/CylinderVisualizerAdapter.h>
#include <EditorFramework/Visualizers/DirectionVisualizerAdapter.h>
#include <EditorFramework/Visualizers/PositionVisualizerAdapter.h>
#include <EditorFramework/Visualizers/SphereVisualizerAdapter.h>
#include <EditorFramework/Visualizers/VisualizerAdapterRegistry.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/TraceWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <Foundation/System/EnvironmentVariableUtils.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/PropertyGrid/DefaultState.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <GuiFoundation/UIServices/QtProgressbar.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Document/PrefabUtils.h>

#include <QSvgRenderer>
#include <ads/DockManager.h>

void WCompilerPreferences_PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e);
void WCodeEditorPreferences_PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e);

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, EditorFrameworkMain)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "GuiFoundation",
    "PropertyGrid",
    "ManipulatorAdapterRegistry",
    "DefaultState"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    WDefaultState::RegisterDefaultStateProvider(WExposedParametersAsTypeDefaultStateProvider::CreateProvider);
    WDefaultState::RegisterDefaultStateProvider(WExposedParametersDefaultStateProvider::CreateProvider);
    WDefaultState::RegisterDefaultStateProvider(WDynamicDefaultStateProvider::CreateProvider);
    WProjectActions::RegisterActions();
    WAssetActions::RegisterActions();
    WAssetBrowserContextMenu::RegisterActions();
    WViewActions::RegisterActions();
    WViewLightActions::RegisterActions();
    WGameObjectContextActions::RegisterActions();
    WGameObjectDocumentActions::RegisterActions();
    WGameObjectSelectionActions::RegisterActions();
    WQuadViewActions::RegisterActions();
    WTransformGizmoActions::RegisterActions();
    WTranslateGizmoAction::RegisterActions();
    WCommonAssetActions::RegisterActions();
    WCameraModeSwitchActions::RegisterActions();
    WWindowLayoutActions::RegisterActions();

    // Default Asset Menu Bar
    // All asset menu bar mappings should derive from this to allow for actions to be defined that show up in every asset document editor's menu bar.
    {
      const char* szMenuBar = "AssetMenuBar";
      WActionMapManager::RegisterActionMap(szMenuBar);
      WStandardMenus::MapActions(szMenuBar, WStandardMenuTypes::Default | WStandardMenuTypes::Edit| WStandardMenuTypes::Asset);
      WProjectActions::MapActions(szMenuBar);
      WDocumentActions::MapMenuActions(szMenuBar, "G.File.Common");
      WAssetActions::MapMenuActions(szMenuBar);
      WCommandHistoryActions::MapActions(szMenuBar);
    }

    // Default Asset Toolbar
    // All asset toolbar mappings should derive from this to allow for actions to be defined that show up in every asset document editor's tool bar.
    {
      const char* szToolbar = "AssetToolbar";
      WActionMapManager::RegisterActionMap(szToolbar);

      WDocumentActions::MapToolbarActions(szToolbar);
      WCommandHistoryActions::MapActions(szToolbar, "");
      WAssetActions::MapToolBarActions(szToolbar, true);
    }

    // Default Asset View Toolbar
    // All asset view toolbar mappings should derive from this or its derived "SimpleAssetViewToolbar" to allow for actions to be defined that show up in every asset document editor's view toolbar.
    {
      WActionMapManager::RegisterActionMap("AssetViewToolbar");
      // Convenience mapping that adds the most common view settings:
      const char* szSimpleViewToolbar = "SimpleAssetViewToolbar";
      WActionMapManager::RegisterActionMap(szSimpleViewToolbar, "AssetViewToolbar");
      WViewActions::MapToolbarActions(szSimpleViewToolbar, WViewActions::RenderMode /*| WViewActions::ActivateRemoteProcess*/);
      WViewLightActions::MapToolbarActions(szSimpleViewToolbar);
    }

    WActionMapManager::RegisterActionMap("SettingsTabMenuBar");
    WStandardMenus::MapActions("SettingsTabMenuBar", WStandardMenuTypes::Default);
    WProjectActions::MapActions("SettingsTabMenuBar");

    WActionMapManager::RegisterActionMap("AssetBrowserToolBar");
    WAssetActions::MapToolBarActions("AssetBrowserToolBar", false);

    // Plugins map actions here to offer operations on the asset types they own.
    // The selection is not part of the action context, it has to be read from WAssetBrowserSelection.
    WActionMapManager::RegisterActionMap("AssetBrowserContextMenu");
    WAssetBrowserContextMenu::MapActions();

    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WFileBrowserAttribute>(), [](const WRTTI* pRtti)->WQtPropertyWidget* { return new WQtFilePropertyWidget(); });
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WExternalFileBrowserAttribute>(), [](const WRTTI* pRtti)->WQtPropertyWidget* { return new WQtExternalFilePropertyWidget(); });
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WAssetBrowserAttribute>(), [](const WRTTI* pRtti)->WQtPropertyWidget* { return new WQtAssetPropertyWidget(); });
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WDynamicEnumAttribute>(), [](const WRTTI* pRtti)->WQtPropertyWidget* { return new WQtDynamicEnumPropertyWidget(); });
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WDynamicStringEnumAttribute>(), [](const WRTTI* pRtti)->WQtPropertyWidget* { return new WQtDynamicStringEnumPropertyWidget(); });
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WExposedParametersAttribute>(), [](const WRTTI* pRtti)->WQtPropertyWidget* { return new WQtExposedParametersPropertyWidget(); });
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WGameObjectReferenceAttribute>(), [](const WRTTI* pRtti)->WQtPropertyWidget* { return new WQtGameObjectReferencePropertyWidget(); });
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WExposedBone>(), [](const WRTTI* pRtti)->WQtPropertyWidget* { return new WQtExposedBoneWidget(); });
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WBlackboardCondition>(), [](const WRTTI* pRtti)->WQtPropertyWidget* { return new WQtBlackboardConditionWidget(); });
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WCompilerPreferences>(), [](const WRTTI* pRtti)->WQtPropertyWidget* { return new WQtCompilerPreferencesWidget(); });
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WCodeEditorPreferences>(), [](const WRTTI* pRtti)->WQtPropertyWidget* { return new WQtCodeEditorPreferencesWidget(); });
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WImageSliderUiAttribute>(), [](const WRTTI* pRtti)->WQtPropertyWidget* { return new WQtPropertyEditorSliderWidget(); });
    WQtPropertyGridWidget::GetFactory().RegisterCreator(WGetStaticRTTI<WRttiTypeStringAttribute>(), [](const WRTTI* pRtti)->WQtPropertyWidget* { return new WQtRttiTypeStringPropertyWidget(); });

    WManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(WGetStaticRTTI<WSphereManipulatorAttribute>(), [](const WRTTI* pRtti)->WManipulatorAdapter* { return W_DEFAULT_NEW(WSphereManipulatorAdapter); });
    WManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(WGetStaticRTTI<WCapsuleManipulatorAttribute>(), [](const WRTTI* pRtti)->WManipulatorAdapter* { return W_DEFAULT_NEW(WCapsuleManipulatorAdapter); });
    WManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(WGetStaticRTTI<WBoxManipulatorAttribute>(), [](const WRTTI* pRtti)->WManipulatorAdapter* { return W_DEFAULT_NEW(WBoxManipulatorAdapter); });
    WManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(WGetStaticRTTI<WConeAngleManipulatorAttribute>(), [](const WRTTI* pRtti)->WManipulatorAdapter* { return W_DEFAULT_NEW(WConeAngleManipulatorAdapter); });
    WManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(WGetStaticRTTI<WConeLengthManipulatorAttribute>(), [](const WRTTI* pRtti)->WManipulatorAdapter* { return W_DEFAULT_NEW(WConeLengthManipulatorAdapter); });
    WManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(WGetStaticRTTI<WNonUniformBoxManipulatorAttribute>(), [](const WRTTI* pRtti)->WManipulatorAdapter* { return W_DEFAULT_NEW(WNonUniformBoxManipulatorAdapter); });
    WManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(WGetStaticRTTI<WTransformManipulatorAttribute>(), [](const WRTTI* pRtti)->WManipulatorAdapter* { return W_DEFAULT_NEW(WTransformManipulatorAdapter); });
    WManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(WGetStaticRTTI<WBoneManipulatorAttribute>(), [](const WRTTI* pRtti)->WManipulatorAdapter* { return W_DEFAULT_NEW(WBoneManipulatorAdapter); });
    WManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(WGetStaticRTTI<WSplineManipulatorAttribute>(), [](const WRTTI* pRtti)->WManipulatorAdapter* { return W_DEFAULT_NEW(WSplineManipulatorAdapter); });
    WManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(WGetStaticRTTI<WSplineTangentManipulatorAttribute>(), [](const WRTTI* pRtti)->WManipulatorAdapter* { return W_DEFAULT_NEW(WSplineTangentManipulatorAdapter); });

    WVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(WGetStaticRTTI<WBoxVisualizerAttribute>(), [](const WRTTI* pRtti)->WVisualizerAdapter* { return W_DEFAULT_NEW(WBoxVisualizerAdapter); });
    WVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(WGetStaticRTTI<WSphereVisualizerAttribute>(), [](const WRTTI* pRtti)->WVisualizerAdapter* { return W_DEFAULT_NEW(WSphereVisualizerAdapter); });
    WVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(WGetStaticRTTI<WCapsuleVisualizerAttribute>(), [](const WRTTI* pRtti)->WVisualizerAdapter* { return W_DEFAULT_NEW(WCapsuleVisualizerAdapter); });
    WVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(WGetStaticRTTI<WCylinderVisualizerAttribute>(), [](const WRTTI* pRtti)->WVisualizerAdapter* { return W_DEFAULT_NEW(WCylinderVisualizerAdapter); });
    WVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(WGetStaticRTTI<WDirectionVisualizerAttribute>(), [](const WRTTI* pRtti)->WVisualizerAdapter* { return W_DEFAULT_NEW(WDirectionVisualizerAdapter); });
    WVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(WGetStaticRTTI<WConeVisualizerAttribute>(), [](const WRTTI* pRtti)->WVisualizerAdapter* { return W_DEFAULT_NEW(WConeVisualizerAdapter); });
    WVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(WGetStaticRTTI<WCameraVisualizerAttribute>(), [](const WRTTI* pRtti)->WVisualizerAdapter* { return W_DEFAULT_NEW(WCameraVisualizerAdapter); });
    WVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(WGetStaticRTTI<WPositionVisualizerAttribute>(), [](const WRTTI* pRtti)->WVisualizerAdapter* { return W_DEFAULT_NEW(WPositionVisualizerAdapter); });

    WPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(WCompilerPreferences_PropertyMetaStateEventHandler);
    WPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(WCodeEditorPreferences_PropertyMetaStateEventHandler);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WDefaultState::UnregisterDefaultStateProvider(WExposedParametersAsTypeDefaultStateProvider::CreateProvider);
    WDefaultState::UnregisterDefaultStateProvider(WExposedParametersDefaultStateProvider::CreateProvider);
    WDefaultState::UnregisterDefaultStateProvider(WDynamicDefaultStateProvider::CreateProvider);
    WProjectActions::UnregisterActions();
    WAssetActions::UnregisterActions();
    WAssetBrowserContextMenu::UnregisterActions();
    WViewActions::UnregisterActions();
    WViewLightActions::UnregisterActions();
    WGameObjectContextActions::UnregisterActions();
    WGameObjectDocumentActions::UnregisterActions();
    WGameObjectSelectionActions::UnregisterActions();
    WQuadViewActions::UnregisterActions();
    WTransformGizmoActions::UnregisterActions();
    WTranslateGizmoAction::UnregisterActions();
    WCommonAssetActions::UnregisterActions();
    WCameraModeSwitchActions::UnregisterActions();
    WWindowLayoutActions::UnregisterActions();

    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WFileBrowserAttribute>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WExternalFileBrowserAttribute>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WAssetBrowserAttribute>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WDynamicEnumAttribute>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WDynamicStringEnumAttribute>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WGameObjectReferenceAttribute>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WExposedParametersAttribute>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WExposedBone>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WBlackboardCondition>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WCompilerPreferences>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WCodeEditorPreferences>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WImageSliderUiAttribute>());
    WQtPropertyGridWidget::GetFactory().UnregisterCreator(WGetStaticRTTI<WRttiTypeStringAttribute>());

    WManipulatorAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(WGetStaticRTTI<WSphereManipulatorAttribute>());
    WManipulatorAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(WGetStaticRTTI<WCapsuleManipulatorAttribute>());
    WManipulatorAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(WGetStaticRTTI<WBoxManipulatorAttribute>());
    WManipulatorAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(WGetStaticRTTI<WConeAngleManipulatorAttribute>());
    WManipulatorAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(WGetStaticRTTI<WConeLengthManipulatorAttribute>());
    WManipulatorAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(WGetStaticRTTI<WNonUniformBoxManipulatorAttribute>());
    WManipulatorAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(WGetStaticRTTI<WTransformManipulatorAttribute>());
    WManipulatorAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(WGetStaticRTTI<WBoneManipulatorAttribute>());

    WVisualizerAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(WGetStaticRTTI<WBoxVisualizerAttribute>());
    WVisualizerAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(WGetStaticRTTI<WSphereVisualizerAttribute>());
    WVisualizerAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(WGetStaticRTTI<WCapsuleVisualizerAttribute>());
    WVisualizerAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(WGetStaticRTTI<WCylinderVisualizerAttribute>());
    WVisualizerAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(WGetStaticRTTI<WDirectionVisualizerAttribute>());
    WVisualizerAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(WGetStaticRTTI<WConeVisualizerAttribute>());
    WVisualizerAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(WGetStaticRTTI<WCameraVisualizerAttribute>());

    WPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(WCompilerPreferences_PropertyMetaStateEventHandler);
    WPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(WCodeEditorPreferences_PropertyMetaStateEventHandler);
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

WCommandLineOptionBool opt_Safe("_Editor", "-safe", "In safe-mode the editor minimizes the risk of crashing, for instance by not loading previous projects and scenes.", false);
WCommandLineOptionBool opt_Dashboard("_Editor", "-dashboard", "Starts the editor without loading the last project and its documents, so that the dashboard is shown instead.", false);

/// Tells the editor that no user is present to interact with it.
///
/// Questions are not answered with their *safest* option but with the one that continues as if a user had
/// let the operation proceed, since an editor that is only half-started is of no use to an automated caller.
/// Notably safe mode is declined during startup, because it would prevent the project from loading at all.
/// Use '-safe' to actually get safe mode.
WCommandLineOptionBool opt_Unattended("_Editor", "-unattended",
  "Runs the editor without a user present, e.g. driven by a script or an AI agent.\n"
  "Nothing modal is displayed, questions are answered with the option that lets the operation continue.\n"
  "Without this, a dialog that nobody closes blocks the editor indefinitely.",
  false);

WCommandLineOptionPath opt_CreateProject("_Editor", "-createProject",
  "Creates a new project in the given (absolute) directory and opens it.\n"
  "\n"
  "The directory must not exist yet or must be empty. Use -projectTemplate to create the project from a project\n"
  "template, and -pluginTemplate to choose which plugins a project without a project template starts with.\n"
  "\n"
  "Example:\n"
  "  -createProject \"C:/Projects/MyGame\" -projectTemplate \"Basic FPS\"\n",
  "");

WCommandLineOptionString opt_ProjectTemplate("_Editor", "-projectTemplate",
  "Name of the project template that -createProject copies, e.g. 'Basic FPS'.\n"
  "\n"
  "Without this, a blank project is created. Use -listTemplates to see which templates exist.\n"
  "A project template brings its own plugin selection, so -pluginTemplate is ignored when this is given.",
  "");

WCommandLineOptionString opt_PluginTemplate("_Editor", "-pluginTemplate",
  "Name of the plugin template that -createProject enables in a blank project, e.g. 'General3D'.\n"
  "\n"
  "Only used when no -projectTemplate is given. Use -listTemplates to see which templates exist.",
  "General3D");

WCommandLineOptionBool opt_ListTemplates("_Editor", "-listTemplates",
  "Logs the available project templates and plugin templates for -createProject, then continues as usual.", false);

void WQtEditorApp::LogAvailableTemplates()
{
  DetectAvailablePluginBundles(WOSFile::GetApplicationDirectory());

  {
    WDynamicArray<WString> templates;
    WProjectCreation::FindProjectTemplates(templates);

    WLog::Info("Project templates ({}):", templates.GetCount());
    WLog::Info("  <none> (blank project)");

    for (const WString& sName : templates)
    {
      WLog::Info("  {}", sName);
    }
  }

  {
    WDynamicArray<WString> templates;
    WProjectCreation::FindPluginTemplates(GetPluginBundles(), templates);

    WLog::Info("Plugin templates ({}):", templates.GetCount());

    for (const WString& sName : templates)
    {
      WLog::Info("  {}", sName);
    }
  }
}

WResult WQtEditorApp::CreateProjectFromCommandLine(WStringView sTargetDirectory)
{
  // the bundles have to be known before a plugin selection can be written for a blank project
  DetectAvailablePluginBundles(WOSFile::GetApplicationDirectory());

  WProjectCreationOptions options;
  options.m_sTargetDirectory = sTargetDirectory;
  options.m_sProjectTemplate = opt_ProjectTemplate.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified);
  options.m_sPluginTemplate = opt_PluginTemplate.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified);

  const WStatus res = WProjectCreation::CreateProject(options, GetPluginBundles());

  if (res.Failed())
  {
    WLog::Error("Failed to create project '{}': {}", sTargetDirectory, res.GetMessageString());
    return W_FAILURE;
  }

  WLog::Success("Created project '{}'.", sTargetDirectory);
  return W_SUCCESS;
}

void WQtEditorApp::SetupSilentAsserts()
{
  WEnvironmentVariableUtils::SetValueInt("W_SILENT_ASSERTS", 1).IgnoreResult();
}

void WQtEditorApp::StartupEditor()
{
  // Has to happen before anything below can ask a question - the startup flags, which the other
  // StartupEditor() overload derives this from, are only set further down.
  const bool bUnattended = opt_Unattended.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified);
  if (bUnattended)
  {
    WQtUiServices::SetUnattended();
    SetupSilentAsserts();
  }

  if (!bUnattended)
  {
    WStringBuilder sTemp = WOSFile::GetTempDataFolder("WEditor");
    sTemp.AppendPath("WEditorCrashIndicator");

    if (WOSFile::ExistsFile(sTemp))
    {
      WOSFile::DeleteFile(sTemp).IgnoreResult();

      // Unattended this is declined: the indicator file is left behind by any hard process termination,
      // so an automated caller that kills a stuck editor would otherwise get safe mode at every
      // subsequent launch, which prevents the project from loading at all. Use '-safe' to really get it.
      if (WQtUiServices::GetSingleton()->MessageBoxQuestion("It seems the editor ran into problems last time.\n\nDo you want to run it in safe mode, to deactivate automatic project loading and document restoration?", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::No) == QMessageBox::StandardButton::Yes)
      {
        opt_Safe.GetOptions(sTemp);
        WCommandLineUtils::GetGlobalInstance()->InjectCustomArgument(sTemp);
      }
    }
  }

  WBitflags<StartupFlags> startupFlags;

  startupFlags.AddOrRemove(StartupFlags::SafeMode, opt_Safe.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified));
  startupFlags.AddOrRemove(StartupFlags::Dashboard, opt_Dashboard.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified));
  startupFlags.AddOrRemove(StartupFlags::Unattended, bUnattended);

  StartupEditor(startupFlags);
}

void WQtEditorApp::StartupEditor(WBitflags<StartupFlags> startupFlags, const char* szUserDataFolder)
{
  W_PROFILE_SCOPE("StartupEditor");

  QCoreApplication::setOrganizationDomain("www.ezengine.net");
  QCoreApplication::setOrganizationName("WorldEngine Project");
  QCoreApplication::setApplicationName(WApplication::GetApplicationInstance()->GetApplicationName().GetData());
  QGuiApplication::setApplicationDisplayName(QString::fromUtf8("寰宇引擎"));
  QCoreApplication::setApplicationVersion("1.0.0");

  m_StartupFlags = startupFlags;

  if (IsInUnattendedMode())
  {
    WQtUiServices::SetUnattended();
    SetupSilentAsserts();
  }

  auto* pCmd = WCommandLineUtils::GetGlobalInstance();

  if (!IsInHeadlessMode())
  {
    SetupAndShowSplashScreen();

    m_pProgressbar = W_DEFAULT_NEW(WProgress);
    m_pQtProgressbar = W_DEFAULT_NEW(WQtProgressbar);

    WProgress::SetGlobalProgressbar(m_pProgressbar);
    m_pQtProgressbar->SetProgressbar(m_pProgressbar);
  }

  // custom command line arguments
  {
    // Make sure to disable the fileserve plugin
    pCmd->InjectCustomArgument("-fs_off");
  }

  const bool bNoRestore = m_StartupFlags.IsAnySet(StartupFlags::UnitTest | StartupFlags::SafeMode | StartupFlags::Headless | StartupFlags::Dashboard);

  const WString sApplicationName = pCmd->GetStringOption("-appname", 0, WApplication::GetApplicationInstance()->GetApplicationName());
  WApplication::GetApplicationInstance()->SetApplicationName(sApplicationName);

  QLocale::setDefault(QLocale(QLocale::English));

  m_pEngineViewProcess = new WEditorEngineProcessConnection;

  m_LongOpControllerManager.Startup(&m_pEngineViewProcess->GetCommunicationChannel());

  if (!IsInHeadlessMode())
  {
    W_PROFILE_SCOPE("WQtContainerWindow");
    SetStyleSheet();

    WQtContainerWindow* pContainer = new WQtContainerWindow();
    pContainer->show();
  }

  WDocumentManager::s_Requests.AddEventHandler(WMakeDelegate(&WQtEditorApp::DocumentManagerRequestHandler, this));
  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WQtEditorApp::DocumentManagerEventHandler, this));
  WDocument::s_EventsAny.AddEventHandler(WMakeDelegate(&WQtEditorApp::DocumentEventHandler, this));
  WToolsProject::s_Requests.AddEventHandler(WMakeDelegate(&WQtEditorApp::ProjectRequestHandler, this));
  WToolsProject::s_Events.AddEventHandler(WMakeDelegate(&WQtEditorApp::ProjectEventHandler, this));
  WEditorEngineProcessConnection::s_Events.AddEventHandler(WMakeDelegate(&WQtEditorApp::EngineProcessMsgHandler, this));
  WQtUiServices::s_Events.AddEventHandler(WMakeDelegate(&WQtEditorApp::UiServicesEvents, this));

  WStartup::StartupCoreSystems();

  {
    // Make sure that we have at least 4 worker threads for short running and 4 worker threads for long running tasks.
    // Otherwise the Editor might deadlock during asset transform.
    WInt32 iLongThreads = WMath::Max(4, (WInt32)WTaskSystem::GetNumAllocatedWorkerThreads(WWorkerThreadType::LongTasks));
    WInt32 iShortThreads = WMath::Max(4, (WInt32)WTaskSystem::GetNumAllocatedWorkerThreads(WWorkerThreadType::ShortTasks));
    WTaskSystem::SetWorkerThreadCount(iShortThreads, iLongThreads);
  }

  {
    W_PROFILE_SCOPE("Filesystem");
    WFileSystem::DetectSdkRootDirectory().IgnoreResult();

    const WString sAppDir = WApplicationServices::GetSingleton()->GetApplicationDataFolder();
    WString sUserData = WApplicationServices::GetSingleton()->GetApplicationUserDataFolder();
    if (!WStringUtils::IsNullOrEmpty(szUserDataFolder))
    {
      sUserData = szUserDataFolder;
    }
    // make sure these folders exist
    WFileSystem::CreateDirectoryStructure(sAppDir).IgnoreResult();
    WFileSystem::CreateDirectoryStructure(sUserData).IgnoreResult();

    WFileSystem::AddDataDirectory("", "AbsPaths", ":", WDataDirUsage::AllowWrites).IgnoreResult();             // for absolute paths
    WFileSystem::AddDataDirectory(">appdir/", "AppBin", "bin", WDataDirUsage::AllowWrites).IgnoreResult();     // writing to the binary directory
    WFileSystem::AddDataDirectory(sAppDir, "AppData", "app").IgnoreResult();                                    // app specific data
    WFileSystem::AddDataDirectory(sUserData, "AppData", "appdata", WDataDirUsage::AllowWrites).IgnoreResult(); // for writing app user data
  }

  {
    W_PROFILE_SCOPE("Logging");
    WInt32 iApplicationID = pCmd->GetIntOption("-appid", 0);
    WStringBuilder sLogFile;
    if (m_StartupFlags.IsSet(StartupFlags::Background))
      sLogFile.SetFormat(":appdata/Logs/LogEditorProcessor_{0}.htm", iApplicationID);
    else
      sLogFile.SetFormat(":appdata/Logs/LogEditor_{0}.htm", iApplicationID);

    m_LogHTML.BeginLog(sLogFile, sApplicationName);

    WGlobalLog::AddLogWriter(WLogWriter::Console::LogMessageHandler);
    WGlobalLog::AddLogWriter(WLogWriter::VisualStudio::LogMessageHandler);
    WGlobalLog::AddLogWriter(WLoggingEvent::Handler(&WLogWriter::HTML::LogMessageHandler, &m_LogHTML));
    WGlobalLog::AddLogWriter(WLogWriter::Tracing::LogMessageHandler);
  }
  WUniquePtr<WTranslatorFromFiles> pTranslatorEn = W_DEFAULT_NEW(WTranslatorFromFiles);
  m_pTranslatorFromFiles = pTranslatorEn.Borrow();

  // WUniquePtr<WTranslatorFromFiles> pTranslatorDe = W_DEFAULT_NEW(WTranslatorFromFiles);

  pTranslatorEn->AddTranslationFilesFromFolder(":app/Localization/en");
  // pTranslatorDe->LoadTranslationFilesFromFolder(":app/Localization/de");

  WTranslationLookup::AddTranslator(W_DEFAULT_NEW(WTranslatorMakeMoreReadable));
  // WTranslationLookup::AddTranslator(W_DEFAULT_NEW(WTranslatorLogMissing));
  WTranslationLookup::AddTranslator(std::move(pTranslatorEn));
  // WTranslationLookup::AddTranslator(std::move(pTranslatorDe));

  LoadEditorPreferences();
  WCppProject::LoadPreferences();

  WQtUiServices::GetSingleton()->LoadState();

  if (!IsInHeadlessMode())
  {
    WActionManager::LoadShortcutAssignment();

    LoadRecentFiles();

    ShowSettingsDocument();

    CreatePanels();

    if (!IsInUnitTestMode())
    {
      connect(m_pVersionChecker.Borrow(), &WQtVersionChecker::VersionCheckCompleted, this, &WQtEditorApp::SlotVersionCheckCompleted, Qt::QueuedConnection);

      m_pVersionChecker->Initialize();
      m_pVersionChecker->Check(false);
    }
  }

  LoadEditorPlugins();

  if (!IsInHeadlessMode() && !IsInSafeMode())
  {
    WWindowLayoutActions::RestoreUserLayout();
  }

  CloseSplashScreen();

  m_bIsRunning = true;
  {
    WEditorAppEvent e;
    e.m_Type = WEditorAppEvent::Type::EditorStarted;
    m_Events.Broadcast(e);
  }


  WEditorPreferencesUser* pPreferences = WPreferences::QueryPreferences<WEditorPreferencesUser>();

  if (opt_ListTemplates.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified))
  {
    LogAvailableTemplates();
  }

  const WString sCreateProject = opt_CreateProject.GetOptionValue(WCommandLineOption::LogMode::AlwaysIfSpecified);

  if (!sCreateProject.IsEmpty())
  {
    if (CreateProjectFromCommandLine(sCreateProject).Succeeded())
    {
      // a project template already brings an 'WProject' file, a blank project does not, and only the
      // 'create' path writes one - so which one this is decides how the new project has to be opened
      WStringBuilder sProjectFile = sCreateProject;
      sProjectFile.AppendPath("WProject");

      CreateOrOpenProject(!WOSFile::ExistsFile(sProjectFile), sProjectFile).IgnoreResult();
    }
  }
  else if (pCmd->GetStringOptionArguments("-newproject") > 0)
  {
    CreateOrOpenProject(true, pCmd->GetAbsolutePathOption("-newproject")).IgnoreResult();
  }
  else if (pCmd->GetStringOptionArguments("-project") > 0)
  {
    for (WUInt32 doc = 0; doc < pCmd->GetStringOptionArguments("-documents"); ++doc)
    {
      m_DocumentsToOpen.PushBack(pCmd->GetStringOption("-documents", doc));
    }

    CreateOrOpenProject(false, pCmd->GetAbsolutePathOption("-project")).IgnoreResult();
  }
  else if (!bNoRestore && pPreferences->m_bLoadLastProjectAtStartup)
  {
    if (!m_RecentProjects.GetFileList().IsEmpty())
    {
      CreateOrOpenProject(false, m_RecentProjects.GetFileList()[0].m_File).IgnoreResult();
    }
  }
  else if (!IsInHeadlessMode())
  {
    // Show the window maximized when no project is being loaded
    if (WQtContainerWindow::GetContainerWindow())
    {
      WQtContainerWindow::GetContainerWindow()->showMaximized();
    }
  }

  if (!IsInHeadlessMode())
  {
    // Now that all plugins have been loaded, populate the asset check rules
    WQtAssetCheckPanel::GetSingleton()->FillRuleList();
  }

  connect(m_pTimer, SIGNAL(timeout()), this, SLOT(SlotTimedUpdate()), Qt::QueuedConnection);
  m_pTimer->start(1);

  // Setup auto-save timer - polls every 20 seconds and checks document modification age
  connect(m_pAutoSaveTimer, SIGNAL(timeout()), this, SLOT(SlotAutoSave()), Qt::QueuedConnection);
  m_pAutoSaveTimer->start(20 * 1000);

  // The docking system (ADS) sometimes leaves a resize cursor stuck after layout restore.
  QTimer::singleShot(500, []()
    { while (QApplication::overrideCursor()) { QApplication::restoreOverrideCursor(); } });

  if (m_bWroteCrashIndicatorFile)
  {
    QTimer::singleShot(1000, [this]()
      {
        WStringBuilder sTemp = WOSFile::GetTempDataFolder("WEditor");
        sTemp.AppendPath("WEditorCrashIndicator");
        WOSFile::DeleteFile(sTemp).IgnoreResult();
        m_bWroteCrashIndicatorFile = false;
        //
      });
  }

  if (m_StartupFlags.AreNoneSet(StartupFlags::Headless | StartupFlags::UnitTest) && !WToolsProject::GetSingleton()->IsProjectOpen())
  {
    GuiOpenDashboard();
  }

  WStackTraceLogParser::Register();
}

void WQtEditorApp::ShutdownEditor()
{
  m_bIsRunning = false;
  WStackTraceLogParser::Unregister();

  // WToolsProject::SaveProjectState();

  m_pTimer->stop();

  WToolsProject::CloseProject();

  m_LongOpControllerManager.Shutdown();

  WEditorEngineProcessConnection::s_Events.RemoveEventHandler(WMakeDelegate(&WQtEditorApp::EngineProcessMsgHandler, this));
  WToolsProject::s_Requests.RemoveEventHandler(WMakeDelegate(&WQtEditorApp::ProjectRequestHandler, this));
  WToolsProject::s_Events.RemoveEventHandler(WMakeDelegate(&WQtEditorApp::ProjectEventHandler, this));
  WDocument::s_EventsAny.RemoveEventHandler(WMakeDelegate(&WQtEditorApp::DocumentEventHandler, this));
  WDocumentManager::s_Requests.RemoveEventHandler(WMakeDelegate(&WQtEditorApp::DocumentManagerRequestHandler, this));
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(&WQtEditorApp::DocumentManagerEventHandler, this));
  WQtUiServices::s_Events.RemoveEventHandler(WMakeDelegate(&WQtEditorApp::UiServicesEvents, this));

  WQtUiServices::GetSingleton()->SaveState();

  CloseSettingsDocument();

  if (!IsInHeadlessMode())
  {
    delete WQtContainerWindow::GetContainerWindow();
  }
  // HACK to figure out why the panels are not always properly destroyed together with the ContainerWindows
  // if you run into this, please try to figure this out
  // every WQtApplicationPanel actually registers itself with a container window in its constructor
  // there its Qt 'parent' is set to the container window (there is only one)
  // that means, when the application is shut down, all WQtApplicationPanel instances should get deleted by their parent
  // ie. the container window
  // however, SOMETIMES this does not happen
  // it seems to be related to whether a panel has been opened/closed (ie. shown/hidden), and maybe also with the restored state
  {
    const auto& Panels = WQtApplicationPanel::GetAllApplicationPanels();
    WUInt32 uiNumPanels = Panels.GetCount();

    W_ASSERT_DEBUG(uiNumPanels == 0, "Not all panels have been cleaned up correctly");

    for (WUInt32 i = 0; i < uiNumPanels; ++i)
    {
      WQtApplicationPanel* pPanel = Panels[i];
      delete pPanel;
    }
  }


  QCoreApplication::sendPostedEvents();
  qApp->processEvents();

  delete m_pEngineViewProcess;

  // Unload potential plugin referenced clipboard data to prevent crash on shutdown.
  QApplication::clipboard()->clear();
  WPlugin::UnloadAllPlugins();

  if (m_bWroteCrashIndicatorFile)
  {
    // orderly shutdown -> make sure the crash indicator file is gone
    WStringBuilder sTemp = WOSFile::GetTempDataFolder("WEditor");
    sTemp.AppendPath("WEditorCrashIndicator");
    WOSFile::DeleteFile(sTemp).IgnoreResult();
    m_bWroteCrashIndicatorFile = false;
  }

  // make sure no one tries to load any further images in parallel
  WQtImageCache::GetSingleton()->StopRequestProcessing(true);

  WTranslationLookup::Clear();

  WGlobalLog::RemoveLogWriter(WLogWriter::Console::LogMessageHandler);
  WGlobalLog::RemoveLogWriter(WLogWriter::VisualStudio::LogMessageHandler);
  WGlobalLog::RemoveLogWriter(WLoggingEvent::Handler(&WLogWriter::HTML::LogMessageHandler, &m_LogHTML));
  m_LogHTML.EndLog();

  W_DEFAULT_DELETE(m_pQtProgressbar);
  W_DEFAULT_DELETE(m_pProgressbar);
}

void WQtEditorApp::CreatePanels()
{
  W_PROFILE_SCOPE("CreatePanels");

  WQtContainerWindow* pMainWnd = WQtContainerWindow::GetContainerWindow();
  ads::CDockManager* pDockManager = pMainWnd->GetDockManager();

  WQtApplicationPanel* pAssetBrowserPanel = new WQtAssetBrowserPanel(pDockManager);
  WQtApplicationPanel* pAssetCuratorPanel = new WQtAssetCuratorPanel(pDockManager);
  WQtApplicationPanel* pLogPanel = new WQtLogPanel(pDockManager);
  WQtApplicationPanel* pCVarPanel = new WQtCVarPanel(pDockManager);
  WQtApplicationPanel* pLongOpsPanel = new WQtLongOpsPanel(pDockManager);
  WQtApplicationPanel* pAssetCheckPanel = new WQtAssetCheckPanel(pDockManager);

  pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pAssetBrowserPanel);
  pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pAssetCuratorPanel);
  pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pLogPanel);
  pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pCVarPanel);
  pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pLongOpsPanel);
  pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pAssetCheckPanel);

  // by default these panels can be hidden
  pCVarPanel->toggleView(false);
  pLongOpsPanel->toggleView(false);
  pAssetCheckPanel->toggleView(false);

  pAssetBrowserPanel->raise();
}

WCommandLineOptionBool opt_NoSplashScreen("_Editor", "-NoSplash", "Disables the editor splash-screen", false);

void WQtEditorApp::SetupAndShowSplashScreen()
{
  W_ASSERT_DEV(m_pSplashScreen == nullptr, "Splash screen shouldn't exist already.");

  if (m_StartupFlags.IsAnySet(WQtEditorApp::StartupFlags::UnitTest))
    return;

  if (opt_NoSplashScreen.GetOptionValue(WCommandLineOption::LogMode::Never))
    return;

  bool bShowSplashScreen = true;

  // preferences are not yet available here
  {
    QSettings s;
    s.beginGroup("EditorPreferences");
    bShowSplashScreen = s.value("ShowSplashscreen", true).toBool();
    s.endGroup();
  }

  if (!bShowSplashScreen)
    return;

  // QSvgRenderer svgRenderer(QString(":/Splash/Splash/splash.svg"));

  // const qreal PixelRatio = qApp->primaryScreen()->devicePixelRatio();

  //// TODO: When migrating to Qt 5.15 or newer this should have a fixed square size and
  //// let the aspect ratio mode of the svg renderer handle the difference
  // QPixmap splashPixmap(QSize(187, 256) * PixelRatio);
  // splashPixmap.fill(Qt::transparent);
  //{
  //   QPainter painter;
  //   painter.begin(&splashPixmap);
  //   svgRenderer.render(&painter);
  //   painter.end();
  // }

  QPixmap splashPixmap(QString(":/Splash/Splash/splash.png"));

  // splashPixmap.setDevicePixelRatio(PixelRatio);

  m_pSplashScreen = new QSplashScreen(splashPixmap);
  m_pSplashScreen->setMask(splashPixmap.mask());

  // Don't set always on top if a debugger is attached to prevent it being stuck over the debugger.
  if (!WSystemInformation::IsDebuggerAttached())
  {
    m_pSplashScreen->setWindowFlag(Qt::WindowStaysOnTopHint, true);
  }
  m_pSplashScreen->show();
}

void WQtEditorApp::CloseSplashScreen()
{
  if (!m_pSplashScreen)
    return;

  W_ASSERT_DEBUG(QThread::currentThread() == this->thread(), "CloseSplashScreen must be called from the main thread");
  QSplashScreen* pLocalSplashScreen = m_pSplashScreen;
  m_pSplashScreen = nullptr;

  pLocalSplashScreen->finish(WQtContainerWindow::GetContainerWindow());
  // if the deletion is done 'later', the splashscreen can end up as the parent window of other things
  // like messageboxes, and then the deletion will make the app crash
  delete pLocalSplashScreen;
}
