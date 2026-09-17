#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class WGameObjectDocument;

///
class W_EDITORFRAMEWORK_DLL WGameObjectSelectionActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(WStringView sMapping);
  static void MapContextMenuActions(WStringView sMapping);
  static void MapViewContextMenuActions(WStringView sMapping);

  static WActionDescriptorHandle s_hSelectionCategory;
  static WActionDescriptorHandle s_hShowInScenegraph;
  static WActionDescriptorHandle s_hFocusOnSelection;
  static WActionDescriptorHandle s_hFocusOnSelectionAllViews;
  static WActionDescriptorHandle s_hSnapCameraToObject;
  static WActionDescriptorHandle s_hMoveCameraHere;
};

///
class W_EDITORFRAMEWORK_DLL WGameObjectSelectionAction : public WButtonAction
{
  W_ADD_DYNAMIC_REFLECTION(WGameObjectSelectionAction, WButtonAction);

public:
  enum class ActionType
  {
    ShowInScenegraph,
    FocusOnSelection,
    FocusOnSelectionAllViews,
    SnapCameraToObject,
    MoveCameraHere,
  };

  WGameObjectSelectionAction(const WActionContext& context, const char* szName, ActionType type);
  ~WGameObjectSelectionAction();

  virtual void Execute(const WVariant& value) override;

private:
  void SelectionEventHandler(const WSelectionManagerEvent& e);

  void UpdateEnableState();

  WGameObjectDocument* m_pSceneDocument;
  ActionType m_Type;
};
