#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/GameObjectDocumentActions.h>
#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorFramework/Preferences/ScenePreferences.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WGameObjectDocumentAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCameraSpeedSliderAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WActionDescriptorHandle WGameObjectDocumentActions::s_hGameObjectCategory;
WActionDescriptorHandle WGameObjectDocumentActions::s_hRenderSelectionOverlay;
WActionDescriptorHandle WGameObjectDocumentActions::s_hRenderVisualizers;
WActionDescriptorHandle WGameObjectDocumentActions::s_hRenderShapeIcons;
WActionDescriptorHandle WGameObjectDocumentActions::s_hRenderGrid;
WActionDescriptorHandle WGameObjectDocumentActions::s_hAddAmbientLight;
WActionDescriptorHandle WGameObjectDocumentActions::s_hSimulationSpeedMenu;
WActionDescriptorHandle WGameObjectDocumentActions::s_hSimulationSpeed[10];
WActionDescriptorHandle WGameObjectDocumentActions::s_hCameraSpeed;
WActionDescriptorHandle WGameObjectDocumentActions::s_hPickTransparent;

void WGameObjectDocumentActions::RegisterActions()
{
  s_hGameObjectCategory = W_REGISTER_CATEGORY("GameObjectCategory");
  s_hRenderSelectionOverlay = W_REGISTER_ACTION_1("Scene.Render.SelectionOverlay", WActionScope::Document, "Scene", "S", WGameObjectDocumentAction,
    WGameObjectDocumentAction::ActionType::RenderSelectionOverlay);
  s_hRenderVisualizers = W_REGISTER_ACTION_1("Scene.Render.Visualizers", WActionScope::Document, "Scene", "V", WGameObjectDocumentAction,
    WGameObjectDocumentAction::ActionType::RenderVisualizers);
  s_hRenderShapeIcons = W_REGISTER_ACTION_1("Scene.Render.ShapeIcons", WActionScope::Document, "Scene", "I", WGameObjectDocumentAction,
    WGameObjectDocumentAction::ActionType::RenderShapeIcons);
  s_hRenderGrid = W_REGISTER_ACTION_1(
    "Scene.Render.Grid", WActionScope::Document, "Scene", "G", WGameObjectDocumentAction, WGameObjectDocumentAction::ActionType::RenderGrid);
  s_hAddAmbientLight = W_REGISTER_ACTION_1("Scene.Render.AddAmbient", WActionScope::Document, "Scene", "", WGameObjectDocumentAction,
    WGameObjectDocumentAction::ActionType::AddAmbientLight);

  s_hSimulationSpeedMenu = W_REGISTER_MENU_WITH_ICON("Scene.Simulation.Speed.Menu", "");
  s_hSimulationSpeed[0] = W_REGISTER_ACTION_2("Scene.Simulation.Speed.01", WActionScope::Document, "Simulation - Speed", "",
    WGameObjectDocumentAction, WGameObjectDocumentAction::ActionType::SimulationSpeed, 0.1f);
  s_hSimulationSpeed[1] = W_REGISTER_ACTION_2("Scene.Simulation.Speed.025", WActionScope::Document, "Simulation - Speed", "",
    WGameObjectDocumentAction, WGameObjectDocumentAction::ActionType::SimulationSpeed, 0.25f);
  s_hSimulationSpeed[2] = W_REGISTER_ACTION_2("Scene.Simulation.Speed.05", WActionScope::Document, "Simulation - Speed", "",
    WGameObjectDocumentAction, WGameObjectDocumentAction::ActionType::SimulationSpeed, 0.5f);
  s_hSimulationSpeed[3] = W_REGISTER_ACTION_2("Scene.Simulation.Speed.1", WActionScope::Document, "Simulation - Speed", "",
    WGameObjectDocumentAction, WGameObjectDocumentAction::ActionType::SimulationSpeed, 1.0f);
  s_hSimulationSpeed[4] = W_REGISTER_ACTION_2("Scene.Simulation.Speed.15", WActionScope::Document, "Simulation - Speed", "",
    WGameObjectDocumentAction, WGameObjectDocumentAction::ActionType::SimulationSpeed, 1.5f);
  s_hSimulationSpeed[5] = W_REGISTER_ACTION_2("Scene.Simulation.Speed.2", WActionScope::Document, "Simulation - Speed", "",
    WGameObjectDocumentAction, WGameObjectDocumentAction::ActionType::SimulationSpeed, 2.0f);
  s_hSimulationSpeed[6] = W_REGISTER_ACTION_2("Scene.Simulation.Speed.3", WActionScope::Document, "Simulation - Speed", "",
    WGameObjectDocumentAction, WGameObjectDocumentAction::ActionType::SimulationSpeed, 3.0f);
  s_hSimulationSpeed[7] = W_REGISTER_ACTION_2("Scene.Simulation.Speed.4", WActionScope::Document, "Simulation - Speed", "",
    WGameObjectDocumentAction, WGameObjectDocumentAction::ActionType::SimulationSpeed, 4.0f);
  s_hSimulationSpeed[8] = W_REGISTER_ACTION_2("Scene.Simulation.Speed.5", WActionScope::Document, "Simulation - Speed", "",
    WGameObjectDocumentAction, WGameObjectDocumentAction::ActionType::SimulationSpeed, 5.0f);
  s_hSimulationSpeed[9] = W_REGISTER_ACTION_2("Scene.Simulation.Speed.10", WActionScope::Document, "Simulation - Speed", "",
    WGameObjectDocumentAction, WGameObjectDocumentAction::ActionType::SimulationSpeed, 10.0f);

  s_hCameraSpeed = W_REGISTER_ACTION_1(
    "Scene.Camera.Speed", WActionScope::Document, "Camera", "", WCameraSpeedSliderAction, WCameraSpeedSliderAction::ActionType::CameraSpeed);

  s_hPickTransparent = W_REGISTER_ACTION_1("Scene.Render.PickTransparent", WActionScope::Document, "Scene", "U", WGameObjectDocumentAction,
    WGameObjectDocumentAction::ActionType::PickTransparent);
}

void WGameObjectDocumentActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hGameObjectCategory);
  WActionManager::UnregisterAction(s_hRenderSelectionOverlay);
  WActionManager::UnregisterAction(s_hRenderVisualizers);
  WActionManager::UnregisterAction(s_hRenderShapeIcons);
  WActionManager::UnregisterAction(s_hRenderGrid);
  WActionManager::UnregisterAction(s_hAddAmbientLight);

  WActionManager::UnregisterAction(s_hSimulationSpeedMenu);
  for (int i = 0; i < W_ARRAY_SIZE(s_hSimulationSpeed); ++i)
    WActionManager::UnregisterAction(s_hSimulationSpeed[i]);

  WActionManager::UnregisterAction(s_hCameraSpeed);
  WActionManager::UnregisterAction(s_hPickTransparent);
}

void WGameObjectDocumentActions::MapMenuActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  {
    pMap->MapAction(s_hGameObjectCategory, "G.View", 0.9f);

    const WStringView sSubPath = "GameObjectCategory";
    pMap->MapAction(s_hRenderSelectionOverlay, sSubPath, 1.0f);
    pMap->MapAction(s_hRenderVisualizers, sSubPath, 2.0f);
    pMap->MapAction(s_hRenderShapeIcons, sSubPath, 3.0f);
    pMap->MapAction(s_hRenderGrid, sSubPath, 4.0f);
    pMap->MapAction(s_hPickTransparent, sSubPath, 5.0f);
    pMap->MapAction(s_hAddAmbientLight, sSubPath, 6.0f);
    pMap->MapAction(s_hCameraSpeed, sSubPath, 7.0f);
  }
}

void WGameObjectDocumentActions::MapMenuSimulationSpeed(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  {
    const WStringView sSubPath = "GameObjectCategory";

    pMap->MapAction(s_hGameObjectCategory, "G.Scene", 1.0f);
    pMap->MapAction(s_hSimulationSpeedMenu, sSubPath, 3.0f);

    WStringBuilder sSubPathSim(sSubPath, "/Scene.Simulation.Speed.Menu");
    for (WUInt32 i = 0; i < W_ARRAY_SIZE(s_hSimulationSpeed); ++i)
      pMap->MapAction(s_hSimulationSpeed[i], "G.Scene", sSubPathSim, i + 1.0f);
  }
}

void WGameObjectDocumentActions::MapToolbarActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  {
    pMap->MapAction(s_hGameObjectCategory, "", 12.0f);

    const WStringView sSubPath("GameObjectCategory");
    pMap->MapAction(s_hRenderSelectionOverlay, sSubPath, 4.0f);
    pMap->MapAction(s_hRenderVisualizers, sSubPath, 5.0f);
    pMap->MapAction(s_hRenderShapeIcons, sSubPath, 6.0f);
    pMap->MapAction(s_hRenderGrid, sSubPath, 6.5f);
    pMap->MapAction(s_hCameraSpeed, sSubPath, 7.0f);
  }
}

