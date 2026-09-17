#include <RTSPlugin/RTSPluginPCH.h>

#include <RTSPlugin/GameMode/GameMode.h>
#include <RTSPlugin/GameState/RTSGameState.h>
#include <RmlUiPlugin/Components/RmlUiCanvas2DComponent.h>
#include <RmlUiPlugin/RmlUiContext.h>
#include <RmlUiPlugin/RmlUiSingleton.h>

RtsGameMode::RtsGameMode() = default;
RtsGameMode::~RtsGameMode() = default;

WRmlUiContext* RtsGameMode::SetUiActive(WWorld* pWorld, WTempHashedString sName, bool bActive)
{
  WGameObject* pUIObject = nullptr;
  if (pWorld->TryGetObjectWithGlobalKey(sName, pUIObject))
  {
    WRmlUiCanvas2DComponent* pUiComponent = nullptr;
    if (pUIObject->TryGetComponentOfBaseType(pUiComponent))
    {
      pUiComponent->SetActiveFlag(bActive);

      return pUiComponent->GetRmlContext();
    }
  }

  return nullptr;
}

void RtsGameMode::ActivateMode(WWorld* pMainWorld, WViewHandle hView, WCamera* pMainCamera)
{
  if (m_bFirstActivation)
  {
    m_bFirstActivation = false;

    m_pGameState = RTSGameState::GetSingleton();
    m_pMainWorld = pMainWorld;
    m_hMainView = hView;
    m_pMainCamera = pMainCamera;

    SetupSelectModeUI();

    OnFirstActivation();
  }

  OnActivateMode();
}

void RtsGameMode::DeactivateMode()
{
  OnDeactivateMode();
}

void RtsGameMode::ProcessInput(const RtsMouseInputState& mouseInput)
{
  bool bUiWantsInput = false;

  if (WRmlUi::GetSingleton() != nullptr)
  {
    // do not process input, when RmlUi already wants to work with it
    bUiWantsInput = WRmlUi::GetSingleton()->AnyContextWantsInput();
  }

  OnProcessInput(mouseInput, bUiWantsInput);
}

void RtsGameMode::BeforeWorldUpdate()
{
  OnBeforeWorldUpdate();
}

void RtsGameMode::DoDefaultCameraInput(const RtsMouseInputState& MouseInput)
{
  WView* pView = nullptr;
  if (!WRenderWorld::TryGetView(m_hMainView, pView))
    return;

  const auto vp = pView->GetViewport();

  float movePosX, moveNegX, movePosY, moveNegY, zoomIn, zoomOut;
  WInputManager::GetInputActionState("Game", "CamMovePosX", &movePosX);
  WInputManager::GetInputActionState("Game", "CamMoveNegX", &moveNegX);
  WInputManager::GetInputActionState("Game", "CamMovePosY", &movePosY);
  WInputManager::GetInputActionState("Game", "CamMoveNegY", &moveNegY);
  WInputManager::GetInputActionState("Game", "CamZoomIn", &zoomIn);
  WInputManager::GetInputActionState("Game", "CamZoomOut", &zoomOut);

  const float moveX = movePosX - moveNegX;
  const float moveY = movePosY - moveNegY;
  const float zoom = -zoomIn + zoomOut;

  const bool bMoveCamera = MouseInput.m_RightClickState != WKeyState::Up;

  const float fDimY = m_pMainCamera->GetFovOrDim();
  const float fDimX = (fDimY / vp.height) * vp.width;

  float fZoom = m_pGameState->GetCameraZoom();

  if (zoom != 0.0f)
  {
    if (zoom > 0)
      fZoom *= 1.1f;
    else
      fZoom *= 1.0f / 1.1f;

    fZoom = m_pGameState->SetCameraZoom(fZoom);

    WVec3 pos = m_pMainCamera->GetCenterPosition();
    pos.z = fZoom;
    m_pMainCamera->LookAt(pos, pos + m_pMainCamera->GetCenterDirForwards(), m_pMainCamera->GetCenterDirUp());
  }

  if (bMoveCamera)
  {
    const float fMoveScale = 0.005f * fZoom;

    const float fMoveX = fDimX * moveX * fMoveScale;
    const float fMoveY = fDimY * moveY * fMoveScale;

    m_pMainCamera->MoveGlobally(-fMoveY, fMoveX, 0);
  }
}

bool RtsMouseInputState::HasMouseMoved(WVec2U32 vStart, WVec2U32 vNow)
{
  const WVec2 v1((float)vNow.x, (float)vNow.y);
  const WVec2 v2((float)vStart.x, (float)vStart.y);

  return (v1 - v2).GetLength() > 3.0f;
}

WColor RtsGameMode::GetTeamColor(WUInt16 uiTeam)
{
  switch (uiTeam)
  {
    case 0:
      return WColorGammaUB(255, 0, 0);
    case 1:
      return WColorGammaUB(0, 255, 0);
    case 2:
      return WColorGammaUB(0, 0, 255);
    case 3:
      return WColorGammaUB(255, 255, 0);
  }

  return WColor::White;
}

void RtsGameMode::SetupSelectModeUI()
{
  if (m_hSelectModeUIComponent.IsInvalidated())
  {
    WGameObject* pSelectModeUIObject = nullptr;
    if (!m_pMainWorld->TryGetObjectWithGlobalKey(WTempHashedString("game-ui"), pSelectModeUIObject))
      return;

    WRmlUiCanvas2DComponent* pUiComponent = nullptr;
    if (!pSelectModeUIObject->TryGetComponentOfBaseType(pUiComponent))
      return;

    WRmlUiContext* pRmlContext = pUiComponent->GetOrCreateRmlContext();

    pRmlContext->RegisterEventHandler("switchMode", [](Rml::Event& e)
      {
        auto& sValue = e.GetTargetElement()->GetId();
        if (sValue == "battle")
        {
          RTSGameState::GetSingleton()->SwitchToGameMode(RtsActiveGameMode::BattleMode);
        }
        else if (sValue == "edit")
        {
          RTSGameState::GetSingleton()->SwitchToGameMode(RtsActiveGameMode::EditLevelMode);
        }
        else if (sValue == "mainmenu")
        {
          RTSGameState::GetSingleton()->SwitchToGameMode(RtsActiveGameMode::MainMenuMode);
        }
        else if (sValue == "settingsmenu")
        {
          RTSGameState::GetSingleton()->SwitchToGameMode(RtsActiveGameMode::SettingsMenuMode);
        }
        //
      });

    m_hSelectModeUIComponent = pUiComponent->GetHandle();
  }

  WRmlUiCanvas2DComponent* pUiComponent = nullptr;
  if (m_pMainWorld->TryGetComponent(m_hSelectModeUIComponent, pUiComponent))
  {
    const RtsActiveGameMode mode = RTSGameState::GetSingleton()->GetActiveGameMode();
    const char* szActiveModeId = "battle";
    if (mode == RtsActiveGameMode::EditLevelMode)
    {
      szActiveModeId = "edit";
    }
    else if (mode == RtsActiveGameMode::MainMenuMode)
    {
      szActiveModeId = "mainmenu";
    }
    else if (mode == RtsActiveGameMode::SettingsMenuMode)
    {
      szActiveModeId = "settingsmenu";
    }

    if (auto pActiveModeElement = pUiComponent->GetRmlContext()->GetDocument(0)->GetElementById(szActiveModeId))
    {
      pActiveModeElement->SetAttribute("checked", "");
    }
  }
}
