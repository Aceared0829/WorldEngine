#include <RTSPlugin/RTSPluginPCH.h>

#include <Core/Input/InputManager.h>
#include <Core/System/Window.h>
#include <Core/World/World.h>
#include <RTSPlugin/Components/ComponentMessages.h>
#include <RTSPlugin/Components/SelectableComponent.h>
#include <RTSPlugin/GameMode/BattleMode/BattleMode.h>
#include <RTSPlugin/GameMode/EditLevelMode/EditLevelMode.h>
#include <RTSPlugin/GameMode/GameMode.h>
#include <RTSPlugin/GameMode/MainMenuMode/MainMenuMode.h>
#include <RTSPlugin/GameMode/SettingsMenuMode/SettingsMenuMode.h>
#include <RTSPlugin/GameState/RTSGameState.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(RTSGameState, 1, WRTTIDefaultAllocator<RTSGameState>)
W_END_DYNAMIC_REFLECTED_TYPE;

RTSGameState* RTSGameState::s_pSingleton = nullptr;

RTSGameState::RTSGameState()
{
  s_pSingleton = this;
}

RTSGameState::~RTSGameState() = default;

void RTSGameState::RequestQuit(WStringView sRequestedBy)
{
  if (sRequestedBy == "window")
  {
    SUPER::RequestQuit(sRequestedBy);
    return;
  }

  if (sRequestedBy == "editor-esc")
  {
    SwitchToGameMode(RtsActiveGameMode::MainMenuMode);
    return;
  }

  SUPER::RequestQuit(sRequestedBy);
}

void RTSGameState::GetStartupOptions(WString& out_sScene, WString& out_sPreloadCollection)
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

float RTSGameState::GetCameraZoom() const
{
  return m_fCameraZoom;
}

float RTSGameState::SetCameraZoom(float fZoom)
{
  m_fCameraZoom = WMath::Clamp(fZoom, 1.0f, 50.0f);

  return m_fCameraZoom;
}

void RTSGameState::OnActivation(WWorld* pWorld, WStringView sStartPosition, const WTransform& startPositionOffset)
{
  W_LOG_BLOCK("GameState::Activate");

  SUPER::OnActivation(pWorld, sStartPosition, startPositionOffset);

  PreloadAssets();

  m_pMainMenuMode = W_DEFAULT_NEW(RtsMainMenuMode);
  m_pSettingsMenuMode = W_DEFAULT_NEW(RtsSettingsMenuMode);
  m_pBattleMode = W_DEFAULT_NEW(RtsBattleMode);
  m_pEditLevelMode = W_DEFAULT_NEW(RtsEditLevelMode);

  SwitchToGameMode(RtsActiveGameMode::EditLevelMode);
}

void RTSGameState::OnDeactivation()
{
  W_LOG_BLOCK("GameState::Deactivate");

  SetActiveGameMode(RtsActiveGameMode::None);

  // Brings the hardware mouse cursor back.
  WInputManager::ClearMouseCursor();

  SUPER::OnDeactivation();
}

void RTSGameState::PreloadAssets()
{
  // Load all assets that are referenced in some Collections

  m_hCollectionSpace = WResourceManager::LoadResource<WCollectionResource>("{ 7cd0dfa6-d2bb-433e-9fa2-b17bfae42b6b }");
  m_hCollectionFederation = WResourceManager::LoadResource<WCollectionResource>("{ 1edd3af8-6d59-4825-b853-ee8d7a60cb03 }");
  m_hCollectionKlingons = WResourceManager::LoadResource<WCollectionResource>("{ c683d049-0e54-4c42-9764-a122f9dbc69d }");

  // Register the loaded assets with the names defined in the collections
  // This allows to easily spawn those objects with human readable names instead of GUIDs
  {
    WResourceLock<WCollectionResource> pCollection(m_hCollectionSpace, WResourceAcquireMode::BlockTillLoaded);
    pCollection->RegisterNames();
  }
  {
    WResourceLock<WCollectionResource> pCollection(m_hCollectionFederation, WResourceAcquireMode::BlockTillLoaded);
    pCollection->RegisterNames();
  }
  {
    WResourceLock<WCollectionResource> pCollection(m_hCollectionKlingons, WResourceAcquireMode::BlockTillLoaded);
    pCollection->RegisterNames();
  }
}

