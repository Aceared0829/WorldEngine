#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/GameObjectSelectionActions.h>
#include <EditorFramework/Document/GameObjectDocument.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WGameObjectSelectionAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WActionDescriptorHandle WGameObjectSelectionActions::s_hSelectionCategory;
WActionDescriptorHandle WGameObjectSelectionActions::s_hShowInScenegraph;
WActionDescriptorHandle WGameObjectSelectionActions::s_hFocusOnSelection;
WActionDescriptorHandle WGameObjectSelectionActions::s_hFocusOnSelectionAllViews;
WActionDescriptorHandle WGameObjectSelectionActions::s_hSnapCameraToObject;
WActionDescriptorHandle WGameObjectSelectionActions::s_hMoveCameraHere;

void WGameObjectSelectionActions::RegisterActions()
{
  s_hSelectionCategory = W_REGISTER_CATEGORY("G.Selection");
  s_hShowInScenegraph = W_REGISTER_ACTION_1("Selection.ShowInScenegraph", WActionScope::Document, "Scene - Selection", "Ctrl+T",
    WGameObjectSelectionAction, WGameObjectSelectionAction::ActionType::ShowInScenegraph);
  s_hFocusOnSelection = W_REGISTER_ACTION_1("Selection.FocusSingleView", WActionScope::Document, "Scene - Selection", "F",
    WGameObjectSelectionAction, WGameObjectSelectionAction::ActionType::FocusOnSelection);
  s_hFocusOnSelectionAllViews = W_REGISTER_ACTION_1("Selection.FocusAllViews", WActionScope::Document, "Scene - Selection", "Shift+F",
    WGameObjectSelectionAction, WGameObjectSelectionAction::ActionType::FocusOnSelectionAllViews);
  s_hSnapCameraToObject = W_REGISTER_ACTION_1("Scene.Camera.SnapCameraToObject", WActionScope::Document, "Camera", "", WGameObjectSelectionAction,
    WGameObjectSelectionAction::ActionType::SnapCameraToObject);
  s_hMoveCameraHere = W_REGISTER_ACTION_1("Scene.Camera.MoveCameraHere", WActionScope::Document, "Camera", "C", WGameObjectSelectionAction,
    WGameObjectSelectionAction::ActionType::MoveCameraHere);
}

void WGameObjectSelectionActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hSelectionCategory);
  WActionManager::UnregisterAction(s_hShowInScenegraph);
  WActionManager::UnregisterAction(s_hFocusOnSelection);
  WActionManager::UnregisterAction(s_hFocusOnSelectionAllViews);
  WActionManager::UnregisterAction(s_hSnapCameraToObject);
  WActionManager::UnregisterAction(s_hMoveCameraHere);
}

void WGameObjectSelectionActions::MapActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hSelectionCategory, "G.Edit", 5.0f);

  pMap->MapAction(s_hShowInScenegraph, "G.Selection", 2.0f);
  pMap->MapAction(s_hFocusOnSelection, "G.Selection", 3.0f);
  pMap->MapAction(s_hFocusOnSelectionAllViews, "G.Selection", 3.5f);
  pMap->MapAction(s_hSnapCameraToObject, "G.Selection", 8.0f);
  pMap->MapAction(s_hMoveCameraHere, "G.Selection", 10.0f);
}

void WGameObjectSelectionActions::MapContextMenuActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hSelectionCategory, "", 5.0f);

  pMap->MapAction(s_hFocusOnSelection, "G.Selection", 1.0f);
}


void WGameObjectSelectionActions::MapViewContextMenuActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hSelectionCategory, "", 5.0f);

  pMap->MapAction(s_hMoveCameraHere, "G.Selection", 1.5f);
  pMap->MapAction(s_hSnapCameraToObject, "G.Selection", 8.0f);
}

WGameObjectSelectionAction::WGameObjectSelectionAction(
  const WActionContext& context, const char* szName, WGameObjectSelectionAction::ActionType type)
  : WButtonAction(context, szName, false, "")
{
  m_Type = type;
  m_pSceneDocument = const_cast<WGameObjectDocument*>(static_cast<const WGameObjectDocument*>(context.m_pDocument));

  switch (m_Type)
  {
    case ActionType::ShowInScenegraph:
      SetIconPath(":/EditorFramework/Icons/Scenegraph.svg");
      break;
    case ActionType::FocusOnSelection:
      SetIconPath(":/EditorFramework/Icons/FocusOnSelection.svg");
      break;
    case ActionType::FocusOnSelectionAllViews:
      SetIconPath(":/EditorFramework/Icons/FocusOnSelectionAllViews.svg");
      break;
    case ActionType::SnapCameraToObject:
      // SetIconPath(":/EditorFramework/Icons/SnapToCamera.svg"); // TODO Icon
      break;
    case ActionType::MoveCameraHere:
      SetIconPath(":/EditorFramework/Icons/MoveCameraHere.svg");
      break;
  }

  UpdateEnableState();

  m_Context.m_pDocument->GetSelectionManager()->m_Events.AddEventHandler(WMakeDelegate(&WGameObjectSelectionAction::SelectionEventHandler, this));
}


WGameObjectSelectionAction::~WGameObjectSelectionAction()
{
  m_Context.m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(
    WMakeDelegate(&WGameObjectSelectionAction::SelectionEventHandler, this));
}

void WGameObjectSelectionAction::Execute(const WVariant& value)
{
  switch (m_Type)
  {
    case ActionType::ShowInScenegraph:
      m_pSceneDocument->TriggerShowSelectionInScenegraph();
      return;
    case ActionType::FocusOnSelection:
      m_pSceneDocument->TriggerFocusOnSelection(false);
      return;
    case ActionType::FocusOnSelectionAllViews:
      m_pSceneDocument->TriggerFocusOnSelection(true);
      return;
    case ActionType::SnapCameraToObject:
      m_pSceneDocument->SnapCameraToObject();
      break;
    case ActionType::MoveCameraHere:
      m_pSceneDocument->MoveCameraHere();
      break;
  }
}

void WGameObjectSelectionAction::SelectionEventHandler(const WSelectionManagerEvent& e)
{
  UpdateEnableState();
}

void WGameObjectSelectionAction::UpdateEnableState()
{
  if (m_Type == ActionType::FocusOnSelection || m_Type == ActionType::FocusOnSelectionAllViews || m_Type == ActionType::ShowInScenegraph)
  {
    SetEnabled(!m_Context.m_pDocument->GetSelectionManager()->IsSelectionEmpty());
  }

  if (m_Type == ActionType::SnapCameraToObject)
  {
    SetEnabled(m_Context.m_pDocument->GetSelectionManager()->GetSelection().GetCount() == 1);
  }
}
