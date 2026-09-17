#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/CommonAssetActions.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCommonAssetAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WActionDescriptorHandle WCommonAssetActions::s_hCategory;
WActionDescriptorHandle WCommonAssetActions::s_hPause;
WActionDescriptorHandle WCommonAssetActions::s_hRestart;
WActionDescriptorHandle WCommonAssetActions::s_hLoop;
WActionDescriptorHandle WCommonAssetActions::s_hSimulationSpeedMenu;
WActionDescriptorHandle WCommonAssetActions::s_hSimulationSpeed[10];
WActionDescriptorHandle WCommonAssetActions::s_hGrid;
WActionDescriptorHandle WCommonAssetActions::s_hVisualizers;


void WCommonAssetActions::RegisterActions()
{
  s_hCategory = W_REGISTER_CATEGORY("CommonAssetCategory");
  s_hPause = W_REGISTER_ACTION_1("Common.Pause", WActionScope::Document, "Animations", "Pause", WCommonAssetAction, WCommonAssetAction::ActionType::Pause);
  s_hRestart = W_REGISTER_ACTION_1("Common.Restart", WActionScope::Document, "Animations", "F5", WCommonAssetAction, WCommonAssetAction::ActionType::Restart);
  s_hLoop = W_REGISTER_ACTION_1("Common.Loop", WActionScope::Document, "Animations", "", WCommonAssetAction, WCommonAssetAction::ActionType::Loop);
  s_hGrid = W_REGISTER_ACTION_1("Common.Grid", WActionScope::Document, "Misc", "G", WCommonAssetAction, WCommonAssetAction::ActionType::Grid);
  s_hVisualizers = W_REGISTER_ACTION_1("Common.Visualizers", WActionScope::Document, "Misc", "V", WCommonAssetAction, WCommonAssetAction::ActionType::Visualizers);

  s_hSimulationSpeedMenu = W_REGISTER_MENU_WITH_ICON("Common.Speed.Menu", ":/EditorFramework/Icons/Speed.svg");
  s_hSimulationSpeed[0] = W_REGISTER_ACTION_2("Common.Speed.01", WActionScope::Document, "Animations", "Ctrl+1", WCommonAssetAction, WCommonAssetAction::ActionType::SimulationSpeed, 0.1f);
  s_hSimulationSpeed[1] = W_REGISTER_ACTION_2("Common.Speed.025", WActionScope::Document, "Animations", "Ctrl+2", WCommonAssetAction, WCommonAssetAction::ActionType::SimulationSpeed, 0.25f);
  s_hSimulationSpeed[2] = W_REGISTER_ACTION_2("Common.Speed.05", WActionScope::Document, "Animations", "Ctrl+3", WCommonAssetAction, WCommonAssetAction::ActionType::SimulationSpeed, 0.5f);
  s_hSimulationSpeed[3] = W_REGISTER_ACTION_2("Common.Speed.1", WActionScope::Document, "Animations", "Ctrl+4", WCommonAssetAction, WCommonAssetAction::ActionType::SimulationSpeed, 1.0f);
  s_hSimulationSpeed[4] = W_REGISTER_ACTION_2("Common.Speed.15", WActionScope::Document, "Animations", "Ctrl+5", WCommonAssetAction, WCommonAssetAction::ActionType::SimulationSpeed, 1.5f);
  s_hSimulationSpeed[5] = W_REGISTER_ACTION_2("Common.Speed.2", WActionScope::Document, "Animations", "Ctrl+6", WCommonAssetAction, WCommonAssetAction::ActionType::SimulationSpeed, 2.0f);
  s_hSimulationSpeed[6] = W_REGISTER_ACTION_2("Common.Speed.3", WActionScope::Document, "Animations", "Ctrl+7", WCommonAssetAction, WCommonAssetAction::ActionType::SimulationSpeed, 3.0f);
  s_hSimulationSpeed[7] = W_REGISTER_ACTION_2("Common.Speed.4", WActionScope::Document, "Animations", "Ctrl+8", WCommonAssetAction, WCommonAssetAction::ActionType::SimulationSpeed, 4.0f);
  s_hSimulationSpeed[8] = W_REGISTER_ACTION_2("Common.Speed.5", WActionScope::Document, "Animations", "Ctrl+9", WCommonAssetAction, WCommonAssetAction::ActionType::SimulationSpeed, 5.0f);
  s_hSimulationSpeed[9] = W_REGISTER_ACTION_2("Common.Speed.10", WActionScope::Document, "Animations", "Ctrl+0", WCommonAssetAction, WCommonAssetAction::ActionType::SimulationSpeed, 10.0f);
}

void WCommonAssetActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hCategory);
  WActionManager::UnregisterAction(s_hPause);
  WActionManager::UnregisterAction(s_hRestart);
  WActionManager::UnregisterAction(s_hLoop);
  WActionManager::UnregisterAction(s_hSimulationSpeedMenu);
  WActionManager::UnregisterAction(s_hGrid);
  WActionManager::UnregisterAction(s_hVisualizers);

  for (int i = 0; i < W_ARRAY_SIZE(s_hSimulationSpeed); ++i)
    WActionManager::UnregisterAction(s_hSimulationSpeed[i]);
}

void WCommonAssetActions::MapToolbarActions(WStringView sMapping, WUInt32 uiStateMask)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCategory, "", 11.0f);

  const char* szSubPath = "CommonAssetCategory";

  if (uiStateMask & WCommonAssetUiState::Pause)
  {
    pMap->MapAction(s_hPause, szSubPath, 0.5f);
  }

  if (uiStateMask & WCommonAssetUiState::Restart)
  {
    pMap->MapAction(s_hRestart, szSubPath, 1.0f);
  }

  if (uiStateMask & WCommonAssetUiState::Loop)
  {
    pMap->MapAction(s_hLoop, szSubPath, 2.0f);
  }

  if (uiStateMask & WCommonAssetUiState::SimulationSpeed)
  {
    pMap->MapAction(s_hSimulationSpeedMenu, szSubPath, 3.0f);

    WStringBuilder sSubPath(szSubPath, "/Common.Speed.Menu");

    for (WUInt32 i = 0; i < W_ARRAY_SIZE(s_hSimulationSpeed); ++i)
    {
      pMap->MapAction(s_hSimulationSpeed[i], sSubPath, i + 1.0f);
    }
  }

  if (uiStateMask & WCommonAssetUiState::Grid)
  {
    pMap->MapAction(s_hGrid, szSubPath, 4.0f);
  }

  if (uiStateMask & WCommonAssetUiState::Visualizers)
  {
    pMap->MapAction(s_hVisualizers, szSubPath, 5.0f);
  }
}

WCommonAssetAction::WCommonAssetAction(const WActionContext& context, const char* szName, WCommonAssetAction::ActionType type, float fSimSpeed)
  : WButtonAction(context, szName, false, "")
{
  m_Type = type;
  m_fSimSpeed = fSimSpeed;

  m_pAssetDocument = const_cast<WAssetDocument*>(static_cast<const WAssetDocument*>(context.m_pDocument));
  m_pAssetDocument->m_CommonAssetUiChangeEvent.AddEventHandler(WMakeDelegate(&WCommonAssetAction::CommonUiEventHandler, this));

  switch (m_Type)
  {
    case ActionType::Pause:
      SetCheckable(true);
      SetIconPath(":/EditorFramework/Icons/Pause.svg");
      SetChecked(m_pAssetDocument->GetCommonAssetUiState(WCommonAssetUiState::Pause) != 0.0f);
      break;

    case ActionType::Restart:
      SetIconPath(":/EditorFramework/Icons/Restart.svg");
      break;

    case ActionType::Loop:
      SetCheckable(true);
      SetIconPath(":/EditorFramework/Icons/Loop.svg");
      SetChecked(m_pAssetDocument->GetCommonAssetUiState(WCommonAssetUiState::Loop) != 0.0f);
      break;

    case ActionType::Grid:
      SetCheckable(true);
      SetIconPath(":/EditorFramework/Icons/Grid.svg");
      SetChecked(m_pAssetDocument->GetCommonAssetUiState(WCommonAssetUiState::Grid) != 0.0f);
      break;

    case ActionType::Visualizers:
      SetCheckable(true);
      SetIconPath(":/EditorFramework/Icons/Visualizers.svg");
      SetChecked(m_pAssetDocument->GetCommonAssetUiState(WCommonAssetUiState::Visualizers) != 0.0f);
      break;

    default:
      break;
  }

  UpdateState();
}


