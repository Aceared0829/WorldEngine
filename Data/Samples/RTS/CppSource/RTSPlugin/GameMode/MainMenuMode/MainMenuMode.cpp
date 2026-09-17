#include <RTSPlugin/RTSPluginPCH.h>

#include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#include <Foundation/System/Screen.h>
#include <RTSPlugin/GameMode/MainMenuMode/MainMenuMode.h>
#include <RTSPlugin/GameState/RTSGameState.h>
#include <RmlUiPlugin/Components/RmlUiCanvas2DComponent.h>
#include <RmlUiPlugin/RmlUiContext.h>

RtsMainMenuMode::RtsMainMenuMode() = default;
RtsMainMenuMode::~RtsMainMenuMode() = default;

void RtsMainMenuMode::OnActivateMode()
{
  W_LOCK(m_pMainWorld->GetWriteMarker());

  SetUiActive(m_pMainWorld, WTempHashedString("app-menu"), true);

  WGameObject* pUIObject = nullptr;
  if (m_pMainWorld->TryGetObjectWithGlobalKey(WTempHashedString("app-menu"), pUIObject))
  {
    WRmlUiCanvas2DComponent* pUiComponent = nullptr;
    if (pUIObject->TryGetComponentOfBaseType(pUiComponent))
    {
      m_hMainMenu = pUiComponent->GetHandle();

      WRmlUiContext* pRmlContext = pUiComponent->GetOrCreateRmlContext();

      pRmlContext->RegisterEventHandler("Game-Resume", [this](Rml::Event& e)
        { m_pGameState->SwitchToGameMode(RtsActiveGameMode::EditLevelMode); });

      pRmlContext->RegisterEventHandler("Game-Settings", [this](Rml::Event& e)
        { m_pGameState->SwitchToGameMode(RtsActiveGameMode::SettingsMenuMode); });

      pRmlContext->RegisterEventHandler("Game-Exit", [this](Rml::Event& e)
        { m_pGameState->RequestQuit("game"); });
    }
  }
}

void RtsMainMenuMode::OnDeactivateMode()
{
  W_LOCK(m_pMainWorld->GetWriteMarker());

  SetUiActive(m_pMainWorld, WTempHashedString("app-menu"), false);
}

void RtsMainMenuMode::OnBeforeWorldUpdate()
{
}

void RtsMainMenuMode::OnProcessInput(const RtsMouseInputState& MouseInput, bool bUiWantsInput)
{
  if (WInputManager::GetInputSlotState(WInputSlot_KeyEscape) == WKeyState::Pressed)
  {
    m_pGameState->SwitchToGameMode(RtsActiveGameMode::EditLevelMode);
  }
}
