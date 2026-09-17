#include <AsteroidsPlugin/AsteroidsPluginPCH.h>

#include <AsteroidsPlugin/GameState/AsteroidsGameState.h>
#include <Core/Input/DeviceTypes/Controller.h>
#include <Core/Input/InputManager.h>
#include <Core/System/Window.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Logging/Log.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(AsteroidsGameState, 1, WRTTIDefaultAllocator<AsteroidsGameState>)
W_END_DYNAMIC_REFLECTED_TYPE;

const char* szPlayerActions[MaxPlayerActions] = {"Forwards", "Backwards", "Left", "Right", "RotLeft", "RotRight", "Shoot"};
const char* szControlerKeys[MaxPlayerActions] = {"leftstick_posy", "leftstick_negy", "leftstick_negx", "leftstick_posx", "rightstick_negx", "rightstick_posx", "right_trigger"};

namespace
{
  static void RegisterInputAction(const char* szInputSet, const char* szInputAction, const char* szKey1, const char* szKey2 = nullptr, const char* szKey3 = nullptr)
  {
    WInputActionConfig cfg;

    cfg = WInputManager::GetInputActionConfig(szInputSet, szInputAction);
    cfg.m_bApplyTimeScaling = true;

    if (szKey1 != nullptr)
      cfg.m_sInputSlotTrigger[0] = szKey1;
    if (szKey2 != nullptr)
      cfg.m_sInputSlotTrigger[1] = szKey2;
    if (szKey3 != nullptr)
      cfg.m_sInputSlotTrigger[2] = szKey3;

    WInputManager::SetInputActionConfig(szInputSet, szInputAction, cfg, true);
  }
} // namespace

AsteroidsGameState::AsteroidsGameState() = default;
AsteroidsGameState::~AsteroidsGameState() = default;

void AsteroidsGameState::GetStartupOptions(WString& out_sScene, WString& out_sPreloadCollection)
{
  // replace this to load a certain scene at startup
  // the default implementation looks at the command line "-scene" argument

  // if we have a "-scene" command line argument, it was launched from the editor and we should load that
  if (WCommandLineUtils::GetGlobalInstance()->HasOption("-scene"))
  {
    out_sScene = WCommandLineUtils::GetGlobalInstance()->GetStringOption("-scene");
  }
  else
  {
    // otherwise, we use the hardcoded 'Main.WScene'
    // if that doesn't exist, this function has to be adjusted
    // note that you can return an asset GUID here, instead of a path
    out_sScene = "AssetCache/Common/Scenes/Main.WBinScene";
  }
}

void AsteroidsGameState::OnActivation(WWorld* pWorld, WStringView sStartPosition, const WTransform& startPositionOffset)
{
  W_LOG_BLOCK("AsteroidGameState::Activate");

  SUPER::OnActivation(pWorld, sStartPosition, startPositionOffset);

  CreateGameLevel();
}

void AsteroidsGameState::OnDeactivation()
{
  W_LOG_BLOCK("AsteroidGameState::Deactivate");

  DestroyLevel();

  SUPER::OnDeactivation();
}

void AsteroidsGameState::BeforeWorldUpdate()
{
  m_MainCamera.SetCameraMode(WCameraMode::OrthoFixedHeight, 40, -10, 10);
  m_MainCamera.LookAt(WVec3::MakeZero(), WVec3(0, 0, 1), WVec3(0, 1, 0));
}

void AsteroidsGameState::OnChangedMainWorld(WWorld* pPrevWorld, WWorld* pNewWorld, WStringView sStartPosition, const WTransform& startPositionOffset)
{
  SUPER::OnChangedMainWorld(pPrevWorld, pNewWorld, sStartPosition, startPositionOffset);

  // called whenever the main world is changed, ie when transitioning between levels
  // may need to update references to the world here or reset some state
}

void AsteroidsGameState::ConfigureInputActions()
{
  // calling the base class is necessary to register the default key bindings
  // e.g. ESC to quit, F1 to open the console
  // if you want more control, take a look at what WGameState::ConfigureInputActions() does
  // and register only specific actions (see WGameApplication::RegisterGameApplicationInputActions())
  AsteroidsGameStateBase::ConfigureInputActions();

  if (auto pController = WInputManager::GetInputDeviceOfType<WInputDeviceController>())
  {
    pController->EnableVibration(0, true);
    pController->EnableVibration(1, true);
    pController->EnableVibration(2, true);
    pController->EnableVibration(3, true);
  }

  // setup all controllers
  for (WInt32 iPlayer = 0; iPlayer < MaxPlayers; ++iPlayer)
  {
    for (WInt32 iAction = 0; iAction < MaxPlayerActions; ++iAction)
    {
      WStringBuilder sAction;
      sAction.SetFormat("Player{0}_{1}", iPlayer, szPlayerActions[iAction]);

      WStringBuilder sKey;
      sKey.SetFormat("controller{0}_{1}", iPlayer, szControlerKeys[iAction]);

      RegisterInputAction("Game", sAction.GetData(), sKey.GetData());
    }
  }

  // some more keyboard key bindings

  RegisterInputAction("Game", "Player1_Forwards", nullptr, WInputSlot_KeyW);
  RegisterInputAction("Game", "Player1_Backwards", nullptr, WInputSlot_KeyS);
  RegisterInputAction("Game", "Player1_Left", nullptr, WInputSlot_KeyA);
  RegisterInputAction("Game", "Player1_Right", nullptr, WInputSlot_KeyD);
  RegisterInputAction("Game", "Player1_Shoot", nullptr, WInputSlot_KeySpace);
  RegisterInputAction("Game", "Player1_RotLeft", nullptr, WInputSlot_KeyLeft);
  RegisterInputAction("Game", "Player1_RotRight", nullptr, WInputSlot_KeyRight);
}

void AsteroidsGameState::ProcessInput()
{
  W_LOCK(m_pMainWorld->GetWriteMarker());

  for (WInt32 iPlayer = 0; iPlayer < MaxPlayers; ++iPlayer)
    m_pLevel->UpdatePlayerInput(iPlayer);
}

void AsteroidsGameState::CreateGameLevel()
{
  m_pLevel = W_DEFAULT_NEW(Level);

  WWorldDesc desc("Asteroids - World");
  m_pLevel->SetupLevel(W_DEFAULT_NEW(WWorld, desc));

  ChangeMainWorld(m_pLevel->GetWorld(), {}, WTransform::MakeIdentity());
}

void AsteroidsGameState::DestroyLevel()
{
  m_pLevel = nullptr;
}