void RTSGameState::BeforeWorldUpdate()
{
  if (IsLoadingSceneInBackground())
    return;

  W_LOCK(m_pMainWorld->GetWriteMarker());

  WGameObject* pGameUiObject = nullptr;
  if (m_pMainWorld->TryGetObjectWithGlobalKey(WTempHashedString("game-ui"), pGameUiObject))
  {
    WRmlUiCanvas2DComponent* pGameUiComponent = nullptr;
    if (pGameUiObject->TryGetComponentOfBaseType(pGameUiComponent))
    {
      pGameUiComponent->SetCustomScale(m_fUiScale);
    }
  }

  SetActiveGameMode(m_GameModeToSwitchTo);

  m_SelectedUnits.RemoveDeadObjects();

  if (m_pActiveGameMode)
  {
    m_pActiveGameMode->BeforeWorldUpdate();
  }

  // update the sound listener position to be the same as the camera position
  if (WSoundInterface* pSoundInterface = WSingletonRegistry::GetSingletonInstance<WSoundInterface>())
  {
    const WVec3 pos = m_MainCamera.GetCenterPosition();
    const WVec3 dir = m_MainCamera.GetCenterDirForwards();
    const WVec3 up = m_MainCamera.GetCenterDirUp();

    pSoundInterface->SetListener(0, pos, dir, up, WVec3::MakeZero());
  }
}

void RTSGameState::ConfigureMainCamera()
{
  SUPER::ConfigureMainCamera();

  m_fCameraZoom = 20.0f;
  WVec3 vCameraPos = WVec3(0.0f, 0.0f, m_fCameraZoom);

  WCoordinateSystem coordSys;

  if (m_pMainWorld)
  {
    m_pMainWorld->GetCoordinateSystem(vCameraPos, coordSys);
  }
  else
  {
    coordSys.m_vForwardDir.Set(1, 0, 0);
    coordSys.m_vRightDir.Set(0, 1, 0);
    coordSys.m_vUpDir.Set(0, 0, 1);
  }

  m_MainCamera.SetCameraMode(WCameraMode::PerspectiveFixedFovY, 45.0f, 0.1f, 100);
  m_MainCamera.LookAt(vCameraPos, vCameraPos - coordSys.m_vUpDir, coordSys.m_vForwardDir);
}


void RTSGameState::OnChangedMainWorld(WWorld* pPrevWorld, WWorld* pNewWorld, WStringView sStartPosition, const WTransform& startPositionOffset)
{
  SUPER::OnChangedMainWorld(pPrevWorld, pNewWorld, sStartPosition, startPositionOffset);

  m_SelectedUnits.Clear();
  m_SelectedUnits.SetWorld(pNewWorld);
}

void RTSGameState::SwitchToGameMode(RtsActiveGameMode mode)
{
  // can't just switch game modes in the middle of a frame, so delay this to the next frame
  m_GameModeToSwitchTo = mode;
}

void RTSGameState::SetActiveGameMode(RtsActiveGameMode mode)
{
  if (m_ActiveGameMode == mode)
    return;

  if (m_pActiveGameMode)
  {
    m_pActiveGameMode->DeactivateMode();
  }

  m_PrevGameMode = m_ActiveGameMode;
  m_ActiveGameMode = mode;

  switch (m_ActiveGameMode)
  {
    case RtsActiveGameMode::None:
      m_pActiveGameMode = nullptr;
      break;

    case RtsActiveGameMode::MainMenuMode:
      m_pActiveGameMode = m_pMainMenuMode.Borrow();
      break;

    case RtsActiveGameMode::SettingsMenuMode:
      m_pActiveGameMode = m_pSettingsMenuMode.Borrow();
      break;

    case RtsActiveGameMode::BattleMode:
      m_pActiveGameMode = m_pBattleMode.Borrow();
      break;

    case RtsActiveGameMode::EditLevelMode:
      m_pActiveGameMode = m_pEditLevelMode.Borrow();
      break;
  }


  if (m_pActiveGameMode)
  {
    m_pActiveGameMode->ActivateMode(m_pMainWorld, m_hMainView, &m_MainCamera);
  }
}

