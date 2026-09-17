#include <PacManPlugin/PacManPluginPCH.h>

#include <Core/Input/InputManager.h>
#include <Core/Interfaces/SoundInterface.h>
#include <Core/System/Window.h>
#include <Core/Utils/Blackboard.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/Log.h>
#include <GameEngine/Input/InputDebugVis.h>
#include <PacManPlugin/GameState/PacManGameState.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/MeshComponent.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(PacManGameState, 1, WRTTIDefaultAllocator<PacManGameState>)
W_END_DYNAMIC_REFLECTED_TYPE;

PacManGameState::PacManGameState() = default;
PacManGameState::~PacManGameState() = default;

WHashedString PacManGameState::s_sStats = WMakeHashedString("Stat");
WHashedString PacManGameState::s_sCoinsEaten = WMakeHashedString("CoinsEaten");
WHashedString PacManGameState::s_sPacManState = WMakeHashedString("PacManState");

void PacManGameState::GetStartupOptions(WString& out_sScene, WString& out_sPreloadCollection)
{
  // if we have a "-scene" command line argument, it was launched from the editor and we should load that
  // otherwise, we use the hardcoded 'Main.WScene' of the PacMan project
  if (WCommandLineUtils::GetGlobalInstance()->HasOption("-scene"))
  {
    out_sScene = WCommandLineUtils::GetGlobalInstance()->GetStringOption("-scene");
  }
  else
  {
    out_sScene = "AssetCache/Common/Scenes/Main.WBinScene";
  }
}

void PacManGameState::OnActivation(WWorld* pWorld, WStringView sStartPosition, const WTransform& startPositionOffset)
{
  // this is called shortly after the game state was created, and before the game starts to properly run
  // so here you could do general startup stuff

  W_LOG_BLOCK("GameState::Activate");

  SUPER::OnActivation(pWorld, sStartPosition, startPositionOffset);

  ResetState();

  {
    m_pLeftStick = W_DEFAULT_NEW(WVirtualThumbStick);
    m_pLeftStick->SetInputArea(WVec2(0, 0), WVec2(0.3f, 1), 0.07f, 1.0f, WVirtualThumbStick::CenterMode::Swipe);
    m_pLeftStick->SetFlags(WVirtualThumbStick::Flags::OnlyMaxAxis);
    m_pLeftStick->SetTriggerInputSlot(WVirtualThumbStick::Input::Touchpoint);
    m_pLeftStick->SetThumbstickOutput(WVirtualThumbStick::Output::Controller0_LeftStick);
    m_pLeftStick->SetAreaFocusMode(WInputActionConfig::OnEnterArea::ActivateImmediately, WInputActionConfig::OnLeaveArea::KeepFocus);
    m_pLeftStick->SetEnabled(false);
  }
  {
    m_pRightStick = W_DEFAULT_NEW(WVirtualThumbStick);
    m_pRightStick->SetInputArea(WVec2(0.8f, 0), WVec2(1.0f, 0.2f), 0.05f, 0.0f, WVirtualThumbStick::CenterMode::InputArea);
    m_pRightStick->SetTriggerInputSlot(WVirtualThumbStick::Input::Touchpoint);
    m_pRightStick->SetThumbstickOutput(WVirtualThumbStick::Output::Controller0_RightStick);
    m_pRightStick->SetAreaFocusMode(WInputActionConfig::OnEnterArea::RequireKeyUp, WInputActionConfig::OnLeaveArea::LoseFocus);
    m_pRightStick->SetEnabled(false);
  }

  if (WSoundInterface* pSoundInterface = WSingletonRegistry::GetSingletonInstance<WSoundInterface>())
  {
    // adjust the volume of the sound groups
    // this would usually be a user setting
    pSoundInterface->SetSoundGroupVolume("Music", 0.7f);
    pSoundInterface->SetSoundGroupVolume("Effects", 0.9f);
  }
}


void PacManGameState::OnDeactivation()
{
  // this is run when the game is shutting down

  W_LOG_BLOCK("GameState::Deactivate");

  SUPER::OnDeactivation();
}

