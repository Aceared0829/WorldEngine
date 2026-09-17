#pragma once

#include <EditorPluginScene/EditorPluginSceneDLL.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class WPreferences;

///
class W_EDITORPLUGINSCENE_DLL WSceneActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions(WStringView sMapping);
  static void MapToolbarActions(WStringView sMapping);
  static void MapViewContextMenuActions(WStringView sMapping);

  static WActionDescriptorHandle s_hSceneCategory;
  static WActionDescriptorHandle s_hSceneUtilsMenu;
  static WActionDescriptorHandle s_hExportScene;
  static WActionDescriptorHandle s_hGameModeSimulate;
  static WActionDescriptorHandle s_hGameModePlay;
  static WActionDescriptorHandle s_hGameModePlayFromHere;
  static WActionDescriptorHandle s_hGameModeStop;
  static WActionDescriptorHandle s_hGameModePause;
  static WActionDescriptorHandle s_hUtilExportSceneToOBJ;
  static WActionDescriptorHandle s_hKeepSimulationChanges;
  static WActionDescriptorHandle s_hCreateThumbnail;
  static WActionDescriptorHandle s_hFavoriteCamsMenu;
  static WActionDescriptorHandle s_hStoreEditorCamera[10];
  static WActionDescriptorHandle s_hRestoreEditorCamera[10];
  static WActionDescriptorHandle s_hJumpToCamera[10];
  static WActionDescriptorHandle s_hCreateLevelCamera[10];
};

///
class W_EDITORPLUGINSCENE_DLL WSceneAction : public WButtonAction
{
  W_ADD_DYNAMIC_REFLECTION(WSceneAction, WButtonAction);

public:
  enum class ActionType
  {
    ExportAndRunScene,
    StartGameModeSimulate,
    StartGameModePlay,
    StartGameModePlayFromHere,
    StopGameMode,
    PauseSimulation,
    ExportSceneToOBJ,
    KeepSimulationChanges,
    CreateThumbnail,

    StoreEditorCamera0,
    StoreEditorCamera1,
    StoreEditorCamera2,
    StoreEditorCamera3,
    StoreEditorCamera4,
    StoreEditorCamera5,
    StoreEditorCamera6,
    StoreEditorCamera7,
    StoreEditorCamera8,
    StoreEditorCamera9,

    RestoreEditorCamera0,
    RestoreEditorCamera1,
    RestoreEditorCamera2,
    RestoreEditorCamera3,
    RestoreEditorCamera4,
    RestoreEditorCamera5,
    RestoreEditorCamera6,
    RestoreEditorCamera7,
    RestoreEditorCamera8,
    RestoreEditorCamera9,

    JumpToCamera0,
    JumpToCamera1,
    JumpToCamera2,
    JumpToCamera3,
    JumpToCamera4,
    JumpToCamera5,
    JumpToCamera6,
    JumpToCamera7,
    JumpToCamera8,
    JumpToCamera9,

    CreateLevelCamera0,
    CreateLevelCamera1,
    CreateLevelCamera2,
    CreateLevelCamera3,
    CreateLevelCamera4,
    CreateLevelCamera5,
    CreateLevelCamera6,
    CreateLevelCamera7,
    CreateLevelCamera8,
    CreateLevelCamera9,
  };

  WSceneAction(const WActionContext& context, const char* szName, ActionType type);
  ~WSceneAction();

  virtual void Execute(const WVariant& value) override;

  void LaunchPlayer(const char* szPlayerApp);
  QStringList GetPlayerCommandLine(WStringBuilder& out_sSingleLine) const;

private:
  void SceneEventHandler(const WGameObjectEvent& e);
  void UpdateState();

  WSceneDocument* m_pSceneDocument;
  ActionType m_Type;
};