WGameObjectDocumentAction::WGameObjectDocumentAction(
  const WActionContext& context, const char* szName, WGameObjectDocumentAction::ActionType type, float fSimSpeed)
  : WButtonAction(context, szName, false, "")
{
  m_Type = type;
  // TODO const cast
  m_pGameObjectDocument = const_cast<WGameObjectDocument*>(static_cast<const WGameObjectDocument*>(context.m_pDocument));
  m_pGameObjectDocument->m_GameObjectEvents.AddEventHandler(WMakeDelegate(&WGameObjectDocumentAction::SceneEventHandler, this));
  m_fSimSpeed = fSimSpeed;

  switch (m_Type)
  {
    case ActionType::RenderSelectionOverlay:
      SetCheckable(true);
      SetIconPath(":/EditorFramework/Icons/Selection.svg");
      SetChecked(m_pGameObjectDocument->GetRenderSelectionOverlay());
      break;

    case ActionType::RenderVisualizers:
      SetCheckable(true);
      SetIconPath(":/EditorFramework/Icons/Visualizers.svg");
      SetChecked(m_pGameObjectDocument->GetRenderVisualizers());
      break;

    case ActionType::RenderShapeIcons:
      SetCheckable(true);
      SetIconPath(":/EditorFramework/Icons/ShapeIcons.svg");
      SetChecked(m_pGameObjectDocument->GetRenderShapeIcons());
      break;

    case ActionType::RenderGrid:
    {
      WScenePreferencesUser* pPreferences = WPreferences::QueryPreferences<WScenePreferencesUser>(m_pGameObjectDocument);
      pPreferences->m_ChangedEvent.AddEventHandler(WMakeDelegate(&WGameObjectDocumentAction::OnPreferenceChange, this));

      SetCheckable(true);
      SetIconPath(":/EditorFramework/Icons/Grid.svg");
      SetChecked(pPreferences->GetShowGrid());
    }
    break;

    case ActionType::AddAmbientLight:
      SetCheckable(true);
      SetChecked(m_pGameObjectDocument->GetAddAmbientLight());
      break;

    case ActionType::SimulationSpeed:
      SetCheckable(true);
      SetChecked(m_pGameObjectDocument->GetSimulationSpeed() == m_fSimSpeed);
      break;

    case ActionType::PickTransparent:
      SetCheckable(true);
      SetChecked(m_pGameObjectDocument->GetPickTransparent());
      break;
  }
}

WGameObjectDocumentAction::~WGameObjectDocumentAction()
{
  m_pGameObjectDocument->m_GameObjectEvents.RemoveEventHandler(WMakeDelegate(&WGameObjectDocumentAction::SceneEventHandler, this));

  switch (m_Type)
  {
    case ActionType::RenderGrid:
    {
      WScenePreferencesUser* pPreferences = WPreferences::QueryPreferences<WScenePreferencesUser>(m_pGameObjectDocument);

      pPreferences->m_ChangedEvent.RemoveEventHandler(WMakeDelegate(&WGameObjectDocumentAction::OnPreferenceChange, this));
    }
    break;
    default:
      break;
  }
}

void WGameObjectDocumentAction::Execute(const WVariant& value)
{
  switch (m_Type)
  {
    case ActionType::RenderSelectionOverlay:
      m_pGameObjectDocument->SetRenderSelectionOverlay(!m_pGameObjectDocument->GetRenderSelectionOverlay());
      return;

    case ActionType::RenderVisualizers:
      m_pGameObjectDocument->SetRenderVisualizers(!m_pGameObjectDocument->GetRenderVisualizers());
      return;

    case ActionType::RenderShapeIcons:
      m_pGameObjectDocument->SetRenderShapeIcons(!m_pGameObjectDocument->GetRenderShapeIcons());
      return;

    case ActionType::RenderGrid:
    {
      auto pPref = WPreferences::QueryPreferences<WScenePreferencesUser>(m_pGameObjectDocument);
      pPref->SetShowGrid(!pPref->GetShowGrid());
      m_pGameObjectDocument->ShowDocumentStatus(WFmt("Show Grid: {}", pPref->GetShowGrid() ? "ON" : "OFF"));
      return;
    }

    case ActionType::AddAmbientLight:
      m_pGameObjectDocument->SetAddAmbientLight(!m_pGameObjectDocument->GetAddAmbientLight());
      return;

    case ActionType::SimulationSpeed:
      m_pGameObjectDocument->SetSimulationSpeed(m_fSimSpeed);
      return;

    case ActionType::PickTransparent:
      m_pGameObjectDocument->SetPickTransparent(!m_pGameObjectDocument->GetPickTransparent());
      return;

    default:
      break;
  }
}