void PacManGameState::AfterWorldUpdate()
{
  // this is called once each frame after the WWorld got updated
  // here we use it to evaluate the current state and to also draw some text on screen
  // all of this could also be done in ProcessInput() instead, especially since the debug-drawing can be done at any time during the frame
  // but in a more complex game you may want to do some things right after the world update

  SUPER::AfterWorldUpdate();

  if (!m_pMainWorld)
    return;

  if (m_uiNumCoinsTotal == 0)
  {
    // we don't know the number of coins in the scene yet, so lets iterate over all objects and count how many coins we find

    W_LOCK(m_pMainWorld->GetWriteMarker());

    for (auto it = m_pMainWorld->GetObjects(); it.IsValid(); ++it)
    {
      // we just use the name of the objects to determine that this is a coin
      if (it->GetName() == "Coin")
      {
        ++m_uiNumCoinsTotal;
      }
    }
  }

  // get the global blackboard in which we track the state
  auto pBlackboard = WBlackboard::GetOrCreateGlobal(s_sStats);

  const WInt32 iNumCoinsFound = pBlackboard->GetEntryValue(s_sCoinsEaten, 0).Get<WInt32>();
  const WInt32 iPacManState = pBlackboard->GetEntryValue(s_sPacManState, 1).Get<WInt32>();

  WDebugRenderer::DrawInfoText(m_pMainWorld, WDebugTextPlacement::TopCenter, "Stats", WFmt("Coins: {} / {}", iNumCoinsFound, m_uiNumCoinsTotal));

  if (iPacManState == PacManState::Alive && m_uiNumCoinsTotal > 0 && iNumCoinsFound == m_uiNumCoinsTotal)
  {
    // let the ghosts and PacMan know when he ate all the coins
    pBlackboard->SetEntryValue(s_sPacManState, PacManState::WonGame);

    // play a sound, the GUID of the sound asset was copied from the editor
    // WSoundInterface::PlaySound("{ a10b9065-0b4d-4eff-a9ac-2f712dc28c1c }", WTransform::MakeIdentity()).IgnoreResult(); // FMOD
    WSoundInterface::PlaySound(m_pMainWorld, "{ 2281d82a-cf87-4747-b664-a41ebc74c052 }", WTransform::MakeIdentity()).IgnoreResult(); // MiniAudio
  }

  if (m_bShowSceneExportError)
  {
    WDebugRenderer::DrawInfoText(m_pMainWorld, WDebugTextPlacement::TopCenter, "Stats", "Cannot reload scene!\n\nThe scene must be transformed/exported first.\nUse 'Transform All' in the editor or export the scene.", WColor::OrangeRed);
  }
  else if (iPacManState == PacManState::EatenByGhost)
  {
    if (m_bTouchInput)
    {
      WDebugRenderer::DrawInfoText(m_pMainWorld, WDebugTextPlacement::TopCenter, "Stats", "YOU LOST!\n\nSwipe top-right screen to play again.", WColor::LightPink);
    }
    else
    {
      WDebugRenderer::DrawInfoText(m_pMainWorld, WDebugTextPlacement::TopCenter, "Stats", "YOU LOST!\n\nPress SPACE to play again.", WColor::Red);
    }
  }
  else if (iPacManState == PacManState::WonGame)
  {
    if (m_bTouchInput)
    {
      WDebugRenderer::DrawInfoText(m_pMainWorld, WDebugTextPlacement::TopCenter, "Stats", "YOU WIN!\n\nSwipe top-right screen to play again.", WColor::LightPink);
    }
    else
    {
      WDebugRenderer::DrawInfoText(m_pMainWorld, WDebugTextPlacement::TopCenter, "Stats", "YOU WIN!\n\nnPress SPACE to play again", WColor::LightPink);
    }
  }

  {
    if (WInputManager::GetInputSlotState(WInputManager::GetInputSlotTouchPoint(0)) == WKeyState::Down)
    {
      m_bTouchInput = true;
      m_pLeftStick->SetEnabled(true);
      m_pRightStick->SetEnabled(true);
    }

    if (m_bTouchInput)
    {
      WDebugRenderer::DrawInfoText(m_pMainWorld, WDebugTextPlacement::TopLeft, "Manual", "Swipe left screen area to steer.");
      WDebugRenderer::DrawInfoText(m_pMainWorld, WDebugTextPlacement::TopRight, "Manual", "Swipe top-right screen to reset.");
    }

    WView* pView = nullptr;
    if (WRenderWorld::TryGetView(m_hMainView, pView))
    {
      const WRectFloat viewport = pView->GetViewport();
      const WVec2 resolution = viewport.GetExtents();

      m_pLeftStick->SetInputCoordinateAspectRatio(resolution.x / resolution.y);
      m_pRightStick->SetInputCoordinateAspectRatio(resolution.x / resolution.y);

      WInputDebugVis::DebugRender(m_pMainWorld, resolution, *m_pLeftStick);
      WInputDebugVis::DebugRender(m_pMainWorld, resolution, *m_pRightStick);
    }
  }
}


