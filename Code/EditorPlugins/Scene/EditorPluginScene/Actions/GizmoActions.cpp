#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Actions/TransformGizmoActions.h>
#include <EditorPluginScene/Actions/GizmoActions.h>
#include <EditorPluginScene/EditTools/GreyBoxEditTool.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>

WActionDescriptorHandle WSceneGizmoActions::s_hGreyBoxingGizmo;

void WSceneGizmoActions::RegisterActions()
{
  s_hGreyBoxingGizmo = W_REGISTER_ACTION_1("Gizmo.Mode.GreyBoxing", WActionScope::Document, "Gizmo", "B", WGizmoAction, WGetStaticRTTI<WGreyBoxEditTool>());
}

void WSceneGizmoActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hGreyBoxingGizmo);
}

void WSceneGizmoActions::MapMenuActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hGreyBoxingGizmo, "G.Gizmos", 5.0f);
}

void WSceneGizmoActions::MapToolbarActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  const WStringView sSubPath("GizmoCategory");
  pMap->MapAction(s_hGreyBoxingGizmo, sSubPath, 5.0f);
}
