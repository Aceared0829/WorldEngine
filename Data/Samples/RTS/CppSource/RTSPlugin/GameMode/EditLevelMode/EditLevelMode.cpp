#include <RTSPlugin/RTSPluginPCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/Utils/Blackboard.h>
#include <RTSPlugin/GameMode/EditLevelMode/EditLevelMode.h>
#include <RTSPlugin/GameState/RTSGameState.h>
#include <RmlUiPlugin/Components/RmlUiCanvas2DComponent.h>
#include <RmlUiPlugin/RmlUiContext.h>

const char* g_BuildItemTypes[] = {
  "FederationShip1",
  "FederationShip2",
  "FederationShip3",
  "KlingonShip1",
  "KlingonShip2",
  "KlingonShip3",
};

static WHashedString s_sTeam = WMakeHashedString("Team");
static WHashedString s_sShipType = WMakeHashedString("ShipType");
static WHashedString s_sSelectKey = WMakeHashedString("SelectKey");
static WHashedString s_sCreateKey = WMakeHashedString("CreateKey");
static WHashedString s_sRemoveKey = WMakeHashedString("RemoveKey");
static WHashedString s_sShowEditWidget = WMakeHashedString("ShowEditWidget");

RtsEditLevelMode::RtsEditLevelMode()
{
  // create a blackboard to easily share state with the RML UI
  m_pBlackboard = WBlackboard::Create("game-ui");

  m_pBlackboard->SetEntryValue(s_sTeam, 0);
  m_pBlackboard->SetEntryValue(s_sShipType, 0);
  m_pBlackboard->SetEntryValue(s_sSelectKey, WVariant());
  m_pBlackboard->SetEntryValue(s_sCreateKey, WVariant());
  m_pBlackboard->SetEntryValue(s_sRemoveKey, WVariant());
}

RtsEditLevelMode::~RtsEditLevelMode() = default;

void RtsEditLevelMode::OnActivateMode()
{
  m_pBlackboard->SetEntryValue(s_sShowEditWidget, true);
  SetUiActive(m_pMainWorld, WTempHashedString("game-ui"), true);

  SetupEditUI();
}

void RtsEditLevelMode::OnDeactivateMode()
{
  W_LOCK(m_pMainWorld->GetWriteMarker());

  m_pBlackboard->SetEntryValue(s_sShowEditWidget, false);
  SetUiActive(m_pMainWorld, WTempHashedString("game-ui"), false);
}

void RtsEditLevelMode::OnBeforeWorldUpdate()
{
  SetupSelectModeUI();

  m_pGameState->RenderUnitSelection();
}

void RtsEditLevelMode::SetupEditUI()
{
  // Set blackboard values
  {
    m_pBlackboard->SetEntryValue(s_sSelectKey, WInputManager::GetInputSlotDisplayName(WInputSlot_MouseButton0));
    m_pBlackboard->SetEntryValue(s_sCreateKey, WInputManager::GetInputSlotDisplayName("game-ui", "PlaceObject"));
    m_pBlackboard->SetEntryValue(s_sRemoveKey, WInputManager::GetInputSlotDisplayName("game-ui", "RemoveObject"));
  }

  if (m_hEditUIComponent.IsInvalidated())
  {
    WGameObject* pEditUIObject = nullptr;
    if (!m_pMainWorld->TryGetObjectWithGlobalKey(WTempHashedString("game-ui"), pEditUIObject))
      return;

    WRmlUiCanvas2DComponent* pUiComponent = nullptr;
    if (!pEditUIObject->TryGetComponentOfBaseType(pUiComponent))
      return;

    pUiComponent->AddBlackboardBinding(m_pBlackboard);

    m_hEditUIComponent = pUiComponent->GetHandle();
  }
}

void RtsEditLevelMode::OnFirstActivation()
{
  WInputActionConfig cfg;

  // Level Editing
  {
    cfg.m_sInputSlotTrigger[0] = WInputSlot_KeySpace;
    WInputManager::SetInputActionConfig("game-ui", "PlaceObject", cfg, true);

    cfg.m_sInputSlotTrigger[0] = WInputSlot_KeyDelete;
    WInputManager::SetInputActionConfig("game-ui", "RemoveObject", cfg, true);
  }
}

void RtsEditLevelMode::OnProcessInput(const RtsMouseInputState& MouseInput, bool bUiWantsInput)
{
  if (WInputManager::GetInputSlotState(WInputSlot_KeyEscape) == WKeyState::Pressed)
  {
    m_pGameState->SwitchToGameMode(RtsActiveGameMode::MainMenuMode);
    return;
  }

  if (bUiWantsInput)
    return;

  DoDefaultCameraInput(MouseInput);

  WVec3 vPickedGroundPlanePos;
  if (m_pGameState->PickGroundPlanePosition(vPickedGroundPlanePos).Failed())
    return;

  if (WInputManager::GetInputActionState("game-ui", "PlaceObject") == WKeyState::Pressed)
  {
    WGameObject* pSpawned = nullptr;

    WUInt16 uiTeam = static_cast<WUInt16>(m_pBlackboard->GetEntryValue(s_sTeam).Get<int>());
    int iShipType = m_pBlackboard->GetEntryValue(s_sShipType).Get<int>();
    pSpawned = m_pGameState->SpawnNamedObjectAt(WTransform(vPickedGroundPlanePos, WQuat::MakeIdentity()), g_BuildItemTypes[iShipType], uiTeam);

    WMsgSetColor msg;
    msg.m_Color = RtsGameMode::GetTeamColor(uiTeam);

    pSpawned->PostMessageRecursive(msg, WTime::MakeZero(), WObjectMsgQueueType::AfterInitialized);

    return;
  }

  auto& unitSelection = m_pGameState->m_SelectedUnits;

  if (WInputManager::GetInputActionState("game-ui", "RemoveObject") == WKeyState::Pressed)
  {
    for (WUInt32 i = 0; i < unitSelection.GetCount(); ++i)
    {
      WGameObjectHandle hObject = unitSelection.GetObject(i);
      m_pMainWorld->DeleteObjectDelayed(hObject);
    }

    return;
  }

  m_pGameState->DetectHoveredSelectable();

  if (MouseInput.m_LeftClickState == WKeyState::Released)
  {
    m_pGameState->SelectUnits();
  }
}