WResult PacManGameState::SpawnPlayer(WStringView sStartPosition, const WTransform& startPositionOffset)
{
  // this is called every time we switch to a new scene
  // some games may want to create the 'player object' here
  // since our game always already has a player object, we don't need to do anything like that here
  // but since it is also called when we reset the scene, it is a good point in time to reset the current state

  ResetState();
  return W_SUCCESS;
}

void PacManGameState::ResetState()
{
  // we use a global blackboard to store the overall state of the game (https://ezengine.net/pages/docs/misc/blackboards.html)

  m_uiNumCoinsTotal = 0;

  auto pBlackboard = WBlackboard::GetOrCreateGlobal(s_sStats);

  // 'reset' the state
  pBlackboard->SetEntryValue(s_sCoinsEaten, 0);
  pBlackboard->SetEntryValue(s_sPacManState, PacManState::Alive);
}

// a helper function to bind one or several keys to an input action
static void RegisterInputAction(const char* szInputSet, const char* szInputAction, const char* szKey1, const char* szKey2 = nullptr, const char* szKey3 = nullptr)
{
  WInputActionConfig cfg;
  cfg.m_bApplyTimeScaling = true;
  cfg.m_sInputSlotTrigger[0] = szKey1;
  cfg.m_sInputSlotTrigger[1] = szKey2;
  cfg.m_sInputSlotTrigger[2] = szKey3;

  WInputManager::SetInputActionConfig(szInputSet, szInputAction, cfg, true);
}

void PacManGameState::ConfigureInputActions()
{
  // this function is called once at startup
  // here we can add additional input actions that we want to handle on the game-state level
  // see https://ezengine.net/pages/docs/input/input-overview.html

  SUPER::ConfigureInputActions();

  // we want to be able to reset the game to the start state, using the spacebar
  RegisterInputAction("Game", "Reset", WInputSlot_KeySpace, WInputSlot_Controller0_ButtonStart, WInputSlot_Controller0_RightStick_PosX);
}

void PacManGameState::ProcessInput()
{
  SUPER::ProcessInput();

  if (WInputManager::GetInputActionState("Game", "Reset") == WKeyState::Released)
  {
    ResetState();

    // We just kick off a scene load. The 'scene file' is the asset GUID of the 'Level1.WScene' document.
    WString sScene;
    WString sPreloadCollection;
    GetStartupOptions(sScene, sPreloadCollection);

    // Check if the exported scene file exists before attempting to load it
    if (!WFileSystem::ExistsFile(sScene))
    {
      // The scene hasn't been exported yet, show an error message
      WLog::Warning("Cannot reload scene '{}'. The scene must be transformed/exported before it can be reloaded.", sScene);
      m_bShowSceneExportError = true;
    }
    else
    {
      m_bShowSceneExportError = false;
      LoadScene(sScene, sPreloadCollection, {}, WTransform::MakeIdentity());

      // scene loading happens in the background, and once it is ready, will switch automatically to the new scene
    }
  }
}

void PacManGameState::ConfigureMainCamera()
{
  SUPER::ConfigureMainCamera();

  // we use a fixed camera from the level, so we don't need to setup a custom camera from code
  // but if we wanted, we could ignore the camera from the scene and create our own camera here
  // and then update it in ProcessInput()
}


W_STATICLINK_FILE(PacManPlugin, PacManPlugin_GameState_PacManGameState);