WGameObject* RTSGameState::DetectHoveredSelectable()
{
  m_hHoveredSelectable.Invalidate();
  WGameObject* pSelected = PickSelectableObject();

  if (pSelected != nullptr)
  {
    m_hHoveredSelectable = pSelected->GetHandle();
    return pSelected;
  }

  return nullptr;
}

void RTSGameState::SelectUnits()
{
  WGameObject* pSelected = PickSelectableObject();

  if (pSelected != nullptr)
  {
    if (WInputManager::GetInputSlotState(WInputSlot_KeyLeftCtrl) == WKeyState::Down || WInputManager::GetInputSlotState(WInputSlot_KeyRightCtrl) == WKeyState::Down)
    {
      m_SelectedUnits.ToggleSelection(pSelected->GetHandle());
    }
    else
    {
      m_SelectedUnits.Clear();
      m_SelectedUnits.AddObject(pSelected->GetHandle());
    }
  }
  else
  {
    m_SelectedUnits.Clear();
  }
}

void RTSGameState::RenderUnitSelection() const
{
  WBoundingBox bbox;

  for (WUInt32 i = 0; i < m_SelectedUnits.GetCount(); ++i)
  {
    WGameObjectHandle hObject = m_SelectedUnits.GetObject(i);

    WGameObject* pObject;
    if (!m_pMainWorld->TryGetObject(hObject, pObject))
      continue;

    RtsSelectableComponent* pSelectable;
    if (!pObject->TryGetComponentOfBaseType(pSelectable))
      continue;

    const float fRadius = pSelectable->m_fSelectionRadius * 1.1f;

    WTransform t = pObject->GetGlobalTransform();
    t.m_vScale.Set(1.0f);
    t.m_qRotation.SetIdentity();

    bbox = WBoundingBox::MakeFromCenterAndHalfExtents(WVec3::MakeZero(), WVec3(fRadius, fRadius, 0));
    WDebugRenderer::DrawLineBoxCorners(m_pMainWorld, bbox, 0.1f, WColor::White, t);

    RenderUnitHealthbar(pObject, fRadius);
  }

  // hovered unit
  {
    WGameObject* pObject;
    if (m_pMainWorld->TryGetObject(m_hHoveredSelectable, pObject))
    {
      RtsSelectableComponent* pSelectable;
      if (pObject->TryGetComponentOfBaseType(pSelectable))
      {
        const float fRadius = pSelectable->m_fSelectionRadius * 1.1f;

        WTransform t = pObject->GetGlobalTransform();
        t.m_vScale.Set(1.0f);
        t.m_qRotation.SetIdentity();

        bbox = WBoundingBox::MakeFromCenterAndHalfExtents(WVec3::MakeZero(), WVec3(fRadius, fRadius, 0));
        WDebugRenderer::DrawLineBoxCorners(m_pMainWorld, bbox, 0.1f, WColor::DodgerBlue, t);

        RenderUnitHealthbar(pObject, fRadius);
      }
    }
  }
}

void RTSGameState::RenderUnitHealthbar(WGameObject* pObject, float fSelectableRadius) const
{
  RtsMsgGatherUnitStats msgStats;
  pObject->SendMessageRecursive(msgStats);

  if (msgStats.m_uiMaxHealth > 0)
  {
    const float percentage = msgStats.m_uiCurHealth / (float)msgStats.m_uiMaxHealth;
    const float fOffset = 0.01f;

    WVec3 pos = pObject->GetGlobalPosition();
    pos.x += fSelectableRadius - 0.04f - fOffset;

    WColor c = WColor::Lime;

    if (percentage < 0.3f)
      c = WColor::Red;
    else if (percentage < 0.6f)
      c = WColor::Orange;
    else if (percentage < 0.8f)
      c = WColor::Yellow;

    WBoundingBox bbox = WBoundingBox::MakeFromCenterAndHalfExtents(WVec3::MakeZero(), WVec3(0.04f, fSelectableRadius * percentage - fOffset, 0));
    WDebugRenderer::DrawSolidBox(m_pMainWorld, bbox, c, WTransform(pos));
  }
}

