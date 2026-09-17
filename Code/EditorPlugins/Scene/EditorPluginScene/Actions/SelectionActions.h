#pragma once

#include <EditorPluginScene/EditorPluginSceneDLL.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

///
class W_EDITORPLUGINSCENE_DLL WSelectionActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(WStringView sMapping);
  static void MapPrefabActions(WStringView sMapping, float fPriority);
  static void MapContextMenuActions(WStringView sMapping);
  static void MapViewContextMenuActions(WStringView sMapping);

  static WActionDescriptorHandle s_hGroupSelectedItems;
  static WActionDescriptorHandle s_hCreateEmptyChildObject;
  static WActionDescriptorHandle s_hCreateEmptyObjectAtPosition;
  static WActionDescriptorHandle s_hHideSelectedObjects;
  static WActionDescriptorHandle s_hHideUnselectedObjects;
  static WActionDescriptorHandle s_hShowHiddenObjects;
  static WActionDescriptorHandle s_hPrefabMenu;
  static WActionDescriptorHandle s_hCreatePrefab;
  static WActionDescriptorHandle s_hRevertPrefab;
  static WActionDescriptorHandle s_hUnlinkFromPrefab;
  static WActionDescriptorHandle s_hOpenPrefabDocument;
  static WActionDescriptorHandle s_hDuplicateSpecial;
  static WActionDescriptorHandle s_hDeltaTransform;
  static WActionDescriptorHandle s_hSnapObjectToCamera;
  static WActionDescriptorHandle s_hAttachToObject;
  static WActionDescriptorHandle s_hDetachFromParent;
  static WActionDescriptorHandle s_hConvertToEnginePrefab;
  static WActionDescriptorHandle s_hConvertToEditorPrefab;
  static WActionDescriptorHandle s_hCopyReference;
  static WActionDescriptorHandle s_hSelectParent;
  static WActionDescriptorHandle s_hSetActiveParent;
  static WActionDescriptorHandle s_hClearActiveParent;
  static WActionDescriptorHandle s_hUndoSelection;
};

///
class W_EDITORPLUGINSCENE_DLL WSelectionAction : public WButtonAction
{
  W_ADD_DYNAMIC_REFLECTION(WSelectionAction, WButtonAction);

public:
  enum class ActionType
  {
    GroupSelectedItems,
    CreateEmptyChildObject,
    CreateEmptyObjectAtPosition,
    HideSelectedObjects,
    HideUnselectedObjects,
    ShowHiddenObjects,

    CreatePrefab,
    RevertPrefab,
    UnlinkFromPrefab,
    OpenPrefabDocument,
    ConvertToEnginePrefab,
    ConvertToEditorPrefab,

    DuplicateSpecial,
    DeltaTransform,
    SnapObjectToCamera,
    AttachToObject,
    DetachFromParent,
    CopyReference,
    SelectParent,

    SetActiveParent,
    ClearActiveParent,

    UndoSelection,
  };

  WSelectionAction(const WActionContext& context, const char* szName, ActionType type);
  ~WSelectionAction();

  virtual void Execute(const WVariant& value) override;

  void OpenPrefabDocument();

  void CreatePrefab();

private:
  void SelectionEventHandler(const WSelectionManagerEvent& e);

  void UpdateEnableState();

  WSceneDocument* m_pSceneDocument;
  ActionType m_Type;
};
