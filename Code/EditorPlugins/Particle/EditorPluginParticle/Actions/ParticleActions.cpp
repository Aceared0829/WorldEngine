#include <EditorPluginParticle/EditorPluginParticlePCH.h>

#include <EditorPluginParticle/Actions/ParticleActions.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAsset.h>
#include <GuiFoundation/Action/ActionManager.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WActionDescriptorHandle WParticleActions::s_hCategory;
WActionDescriptorHandle WParticleActions::s_hPauseEffect;
WActionDescriptorHandle WParticleActions::s_hRestartEffect;
WActionDescriptorHandle WParticleActions::s_hAutoRestart;
WActionDescriptorHandle WParticleActions::s_hSimulationSpeedMenu;
WActionDescriptorHandle WParticleActions::s_hSimulationSpeed[10];
WActionDescriptorHandle WParticleActions::s_hRenderVisualizers;


void WParticleActions::RegisterActions()
{
  s_hCategory = W_REGISTER_CATEGORY("ParticleCategory");
  s_hPauseEffect =
    W_REGISTER_ACTION_1("PFX.Pause", WActionScope::Document, "Particles", "Pause", WParticleAction, WParticleAction::ActionType::PauseEffect);
  s_hRestartEffect =
    W_REGISTER_ACTION_1("PFX.Restart", WActionScope::Document, "Particles", "F5", WParticleAction, WParticleAction::ActionType::RestartEffect);
  s_hAutoRestart =
    W_REGISTER_ACTION_1("PFX.AutoRestart", WActionScope::Document, "Particles", "", WParticleAction, WParticleAction::ActionType::AutoRestart);

  s_hSimulationSpeedMenu = W_REGISTER_MENU_WITH_ICON("PFX.Speed.Menu", ":/EditorFramework/Icons/Speed.svg");
  s_hSimulationSpeed[0] = W_REGISTER_ACTION_2(
    "PFX.Speed.01", WActionScope::Document, "Particles", "Ctrl+1", WParticleAction, WParticleAction::ActionType::SimulationSpeed, 0.1f);
  s_hSimulationSpeed[1] = W_REGISTER_ACTION_2(
    "PFX.Speed.025", WActionScope::Document, "Particles", "Ctrl+2", WParticleAction, WParticleAction::ActionType::SimulationSpeed, 0.25f);
  s_hSimulationSpeed[2] = W_REGISTER_ACTION_2(
    "PFX.Speed.05", WActionScope::Document, "Particles", "Ctrl+3", WParticleAction, WParticleAction::ActionType::SimulationSpeed, 0.5f);
  s_hSimulationSpeed[3] = W_REGISTER_ACTION_2(
    "PFX.Speed.1", WActionScope::Document, "Particles", "Ctrl+4", WParticleAction, WParticleAction::ActionType::SimulationSpeed, 1.0f);
  s_hSimulationSpeed[4] = W_REGISTER_ACTION_2(
    "PFX.Speed.15", WActionScope::Document, "Particles", "Ctrl+5", WParticleAction, WParticleAction::ActionType::SimulationSpeed, 1.5f);
  s_hSimulationSpeed[5] = W_REGISTER_ACTION_2(
    "PFX.Speed.2", WActionScope::Document, "Particles", "Ctrl+6", WParticleAction, WParticleAction::ActionType::SimulationSpeed, 2.0f);
  s_hSimulationSpeed[6] = W_REGISTER_ACTION_2(
    "PFX.Speed.3", WActionScope::Document, "Particles", "Ctrl+7", WParticleAction, WParticleAction::ActionType::SimulationSpeed, 3.0f);
  s_hSimulationSpeed[7] = W_REGISTER_ACTION_2(
    "PFX.Speed.4", WActionScope::Document, "Particles", "Ctrl+8", WParticleAction, WParticleAction::ActionType::SimulationSpeed, 4.0f);
  s_hSimulationSpeed[8] = W_REGISTER_ACTION_2(
    "PFX.Speed.5", WActionScope::Document, "Particles", "Ctrl+9", WParticleAction, WParticleAction::ActionType::SimulationSpeed, 5.0f);
  s_hSimulationSpeed[9] = W_REGISTER_ACTION_2(
    "PFX.Speed.10", WActionScope::Document, "Particles", "Ctrl+0", WParticleAction, WParticleAction::ActionType::SimulationSpeed, 10.0f);
  s_hRenderVisualizers = W_REGISTER_ACTION_1(
    "PFX.Render.Visualizers", WActionScope::Document, "Particles", "V", WParticleAction, WParticleAction::ActionType::RenderVisualizers);
}

void WParticleActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hCategory);
  WActionManager::UnregisterAction(s_hPauseEffect);
  WActionManager::UnregisterAction(s_hRestartEffect);
  WActionManager::UnregisterAction(s_hAutoRestart);
  WActionManager::UnregisterAction(s_hSimulationSpeedMenu);
  WActionManager::UnregisterAction(s_hRenderVisualizers);

  for (int i = 0; i < W_ARRAY_SIZE(s_hSimulationSpeed); ++i)
    WActionManager::UnregisterAction(s_hSimulationSpeed[i]);
}

void WParticleActions::MapActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCategory, "", 11.0f);

  const char* szSubPath = "ParticleCategory";

  pMap->MapAction(s_hPauseEffect, szSubPath, 0.5f);
  pMap->MapAction(s_hRestartEffect, szSubPath, 1.0f);
  pMap->MapAction(s_hAutoRestart, szSubPath, 2.0f);

  pMap->MapAction(s_hSimulationSpeedMenu, szSubPath, 3.0f);

  WStringBuilder sSubPath(szSubPath, "/PFX.Speed.Menu");

  for (WUInt32 i = 0; i < W_ARRAY_SIZE(s_hSimulationSpeed); ++i)
    pMap->MapAction(s_hSimulationSpeed[i], sSubPath, i + 1.0f);

  pMap->MapAction(s_hRenderVisualizers, szSubPath, 4.0f);
}

WParticleAction::WParticleAction(const WActionContext& context, const char* szName, WParticleAction::ActionType type, float fSimSpeed)
  : WButtonAction(context, szName, false, "")
{
  m_Type = type;
  m_fSimSpeed = fSimSpeed;

  m_pEffectDocument = const_cast<WParticleEffectAssetDocument*>(static_cast<const WParticleEffectAssetDocument*>(context.m_pDocument));
  m_pEffectDocument->m_Events.AddEventHandler(WMakeDelegate(&WParticleAction::EffectEventHandler, this));

  switch (m_Type)
  {
    case ActionType::PauseEffect:
      SetIconPath(":/EditorFramework/Icons/Pause.svg");
      break;

    case ActionType::RestartEffect:
      SetIconPath(":/EditorFramework/Icons/Restart.svg");
      break;

    case ActionType::AutoRestart:
      SetIconPath(":/EditorFramework/Icons/Loop.svg");
      break;

    case ActionType::RenderVisualizers:
      SetCheckable(true);
      SetIconPath(":/EditorFramework/Icons/Visualizers.svg");
      SetChecked(m_pEffectDocument->GetRenderVisualizers());
      break;

    default:
      break;
  }

  UpdateState();
}


WParticleAction::~WParticleAction()
{
  m_pEffectDocument->m_Events.RemoveEventHandler(WMakeDelegate(&WParticleAction::EffectEventHandler, this));
}

void WParticleAction::Execute(const WVariant& value)
{
  switch (m_Type)
  {
    case ActionType::PauseEffect:
      m_pEffectDocument->SetSimulationPaused(!m_pEffectDocument->GetSimulationPaused());
      return;

    case ActionType::RestartEffect:
      m_pEffectDocument->TriggerRestartEffect();
      return;

    case ActionType::AutoRestart:
      m_pEffectDocument->SetAutoRestart(!m_pEffectDocument->GetAutoRestart());
      return;

    case ActionType::SimulationSpeed:
      m_pEffectDocument->SetSimulationSpeed(m_fSimSpeed);
      return;

    case ActionType::RenderVisualizers:
      m_pEffectDocument->SetRenderVisualizers(!m_pEffectDocument->GetRenderVisualizers());
      return;
  }
}

void WParticleAction::EffectEventHandler(const WParticleEffectAssetEvent& e)
{
  switch (e.m_Type)
  {
    case WParticleEffectAssetEvent::AutoRestartChanged:
    case WParticleEffectAssetEvent::SimulationSpeedChanged:
    case WParticleEffectAssetEvent::RenderVisualizersChanged:
      UpdateState();
      break;

    default:
      break;
  }
}

void WParticleAction::UpdateState()
{
  if (m_Type == ActionType::PauseEffect)
  {
    SetCheckable(true);
    SetChecked(m_pEffectDocument->GetSimulationPaused());
  }

  if (m_Type == ActionType::AutoRestart)
  {
    SetCheckable(true);
    SetChecked(m_pEffectDocument->GetAutoRestart());
  }

  if (m_Type == ActionType::SimulationSpeed)
  {
    SetCheckable(true);
    SetChecked(m_pEffectDocument->GetSimulationSpeed() == m_fSimSpeed);
  }

  if (m_Type == ActionType::RenderVisualizers)
  {
    SetCheckable(true);
    SetChecked(m_pEffectDocument->GetRenderVisualizers());
  }
}
