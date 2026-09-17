#include <GameEngine/GameEnginePCH.h>

#include <Core/Collection/CollectionResource.h>
#include <Core/Input/DeviceTypes/MouseKeyboard.h>
#include <Core/Input/InputManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <GameEngine/Configuration/InputConfig.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameState/FallbackGameState.h>
#include <GameEngine/Gameplay/PlayerStartPointComponent.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WFallbackGameState, 1, WRTTIDefaultAllocator<WFallbackGameState>)
W_END_DYNAMIC_REFLECTED_TYPE;

WFallbackGameState::WFallbackGameState() = default;

void WFallbackGameState::OnActivation(WWorld* pWorld, WStringView sStartPosition, const WTransform& startPositionOffset)
{
  SUPER::OnActivation(pWorld, sStartPosition, startPositionOffset);

  // if we already have a scene (editor use case), just use that and don't create any other world
  if (pWorld != nullptr)
  {
    m_bAllowOpenMenu = false;
    return;
  }

  // if we allow the player to open the menu via the OS key, we want OS not to handle these keys (Windows key)
  if (auto* pDevice = WInputManager::GetInputDeviceOfType<WInputDeviceMouseKeyboard>())
  {
    pDevice->SetDisableOSHotkeys(true);
  }

  // otherwise we need to load a scene

  if (!WFileSystem::ExistsFile(":project/WProject"))
  {
    m_bShowMenu = true;

    if (WCommandLineUtils::GetGlobalInstance()->HasOption("-project"))
      m_State = State::BadProject;
    else
      m_State = State::NoProject;
  }
  else
  {
    WString sSceneFile;
    WString sPreloadCollection;
    GetStartupOptions(sSceneFile, sPreloadCollection);

    if (sSceneFile.IsEmpty())
    {
      SwitchToLoadingScreen("");

      m_bShowMenu = true;
      m_State = State::NoScene;
    }
  }
}

bool WFallbackGameState::IsFallbackGameState() const
{
  // only this class is a fallback, derived ones are not
  return WGetStaticRTTI<WFallbackGameState>() == GetDynamicRTTI();
}

WResult WFallbackGameState::SpawnPlayer(WStringView sStartPosition, const WTransform& startPositionOffset)
{
  m_iActiveCameraComponentIndex = -3;

  if (SUPER::SpawnPlayer(sStartPosition, startPositionOffset).Succeeded())
    return W_SUCCESS;

  if (m_pMainWorld)
  {
    // TODO: find sStartPosition as base location

    m_MainCamera.LookAt(startPositionOffset.m_vPosition, startPositionOffset.m_vPosition + startPositionOffset.m_qRotation * WVec3(1, 0, 0),
      startPositionOffset.m_qRotation * WVec3(0, 0, 1));
  }

  return W_FAILURE;
}

static WHybridArray<WGameAppInputConfig, 16> g_AllInput;

