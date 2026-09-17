#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <GameEngine/GameState/FallbackGameState.h>
#include <RTSPlugin/GameMode/GameMode.h>
#include <Utilities/DataStructures/ObjectSelection.h>

enum class RtsActiveGameMode
{
  None,
  MainMenuMode,
  SettingsMenuMode,
  EditLevelMode,
  BattleMode,
};

class RtsGameMode;
class RtsMainMenuMode;
class RtsSettingsMenuMode;
class RtsBattleMode;
class RtsEditLevelMode;

using WCollectionResourceHandle = WTypedResourceHandle<class WCollectionResource>;

// the WFallbackGameState adds a free flying camera and a scene switching menu, so can be useful in the very beginning
// but generally it's better to use WGameState instead
// using RTSGameStateBase = WFallbackGameState;
using RTSGameStateBase = WGameState;

class RTSGameState : public RTSGameStateBase
{
  W_ADD_DYNAMIC_REFLECTION(RTSGameState, RTSGameStateBase);

  static RTSGameState* s_pSingleton;

public:
  RTSGameState();
  ~RTSGameState();

  static RTSGameState* GetSingleton() { return s_pSingleton; }

  virtual void RequestQuit(WStringView sRequestedBy) override;

protected:
  virtual void ConfigureMainCamera() override;
  virtual void OnChangedMainWorld(WWorld* pPrevWorld, WWorld* pNewWorld, WStringView sStartPosition, const WTransform& startPositionOffset) override;
  virtual void GetStartupOptions(WString& out_sScene, WString& out_sPreloadCollection) override;

private:
  virtual void OnActivation(WWorld* pWorld, WStringView sStartPosition, const WTransform& startPositionOffset) override;
  virtual void OnDeactivation() override;
  virtual void BeforeWorldUpdate() override;
  void PreloadAssets();

  WCollectionResourceHandle m_hCollectionSpace;
  WCollectionResourceHandle m_hCollectionFederation;
  WCollectionResourceHandle m_hCollectionKlingons;

  WDeque<WGameObjectHandle> m_SpawnedObjects;

  //////////////////////////////////////////////////////////////////////////
  // Camera
public:
  float GetCameraZoom() const;
  float SetCameraZoom(float fZoom);

  //////////////////////////////////////////////////////////////////////////
  // UI
  float m_fUiScale = 1.0f;

  //////////////////////////////////////////////////////////////////////////
  // Game Mode
public:
  void SwitchToGameMode(RtsActiveGameMode mode);
  RtsActiveGameMode GetActiveGameMode() const { return m_GameModeToSwitchTo; }
  RtsActiveGameMode GetPrevGameMode() const { return m_PrevGameMode; }

private:
  void SetActiveGameMode(RtsActiveGameMode mode);

  RtsActiveGameMode m_GameModeToSwitchTo = RtsActiveGameMode::None;
  RtsActiveGameMode m_PrevGameMode = RtsActiveGameMode::None;
  RtsActiveGameMode m_ActiveGameMode = RtsActiveGameMode::None;
  RtsGameMode* m_pActiveGameMode = nullptr;

  // all the modes that the game has
  WUniquePtr<RtsMainMenuMode> m_pMainMenuMode;
  WUniquePtr<RtsSettingsMenuMode> m_pSettingsMenuMode;
  WUniquePtr<RtsBattleMode> m_pBattleMode;
  WUniquePtr<RtsEditLevelMode> m_pEditLevelMode;

  //////////////////////////////////////////////////////////////////////////
  // Input Handling
private:
  virtual void ConfigureMainWindowInputDevices(WWindow* pWindow) override;
  virtual void ConfigureInputActions() override;
  virtual void ProcessInput() override;
  void UpdateMousePosition();
  void UpdateMouseCursor();

  RtsMouseInputState m_MouseInputState;
  float m_fCameraZoom = 10.0f;
  WTime m_CursorAnimation;

  //////////////////////////////////////////////////////////////////////////
  // Picking
public:
  WResult PickGroundPlanePosition(WVec3& out_vPositon) const;
  WGameObject* PickSelectableObject() const;
  void InspectObjectsInArea(const WVec2& vPosition, float fRadius, WSpatialSystem::QueryCallback callback) const;

private:
  WResult ComputePickingRay();

  WVec3 m_vCurrentPickingRayStart;
  WVec3 m_vCurrentPickingRayDir;

  //////////////////////////////////////////////////////////////////////////
  // Spawning Objects
public:
  WGameObject* SpawnNamedObjectAt(const WTransform& transform, const char* szObjectName, WUInt16 uiTeamID);

  //////////////////////////////////////////////////////////////////////////
  // Units
public:
  WGameObject* DetectHoveredSelectable();
  void SelectUnits();
  void RenderUnitSelection() const;
  void RenderUnitHealthbar(WGameObject* pObject, float fSelectableRadius) const;

  WGameObjectHandle m_hHoveredSelectable;
  WObjectSelection m_SelectedUnits;
};