WCommonAssetAction::~WCommonAssetAction()
{
  m_pAssetDocument->m_CommonAssetUiChangeEvent.RemoveEventHandler(WMakeDelegate(&WCommonAssetAction::CommonUiEventHandler, this));
}

void WCommonAssetAction::Execute(const WVariant& value)
{
  switch (m_Type)
  {
    case ActionType::Pause:
      m_pAssetDocument->SetCommonAssetUiState(WCommonAssetUiState::Pause, m_pAssetDocument->GetCommonAssetUiState(WCommonAssetUiState::Pause) == 0.0f ? 1.0f : 0.0f);
      return;

    case ActionType::Restart:
      m_pAssetDocument->SetCommonAssetUiState(WCommonAssetUiState::Restart, 1.0f);
      return;

    case ActionType::Loop:
      m_pAssetDocument->SetCommonAssetUiState(WCommonAssetUiState::Loop, m_pAssetDocument->GetCommonAssetUiState(WCommonAssetUiState::Loop) == 0.0f ? 1.0f : 0.0f);
      return;

    case ActionType::SimulationSpeed:
      m_pAssetDocument->SetCommonAssetUiState(WCommonAssetUiState::SimulationSpeed, m_fSimSpeed);
      return;

    case ActionType::Grid:
      m_pAssetDocument->SetCommonAssetUiState(WCommonAssetUiState::Grid, m_pAssetDocument->GetCommonAssetUiState(WCommonAssetUiState::Grid) == 0.0f ? 1.0f : 0.0f);
      return;

    case ActionType::Visualizers:
      m_pAssetDocument->SetCommonAssetUiState(WCommonAssetUiState::Visualizers, m_pAssetDocument->GetCommonAssetUiState(WCommonAssetUiState::Visualizers) == 0.0f ? 1.0f : 0.0f);
      return;
  }
}

void WCommonAssetAction::CommonUiEventHandler(const WCommonAssetUiState& e)
{
  if (e.m_State == WCommonAssetUiState::Loop || e.m_State == WCommonAssetUiState::SimulationSpeed || e.m_State == WCommonAssetUiState::Grid || e.m_State == WCommonAssetUiState::Visualizers)
  {
    UpdateState();
  }
}

void WCommonAssetAction::UpdateState()
{
  if (m_Type == ActionType::Pause)
  {
    SetCheckable(true);
    SetChecked(m_pAssetDocument->GetCommonAssetUiState(WCommonAssetUiState::Pause) != 0.0f);
  }

  if (m_Type == ActionType::Loop)
  {
    SetCheckable(true);
    SetChecked(m_pAssetDocument->GetCommonAssetUiState(WCommonAssetUiState::Loop) != 0.0f);
  }

  if (m_Type == ActionType::SimulationSpeed)
  {
    SetCheckable(true);
    SetChecked(m_pAssetDocument->GetCommonAssetUiState(WCommonAssetUiState::SimulationSpeed) == m_fSimSpeed);
  }

  if (m_Type == ActionType::Grid)
  {
    SetCheckable(true);
    SetChecked(m_pAssetDocument->GetCommonAssetUiState(WCommonAssetUiState::Grid) != 0.0f);
  }

  if (m_Type == ActionType::Visualizers)
  {
    SetCheckable(true);
    SetChecked(m_pAssetDocument->GetCommonAssetUiState(WCommonAssetUiState::Visualizers) != 0.0f);
  }
}