void WGameObjectDocumentAction::SceneEventHandler(const WGameObjectEvent& e)
{
  switch (e.m_Type)
  {
    case WGameObjectEvent::Type::RenderSelectionOverlayChanged:
    {
      if (m_Type == ActionType::RenderSelectionOverlay)
      {
        SetChecked(m_pGameObjectDocument->GetRenderSelectionOverlay());
      }
    }
    break;

    case WGameObjectEvent::Type::RenderVisualizersChanged:
    {
      if (m_Type == ActionType::RenderVisualizers)
      {
        SetChecked(m_pGameObjectDocument->GetRenderVisualizers());
      }
    }
    break;

    case WGameObjectEvent::Type::RenderShapeIconsChanged:
    {
      if (m_Type == ActionType::RenderShapeIcons)
      {
        SetChecked(m_pGameObjectDocument->GetRenderShapeIcons());
      }
    }
    break;

    case WGameObjectEvent::Type::AddAmbientLightChanged:
    {
      if (m_Type == ActionType::AddAmbientLight)
      {
        SetChecked(m_pGameObjectDocument->GetAddAmbientLight());
      }
    }
    break;

    case WGameObjectEvent::Type::SimulationSpeedChanged:
    {
      if (m_Type == ActionType::SimulationSpeed)
      {
        SetChecked(m_pGameObjectDocument->GetSimulationSpeed() == m_fSimSpeed);
      }
    }
    break;

    case WGameObjectEvent::Type::PickTransparentChanged:
    {
      if (m_Type == ActionType::PickTransparent)
      {
        SetChecked(m_pGameObjectDocument->GetPickTransparent());
      }
    }
    break;

    default:
      break;
  }
}

void WGameObjectDocumentAction::OnPreferenceChange(WPreferences* pref)
{
  WScenePreferencesUser* pPreferences = WPreferences::QueryPreferences<WScenePreferencesUser>(m_pGameObjectDocument);

  switch (m_Type)
  {
    case ActionType::RenderGrid:
    {
      SetChecked(pPreferences->GetShowGrid());
    }
    break;

    default:
      break;
  }
}

WCameraSpeedSliderAction::WCameraSpeedSliderAction(const WActionContext& context, const char* szName, ActionType type)
  : WSliderAction(context, szName)
{
  m_Type = type;
  m_pGameObjectDocument = const_cast<WGameObjectDocument*>(static_cast<const WGameObjectDocument*>(context.m_pDocument));

  switch (m_Type)
  {
    case ActionType::CameraSpeed:
    {
      WScenePreferencesUser* pPreferences = WPreferences::QueryPreferences<WScenePreferencesUser>(m_pGameObjectDocument);

      pPreferences->m_ChangedEvent.AddEventHandler(WMakeDelegate(&WCameraSpeedSliderAction::OnPreferenceChange, this));

      SetRange(0, 24);
    }
    break;
  }

  UpdateState();
}

WCameraSpeedSliderAction::~WCameraSpeedSliderAction()
{
  switch (m_Type)
  {
    case ActionType::CameraSpeed:
    {
      WScenePreferencesUser* pPreferences = WPreferences::QueryPreferences<WScenePreferencesUser>(m_pGameObjectDocument);

      pPreferences->m_ChangedEvent.RemoveEventHandler(WMakeDelegate(&WCameraSpeedSliderAction::OnPreferenceChange, this));
    }
    break;
  }
}

void WCameraSpeedSliderAction::Execute(const WVariant& value)
{
  switch (m_Type)
  {
    case ActionType::CameraSpeed:
    {
      WScenePreferencesUser* pPreferences = WPreferences::QueryPreferences<WScenePreferencesUser>(m_pGameObjectDocument);

      pPreferences->SetCameraSpeed(value.Get<WInt32>());
    }
    break;
  }
}

void WCameraSpeedSliderAction::OnPreferenceChange(WPreferences* pref)
{
  UpdateState();
}

void WCameraSpeedSliderAction::UpdateState()
{
  switch (m_Type)
  {
    case ActionType::CameraSpeed:
    {
      WScenePreferencesUser* pPreferences = WPreferences::QueryPreferences<WScenePreferencesUser>(m_pGameObjectDocument);

      SetValue(pPreferences->GetCameraSpeed());
    }
    break;
  }
}