WResult RTSGameState::ComputePickingRay()
{
  WView* pView = nullptr;
  if (!WRenderWorld::TryGetView(m_hMainView, pView))
    return W_FAILURE;

  WVec3 vMousePos((float)m_MouseInputState.m_MousePos.x, (float)m_MouseInputState.m_MousePos.y, 0);

  pView->ConvertScreenPixelPosToNormalizedPos(vMousePos);

  return pView->ComputePickingRay(vMousePos.x, vMousePos.y, m_vCurrentPickingRayStart, m_vCurrentPickingRayDir);
}

WResult RTSGameState::PickGroundPlanePosition(WVec3& out_vPositon) const
{
  WPlane p;
  p = WPlane::MakeFromNormalAndPoint(WVec3(0, 0, 1), WVec3(0));

  return p.GetRayIntersection(m_vCurrentPickingRayStart, m_vCurrentPickingRayDir, nullptr, &out_vPositon) ? W_SUCCESS : W_FAILURE;
}

WGameObject* RTSGameState::PickSelectableObject() const
{
  struct Payload
  {
    WGameObject* pBestObject = nullptr;
    float fBestDistSQR = WMath::Square(1000.0f);
    WVec3 vGroundPos;
  };

  Payload pl;

  if (PickGroundPlanePosition(pl.vGroundPos).Failed())
    return nullptr;

  WSpatialSystem::QueryCallback cb = [&pl](WGameObject* pObject)
  {
    RtsSelectableComponent* pSelectable = nullptr;
    if (pObject->TryGetComponentOfBaseType(pSelectable))
    {
      const float dist = (pObject->GetGlobalTransform().m_vPosition - pl.vGroundPos).GetLengthSquared();

      if (dist < pl.fBestDistSQR && dist <= WMath::Square(pSelectable->m_fSelectionRadius))
      {
        pl.fBestDistSQR = dist;
        pl.pBestObject = pObject;
      }
    }

    return WVisitorExecution::Continue;
  };

  InspectObjectsInArea(pl.vGroundPos.GetAsVec2(), 1.0f, cb);

  return pl.pBestObject;
}

// BEGIN-DOCS-CODE-SNIPPET: spatial-query
void RTSGameState::InspectObjectsInArea(const WVec2& vPosition, float fRadius, WSpatialSystem::QueryCallback callback) const
{
  WBoundingSphere sphere = WBoundingSphere::MakeFromCenterAndRadius(vPosition.GetAsVec3(0), fRadius);
  WSpatialSystem::QueryParams queryParams;
  queryParams.m_uiCategoryBitmask = RtsSelectableComponent::s_SelectableCategory.GetBitmask();
  m_pMainWorld->GetSpatialSystem()->FindObjectsInSphere(sphere, queryParams, callback);
}
// END-DOCS-CODE-SNIPPET

WGameObject* RTSGameState::SpawnNamedObjectAt(const WTransform& transform, const char* szObjectName, WUInt16 uiTeamID)
{
  WPrefabResourceHandle hPrefab = WResourceManager::LoadResource<WPrefabResource>(szObjectName);

  WResourceLock<WPrefabResource> pPrefab(hPrefab, WResourceAcquireMode::BlockTillLoaded);

  WHybridArray<WGameObject*, 8> CreatedRootObjects;

  WPrefabInstantiationOptions options;
  options.m_pCreatedRootObjectsOut = &CreatedRootObjects;
  options.m_pOverrideTeamID = &uiTeamID;

  pPrefab->InstantiatePrefab(*m_pMainWorld, transform, options);

  return CreatedRootObjects[0];
}
