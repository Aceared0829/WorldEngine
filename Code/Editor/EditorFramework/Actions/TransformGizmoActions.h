#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

enum class ActiveGizmo;
class WGameObjectDocument;
struct WSnapProviderEvent;
struct WGameObjectEvent;

///
class W_EDITORFRAMEWORK_DLL WTransformGizmoActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions(WStringView sMapping);
  static void MapToolbarActions(WStringView sMapping);

  static WActionDescriptorHandle s_hGizmoCategory;
  static WActionDescriptorHandle s_hGizmoMenu;
  static WActionDescriptorHandle s_hNoGizmo;
  static WActionDescriptorHandle s_hTranslateGizmo;
  static WActionDescriptorHandle s_hRotateGizmo;
  static WActionDescriptorHandle s_hScaleGizmo;
  static WActionDescriptorHandle s_hDragToPositionGizmo;
  static WActionDescriptorHandle s_hWorldSpace;
  static WActionDescriptorHandle s_hMoveParentOnly;
  static WActionDescriptorHandle s_SnapSettings;
  static WActionDescriptorHandle s_SnapTranslationMenu;
  static WActionDescriptorHandle s_SnapRotationMenu;
  static WActionDescriptorHandle s_SnapScaleMenu;
};

///
class W_EDITORFRAMEWORK_DLL WGizmoAction : public WButtonAction
{
  W_ADD_DYNAMIC_REFLECTION(WGizmoAction, WButtonAction);

public:
  WGizmoAction(const WActionContext& context, const char* szName, const WRTTI* pGizmoType);
  ~WGizmoAction();

  virtual void Execute(const WVariant& value) override;

protected:
  void UpdateState();
  void GameObjectEventHandler(const WGameObjectEvent& e);

  WGameObjectDocument* m_pGameObjectDocument = nullptr;
  const WRTTI* m_pGizmoType = nullptr;
};

///
class W_EDITORFRAMEWORK_DLL WToggleWorldSpaceGizmo : public WGizmoAction
{
public:
  WToggleWorldSpaceGizmo(const WActionContext& context, const char* szName, const WRTTI* pGizmoType);
  virtual void Execute(const WVariant& value) override;
};

///
class W_EDITORFRAMEWORK_DLL WTransformGizmoAction : public WButtonAction
{
  W_ADD_DYNAMIC_REFLECTION(WTransformGizmoAction, WButtonAction);

public:
  enum class ActionType
  {
    GizmoToggleWorldSpace,
    GizmoToggleMoveParentOnly,
    GizmoSnapSettings,
  };

  WTransformGizmoAction(const WActionContext& context, const char* szName, ActionType type);
  ~WTransformGizmoAction();

  virtual void Execute(const WVariant& value) override;
  void GameObjectEventHandler(const WGameObjectEvent& e);

private:
  void UpdateState();

  WGameObjectDocument* m_pGameObjectDocument;
  ActionType m_Type;
};

///
class W_EDITORFRAMEWORK_DLL WTranslateGizmoAction : public WButtonAction
{
  W_ADD_DYNAMIC_REFLECTION(WTranslateGizmoAction, WButtonAction);

public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(WStringView sMapping);

private:
  static WActionDescriptorHandle s_hSnappingValueMenu;
  static WActionDescriptorHandle s_hSnapPivotToGrid;
  static WActionDescriptorHandle s_hSnapObjectsToGrid;

public:
  enum class ActionType
  {
    SnapSelectionPivotToGrid,
    SnapEachSelectedObjectToGrid,
  };

  WTranslateGizmoAction(const WActionContext& context, const char* szName, ActionType type);

  virtual void Execute(const WVariant& value) override;

private:
  const WGameObjectDocument* m_pSceneDocument;
  ActionType m_Type;
};