static void RegisterInputAction(const char* szInputSet, const char* szInputAction, const char* szKey1, const char* szKey2 = nullptr, const char* szKey3 = nullptr)
{
  WGameAppInputConfig& gacfg = g_AllInput.ExpandAndGetRef();
  gacfg.m_sInputSet = szInputSet;
  gacfg.m_sInputAction = szInputAction;
  gacfg.m_sInputSlotTrigger[0] = szKey1;
  gacfg.m_sInputSlotTrigger[1] = szKey2;
  gacfg.m_sInputSlotTrigger[2] = szKey3;
  gacfg.m_bApplyTimeScaling = true;

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

void WFallbackGameState::ConfigureInputActions()
{
  SUPER::ConfigureInputActions();

  g_AllInput.Clear();

  RegisterInputAction("Game", "MoveForwards", WInputSlot_KeyW);
  RegisterInputAction("Game", "MoveBackwards", WInputSlot_KeyS);
  RegisterInputAction("Game", "MoveLeft", WInputSlot_KeyA);
  RegisterInputAction("Game", "MoveRight", WInputSlot_KeyD);
  RegisterInputAction("Game", "MoveUp", WInputSlot_KeyE);
  RegisterInputAction("Game", "MoveDown", WInputSlot_KeyQ);
  RegisterInputAction("Game", "Run", WInputSlot_KeyLeftShift);

  RegisterInputAction("Game", "TurnLeft", WInputSlot_MouseMoveNegX, WInputSlot_KeyLeft);
  RegisterInputAction("Game", "TurnRight", WInputSlot_MouseMovePosX, WInputSlot_KeyRight);
  RegisterInputAction("Game", "TurnUp", WInputSlot_MouseMoveNegY, WInputSlot_KeyUp);
  RegisterInputAction("Game", "TurnDown", WInputSlot_MouseMovePosY, WInputSlot_KeyDown);

  RegisterInputAction("Game", "NextCamera", WInputSlot_KeyPageDown);
  RegisterInputAction("Game", "PrevCamera", WInputSlot_KeyPageUp);
}

const WCameraComponent* WFallbackGameState::FindActiveCameraComponent()
{
  if (m_iActiveCameraComponentIndex == -1)
    return nullptr;

  const WWorld* pWorld = m_pMainWorld;
  const WCameraComponentManager* pManager = pWorld->GetComponentManager<WCameraComponentManager>();
  if (pManager == nullptr)
    return nullptr;

  auto itComp = pManager->GetComponents();

  WTempHybridArray<const WCameraComponent*, 32> Cameras[WCameraUsageHint::ENUM_COUNT];

  // first find all cameras and sort them by usage type
  while (itComp.IsValid())
  {
    const WCameraComponent* pComp = itComp;

    if (pComp->IsActive())
    {
      Cameras[pComp->GetUsageHint().GetValue()].PushBack(pComp);
    }

    itComp.Next();
  }

  Cameras[WCameraUsageHint::None].Clear();
  Cameras[WCameraUsageHint::RenderTarget].Clear();
  Cameras[WCameraUsageHint::Culling].Clear();
  Cameras[WCameraUsageHint::Shadow].Clear();
  Cameras[WCameraUsageHint::Thumbnail].Clear();

  if (m_iActiveCameraComponentIndex == -3)
  {
    // take first camera of a good usage type
    m_iActiveCameraComponentIndex = 0;
  }

  // take last camera (wrap around)
  if (m_iActiveCameraComponentIndex == -2)
  {
    m_iActiveCameraComponentIndex = 0;
    for (WUInt32 i = 0; i < WCameraUsageHint::ENUM_COUNT; ++i)
    {
      m_iActiveCameraComponentIndex += Cameras[i].GetCount();
    }

    --m_iActiveCameraComponentIndex;
  }

  if (m_iActiveCameraComponentIndex >= 0)
  {
    WInt32 offset = m_iActiveCameraComponentIndex;

    // now find the camera by that index
    for (WUInt32 i = 0; i < WCameraUsageHint::ENUM_COUNT; ++i)
    {
      if (offset < (WInt32)Cameras[i].GetCount())
        return Cameras[i][offset];

      offset -= Cameras[i].GetCount();
    }
  }

  // on overflow, reset to free camera
  m_iActiveCameraComponentIndex = -1;
  return nullptr;
}

void WFallbackGameState::ProcessInput()
{
  SUPER::ProcessInput();

  if (IsInLoadingScreen())
  {
    float fProgress = 0.0f;
    IsLoadingSceneInBackground(&fProgress);

    WDebugRenderer::DrawInfoText(m_pMainWorld, WDebugTextPlacement::TopCenter, "Loading", WFmt("Loading: {}%%", WMath::RoundToInt(fProgress * 100.0f)));
  }

  {
    if (WInputManager::GetExclusiveInputSet().IsEmpty() || WInputManager::GetExclusiveInputSet() == "WPlayer")
    {
      if (DisplayMenu())
      {
        // prevents the currently active scene from getting any input
        WInputManager::SetExclusiveInputSet("WPlayer");
      }
      else
      {
        // allows the active scene to retrieve input again
        WInputManager::SetExclusiveInputSet("");
      }
    }
  }

  if (m_pMainWorld)
  {
    W_LOCK(m_pMainWorld->GetReadMarker());

    if (WInputManager::GetInputActionState("Game", "NextCamera") == WKeyState::Pressed)
      ++m_iActiveCameraComponentIndex;
    if (WInputManager::GetInputActionState("Game", "PrevCamera") == WKeyState::Pressed)
      --m_iActiveCameraComponentIndex;

    const WCameraComponent* pCamComp = FindActiveCameraComponent();
    if (pCamComp)
    {
      return;
    }

    float fRotateSpeed = 180.0f;
    float fMoveSpeed = 10.0f;
    float fInput = 0.0f;

    if (WInputManager::GetInputActionState("Game", "Run", &fInput) != WKeyState::Up)
      fMoveSpeed *= 10.0f;

    if (WInputManager::GetInputActionState("Game", "MoveForwards", &fInput) != WKeyState::Up)
      m_MainCamera.MoveLocally(fInput * fMoveSpeed, 0, 0);
    if (WInputManager::GetInputActionState("Game", "MoveBackwards", &fInput) != WKeyState::Up)
      m_MainCamera.MoveLocally(-fInput * fMoveSpeed, 0, 0);
    if (WInputManager::GetInputActionState("Game", "MoveLeft", &fInput) != WKeyState::Up)
      m_MainCamera.MoveLocally(0, -fInput * fMoveSpeed, 0);
    if (WInputManager::GetInputActionState("Game", "MoveRight", &fInput) != WKeyState::Up)
      m_MainCamera.MoveLocally(0, fInput * fMoveSpeed, 0);

    if (WInputManager::GetInputActionState("Game", "MoveUp", &fInput) != WKeyState::Up)
      m_MainCamera.MoveGlobally(0, 0, fInput * fMoveSpeed);
    if (WInputManager::GetInputActionState("Game", "MoveDown", &fInput) != WKeyState::Up)
      m_MainCamera.MoveGlobally(0, 0, -fInput * fMoveSpeed);

    if (WInputManager::GetInputActionState("Game", "TurnLeft", &fInput) != WKeyState::Up)
      m_MainCamera.RotateGlobally(WAngle(), WAngle(), WAngle::MakeFromDegree(-fRotateSpeed * fInput));
    if (WInputManager::GetInputActionState("Game", "TurnRight", &fInput) != WKeyState::Up)
      m_MainCamera.RotateGlobally(WAngle(), WAngle(), WAngle::MakeFromDegree(fRotateSpeed * fInput));
    if (WInputManager::GetInputActionState("Game", "TurnUp", &fInput) != WKeyState::Up)
      m_MainCamera.RotateLocally(WAngle(), WAngle::MakeFromDegree(fRotateSpeed * fInput), WAngle());
    if (WInputManager::GetInputActionState("Game", "TurnDown", &fInput) != WKeyState::Up)
      m_MainCamera.RotateLocally(WAngle(), WAngle::MakeFromDegree(-fRotateSpeed * fInput), WAngle());
  }
}

void WFallbackGameState::ConfigureMainCamera()
{
  if (!m_pMainWorld)
    return;

  W_LOCK(m_pMainWorld->GetReadMarker());

  // Update the camera transform after world update so the owner node has its final position for this frame.
  // Setting the camera transform in ProcessInput introduces one frame delay.
  if (const WCameraComponent* pCamComp = FindActiveCameraComponent())
  {
    if (pCamComp->GetCameraMode() != WCameraMode::Stereo && m_MainCamera.GetCameraMode() != WCameraMode::Stereo)
    {
      const WGameObject* pOwner = pCamComp->GetOwner();
      WVec3 vPosition = pOwner->GetGlobalPosition();
      WVec3 vForward = pOwner->GetGlobalDirForwards();
      WVec3 vUp = pOwner->GetGlobalDirUp();

      m_MainCamera.LookAt(vPosition, vPosition + vForward, vUp);
    }
  }
}

void WFallbackGameState::FindAvailableScenes()
{
  if (m_bCheckedForScenes)
    return;

  m_bCheckedForScenes = true;

  if (!WFileSystem::ExistsFile(":project/WProject"))
    return;

#if W_ENABLED(W_SUPPORTS_FILE_ITERATORS)
  WFileSystemIterator fsit;
  WStringBuilder sScenePath;

  for (WFileSystem::StartSearch(fsit, "", WFileSystemIteratorFlags::ReportFilesRecursive);
    fsit.IsValid(); fsit.Next())
  {
    fsit.GetStats().GetFullPath(sScenePath);

    if (!sScenePath.HasExtension(".WScene"))
      continue;

    sScenePath.MakeRelativeTo(fsit.GetCurrentSearchTerm()).AssertSuccess();

    m_AvailableScenes.PushBack(sScenePath);
  }
#endif
}

bool WFallbackGameState::DisplayMenu()
{
  if (IsLoadingSceneInBackground() || m_pMainWorld == nullptr)
    return false;

  auto pWorld = m_pMainWorld;

  if (m_State == State::NoProject)
  {
    WDebugRenderer::DrawInfoText(pWorld, WDebugTextPlacement::TopCenter, "_Player", "No project path provided.\n\nUse the command-line argument\n-project \"Path/To/WProject\"\nto tell WPlayer which project to load.\n\nWith the argument\n-scene \"Path/To/Scene.WScene\"\nyou can also directly load a specific scene.\n\nPress ESC to quit.", WColor::Red);

    return false;
  }

  if (m_State == State::BadProject)
  {
    WDebugRenderer::DrawInfoText(pWorld, WDebugTextPlacement::TopCenter, "_Player", WFmt("Invalid project path provided.\nThe given project directory does not exist:\n\n{}\n\nPress ESC to quit.", WGameApplication::GetGameApplicationInstance()->GetAppProjectPath()), WColor::Red);

    return false;
  }

  if (m_bAllowOpenMenu)
  {
    if (WInputManager::GetInputSlotState(WInputSlot_KeyLeftWin) == WKeyState::Pressed || WInputManager::GetInputSlotState(WInputSlot_KeyRightWin) == WKeyState::Pressed)
    {
      m_bShowMenu = !m_bShowMenu;
    }
  }

  if (m_State == State::Ok && !m_bShowMenu)
    return false;

  WDebugRenderer::DrawInfoText(pWorld, WDebugTextPlacement::TopCenter, "_Player", WFmt("Project: '{}'", WGameApplication::GetGameApplicationInstance()->GetAppProjectPath()), WColor::White);

  if (m_State == State::NoScene)
  {
    WDebugRenderer::DrawInfoText(pWorld, WDebugTextPlacement::TopCenter, "_Player", "No scene path provided.\n\nUse the command-line argument\n-scene \"Path/To/Scene.WScene\"\nto directly load a specific scene.", WColor::Orange);
  }
  else if (m_State == State::BadScene)
  {
    WDebugRenderer::DrawInfoText(pWorld, WDebugTextPlacement::TopCenter, "_Player", WFmt("Failed to load scene: '{}'", m_sTitleOfScene), WColor::Red);
  }
  else
  {
    WDebugRenderer::DrawInfoText(pWorld, WDebugTextPlacement::TopCenter, "_Player", WFmt("Scene: '{}'", m_sTitleOfScene), WColor::White);
  }

  if (m_bShowMenu)
  {
    FindAvailableScenes();

    WDebugRenderer::DrawInfoText(pWorld, WDebugTextPlacement::TopCenter, "_Player", "\nSelect scene:\n", WColor::White);

    for (WUInt32 i = 0; i < m_AvailableScenes.GetCount(); ++i)
    {
      const auto& file = m_AvailableScenes[i];

      if (i == m_uiSelectedScene)
      {
        WDebugRenderer::DrawInfoText(pWorld, WDebugTextPlacement::TopCenter, "_Player", WFmt("> {} <", file), WColor::Gold);
      }
      else
      {
        WDebugRenderer::DrawInfoText(pWorld, WDebugTextPlacement::TopCenter, "_Player", WFmt("  {}  ", file), WColor::GhostWhite);
      }
    }

    WDebugRenderer::DrawInfoText(pWorld, WDebugTextPlacement::TopCenter, "_Player", "\nPress 'Return' to load scene.\nPress the 'Windows' key to toggle this menu.", WColor::White);

    if (WInputManager::GetInputSlotState(WInputSlot_KeyEscape) == WKeyState::Pressed)
    {
      m_bShowMenu = false;
    }
    else if (!m_AvailableScenes.IsEmpty())
    {
      if (WInputManager::GetInputSlotState(WInputSlot_KeyUp) == WKeyState::Pressed)
      {
        if (m_uiSelectedScene == 0)
          m_uiSelectedScene = m_AvailableScenes.GetCount() - 1;
        else
          --m_uiSelectedScene;
      }

      if (WInputManager::GetInputSlotState(WInputSlot_KeyDown) == WKeyState::Pressed)
      {
        if (m_uiSelectedScene == m_AvailableScenes.GetCount() - 1)
          m_uiSelectedScene = 0;
        else
          ++m_uiSelectedScene;
      }

      if (WInputManager::GetInputSlotState(WInputSlot_KeyReturn) == WKeyState::Pressed || WInputManager::GetInputSlotState(WInputSlot_KeyNumpadEnter) == WKeyState::Pressed)
      {
        LoadScene(m_AvailableScenes[m_uiSelectedScene], {}, "", WTransform::MakeIdentity());
        m_bShowMenu = false;
      }

      return true;
    }
  }

  return false;
}

void WFallbackGameState::OnBackgroundSceneLoadingFinished(WUniquePtr<WWorld>&& pWorld)
{
  m_State = State::Ok;
  m_bShowMenu = false;

  if (m_pBackgroundSceneLoad)
  {
    m_sTitleOfScene = m_pBackgroundSceneLoad->GetRequestedScene();
  }

  SUPER::OnBackgroundSceneLoadingFinished(std::move(pWorld));
}

void WFallbackGameState::OnBackgroundSceneLoadingFailed(WStringView sReason)
{
  m_State = State::BadScene;
  m_bShowMenu = true;

  if (m_pBackgroundSceneLoad)
  {
    m_sTitleOfScene = m_pBackgroundSceneLoad->GetRequestedScene();
  }

  SUPER::OnBackgroundSceneLoadingFailed(sReason);
}

W_STATICLINK_FILE(GameEngine, GameEngine_GameState_Implementation_FallbackGameState);
